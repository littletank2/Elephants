#include "Elephants/Game.hpp"

#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>

namespace elephants {
namespace {

constexpr float minCameraZoom = 0.55F;
constexpr float maxCameraZoom = 2.20F;

sf::Vector2f herdVisualPosition(const Herd& herd) {
    const float progress = std::clamp(herd.moveProgress, 0.0F, 1.0F);
    return herd.previousPosition + (herd.position - herd.previousPosition) * progress;
}

} // namespace

Game::Game()
    : window_(sf::VideoMode({1280U, 900U}), "Elephants")
    , simulation_()
    , renderer_(simulation_.grid())
    , worldView_(window_.getDefaultView())
    , baseViewSize_(worldView_.getSize()) {
    cameraZoom_ = 0.78F;
    window_.setFramerateLimit(60);
    worldView_.setSize(baseViewSize_ * cameraZoom_);
    worldView_.setCenter(simulation_.herd().position);
    clampCameraToWorld();
    refreshMapRenderMode();
    refreshWindowTitle();
}

void Game::run() {
    sf::Clock clock;

    while (window_.isOpen()) {
        const float dt = clock.restart().asSeconds();
        handleEvents();
        update(dt);
        render();
    }
}

void Game::handleEvents() {
    while (const std::optional event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
            continue;
        }

        if (const auto* pressed = event->getIf<sf::Event::KeyPressed>()) {
            handleKeyPressed(pressed->code);
        } else if (const auto* released = event->getIf<sf::Event::KeyReleased>()) {
            handleKeyReleased(released->code);
        } else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
            handleMouseWheelScrolled(*wheel);
        } else if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            handleMouseButtonPressed(*mousePressed);
        } else if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
            handleMouseButtonReleased(*mouseReleased);
        } else if (const auto* moved = event->getIf<sf::Event::MouseMoved>()) {
            handleMouseMoved(*moved);
        }
    }
}

void Game::handleKeyPressed(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Key::Escape) {
        window_.close();
        return;
    }

    if (key == sf::Keyboard::Key::LShift) {
        leftShiftHeld_ = true;
        refreshSpeedMode();
        refreshWindowTitle();
        return;
    }

    if (key == sf::Keyboard::Key::RShift) {
        rightShiftHeld_ = true;
        refreshSpeedMode();
        refreshWindowTitle();
        return;
    }

    if (key == sf::Keyboard::Key::F) {
        if (!fastToggleKeyHeld_) {
            fastToggled_ = !fastToggled_;
            refreshSpeedMode();
            refreshWindowTitle();
        }

        fastToggleKeyHeld_ = true;
        return;
    }

    if (key == sf::Keyboard::Key::M) {
        mapRenderMode_ = mapRenderMode_ == MapRenderMode::Terrain ? MapRenderMode::GrassHue : MapRenderMode::Terrain;
        refreshMapRenderMode();
        refreshWindowTitle();
        return;
    }

    if (key == sf::Keyboard::Key::Space) {
        paused_ = !paused_;
        refreshWindowTitle();
        return;
    }

    if (key == sf::Keyboard::Key::R) {
        simulation_.reset();
        worldView_.setCenter(simulation_.herd().position);
        cameraZoom_ = 0.78F;
        worldView_.setSize(baseViewSize_ * cameraZoom_);
        clampCameraToWorld();
        refreshSpeedMode();
        refreshMovementInput();
        refreshMapRenderMode();
        paused_ = false;
        refreshWindowTitle();
        return;
    }

    if (key == sf::Keyboard::Key::W) {
        forwardHeld_ = true;
    } else if (key == sf::Keyboard::Key::S) {
        backwardHeld_ = true;
    } else if (key == sf::Keyboard::Key::A) {
        turnLeftHeld_ = true;
    } else if (key == sf::Keyboard::Key::D) {
        turnRightHeld_ = true;
    } else if (key == sf::Keyboard::Key::Up) {
        cameraUpHeld_ = true;
    } else if (key == sf::Keyboard::Key::Down) {
        cameraDownHeld_ = true;
    } else if (key == sf::Keyboard::Key::Left) {
        cameraLeftHeld_ = true;
    } else if (key == sf::Keyboard::Key::Right) {
        cameraRightHeld_ = true;
    }

    refreshMovementInput();
}

