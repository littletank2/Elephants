#include "Elephants/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <numeric>

namespace elephants {
namespace {

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
    generateMap();
    placeHerd();
}

void Simulation::update(float dt) {
    if (result_ != GameResult::Running) {
        return;
    }

    accumulator_ += dt;
    while (accumulator_ >= config_.tickSeconds) {
        step();
        accumulator_ -= config_.tickSeconds;
    }
}

void Simulation::setDirection(HexCoord direction) {
    const auto dirs = HexGrid::directions();
    if (std::find(dirs.begin(), dirs.end(), direction) != dirs.end()) {
        herd_.direction = direction;
    }
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

    std::ostringstream stream;
    stream << state
           << " | herd " << std::fixed << std::setprecision(1) << herd_.size
           << " | hunger " << std::setprecision(0) << herd_.hunger * 100.0F << "%"
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
    return footprintFor(herd_);
}

void Simulation::generateMap() {
    constexpr float seed = 91.7F;

    for (std::size_t i = 0; i < grid_.coords().size(); ++i) {
        tiles_[i] = makeTile(grid_.coords()[i]);

        const float capacity = maxVegetation(tiles_[i]);
        const float initialRatio = 0.55F + smoothNoise(grid_.coords()[i], seed + 11.0F) * 0.45F;
        tiles_[i].vegetation = capacity * initialRatio;
    }
}

void Simulation::placeHerd() {
    herd_ = {};
    herd_.direction = {1, 0};
    herd_.size = config_.initialHerdSize;
    herd_.hunger = 1.0F;

    if (isPassable({0, 0})) {
        herd_.center = {0, 0};
        return;
    }

    const auto found = std::find_if(grid_.coords().begin(), grid_.coords().end(), [this](HexCoord coord) {
        return isPassable(coord);
    });

    herd_.center = found != grid_.coords().end() ? *found : HexCoord{};
}

void Simulation::step() {
    moveHerd();
    feedHerd();
    regenerateTiles();
    updateResult();
}

void Simulation::moveHerd() {
    const HexCoord next = herd_.center + herd_.direction;
    if (isPassable(next)) {
        herd_.center = next;
        return;
    }

    const auto neighbors = grid_.neighbors(herd_.center);
    auto best = herd_.center;
    float bestScore = -1.0F;

    for (HexCoord candidate : neighbors) {
        if (!isPassable(candidate)) {
            continue;
        }

        const Tile& tile = tiles_[grid_.indexOf(candidate)];
        const float score = tile.vegetation * foodAccessibility(tile) + tile.fertility * 1.5F;
        if (score > bestScore) {
            bestScore = score;
            best = candidate;
        }
    }

    herd_.center = best;
}

void Simulation::feedHerd() {
    const auto footprint = footprintFor(herd_);
    const float movementPenalty = tiles_[grid_.indexOf(herd_.center)].terrain == TerrainType::Forest ? 1.22F : 1.0F;
    const float demand = herd_.size * config_.baseConsumptionPerElephant * movementPenalty;
    const float demandPerTile = demand / static_cast<float>(std::max<std::size_t>(1, footprint.size()));
    float eatenTotal = 0.0F;

    std::uniform_real_distribution<float> chance(0.0F, 1.0F);

    for (HexCoord coord : footprint) {
        if (!isPassable(coord)) {
            continue;
        }

        Tile& tile = tiles_[grid_.indexOf(coord)];
        const float accessibleFood = tile.vegetation * foodAccessibility(tile);
        const float eaten = std::min(accessibleFood, demandPerTile);
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

    lastFoodRatio_ = clamp01(eatenTotal / std::max(0.01F, demand));
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
            return 0.020F;
        case TerrainType::Grassland:
            return 0.052F;
        case TerrainType::DryGrassland:
            return 0.026F;
        case TerrainType::Water:
            return 0.0F;
        case TerrainType::Desert:
            return 0.010F;
        case TerrainType::Swamp:
            return 0.018F;
        }

        return 0.0F;
    }();

    return base * (0.45F + tile.fertility * 0.85F);
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

std::vector<HexCoord> Simulation::footprintFor(const Herd& herd) const {
    std::vector<HexCoord> coords;
    coords.push_back(herd.center);

    const auto dirs = HexGrid::directions();
    const int ringCount = herd.size >= 35.0F ? 2 : herd.size >= 14.0F ? 1 : 0;

    if (ringCount >= 1) {
        for (HexCoord dir : dirs) {
            const HexCoord coord = herd.center + dir;
            if (isPassable(coord)) {
                coords.push_back(coord);
            }
        }
    }

    if (ringCount >= 2) {
        for (HexCoord dir : dirs) {
            const HexCoord coord = herd.center + dir * 2;
            if (isPassable(coord)) {
                coords.push_back(coord);
            }
        }
    }

    return coords;
}

} // namespace elephants
