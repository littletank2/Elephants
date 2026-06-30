#pragma once

#include "Elephants/Renderer.hpp"
#include "Elephants/Simulation.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>
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
    void handleMouseWheelScrolled(const sf::Event::MouseWheelScrolled& event);
    void handleMouseButtonPressed(const sf::Event::MouseButtonPressed& event);
    void handleMouseButtonReleased(const sf::Event::MouseButtonReleased& event);
    void handleMouseMoved(const sf::Event::MouseMoved& event);
    void refreshSpeedMode();
    void refreshMovementInput();
    void refreshCamera(float dt);
    void followHerd(float dt);
    void clampCameraToWorld();
    void zoomCamera(float factor, sf::Vector2i focusPixel);
    void refreshMapRenderMode();
    void update(float dt);
    void render();
    void refreshWindowTitle();

    sf::RenderWindow window_;
    Simulation simulation_;
    Renderer renderer_;
    sf::View worldView_;
    sf::Vector2f baseViewSize_;
    bool paused_ = false;
    bool leftShiftHeld_ = false;
    bool rightShiftHeld_ = false;
    bool fastToggled_ = false;
    bool fastToggleKeyHeld_ = false;
    bool forwardHeld_ = false;
    bool backwardHeld_ = false;
    bool turnLeftHeld_ = false;
    bool turnRightHeld_ = false;
    bool cameraUpHeld_ = false;
    bool cameraDownHeld_ = false;
    bool cameraLeftHeld_ = false;
    bool cameraRightHeld_ = false;
    bool cameraDragging_ = false;
    sf::Vector2i lastMousePosition_{};
    MapRenderMode mapRenderMode_ = MapRenderMode::Terrain;
    float titleRefreshTimer_ = 0.0F;
    float cameraSpeed_ = 720.0F;
    float cameraZoom_ = 1.0F;
};

} // namespace elephants

