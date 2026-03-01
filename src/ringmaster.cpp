#include "ringmaster.hpp"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

Ringmaster::Ringmaster(int port, int numPlayers) : port_(port), numPlayers(numPlayers) {}

Potato Ringmaster::createPotato(int numHops) const {
    return Potato(numHops);
}

void Ringmaster::openListeningSocket() {
    mySocket = Socket::createListeningSocket(port_);
}

// Helper function
Ringmaster::PlayerConnection Ringmaster::acceptPlayer() {
    PlayerConnection pc;

    struct sockaddr_storage player_addr;
    socklen_t player_addr_len = sizeof(player_addr);
    int player_fd = ::accept(mySocket.get_fd(), (struct sockaddr *) &player_addr, &player_addr_len);
    if (player_fd < 0) {
        throw std::runtime_error("accept failed");
    }
    pc.playerSocket = Socket(player_fd);

    char ip_str[INET6_ADDRSTRLEN];
    if (player_addr.ss_family == AF_INET) {
        struct sockaddr_in * addr_in = (struct sockaddr_in *) &player_addr;
        inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, sizeof(ip_str));
    } else if (player_addr.ss_family == AF_INET6) {
        struct sockaddr_in6 * addr_in6 = (struct sockaddr_in6 *) &player_addr;
        inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip_str, sizeof(ip_str));
    } else {
        throw std::runtime_error("Unknown address family");
    }
    pc.address = ip_str;

    return pc;
}

void Ringmaster::initializePlayers() {
    for (int i = 0; i < numPlayers; ++i) {
        std::cerr << "Waiting for player " << i + 1 << " to connect...\n"; // Convert to 1-based player ID for printing
        PlayerConnection pc = acceptPlayer();

        uint16_t player_port_net;
        char * buf = reinterpret_cast<char *>(&player_port_net);
        pc.playerSocket.recvAll(buf, sizeof(player_port_net));

        uint16_t player_port = ntohs(player_port_net);
        playerInfos.push_back({i, pc.address, player_port});

        playerSockets.push_back(std::move(pc.playerSocket));

        std::cout << "Player " << i + 1 << " is ready to play\n"; // Convert to 1-based player ID for printing
    }
}

// Helper function
std::string Ringmaster::getNeighborInfo(const PlayerInfo & neighbor) const {
    return std::to_string(neighbor.id) + ":" + neighbor.address + ":" + std::to_string(neighbor.port);
}

void Ringmaster::sendPlayerOwnInfo(const PlayerInfo & playerInfo, const Socket & playerSocket) const {
    std::uint16_t player_id_info = htons(static_cast<std::uint16_t>(playerInfo.id));
    playerSocket.sendAll(reinterpret_cast<const char *>(&player_id_info), sizeof(player_id_info));
}

void Ringmaster::sendTotalNumberOfPlayers(const Socket & playerSocket) const {
    std::uint16_t numPlayers_net = htons(static_cast<std::uint16_t>(numPlayers));
    playerSocket.sendAll(reinterpret_cast<const char *>(&numPlayers_net), sizeof(numPlayers_net));
}

void Ringmaster::sendInfoToPlayers() const {
    if (numPlayers == 1) {
        sendPlayerOwnInfo(playerInfos[0], playerSockets[0]);
        sendTotalNumberOfPlayers(playerSockets[0]);
        return;
    }
    for (size_t i = 0; i < playerSockets.size(); ++i) {
        sendPlayerOwnInfo(playerInfos[i], playerSockets[i]);
        sendTotalNumberOfPlayers(playerSockets[i]);

        int rightIndex = (i + 1) % numPlayers;
        std::string info = getNeighborInfo(playerInfos[rightIndex]);

        if (numPlayers > 2) {
            int leftIndex = (i - 1 + numPlayers) % numPlayers;
            info += "\n" + getNeighborInfo(playerInfos[leftIndex]);
        }

        info += "\n"; // Add a newline at the end to indicate the end of the message

        const Socket & playerSocket = playerSockets[i];

        std::uint16_t info_len = htons(static_cast<std::uint16_t>(info.size()));
        playerSocket.sendAll(reinterpret_cast<const char *>(&info_len), sizeof(info_len));
        playerSocket.sendAll(info.c_str(), info.size());
    }
}