void Game::handleKeyReleased(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Key::LShift) {
        leftShiftHeld_ = false;
        refreshSpeedMode();
        refreshWindowTitle();
    } else if (key == sf::Keyboard::Key::RShift) {
        rightShiftHeld_ = false;
        refreshSpeedMode();
        refreshWindowTitle();
    } else if (key == sf::Keyboard::Key::F) {
        fastToggleKeyHeld_ = false;
    } else if (key == sf::Keyboard::Key::W) {
        forwardHeld_ = false;
        refreshMovementInput();
    } else if (key == sf::Keyboard::Key::S) {
        backwardHeld_ = false;
        refreshMovementInput();
    } else if (key == sf::Keyboard::Key::A) {
        turnLeftHeld_ = false;
        refreshMovementInput();
    } else if (key == sf::Keyboard::Key::D) {
        turnRightHeld_ = false;
        refreshMovementInput();
    } else if (key == sf::Keyboard::Key::Up) {
        cameraUpHeld_ = false;
    } else if (key == sf::Keyboard::Key::Down) {
        cameraDownHeld_ = false;
    } else if (key == sf::Keyboard::Key::Left) {
        cameraLeftHeld_ = false;
    } else if (key == sf::Keyboard::Key::Right) {
        cameraRightHeld_ = false;
    }
}

void Game::handleMouseWheelScrolled(const sf::Event::MouseWheelScrolled& event) {
    if (event.wheel != sf::Mouse::Wheel::Vertical || event.delta == 0.0F) {
        return;
    }

    const float factor = std::pow(0.90F, event.delta);
    zoomCamera(factor, event.position);
}

void Game::handleMouseButtonPressed(const sf::Event::MouseButtonPressed& event) {
    if (event.button != sf::Mouse::Button::Middle) {
        return;
    }

    cameraDragging_ = true;
    lastMousePosition_ = event.position;
}

void Game::handleMouseButtonReleased(const sf::Event::MouseButtonReleased& event) {
    if (event.button != sf::Mouse::Button::Middle) {
        return;
    }

    cameraDragging_ = false;
}

void Game::handleMouseMoved(const sf::Event::MouseMoved& event) {
    if (!cameraDragging_) {
        return;
    }

    const sf::Vector2i currentPosition = event.position;
    const sf::Vector2i delta = currentPosition - lastMousePosition_;
    lastMousePosition_ = currentPosition;

    if (delta.x == 0 && delta.y == 0) {
        return;
    }

    const sf::Vector2f windowSize = sf::Vector2f{static_cast<float>(window_.getSize().x), static_cast<float>(window_.getSize().y)};
    const sf::Vector2f viewSize = worldView_.getSize();
    const sf::Vector2f movement{
        -static_cast<float>(delta.x) * viewSize.x / std::max(1.0F, windowSize.x),
        -static_cast<float>(delta.y) * viewSize.y / std::max(1.0F, windowSize.y),
    };

    worldView_.move(movement);
    clampCameraToWorld();
}

void Game::refreshSpeedMode() {
    const bool fast = leftShiftHeld_ || rightShiftHeld_ || fastToggled_;
    simulation_.setSpeedMode(fast ? HerdSpeedMode::Fast : HerdSpeedMode::Slow);
}

void Game::refreshMovementInput() {
    simulation_.setMovementInput(forwardHeld_, backwardHeld_, turnLeftHeld_, turnRightHeld_);
}

void Game::refreshCamera(float dt) {
    const sf::Vector2f movement{
        (cameraRightHeld_ ? 1.0F : 0.0F) - (cameraLeftHeld_ ? 1.0F : 0.0F),
        (cameraDownHeld_ ? 1.0F : 0.0F) - (cameraUpHeld_ ? 1.0F : 0.0F),
    };

    if (movement.x != 0.0F || movement.y != 0.0F) {
        const float length = std::sqrt(movement.x * movement.x + movement.y * movement.y);
        worldView_.move((movement / length) * (cameraSpeed_ * dt));
    }

    if (!cameraDragging_) {
        followHerd(dt);
    }

    clampCameraToWorld();
}

