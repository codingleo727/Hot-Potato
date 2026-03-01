#include "player.hpp"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

Player::Player(int port) : port_(port) {
}

std::uint16_t Player::get_id() const {
    return my_id;
}

void Player::start(const std::string & ringmasterAddress, std::uint16_t ringmasterPort) {
    openListeningSocket();
    getPort();
    connectToRingmaster(ringmasterAddress, ringmasterPort);
    sendInfoToRingmaster();
    neighborInfos = receiveInfoFromRingmaster();
    connectToNeighbors(neighborInfos);
}

void Player::openListeningSocket() {
    mySocket = Socket::createListeningSocket(0); // Use port 0 to let the OS choose an available port
}

void Player::getPort() {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (::getsockname(mySocket.get_fd(), (struct sockaddr *) &addr, &addr_len) < 0) {
        throw std::runtime_error("getsockname failed");
    }
    if (addr.ss_family == AF_INET) {
        struct sockaddr_in * addr_in = (struct sockaddr_in *) &addr;
        port_ = ntohs(addr_in->sin_port);
    } else if (addr.ss_family == AF_INET6) {
        struct sockaddr_in6 * addr_in6 = (struct sockaddr_in6 *) &addr;
        port_ = ntohs(addr_in6->sin6_port);
    } else {
        throw std::runtime_error("Unknown address family");
    }
}

void Player::connectToRingmaster(const std::string & ringmasterAddress, std::uint16_t ringmaster_port) {
    ringmaster = Socket::connectToServer(ringmasterAddress, ringmaster_port);
    std::cerr << "Connected to ringmaster at " << ringmasterAddress << ":" << ringmaster_port << "\n";
}

Socket Player::connectToNeighbor(const Player::PlayerInfo & info) {
    return Socket::connectToServer(info.address, info.port);
}

void Player::connectToNeighbors(const std::vector<Player::PlayerInfo> & neighborInfos) {
    if (numPlayers == 2) {
        PlayerInfo neighbor = neighborInfos[0];
        if (my_id == 1) {
            leftPlayer = std::move(acceptNeighborConnection(neighbor));
            rightPlayer = std::move(connectToNeighbor(neighbor));
        } else {
            rightPlayer = std::move(connectToNeighbor(neighbor));
            leftPlayer = std::move(acceptNeighborConnection(neighbor));
        }
    } else if (numPlayers > 2) {
        PlayerInfo rightNeighbor = neighborInfos[0];
        rightPlayer = std::move(connectToNeighbor(rightNeighbor));

        PlayerInfo leftNeighbor = neighborInfos[1];
        leftPlayer = std::move(acceptNeighborConnection(leftNeighbor));
    }
}

Socket Player::acceptNeighborConnection(const Player::PlayerInfo & neighborInfo) {
    struct sockaddr_storage neighbor_addr;
    socklen_t neighbor_addr_len = sizeof(neighbor_addr);
    int neighbor_fd = ::accept(mySocket.get_fd(), (struct sockaddr *) & neighbor_addr, & neighbor_addr_len);
    if (neighbor_fd < 0) {
        throw std::runtime_error("accept failed");
    }
    std::string neighbor_ip_str;
    if (neighbor_addr.ss_family == AF_INET) {
        struct sockaddr_in * addr_in = (struct sockaddr_in *) & neighbor_addr;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, sizeof(ip_str));
        neighbor_ip_str = ip_str;
    } else if (neighbor_addr.ss_family == AF_INET6) {
        struct sockaddr_in6 * addr_in6 = (struct sockaddr_in6 *) & neighbor_addr;
        char ip_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip_str, sizeof(ip_str));
        neighbor_ip_str = ip_str;
    } else {
        throw std::runtime_error("Unknown address family");
    }
    
    if (neighbor_ip_str != neighborInfo.address) {
        std::cerr << "Accepted connection from unexpected IP address: " << neighbor_ip_str << " while expecting: " << neighborInfo.address << std::endl;
    }

    return Socket(neighbor_fd);
}

// Helper function
void Player::sendInfoToRingmaster() const {
    std::uint16_t port_net = htons(port_);
    ringmaster.sendAll(reinterpret_cast<const char *>(&port_net), sizeof(port_net));
}

// Helper function
void Player::receiveMyId() {
    std::uint16_t id_net;
    char * buff = reinterpret_cast<char *>(&id_net);
    ringmaster.recvAll(buff, sizeof(id_net));
    my_id = ntohs(id_net) + 1; // Convert from 0-based index to 1-based player ID
}

// Helper function
void Player::receiveTotalNumberOfPlayers() {
    std::uint16_t numPlayers_net;
    char * buff = reinterpret_cast<char *>(&numPlayers_net);
    ringmaster.recvAll(buff, sizeof(numPlayers_net));
    numPlayers = ntohs(numPlayers_net);
}

// Helper function
std::uint16_t Player::receiveInfoLength() {
    std::uint16_t info_len_net;
    char * buff = reinterpret_cast<char *>(&info_len_net);
    ringmaster.recvAll(buff, sizeof(info_len_net));
    return ntohs(info_len_net);
}

