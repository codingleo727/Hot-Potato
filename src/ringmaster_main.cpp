#include <iostream>
#include <cstdlib>
#include "ringmaster.hpp"

int main(int argc, char * argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ringmaster <port> <num_players> <num_hops>" << std::endl;
        return EXIT_FAILURE;
    }
    srand((unsigned int) time(NULL));
    int port = std::stoi(argv[1]);
    int numPlayers = std::stoi(argv[2]);
    int numHops = std::stoi(argv[3]);
    if (numPlayers <= 1) {
        std::cerr << "Number of players must be greater than 1." << std::endl;
        return EXIT_FAILURE;
    }
    if (numHops < 0) {
        std::cerr << "Number of hops must be non-negative." << std::endl;
        return EXIT_FAILURE;
    }
    else if (numHops > 512) {
        std::cerr << "Number of hops must be less than or equal to 512." << std::endl;
        return EXIT_FAILURE;
    }

    try {
        Ringmaster ringmaster(port, numPlayers);
        int gameInfo = ringmaster.startGame(numHops);
        if (gameInfo == 0) {
            Potato potato(0);
            ringmaster.endGame(potato, gameInfo);
        }
        else {
            Potato potato = ringmaster.waitForPotato();
            ringmaster.endGame(potato, gameInfo);
        }
    } catch (const std::exception & e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}