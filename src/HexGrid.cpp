#include "Elephants/HexGrid.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace elephants {
namespace {

constexpr float sqrt3 = 1.7320508075688772F;

int axialDistanceFromOrigin(HexCoord coord) {
    const int s = -coord.q - coord.r;
    return (std::abs(coord.q) + std::abs(coord.r) + std::abs(s)) / 2;
}

HexCoord roundAxial(float q, float r) {
    const float x = q;
    const float z = r;
    const float y = -x - z;

    int rx = static_cast<int>(std::round(x));
    int ry = static_cast<int>(std::round(y));
    int rz = static_cast<int>(std::round(z));

    const float xDiff = std::abs(static_cast<float>(rx) - x);
    const float yDiff = std::abs(static_cast<float>(ry) - y);
    const float zDiff = std::abs(static_cast<float>(rz) - z);

    if (xDiff > yDiff && xDiff > zDiff) {
        rx = -ry - rz;
    } else if (yDiff > zDiff) {
        ry = -rx - rz;
    } else {
        rz = -rx - ry;
    }

    return {rx, rz};
}

} // namespace

HexGrid::HexGrid(int radius, float hexSize)
    : radius_(radius)
    , hexSize_(hexSize) {
    coords_.reserve(static_cast<std::size_t>(1 + 3 * radius * (radius + 1)));

    for (int q = -radius_; q <= radius_; ++q) {
        const int minR = std::max(-radius_, -q - radius_);
        const int maxR = std::min(radius_, -q + radius_);

        for (int r = minR; r <= maxR; ++r) {
            coords_.push_back({q, r});
        }
    }
}

int HexGrid::radius() const {
    return radius_;
}

float HexGrid::hexSize() const {
    return hexSize_;
}

const std::vector<HexCoord>& HexGrid::coords() const {
    return coords_;
}

bool HexGrid::contains(HexCoord coord) const {
    return axialDistanceFromOrigin(coord) <= radius_;
}

std::size_t HexGrid::indexOf(HexCoord coord) const {
    if (!contains(coord)) {
        throw std::out_of_range("Hex coordinate outside of grid");
    }

    for (std::size_t i = 0; i < coords_.size(); ++i) {
        if (coords_[i] == coord) {
            return i;
        }
    }

    throw std::out_of_range("Hex coordinate not indexed");
}

sf::Vector2f HexGrid::toPixel(HexCoord coord) const {
    const float x = hexSize_ * sqrt3 * (static_cast<float>(coord.q) + static_cast<float>(coord.r) / 2.0F);
    const float y = hexSize_ * 1.5F * static_cast<float>(coord.r);
    return {x, y};
}

sf::FloatRect HexGrid::pixelBounds() const {
    if (coords_.empty()) {
        return {};
    }

    float minX = toPixel(coords_.front()).x;
    float maxX = minX;
    float minY = toPixel(coords_.front()).y;
    float maxY = minY;

    for (HexCoord coord : coords_) {
        const sf::Vector2f pixel = toPixel(coord);
        minX = std::min(minX, pixel.x);
        maxX = std::max(maxX, pixel.x);
        minY = std::min(minY, pixel.y);
        maxY = std::max(maxY, pixel.y);
    }

    const float padding = hexSize_;
    return {{minX - padding, minY - padding}, {(maxX - minX) + padding * 2.0F, (maxY - minY) + padding * 2.0F}};
}

HexCoord HexGrid::fromPixel(sf::Vector2f pixel) const {
    const float q = (sqrt3 / 3.0F * pixel.x - pixel.y / 3.0F) / hexSize_;
    const float r = (2.0F / 3.0F * pixel.y) / hexSize_;
    return roundAxial(q, r);
}

std::array<HexCoord, 6> HexGrid::neighbors(HexCoord coord) const {
    const auto dirs = directions();
    return {
        coord + dirs[0],
        coord + dirs[1],
        coord + dirs[2],
        coord + dirs[3],
        coord + dirs[4],
        coord + dirs[5],
    };
}

} // namespace elephants
