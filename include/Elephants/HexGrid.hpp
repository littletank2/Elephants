#pragma once

#include "Elephants/Types.hpp"

#include <SFML/System/Vector2.hpp>

#include <array>
#include <cstddef>
#include <vector>

namespace elephants {

class HexGrid {
public:
    HexGrid(int radius, float hexSize);

    [[nodiscard]] int radius() const;
    [[nodiscard]] float hexSize() const;
    [[nodiscard]] const std::vector<HexCoord>& coords() const;
    [[nodiscard]] bool contains(HexCoord coord) const;
    [[nodiscard]] std::size_t indexOf(HexCoord coord) const;
    [[nodiscard]] sf::Vector2f toPixel(HexCoord coord) const;
    [[nodiscard]] std::array<HexCoord, 6> neighbors(HexCoord coord) const;

    static constexpr std::array<HexCoord, 6> directions() {
        return {
            HexCoord{1, 0},
            HexCoord{1, -1},
            HexCoord{0, -1},
            HexCoord{-1, 0},
            HexCoord{-1, 1},
            HexCoord{0, 1},
        };
    }

private:
    int radius_ = 0;
    float hexSize_ = 0.0F;
    std::vector<HexCoord> coords_;
};

} // namespace elephants
