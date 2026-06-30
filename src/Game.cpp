#include "Elephants/Game.hpp"

#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/VideoMode.hpp>

#include <optional>
#include <string>

namespace elephants {

Game::Game()
    : window_(sf::VideoMode({1280U, 900U}), "Elephants")
    , simulation_()
    , renderer_(simulation_.grid()) {
    window_.setFramerateLimit(60);
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
        refreshSpeedMode();
        refreshMapRenderMode();
        paused_ = false;
        refreshWindowTitle();
        return;
    }

    if (key == sf::Keyboard::Key::E) {
        simulation_.setDirection({1, 0});
    } else if (key == sf::Keyboard::Key::W) {
        simulation_.setDirection({1, -1});
    } else if (key == sf::Keyboard::Key::Q) {
        simulation_.setDirection({0, -1});
    } else if (key == sf::Keyboard::Key::A) {
        simulation_.setDirection({-1, 0});
    } else if (key == sf::Keyboard::Key::S) {
        simulation_.setDirection({-1, 1});
    } else if (key == sf::Keyboard::Key::D) {
        simulation_.setDirection({0, 1});
    }
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
    }
}

void Game::refreshSpeedMode() {
    const bool fast = leftShiftHeld_ || rightShiftHeld_ || fastToggled_;
    simulation_.setSpeedMode(fast ? HerdSpeedMode::Fast : HerdSpeedMode::Slow);
}

void Game::refreshMapRenderMode() {
    renderer_.setMapRenderMode(mapRenderMode_);
}

void Game::update(float dt) {
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
    renderer_.draw(window_, simulation_, paused_);
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
