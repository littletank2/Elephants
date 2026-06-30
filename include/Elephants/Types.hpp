#pragma once

#include <SFML/System/Vector2.hpp>

#include <compare>

namespace elephants {

enum class TerrainType {
    Forest,
    Grassland,
    DryGrassland,
    Water,
    Desert,
    Swamp,
};

enum class HerdSpeedMode {
    Slow,
    Fast,
};

struct HexCoord {
    int q = 0;
    int r = 0;

    /// @brief 
    /// @param  
    /// @return 
    constexpr auto operator<=>(const HexCoord&) const = default;
};

constexpr HexCoord operator+(HexCoord left, HexCoord right) {
    return {left.q + right.q, left.r + right.r};
}

constexpr HexCoord operator*(HexCoord coord, int scale) {
    return {coord.q * scale, coord.r * scale};
}

struct Tile {
    TerrainType terrain = TerrainType::Grassland;
    float rainfall = 0.0F;
    float fertility = 0.5F;
    float vegetation = 0.0F;
    float manure = 0.0F;
};

struct Herd {
    sf::Vector2f previousPosition{0.0F, 0.0F};
    sf::Vector2f position{0.0F, 0.0F};
    HexCoord previousCenter{};
    HexCoord center{};
    float heading = 0.0F;
    float size = 6.0F;
    float hunger = 1.0F;
    float moveProgress = 1.0F;
    HerdSpeedMode speedMode = HerdSpeedMode::Slow;
};

} // namespace elephants