void Game::followHerd(float dt) {
    const sf::Vector2f focus = herdVisualPosition(simulation_.herd());
    const sf::Vector2f viewSize = worldView_.getSize();
    const sf::Vector2f deadzoneHalf = viewSize / 6.0F;
    const sf::Vector2f currentCenter = worldView_.getCenter();
    sf::Vector2f desiredCenter = currentCenter;

    const sf::Vector2f minCorner = currentCenter - deadzoneHalf;
    const sf::Vector2f maxCorner = currentCenter + deadzoneHalf;

    if (focus.x < minCorner.x) {
        desiredCenter.x -= minCorner.x - focus.x;
    } else if (focus.x > maxCorner.x) {
        desiredCenter.x += focus.x - maxCorner.x;
    }

    if (focus.y < minCorner.y) {
        desiredCenter.y -= minCorner.y - focus.y;
    } else if (focus.y > maxCorner.y) {
        desiredCenter.y += focus.y - maxCorner.y;
    }

    const float followStrength = 8.5F;
    const float blend = 1.0F - std::exp(-followStrength * dt);
    worldView_.setCenter(currentCenter + (desiredCenter - currentCenter) * std::clamp(blend, 0.0F, 1.0F));
}

void Game::clampCameraToWorld() {
    const sf::FloatRect bounds = simulation_.grid().pixelBounds();
    if (bounds.size.x <= 0.0F || bounds.size.y <= 0.0F) {
        return;
    }

    const sf::Vector2f size = worldView_.getSize();
    sf::Vector2f center = worldView_.getCenter();

    const float halfWidth = size.x * 0.5F;
    const float halfHeight = size.y * 0.5F;

    if (size.x >= bounds.size.x) {
        center.x = bounds.position.x + bounds.size.x * 0.5F;
    } else {
        center.x = std::clamp(center.x, bounds.position.x + halfWidth, bounds.position.x + bounds.size.x - halfWidth);
    }

    if (size.y >= bounds.size.y) {
        center.y = bounds.position.y + bounds.size.y * 0.5F;
    } else {
        center.y = std::clamp(center.y, bounds.position.y + halfHeight, bounds.position.y + bounds.size.y - halfHeight);
    }

    worldView_.setCenter(center);
}

void Game::zoomCamera(float factor, sf::Vector2i focusPixel) {
    factor = std::clamp(factor, 0.5F, 2.0F);

    const sf::Vector2f before = window_.mapPixelToCoords(focusPixel, worldView_);
    cameraZoom_ = std::clamp(cameraZoom_ * factor, minCameraZoom, maxCameraZoom);
    worldView_.setSize(baseViewSize_ * cameraZoom_);
    const sf::Vector2f after = window_.mapPixelToCoords(focusPixel, worldView_);
    worldView_.move(before - after);
    clampCameraToWorld();
}

void Game::refreshMapRenderMode() {
    renderer_.setMapRenderMode(mapRenderMode_);
}

void Game::update(float dt) {
    refreshMovementInput();
    refreshCamera(dt);

    if (!paused_) {
        simulation_.update(dt);
    }

    titleRefreshTimer_ += dt;
    if (titleRefreshTimer_ >= 0.25F) {
        refreshWindowTitle();
        titleRefreshTimer_ = 0.0F;
    }
}

void Game::render() {
    window_.clear(sf::Color(22, 28, 25));
    renderer_.draw(window_, simulation_, paused_, worldView_);
    window_.display();
}

void Game::refreshWindowTitle() {
    std::string title = "Elephants - " + simulation_.statusText();
    title += mapRenderMode_ == MapRenderMode::Terrain ? " | MAP TERRAIN" : " | MAP GRASS HUE";

    if (paused_) {
        title += " - PAUSED";
    }

    window_.setTitle(title);
}

} // namespace elephants



