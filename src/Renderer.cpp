#include "Elephants/Renderer.hpp"

#include <SFML/Graphics/CircleShape.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace elephants {
namespace {

constexpr float pi = 3.14159265358979323846F;

sf::Color mix(sf::Color a, sf::Color b, float t) {
    t = std::clamp(t, 0.0F, 1.0F);
    const auto channel = [t](std::uint8_t left, std::uint8_t right) {
        return static_cast<std::uint8_t>(static_cast<float>(left) + (static_cast<float>(right) - static_cast<float>(left)) * t);
    };

    return {
        channel(a.r, b.r),
        channel(a.g, b.g),
        channel(a.b, b.b),
        channel(a.a, b.a),
    };
}

float bodyRadiusFor(const Herd& herd) {
    return std::clamp(9.0F + herd.size * 0.24F, 11.0F, 27.0F);
}

} // namespace

Renderer::Renderer(const HexGrid& grid)
    : grid_(grid) {}

void Renderer::setMapRenderMode(MapRenderMode mode) {
    mapRenderMode_ = mode;
}

void Renderer::draw(sf::RenderWindow& window, const Simulation& simulation, bool paused) {
    const sf::Vector2f offset = boardCenterOffset(window.getSize());
    drawTiles(window, simulation, offset);
    drawHerd(window, simulation, offset);
    drawHud(window, simulation, paused);
}

sf::Vector2f Renderer::boardCenterOffset(sf::Vector2u windowSize) const {
    return {
        static_cast<float>(windowSize.x) * 0.5F,
        static_cast<float>(windowSize.y) * 0.54F,
    };
}

sf::ConvexShape Renderer::makeHex(sf::Vector2f center, float radius) const {
    sf::ConvexShape shape;
    shape.setPointCount(6);

    for (std::size_t i = 0; i < 6; ++i) {
        const float angle = pi / 180.0F * (60.0F * static_cast<float>(i) - 30.0F);
        shape.setPoint(i, {center.x + radius * std::cos(angle), center.y + radius * std::sin(angle)});
    }

    return shape;
}

sf::Color Renderer::tileColor(const Tile& tile) const {
    switch (tile.terrain) {
    case TerrainType::Forest:
        return {33, 92, 49};
    case TerrainType::Grassland:
        return {108, 159, 71};
    case TerrainType::DryGrassland:
        return {168, 157, 83};
    case TerrainType::Water:
        return {48, 105, 151};
    case TerrainType::Desert:
        return {196, 171, 94};
    case TerrainType::Swamp:
        return {60, 112, 87};
    }

    return sf::Color::Magenta;
}

float Renderer::vegetationCapacity(const Tile& tile) const {
    switch (tile.terrain) {
    case TerrainType::Forest:
        return 7.2F + tile.fertility * 2.6F;
    case TerrainType::Grassland:
        return 8.5F + tile.fertility * 4.2F;
    case TerrainType::DryGrassland:
        return 4.5F + tile.fertility * 2.3F;
    case TerrainType::Water:
        return 0.0F;
    case TerrainType::Desert:
        return 1.0F + tile.fertility * 1.1F;
    case TerrainType::Swamp:
        return 5.5F + tile.fertility * 2.2F;
    }

    return 0.0F;
}

float Renderer::vegetationRatio(const Tile& tile) const {
    return std::clamp(tile.vegetation / std::max(0.01F, vegetationCapacity(tile)), 0.0F, 1.0F);
}

sf::Color Renderer::vegetationColor(const Tile& tile) const {
    const float amount = vegetationRatio(tile);
    const sf::Color low(85, 73, 47, 155);
    const sf::Color high(48, 150, 65, 170);
    return mix(low, high, amount);
}

sf::Color Renderer::grassHueColor(const Tile& tile) const {
    if (tile.terrain == TerrainType::Water) {
        return tileColor(tile);
    }

    const float amount = vegetationRatio(tile);
    const float hue = 0.0F + amount * 120.0F;
    const float saturation = 0.92F;
    const float value = 0.48F + amount * 0.42F;
    return hsvColor(hue, saturation, value, 235);
}

sf::Color Renderer::hsvColor(float hue, float saturation, float value, std::uint8_t alpha) const {
    hue = std::fmod(hue, 360.0F);
    if (hue < 0.0F) {
        hue += 360.0F;
    }

    saturation = std::clamp(saturation, 0.0F, 1.0F);
    value = std::clamp(value, 0.0F, 1.0F);

    const float chroma = value * saturation;
    const float huePrime = hue / 60.0F;
    const float x = chroma * (1.0F - std::fabs(std::fmod(huePrime, 2.0F) - 1.0F));
    const float m = value - chroma;

    float r = 0.0F;
    float g = 0.0F;
    float b = 0.0F;

    if (huePrime < 1.0F) {
        r = chroma;
        g = x;
    } else if (huePrime < 2.0F) {
        r = x;
        g = chroma;
    } else if (huePrime < 3.0F) {
        g = chroma;
        b = x;
    } else if (huePrime < 4.0F) {
        g = x;
        b = chroma;
    } else if (huePrime < 5.0F) {
        r = x;
        b = chroma;
    } else {
        r = chroma;
        b = x;
    }

    return {
        static_cast<std::uint8_t>(std::clamp((r + m) * 255.0F, 0.0F, 255.0F)),
        static_cast<std::uint8_t>(std::clamp((g + m) * 255.0F, 0.0F, 255.0F)),
        static_cast<std::uint8_t>(std::clamp((b + m) * 255.0F, 0.0F, 255.0F)),
        alpha,
    };
}

