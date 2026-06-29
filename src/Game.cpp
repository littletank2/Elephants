#include "Elephants/Game.hpp"

#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/VideoMode.hpp>

#include <optional>

namespace elephants {

Game::Game()
    : window_(sf::VideoMode({1280U, 900U}), "Elephants")
    , simulation_()
    , renderer_(simulation_.grid()) {
    window_.setFramerateLimit(60);
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

        if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            handleKeyPressed(key->code);
        }
    }
}

void Game::handleKeyPressed(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Key::Escape) {
        window_.close();
        return;
    }

    if (key == sf::Keyboard::Key::Space) {
        paused_ = !paused_;
        refreshWindowTitle();
        return;
    }

    if (key == sf::Keyboard::Key::R) {
        simulation_.reset();
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
    if (paused_) {
        title += " - PAUSED";
    }

    window_.setTitle(title);
}

} // namespace elephants