int Ringmaster::sendPotato(Potato & potato) const {
    potato.decrementHops();
    if (playerSockets.empty()) {
        return -1;
    }
    int randomIndex = rand() % numPlayers;
    playerSockets[randomIndex].sendAll(reinterpret_cast<const char *>(&potato), sizeof(potato));
    return randomIndex;
}

int Ringmaster::startGame(int numHops) {
    std::cout << "Potato Ringmaster\n";
    std::cout << "Players = " << numPlayers << std::endl;
    std::cout << "Hops = " << numHops << std::endl;
    openListeningSocket();
    initializePlayers();
    sendInfoToPlayers();

    if (numHops <= 0) {
        std::cout << "No hops specified. Ending game.\n";
        return 0;
    }
    Potato potato = createPotato(numHops);
    int startingPlayer = sendPotato(potato);
    std::cout << "Ready to start the game, sending potato to player " << startingPlayer + 1 << "\n"; // Convert to 1-based player ID for printing
    return 1;
}

Potato Ringmaster::waitForPotato() const {
    std::vector<struct pollfd> pfds(numPlayers);
    for (int i = 0; i < numPlayers; ++i) {
        int player_fd = playerSockets[i].get_fd();
        pfds[i] = {player_fd, POLLIN, 0};
    }
    int status = ::poll(pfds.data(), numPlayers, -1);
    if (status > 0) {
        for (int i = 0; i < numPlayers; ++i) {
            if (pfds[i].revents & POLLIN) {
                Potato finalPotato;
                playerSockets[i].recvAll(reinterpret_cast<char *>(&finalPotato), sizeof(finalPotato));
                return finalPotato;
            }
        }
    }
    return Potato(-1); // Return a potato with -1 hops to indicate an error
}

void Ringmaster::printTrace(const Potato & potato) const {
    const int * trace = potato.getTrace();
    int traceLength = potato.getTraceLength();
    std::string finalTrace;
    for (int i = 0; i < traceLength; ++i) {
        finalTrace += std::to_string(trace[i]);
        if (i < traceLength - 1) {
            finalTrace += ",";
        }
    }
    std::string finalMessage = "Trace of potato:\n" + finalTrace;
    std::cout << finalMessage << std::endl;
}

void Ringmaster::sendShutdownSignal() const {
    Potato shutdownPotato(-2);
    for (int i = 0; i < numPlayers; ++i) {
        playerSockets[i].sendAll(reinterpret_cast<const char *>(&shutdownPotato), sizeof(shutdownPotato));
    }
}

void Ringmaster::sendFinalMessage(const std::string & finalMessage) const {
    std::uint16_t message_len_net = htons(static_cast<std::uint16_t>(finalMessage.size()));
    for (int i = 0; i < numPlayers; ++i) {
        playerSockets[i].sendAll(reinterpret_cast<const char *>(&message_len_net), sizeof(message_len_net));
        playerSockets[i].sendAll(finalMessage.c_str(), finalMessage.size());
    }
} 

void Ringmaster::waitForPlayersToAcknowledgeShutdown() const {
    std::vector<struct pollfd> pfds(numPlayers);
    for (int i = 0; i < numPlayers; ++i) {
        pfds[i] = {playerSockets[i].get_fd(), POLLIN, 0};
    }

    int closedPlayers = 0;
    while (closedPlayers < numPlayers) {
        int returnedPlayers = ::poll(pfds.data(), numPlayers, -1);
        if (returnedPlayers < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "Error: poll failed while waiting for players to close connections." << std::endl;
            break;
        }

        for (int i = 0; i < numPlayers; ++i) {
            if (pfds[i].revents & POLLIN) {
                char buf[1024];
                ssize_t bytesRead = playerSockets[i].recvSome(buf, sizeof(buf));
                if (bytesRead <= 0) {
                    closedPlayers++;
                    pfds[i].fd = -1; // Mark this player's socket as closed
                }
            }
        }
    }
}

void Ringmaster::tidyUp(const std::string & finalMessage) const {
    sendShutdownSignal();
    sendFinalMessage(finalMessage);
    waitForPlayersToAcknowledgeShutdown();
}

void Ringmaster::endGame(const Potato & potato, int gameInfo) const {
    std::string finalMessage = "Game over. Shutting down...";
    if (gameInfo == 0) {
        tidyUp(finalMessage);
    }
    else {
        int finalHops = potato.getHops();
        if (finalHops < 0) {
            std::cerr << "Error: Failed to receive the final potato from the players." << std::endl;
            return;
        }

        printTrace(potato);
        tidyUp(finalMessage);
    }
}