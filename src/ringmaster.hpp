#pragma once
#ifndef RINGMASTER_HPP
#define RINGMASTER_HPP

#include <vector>
#include <cstdint>
#include <string>
#include "potato.hpp"
#include "Socket.hpp"

class Ringmaster {
public:
    Ringmaster(int port, int numPlayers);

    /**
     * Start the ringmaster by accepting connections from the specified number of players, sending the necessary information to each player, 
     * and then starting the game by sending the initial potato to a random player. 
     * @param numHops the number of hops to set in the initial potato that will be sent to a random player at the start of the game
     * @return 1 if the game was successfully started, 0 if no hops were specified and the game was ended immediately, or -1 if an error occurs while starting the game
     */
    int startGame(int numHops);
    /**
     * Wait for a potato to be received from any player, and then return the received potato.
     * This function will block until a potato is received, and then return the received Potato object.
     * @return the received Potato object, or a Potato with -1 hops if an error occurs while waiting for or receiving the potato
     */
    Potato waitForPotato() const;
    /**
     * Print the trace of the given potato, which is a sequence of player IDs representing the path the potato has taken through the players.
     */
    void endGame(const Potato & potato, int gameInfo) const;
private:
    std::vector<Socket> playerSockets;
    std::uint16_t port_;
    Socket mySocket;
    std::uint16_t numPlayers;

    struct PlayerConnection {
        Socket playerSocket;
        std::string address;
    };

    struct PlayerInfo {
        int id;
        std::string address;
        std::uint16_t port;
    };
    std::vector<PlayerInfo> playerInfos;

    /**
     * Open a listening socket on the specified port and store the port number in the port_ member variable. 
     * This function should be called before accepting any player connections.
     */
    void openListeningSocket();
    
    /**
     * Accept a connection from a player, returning a PlayerConnection struct containing the accepted Socket and the player's IP address. 
     * This function will block until a connection is accepted.
     * @return a PlayerConnection struct containing the accepted Socket and the player's IP address
     */
    PlayerConnection acceptPlayer();
    /**
     * Initialize the player connections by accepting connections from the specified number of players and storing their information in the playerInfos vector. 
     * This function should be called after opening the listening socket and before sending any information to the players.
     */
    void initializePlayers();
    /**
     * Construct a neighbor information string in the format "ID:IP:port" for the given PlayerInfo struct, which contains the player's ID, IP address, and port number. 
     * This function is used to create the neighbor information string that will be sent to each player.
     * @param neighbor the PlayerInfo struct containing the player's ID, IP address, and port number
     * @return a string in the format "ID:IP:port" representing the player's neighbor information
     */
    std::string getNeighborInfo(const PlayerInfo & neighbor) const;
    /**
     * Send the player's own information, including their ID, to the player using the provided Socket. 
     * This function should be called for each player after accepting their connection and before sending any other information to the players.
     * @param playerInfo the PlayerInfo struct containing the player's ID, IP address, and port number
     * @param playerSocket the Socket object representing the connection to the player
     */
    void sendPlayerOwnInfo(const PlayerInfo & playerInfo, const Socket & playerSocket) const;
    /**
     * Send the total number of players in the game to the player using the provided Socket. 
     * This function should be called for each player after sending their own information and before sending any neighbor information to the players.
     * @param playerSocket the Socket object representing the connection to the player
     */
    void sendTotalNumberOfPlayers(const Socket & playerSocket) const;
    /**
     * Send the necessary information to each player, including their own ID, the total number of players, 
     * and the neighbor information for their right and left neighbors. 
     * This function should be called after accepting all player connections and before starting the game.
     */
    void sendInfoToPlayers() const;

    /**
     * Create a new Potato object with the specified number of hops and return it. 
     * This function is used to create the initial potato that will be sent to a random player at the start of the game.
     * @param numHops the number of hops to set in the created Potato object
     * @return a new Potato object with the specified number of hops
     */
    Potato createPotato(int numHops) const;
    /**
     * Send the given potato to a randomly chosen player. 
     * The potato's hops should be decremented before sending it. 
     * This function is used to send the initial potato to a random player at the start of the game, 
     * and can also be used to send a potato back to a player if it is received with 0 hops.
     * @param potato the Potato object to send
     * @return the ID of the player to whom the potato was sent, or -1 if an error occurs while sending the potato
     */
    int sendPotato(Potato & potato) const;

    /**
     * Print the trace of the given potato, which is a sequence of player IDs representing the path the potato has taken through the players. 
     * The trace should be printed in a comma-separated format, with a header line indicating that it is the trace of the potato.
     * @param potato the Potato object whose trace is to be printed
     */
    void printTrace(const Potato & potato) const;

    /**
     * Send a shutdown signal to all players to indicate that the game is over and they should exit. 
     * This function should be called after the game is over and before waiting for the players to acknowledge the shutdown signal.
     */
    void sendShutdownSignal() const;
    /**
     * Send a final message to all players before shutting down the game. 
     * This function should be called after sending the shutdown signal and before waiting for acknowledgements from players.
     * @param finalMessage the final message to send to all players
     */
    void sendFinalMessage(const std::string & finalMessage) const;
    /**
     * Wait for acknowledgements from all players to confirm that they have received the shutdown signal and are ready to exit. 
     * This function should be called after sending the shutdown signal and before tidying up any resources or exiting the program.
     */
    void waitForPlayersToAcknowledgeShutdown() const;
    /**
     * Perform any necessary cleanup after the game is over, such as closing any open connections or releasing any resources. 
     * This function should be called after waiting for the players to acknowledge the shutdown signal and before exiting the program.
     */
    void tidyUp(const std::string & finalMessage) const;
};
#endif