#pragma once

#include "Elephants/Renderer.hpp"
#include "Elephants/Simulation.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Keyboard.hpp>

namespace elephants {

class Game {
public:
    Game();

    void run();

private:
    void handleEvents();
    void handleKeyPressed(sf::Keyboard::Key key);
    void handleKeyReleased(sf::Keyboard::Key key);
    void refreshSpeedMode();
    void refreshMovementInput();
    void refreshMapRenderMode();
    void update(float dt);
    void render();
    void refreshWindowTitle();

    sf::RenderWindow window_;
    Simulation simulation_;
    Renderer renderer_;
    bool paused_ = false;
    bool leftShiftHeld_ = false;
    bool rightShiftHeld_ = false;
    bool fastToggled_ = false;
    bool fastToggleKeyHeld_ = false;
    bool forwardHeld_ = false;
    bool backwardHeld_ = false;
    bool turnLeftHeld_ = false;
    bool turnRightHeld_ = false;
    MapRenderMode mapRenderMode_ = MapRenderMode::Terrain;
    float titleRefreshTimer_ = 0.0F;
};

} // namespace elephants