void Renderer::drawTiles(sf::RenderWindow& window, const Simulation& simulation, sf::Vector2f offset) {
    const auto& tiles = simulation.tiles();
    const auto footprint = simulation.herdFootprint();

    for (std::size_t i = 0; i < grid_.coords().size(); ++i) {
        const HexCoord coord = grid_.coords()[i];
        const Tile& tile = tiles[i];
        const sf::Vector2f center = grid_.toPixel(coord) + offset;

        sf::ConvexShape base = makeHex(center, grid_.hexSize() - 1.0F);
        base.setFillColor(mapRenderMode_ == MapRenderMode::GrassHue ? grassHueColor(tile) : tileColor(tile));
        base.setOutlineColor({20, 27, 23, 210});
        base.setOutlineThickness(1.2F);
        window.draw(base);

        if (mapRenderMode_ == MapRenderMode::Terrain && tile.terrain != TerrainType::Water) {
            sf::ConvexShape vegetation = makeHex(center, (grid_.hexSize() - 7.0F) * std::clamp(vegetationRatio(tile), 0.2F, 1.0F));
            vegetation.setFillColor(vegetationColor(tile));
            window.draw(vegetation);
        }

        if (std::find(footprint.begin(), footprint.end(), coord) != footprint.end()) {
            sf::ConvexShape highlight = makeHex(center, grid_.hexSize() - 2.5F);
            highlight.setFillColor(sf::Color::Transparent);
            highlight.setOutlineColor({246, 232, 148, 210});
            highlight.setOutlineThickness(2.0F);
            window.draw(highlight);
        }
    }
}

void Renderer::drawHerd(sf::RenderWindow& window, const Simulation& simulation, sf::Vector2f offset) {
    const Herd& herd = simulation.herd();
    const sf::Vector2f progressCenter = herd.previousPosition + (herd.position - herd.previousPosition) * std::clamp(herd.moveProgress, 0.0F, 1.0F);
    const sf::Vector2f center = progressCenter + offset;
    const float bodyRadius = bodyRadiusFor(herd);

    sf::CircleShape shadow(bodyRadius * 1.08F);
    shadow.setOrigin({bodyRadius * 1.08F, bodyRadius * 1.08F});
    shadow.setPosition(center + sf::Vector2f{2.0F, 4.0F});
    shadow.setFillColor({9, 12, 11, 90});
    window.draw(shadow);

    sf::CircleShape body(bodyRadius);
    body.setOrigin({bodyRadius, bodyRadius});
    body.setPosition(center);
    body.setFillColor({116, 119, 111});
    body.setOutlineColor({239, 230, 191});
    body.setOutlineThickness(2.0F);
    window.draw(body);

    const sf::Vector2f direction = {std::cos(herd.heading), std::sin(herd.heading)};
    const sf::Vector2f trunkPos = center + direction * (bodyRadius * 0.92F);

    sf::CircleShape trunk(bodyRadius * 0.28F);
    trunk.setOrigin({bodyRadius * 0.28F, bodyRadius * 0.28F});
    trunk.setPosition(trunkPos);
    trunk.setFillColor({91, 94, 88});
    window.draw(trunk);
}

void Renderer::drawHud(sf::RenderWindow& window, const Simulation& simulation, bool paused) {
    const SimulationStats currentStats = simulation.stats();

    sf::RectangleShape panel({318.0F, 58.0F});
    panel.setPosition({18.0F, 18.0F});
    panel.setFillColor({14, 19, 17, 185});
    panel.setOutlineColor({220, 212, 178, 135});
    panel.setOutlineThickness(1.0F);
    window.draw(panel);

    drawBar(window, {34.0F, 32.0F}, {132.0F, 10.0F}, simulation.herd().hunger, {218, 184, 87});
    drawBar(window, {34.0F, 52.0F}, {132.0F, 10.0F}, 1.0F - currentStats.forestRatio, {102, 183, 110});
    drawBar(window, {190.0F, 32.0F}, {112.0F, 10.0F}, currentStats.averageVegetation, {92, 170, 74});

    if (paused || simulation.result() != GameResult::Running) {
        const sf::Color color = simulation.result() == GameResult::Lost ? sf::Color(151, 57, 45, 230) : sf::Color(232, 213, 116, 230);
        sf::RectangleShape marker({34.0F, 34.0F});
        marker.setPosition({284.0F, 38.0F});
        marker.setFillColor(color);
        marker.setOutlineColor({19, 24, 22, 240});
        marker.setOutlineThickness(2.0F);
        window.draw(marker);
    }
}

void Renderer::drawBar(sf::RenderWindow& window, sf::Vector2f position, sf::Vector2f size, float value, sf::Color fill) {
    value = std::clamp(value, 0.0F, 1.0F);

    sf::RectangleShape back(size);
    back.setPosition(position);
    back.setFillColor({36, 43, 39, 230});
    window.draw(back);

    sf::RectangleShape front({size.x * value, size.y});
    front.setPosition(position);
    front.setFillColor(fill);
    window.draw(front);
}

} // namespace elephants
