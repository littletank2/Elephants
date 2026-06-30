#pragma once

#include "Elephants/HexGrid.hpp"
#include "Elephants/Simulation.hpp"
#include "Elephants/Types.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>

#include <cstdint>

namespace elephants {

enum class MapRenderMode {
    Terrain,
    GrassHue,
};

class Renderer {
public:
    explicit Renderer(const HexGrid& grid);

    void draw(sf::RenderWindow& window, const Simulation& simulation, bool paused);
    void setMapRenderMode(MapRenderMode mode);

private:
    [[nodiscard]] sf::Vector2f boardCenterOffset(sf::Vector2u windowSize) const;
    [[nodiscard]] sf::ConvexShape makeHex(sf::Vector2f center, float radius) const;
    [[nodiscard]] sf::Color tileColor(const Tile& tile) const;
    [[nodiscard]] sf::Color vegetationColor(const Tile& tile) const;
    [[nodiscard]] sf::Color grassHueColor(const Tile& tile) const;
    [[nodiscard]] sf::Color hsvColor(float hue, float saturation, float value, std::uint8_t alpha) const;
    [[nodiscard]] float vegetationRatio(const Tile& tile) const;
    [[nodiscard]] float vegetationCapacity(const Tile& tile) const;

    void drawTiles(sf::RenderWindow& window, const Simulation& simulation, sf::Vector2f offset);
    void drawHerd(sf::RenderWindow& window, const Simulation& simulation, sf::Vector2f offset);
    void drawHud(sf::RenderWindow& window, const Simulation& simulation, bool paused);
    void drawBar(sf::RenderWindow& window, sf::Vector2f position, sf::Vector2f size, float value, sf::Color fill);

    const HexGrid& grid_;
    MapRenderMode mapRenderMode_ = MapRenderMode::Terrain;
};

} // namespace elephants
