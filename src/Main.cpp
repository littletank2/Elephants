#include "Elephants/Game.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

int main() {
    try {
        elephants::Game game;
        game.run();
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
