#include <iostream>
#include <cstdlib>
#include "player.hpp"

int main (int argc, char * argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: player <ringmaster_address> <ringmaster_port>" << std::endl;
        return EXIT_FAILURE;
    }
    std::string ringmasterAddress = argv[1];
    int ringmasterPort = std::stoi(argv[2]);
    if (ringmasterPort <= 0 || ringmasterPort > 65535) {
        std::cerr << "Invalid ringmaster port number." << std::endl;
        return EXIT_FAILURE;
    }

    try {
        Player player(0); // Use port 0 to let the OS choose an available port
        player.start(ringmasterAddress, static_cast<std::uint16_t>(ringmasterPort));
        srand((unsigned int) time(NULL) + player.get_id());
        while (true) {
            int result = player.middleGame();
            if (result == -1) {
                std::cerr << "Error: Failed to receive a valid potato. Continuing to wait for potatoes.\n";
                continue; // Continue waiting for valid potatoes
            }
            else if (result == -2) {
                break; // Exit the game loop if a shutdown signal is received
            }
        }
        player.end();
    } catch (const std::exception & e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}