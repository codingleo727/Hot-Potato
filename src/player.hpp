#pragma once
#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <cstdint>
#include <string>
#include <vector>
#include "Socket.hpp"
#include "potato.hpp"

class Player {
public:
    Player(int port);

    /**
     * Start the player by connecting to the ringmaster, sending the player's own information to the ringmaster, 
     * receiving the neighbor information from the ringmaster, and connecting to the neighbor players.
     * @param ringmasterAddress the IP address of the ringmaster to connect to
     * @param ringmasterPort the port number of the ringmaster to connect to
     */
    void start(const std::string & ringmasterAddress, std::uint16_t ringmasterPort);
    /**
     * The main game loop for the player, which will wait for potatoes to be received from either the ringmaster or a neighbor player, 
     * and then pass the potatoes to either the ringmaster or a neighbor player depending on the state of the potato. 
     * @return 1 if the potato is successfully passed to the next player, 0 if there are no hops in the potato remaining, 
     * -1 if an error occurs while waiting for or receiving a potato, -2 if a shutdown signal is received
     */
    int middleGame() const;
    /**
     * Send a shutdown acknowledgement to the ringmaster to confirm that the player has received the shutdown signal and is ready to exit.
     */
    void end();
    /**
     * Get the player's own ID, which is assigned by the ringmaster and received from the ringmaster during the start() function.
     * @return the player's own ID
     */
    std::uint16_t get_id() const;
private:
    Socket leftPlayer;
    Socket rightPlayer;
    Socket ringmaster;
    Socket mySocket;
    std::uint16_t port_;
    std::uint16_t my_id;
    std::uint16_t numPlayers;

    struct PlayerInfo {
        int id = -1;
        std::string address = "";
        std::uint16_t port = 0;
    };
    std::vector<PlayerInfo> neighborInfos;

    /**
     * Open a listening socket on an available port and store the port number in the port_ member variable. 
     * This function should be called before sending the player's own port number to the ringmaster.
     */
    void openListeningSocket();
    /**
     * Connect to the ringmaster using the provided address and port number. 
     * This function will create a new Socket object representing the connection to the ringmaster and store it in the ringmaster member variable.
     * @param ringmasterAddress the IP address of the ringmaster to connect to
     * @param ringmasterPort the port number of the ringmaster to connect to
     */
    void connectToRingmaster(const std::string & ringmasterAddress, std::uint16_t ringmasterPort);
    /**
     * Connect to the neighbor players using the provided vector of PlayerInfo structs, which contain the neighbors' IP addresses and port numbers. 
     * This function will create new Socket objects representing the connections to the neighbor players and store them in the leftPlayer and rightPlayer member variables.
     * @param neighborInfos a vector of PlayerInfo structs, where the first element is the right neighbor's information and the second element is the left neighbor's information
     */
    void connectToNeighbors(const std::vector<PlayerInfo> & neighborInfos);
    /**
     * Connect to a neighbor player using the provided PlayerInfo, which contains the neighbor's IP address and port number. 
     * This function will create a new Socket object representing the connection to the neighbor player and return it.
     * @param info the PlayerInfo struct containing the neighbor's IP address and port number
     * @return a Socket object representing the connection to the neighbor player
     */
    Socket connectToNeighbor(const PlayerInfo & info);
    /**
     * Accept a connection from a neighbor player using the provided PlayerInfo, which contains the neighbor's IP address and port number. 
     * This function will block until a connection is accepted, and then create a new Socket object representing the connection to the neighbor player 
     * and store it in the leftPlayer or rightPlayer member variable depending on whether the neighbor is the left or right neighbor.
     * @param neighborInfo the PlayerInfo struct containing the neighbor's IP address and port number
     */
    Socket acceptNeighborConnection(const PlayerInfo & neighborInfo);
    
    /**
     * Get the port number that the player is listening on. 
     * This function should be called after the player has opened a listening socket and obtained its port number.
     */
    void getPort();
    /**
     * Send the player's own port number to the ringmaster. 
     * This function should be called after the player has opened a listening socket and obtained its port number.
     */
    void sendInfoToRingmaster() const;

    /**
     * Receive the player's own ID from the ringmaster.
     */
    void receiveMyId();
    /**
     * Receive the total number of players in the game from the ringmaster.
     */
    void receiveTotalNumberOfPlayers();
    /**
     * Receive the neighbor information string from the ringmaster.
     * @return the received neighbor information string
     */
    std::uint16_t receiveInfoLength();
    /**
     * Receive the neighbor information string from the ringmaster, blocking until the entire string is received. 
     * The length of the string is determined by the receiveInfoLength() function.
     * @return the received neighbor information string
     */
    std::string receiveInfoString();
    /**
     * Parse a neighbor information string in the format "ID:IP:port" and extract the player's ID, IP address, and port number. 
     * @param playerInfo the neighbor information string to parse
     * @return a PlayerInfo struct containing the parsed ID, IP address, and port number
     */
    PlayerInfo parseString(const std::string & playerInfo);
    /**
     * Receive the neighbor information from the ringmaster, including the neighbor's IP address and port number.
     * Pre-condition: The received information is in the format "IP:port\nIP:port", where the first line is the right neighbor's information and the second line is the left neighbor's information.
     * @return a vector of PlayerInfo structs, where the first element is the right neighbor's information and the second element is the left neighbor's information
     */
    std::vector<PlayerInfo> receiveInfoFromRingmaster();

    /**
     * Receive a potato from either the ringmaster or a neighbor player. 
     * This function will block until a potato is received, and then return the received Potato object.
     * @return the received Potato object, or a Potato with -1 hops if an error occurs while waiting for or receiving the potato
     */
    Potato receivePotato() const;
    /**
     * Pass the given potato to either the ringmaster or a neighbor player, depending on the state of the potato. If the potato's hops are 0, it should be sent back to the ringmaster. 
     * If the potato's hops are greater than 0, it should be sent to a randomly chosen neighbor player. 
     * The player's own ID should be added to the potato's trace before passing it on.
     * @param potato the Potato object to pass
     * @return 0 if the potato was sent back to the ringmaster, 1 if the potato was sent to a neighbor player, -1 if an error occurs while passing the potato
     */
    int passPotato(Potato & potato) const;
    /**
     * Receive a final message from the ringmaster indicating that the game is over, and print the message to standard output.
     */
    void receiveGameOver();
    /**
     * Send a shutdown acknowledgement to the ringmaster to indicate that the player has received the shutdown signal and is ready to exit. 
     * This function should be called after receiving the shutdown signal from the ringmaster and before closing any connections or exiting the program.
     */
    void sendShutdownAcknowledgement() const;
};
#endif