// Helper function
std::string Player::receiveInfoString() {
    std::uint16_t info_len = receiveInfoLength();
    std::string info_str(info_len, '\0');
    ringmaster.recvAll(info_str.data(), info_len);
    return info_str;
}

// Helper function
Player::PlayerInfo Player::parseString(const std::string & playerInfo) {
    PlayerInfo info;

    std::size_t first_colon_pos = playerInfo.find(':');
    if (first_colon_pos == std::string::npos) {
        return info;
    }
    std::string id_str = playerInfo.substr(0, first_colon_pos);
    try {
        info.id = std::stoi(id_str) + 1; // Convert from 0-based index to 1-based player ID
    } catch (const std::exception &) {
        throw std::runtime_error("Invalid player ID in neighbor information");
    }

    std::size_t second_colon_pos = playerInfo.find(':', first_colon_pos + 1);
    if (second_colon_pos == std::string::npos) {
        return info;
    }
    info.address = playerInfo.substr(first_colon_pos + 1, second_colon_pos - first_colon_pos - 1);
    std::string port_str = playerInfo.substr(second_colon_pos + 1);
    try {
        int port = std::stoi(port_str);
        if (port < 0 || port > 65535) {
            throw std::runtime_error("Port number out of range in neighbor information");
        }
        info.port = static_cast<std::uint16_t>(port);
    } catch (const std::exception &) {
        throw std::runtime_error("Invalid port number in neighbor information");
    }

    return info;
}

std::vector<Player::PlayerInfo> Player::receiveInfoFromRingmaster() {
    receiveMyId();
    receiveTotalNumberOfPlayers();

    std::vector<PlayerInfo> neighborInfos;

    std::string info = receiveInfoString();
    size_t newline_pos = info.find('\n');
    if (newline_pos == std::string::npos) {
        throw std::runtime_error("Invalid neighbor information format");
    }
    std::string right_info = info.substr(0, newline_pos);
    neighborInfos.push_back(parseString(right_info));

    if (numPlayers > 2) {
        std::string left_info = info.substr(newline_pos + 1);
        neighborInfos.push_back(parseString(left_info));
    }

    std::cout << "Connected as player " << my_id << " out of " << numPlayers << " total players\n";

    return neighborInfos;
}

Potato Player::receivePotato() const {
    std::vector<struct pollfd> pfds(3);
    pfds[0].fd = ringmaster.get_fd();
    pfds[0].events = POLLIN;

    pfds[1].fd = leftPlayer.get_fd();
    pfds[1].events = POLLIN;

    pfds[2].fd = rightPlayer.get_fd();
    pfds[2].events = POLLIN;

    int status = ::poll(pfds.data(), pfds.size(), -1);
    if (status > 0) {
        for (int i = 0; i < 3; i++) {
            if (pfds[i].revents & POLLIN) {
                Potato potato;
                char * buf = reinterpret_cast<char *>(&potato);
                if (i == 0) {
                    ringmaster.recvAll(buf, sizeof(potato));
                } else if (i == 1) {
                    leftPlayer.recvAll(buf, sizeof(potato));
                } else if (i == 2) {
                    rightPlayer.recvAll(buf, sizeof(potato));
                }
                return potato;
            }
        }
    }
    return Potato(-1); // Return an invalid potato if poll fails or is interrupted
}

int Player::passPotato(Potato & potato) const {
    if (potato.getHops() == -2) {
        return -2; // Indicate that the game is over and the player should exit
    }

    if (potato.getHops() < 0) {
        std::cerr << "Attempted to pass an invalid potato with negative hops. Ignoring.\n";
        return -1; // Do not pass an invalid potato
    }

    if (potato.getHops() == 0) {
        potato.addTrace(my_id);
        ringmaster.sendAll(reinterpret_cast<const char *>(&potato), sizeof(potato));
        std::cout << "I'm it\n";
        return 0;
    } else {
        potato.decrementHops();
        potato.addTrace(my_id);
        if (numPlayers == 2) {
            rightPlayer.sendAll(reinterpret_cast<const char *>(&potato), sizeof(potato));
            std::cout << "Sending potato to " << neighborInfos[0].id << "\n";
        }
        else {
            int randomChoice = rand() % 2;
            if (randomChoice == 0) {
                rightPlayer.sendAll(reinterpret_cast<const char *>(&potato), sizeof(potato));
            } else {
                leftPlayer.sendAll(reinterpret_cast<const char *>(&potato), sizeof(potato));
            }
            std::cout << "Sending potato to " << (randomChoice == 0 ? neighborInfos[0].id : neighborInfos[1].id) << "\n";
        }

        return 1;
    }
}

int Player::middleGame() const {
    Potato potato = receivePotato();
    return passPotato(potato);
}

void Player::receiveGameOver(){
    std::string gameOverStr = receiveInfoString();
    std::cout << gameOverStr << std::endl;
}

void Player::sendShutdownAcknowledgement() const {
    std::uint16_t shutdown = 1;
    ringmaster.sendAll(reinterpret_cast<const char *>(&shutdown), sizeof(shutdown));
}

void Player::end() {
    receiveGameOver();
    sendShutdownAcknowledgement();
}