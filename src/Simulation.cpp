#include "Elephants/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <random>
#include <sstream>

namespace elephants {
namespace {

constexpr float pi = 3.14159265358979323846F;

float clamp01(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

float smoothNoise(HexCoord coord, float seed) {
    const float a = std::sin(static_cast<float>(coord.q) * 12.9898F + static_cast<float>(coord.r) * 78.233F + seed) * 43758.5453F;
    return a - std::floor(a);
}

float distanceFromOrigin(HexCoord coord) {
    const int s = -coord.q - coord.r;
    return static_cast<float>((std::abs(coord.q) + std::abs(coord.r) + std::abs(s)) / 2);
}

bool isWaterLike(TerrainType terrain) {
    return terrain == TerrainType::Water;
}

float vectorLength(sf::Vector2f value) {
    return std::sqrt(value.x * value.x + value.y * value.y);
}

} // namespace

Simulation::Simulation(SimulationConfig config)
    : config_(config)
    , grid_(config_.mapRadius, config_.hexSize)
    , rng_(std::random_device{}()) {
    reset();
}

void Simulation::reset() {
    tiles_.assign(grid_.coords().size(), {});
    result_ = GameResult::Running;
    accumulator_ = 0.0F;
    lastFoodRatio_ = 1.0F;
    forwardHeld_ = false;
    backwardHeld_ = false;
    turnLeftHeld_ = false;
    turnRightHeld_ = false;
    generateMap();
    placeHerd();
}

void Simulation::update(float dt) {
    if (result_ != GameResult::Running) {
        return;
    }

    accumulator_ += dt;
    float stepSeconds = currentTickSeconds();
    while (accumulator_ >= stepSeconds) {
        step();
        accumulator_ -= stepSeconds;
        stepSeconds = currentTickSeconds();
    }

    herd_.moveProgress = std::clamp(accumulator_ / std::max(0.001F, stepSeconds), 0.0F, 1.0F);
}

void Simulation::setDirection(HexCoord direction) {
    const auto dirs = HexGrid::directions();
    if (std::find(dirs.begin(), dirs.end(), direction) != dirs.end()) {
        const sf::Vector2f pixel = grid_.toPixel(direction);
        herd_.heading = normalizeAngle(std::atan2(pixel.y, pixel.x));
    }
}

void Simulation::setSpeedMode(HerdSpeedMode mode) {
    herd_.speedMode = mode;
}

void Simulation::setMovementInput(bool forward, bool backward, bool turnLeft, bool turnRight) {
    forwardHeld_ = forward;
    backwardHeld_ = backward;
    turnLeftHeld_ = turnLeft;
    turnRightHeld_ = turnRight;
}

const HexGrid& Simulation::grid() const {
    return grid_;
}

const std::vector<Tile>& Simulation::tiles() const {
    return tiles_;
}

const Herd& Simulation::herd() const {
    return herd_;
}

GameResult Simulation::result() const {
    return result_;
}

HerdSpeedMode Simulation::speedMode() const {
    return herd_.speedMode;
}

SimulationStats Simulation::stats() const {
    SimulationStats output{};
    float vegetationSum = 0.0F;

    for (const Tile& tile : tiles_) {
        if (isWaterLike(tile.terrain)) {
            continue;
        }

        ++output.passableTiles;
        vegetationSum += clamp01(tile.vegetation / std::max(0.01F, maxVegetation(tile)));

        if (tile.terrain == TerrainType::Forest) {
            ++output.forestTiles;
        }
    }

    if (output.passableTiles > 0) {
        output.forestRatio = static_cast<float>(output.forestTiles) / static_cast<float>(output.passableTiles);
        output.averageVegetation = vegetationSum / static_cast<float>(output.passableTiles);
    }

    output.herdFoodRatio = lastFoodRatio_;
    return output;
}

std::string Simulation::statusText() const {
    const SimulationStats currentStats = stats();
    const char* state = "RUNNING";
    if (result_ == GameResult::Won) {
        state = "WON";
    } else if (result_ == GameResult::Lost) {
        state = "LOST";
    }

    const char* speed = herd_.speedMode == HerdSpeedMode::Fast ? "FAST" : "SLOW";

    std::ostringstream stream;
    stream << state
           << " | herd " << std::fixed << std::setprecision(1) << herd_.size
           << " | hunger " << std::setprecision(0) << herd_.hunger * 100.0F << "%"
           << " | speed " << speed
           << " | forest " << std::setprecision(1) << currentStats.forestRatio * 100.0F << "%"
           << " | veg " << std::setprecision(0) << currentStats.averageVegetation * 100.0F << "%";
    return stream.str();
}

bool Simulation::isPassable(HexCoord coord) const {
    if (!grid_.contains(coord)) {
        return false;
    }

    return !isWaterLike(tiles_[grid_.indexOf(coord)].terrain);
}

std::vector<HexCoord> Simulation::herdFootprint() const {
    return footprintFor(herd_.position);
}

void Simulation::generateMap() {
    constexpr float seed = 91.7F;

    for (std::size_t i = 0; i < grid_.coords().size(); ++i) {
        const HexCoord coord = grid_.coords()[i];
        tiles_[i] = makeTile(coord);

        const float capacity = maxVegetation(tiles_[i]);
        const float patchNoise = smoothNoise(coord, seed + 11.0F);
        const float detailNoise = smoothNoise({coord.q * 2 + 5, coord.r * 2 - 3}, seed + 29.0F);
        const float initialRatio = std::clamp(0.10F + patchNoise * 0.48F + detailNoise * 0.26F + tiles_[i].rainfall * 0.16F, 0.0F, 1.0F);
        tiles_[i].vegetation = capacity * initialRatio;
    }
}

void Simulation::placeHerd() {
    herd_ = {};
    herd_.heading = 0.0F;
    herd_.size = config_.initialHerdSize;
    herd_.hunger = 1.0F;
    herd_.moveProgress = 1.0F;

    auto passableNeighborCount = [this](HexCoord coord) {
        int count = 0;
        for (HexCoord neighbor : grid_.neighbors(coord)) {
            if (isPassable(neighbor)) {
                ++count;
            }
        }

        return count;
    };

    HexCoord chosen = {0, 0};
    int chosenScore = -1;
    const bool originPassable = isPassable({0, 0});

    for (HexCoord coord : grid_.coords()) {
        if (!isPassable(coord)) {
            continue;
        }

        const int score = passableNeighborCount(coord) * 10 - static_cast<int>(distanceFromOrigin(coord));
        if (score > chosenScore || (coord == HexCoord{0, 0} && originPassable && score == chosenScore)) {
            chosen = coord;
            chosenScore = score;
        }
    }

    if (!originPassable && chosenScore < 0) {
        const auto found = std::find_if(grid_.coords().begin(), grid_.coords().end(), [this](HexCoord coord) {
            return isPassable(coord);
        });

        chosen = found != grid_.coords().end() ? *found : HexCoord{};
    }

    herd_.center = chosen;
    herd_.position = grid_.toPixel(herd_.center);
    herd_.previousPosition = herd_.position;
    herd_.previousCenter = herd_.center;
}

void Simulation::step() {
    moveHerd();
    if (herd_.speedMode == HerdSpeedMode::Slow) {
        feedHerd();
    } else {
        applyFoodRatio(config_.fastModeFoodRatio);
    }

    regenerateTiles();
    updateResult();
}

void Simulation::moveHerd() {
    const sf::Vector2f startPosition = herd_.position;
    const HexCoord startCenter = herd_.center;

    const float turn = (turnRightHeld_ ? 1.0F : 0.0F) - (turnLeftHeld_ ? 1.0F : 0.0F);
    herd_.heading = normalizeAngle(herd_.heading + turn * config_.herdTurnRadians);

    const float motion = (forwardHeld_ ? 1.0F : 0.0F) - (backwardHeld_ ? 1.0F : 0.0F);
    sf::Vector2f destination = startPosition;

    if (motion != 0.0F) {
        const sf::Vector2f direction = headingVector();
        destination += direction * (motion * config_.herdStepDistance);

        const HexCoord destinationCenter = grid_.fromPixel(destination);
        if (!isPassable(destinationCenter)) {
            destination = startPosition;
        }
    }

    herd_.previousPosition = startPosition;
    herd_.previousCenter = startCenter;
    herd_.position = destination;
    herd_.center = grid_.fromPixel(herd_.position);
    herd_.moveProgress = 0.0F;
}

void Simulation::feedHerd() {
    const float bodyRadius = bodyRadiusFor(herd_);
    const auto footprint = footprintFor(herd_.position);
    const float movementPenalty = tiles_[grid_.indexOf(herd_.center)].terrain == TerrainType::Forest ? 1.22F : 1.0F;
    const float demand = herd_.size * config_.baseConsumptionPerElephant * movementPenalty;

    float totalWeight = 0.0F;
    for (HexCoord coord : footprint) {
        totalWeight += tileCoverageWeight(coord, herd_.position, bodyRadius);
    }

    if (totalWeight <= 0.0F) {
        applyFoodRatio(0.0F);
        return;
    }

    std::uniform_real_distribution<float> chance(0.0F, 1.0F);
    float eatenTotal = 0.0F;

    for (HexCoord coord : footprint) {
        if (!isPassable(coord)) {
            continue;
        }

        Tile& tile = tiles_[grid_.indexOf(coord)];
        const float weight = tileCoverageWeight(coord, herd_.position, bodyRadius);
        if (weight <= 0.0F) {
            continue;
        }

        const float accessibleFood = tile.vegetation * foodAccessibility(tile);
        const float eaten = std::min(accessibleFood, demand * (weight / totalWeight));
        const float vegetationRemoved = eaten / std::max(0.1F, foodAccessibility(tile));
        tile.vegetation = std::max(0.0F, tile.vegetation - vegetationRemoved);
        tile.manure = std::min(6.0F, tile.manure + eaten * config_.manurePerFood);
        eatenTotal += eaten;

        if (tile.terrain == TerrainType::Forest) {
            const float breakChance = std::min(
                0.35F,
                config_.forestBreakBaseChance + herd_.size * config_.forestBreakChancePerSize
            );

            if (chance(rng_) < breakChance) {
                tile.terrain = TerrainType::Grassland;
                tile.vegetation = std::min(tile.vegetation + 2.0F, maxVegetation(tile));
                tile.fertility = std::min(1.0F, tile.fertility + 0.08F);
            }
        }
    }

    applyFoodRatio(eatenTotal / std::max(0.01F, demand));
}

void Simulation::applyFoodRatio(float foodRatio) {
    lastFoodRatio_ = clamp01(foodRatio);
    herd_.hunger = clamp01(herd_.hunger * 0.84F + lastFoodRatio_ * 0.16F);

    if (lastFoodRatio_ >= config_.herdGrowthFoodThreshold) {
        herd_.size += config_.herdGrowthRate * (lastFoodRatio_ - config_.herdGrowthFoodThreshold + 0.1F);
    } else if (lastFoodRatio_ <= config_.herdStarvationThreshold) {
        herd_.size -= config_.herdStarvationRate * (config_.herdStarvationThreshold - lastFoodRatio_ + 0.1F);
    }

    herd_.size = std::clamp(herd_.size, 0.0F, config_.maxHerdSize);
}

void Simulation::regenerateTiles() {
    for (Tile& tile : tiles_) {
        if (tile.terrain == TerrainType::Water) {
            continue;
        }

        const float manureBoost = tile.manure * config_.manureFertilityBoost;
        const float climateBaseline = 0.18F + tile.rainfall * 0.58F;
        tile.fertility += manureBoost;
        tile.fertility -= std::max(0.0F, tile.fertility - climateBaseline) * config_.fertilityDecay;
        tile.fertility = std::clamp(tile.fertility, 0.04F, 1.0F);

        tile.manure = std::max(0.0F, tile.manure - config_.manureDecay);

        const float capacity = maxVegetation(tile);
        const float missing = std::max(0.0F, capacity - tile.vegetation);
        tile.vegetation += missing * growthRate(tile);
        tile.vegetation = std::clamp(tile.vegetation, 0.0F, capacity);
    }
}

void Simulation::updateResult() {
    const SimulationStats currentStats = stats();
    if (currentStats.passableTiles > 0 && currentStats.forestRatio <= config_.victoryForestRatio) {
        result_ = GameResult::Won;
    } else if (herd_.size < config_.minHerdSize || herd_.hunger <= 0.05F) {
        result_ = GameResult::Lost;
    }
}

Tile Simulation::makeTile(HexCoord coord) {
    const float radiusRatio = distanceFromOrigin(coord) / static_cast<float>(std::max(1, grid_.radius()));
    const float latitudinalDryness = std::abs(static_cast<float>(coord.r)) / static_cast<float>(std::max(1, grid_.radius()));
    const float n1 = smoothNoise(coord, 13.0F);
    const float n2 = smoothNoise({coord.q + 7, coord.r - 3}, 47.0F);
    const float rainfall = clamp01(0.72F - radiusRatio * 0.28F - latitudinalDryness * 0.25F + (n1 - 0.5F) * 0.42F);

    Tile tile{};
    tile.rainfall = rainfall;
    tile.fertility = std::clamp(0.18F + rainfall * 0.62F + (n2 - 0.5F) * 0.18F, 0.06F, 0.95F);

    const bool river = std::abs(coord.q + coord.r / 2) <= 1 && rainfall > 0.45F && n1 > 0.32F;
    if (river || (rainfall > 0.82F && n2 > 0.76F)) {
        tile.terrain = TerrainType::Water;
        tile.fertility = 0.0F;
        return tile;
    }

    if (rainfall > 0.72F) {
        tile.terrain = n2 > 0.64F ? TerrainType::Swamp : TerrainType::Forest;
    } else if (rainfall > 0.48F) {
        tile.terrain = n2 > 0.42F ? TerrainType::Forest : TerrainType::Grassland;
    } else if (rainfall > 0.25F) {
        tile.terrain = n2 > 0.2F ? TerrainType::DryGrassland : TerrainType::Grassland;
    } else {
        tile.terrain = n2 > 0.72F ? TerrainType::DryGrassland : TerrainType::Desert;
    }

    return tile;
}

float Simulation::maxVegetation(const Tile& tile) const {
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

float Simulation::growthRate(const Tile& tile) const {
    const float base = [terrain = tile.terrain] {
        switch (terrain) {
        case TerrainType::Forest:
            return 0.013F;
        case TerrainType::Grassland:
            return 0.034F;
        case TerrainType::DryGrassland:
            return 0.017F;
        case TerrainType::Water:
            return 0.0F;
        case TerrainType::Desert:
            return 0.006F;
        case TerrainType::Swamp:
            return 0.012F;
        }

        return 0.0F;
    }();

    return base * (0.40F + tile.fertility * 0.72F);
}

float Simulation::foodAccessibility(const Tile& tile) const {
    switch (tile.terrain) {
    case TerrainType::Forest:
        return 0.46F;
    case TerrainType::Grassland:
        return 1.0F;
    case TerrainType::DryGrassland:
        return 0.78F;
    case TerrainType::Water:
        return 0.0F;
    case TerrainType::Desert:
        return 0.28F;
    case TerrainType::Swamp:
        return 0.52F;
    }

    return 0.0F;
}

float Simulation::currentTickSeconds() const {
    const float configured = herd_.speedMode == HerdSpeedMode::Fast ? config_.fastTickSeconds : config_.tickSeconds;
    return std::max(0.001F, configured);
}

std::vector<HexCoord> Simulation::footprintFor(const sf::Vector2f& position) const {
    std::vector<HexCoord> coords;
    const float bodyRadius = bodyRadiusFor(herd_);

    for (HexCoord coord : grid_.coords()) {
        if (!isPassable(coord)) {
            continue;
        }

        if (tileCoverageWeight(coord, position, bodyRadius) > 0.0F) {
            coords.push_back(coord);
        }
    }

    return coords;
}

float Simulation::bodyRadiusFor(const Herd& herd) const {
    return std::clamp(9.0F + herd.size * 0.24F, 11.0F, 27.0F);
}

float Simulation::tileCoverageWeight(HexCoord coord, const sf::Vector2f& position, float bodyRadius) const {
    const sf::Vector2f tileCenter = grid_.toPixel(coord);
    const sf::Vector2f delta = tileCenter - position;
    const float distance = vectorLength(delta);
    const float reach = bodyRadius + grid_.hexSize();

    if (distance >= reach) {
        return 0.0F;
    }

    const float normalized = 1.0F - distance / reach;
    return normalized * normalized;
}

float Simulation::normalizeAngle(float angle) {
    const float twoPi = 2.0F * pi;
    while (angle <= -pi) {
        angle += twoPi;
    }

    while (angle > pi) {
        angle -= twoPi;
    }

    return angle;
}

sf::Vector2f Simulation::headingVector() const {
    return {std::cos(herd_.heading), std::sin(herd_.heading)};
}

} // namespace elephants


