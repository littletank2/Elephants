#pragma once

#include "Elephants/Renderer.hpp"
#include "Elephants/Simulation.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

namespace elephants {

class Game {
public:
    Game();

    void run();

private:
    void handleEvents();
    void handleKeyPressed(sf::Keyboard::Key key);
    void update(float dt);
    void render();
    void refreshWindowTitle();

    sf::RenderWindow window_;
    Simulation simulation_;
    Renderer renderer_;
    bool paused_ = false;
    float titleRefreshTimer_ = 0.0F;
};

} // namespace elephants
