#pragma once

#include "Elephants/HexGrid.hpp"
#include "Elephants/Types.hpp"

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace elephants {

enum class GameResult {
    Running,
    Won,
    Lost,
};

struct SimulationConfig {
    int mapRadius = 22;
    float hexSize = 23.0F;
    float tickSeconds = 0.22F;
    float fastTickSeconds = 0.10F;
    float fastModeFoodRatio = 0.0F;
    float herdStepDistance = 39.8F;
    float herdTurnRadians = 0.5235988F;

    float baseConsumptionPerElephant = 0.42F;
    float herdGrowthFoodThreshold = 0.84F;
    float herdStarvationThreshold = 0.33F;
    float herdGrowthRate = 0.16F;
    float herdStarvationRate = 0.22F;
    float minHerdSize = 1.0F;
    float initialHerdSize = 6.0F;
    float maxHerdSize = 80.0F;

    float manurePerFood = 0.18F;
    float manureDecay = 0.018F;
    float manureFertilityBoost = 0.035F;
    float fertilityDecay = 0.004F;

    float forestBreakBaseChance = 0.015F;
    float forestBreakChancePerSize = 0.0018F;
    float victoryForestRatio = 0.05F;
};

struct SimulationStats {
    float forestRatio = 0.0F;
    float averageVegetation = 0.0F;
    float herdFoodRatio = 0.0F;
    int passableTiles = 0;
    int forestTiles = 0;
};

class Simulation {
public:
    explicit Simulation(SimulationConfig config = {});

    void reset();
    void update(float dt);
    void setDirection(HexCoord direction);
    void setSpeedMode(HerdSpeedMode mode);
    void setMovementInput(bool forward, bool backward, bool turnLeft, bool turnRight);

    [[nodiscard]] const HexGrid& grid() const;
    [[nodiscard]] const std::vector<Tile>& tiles() const;
    [[nodiscard]] const Herd& herd() const;
    [[nodiscard]] GameResult result() const;
    [[nodiscard]] HerdSpeedMode speedMode() const;
    [[nodiscard]] SimulationStats stats() const;
    [[nodiscard]] std::string statusText() const;
    [[nodiscard]] bool isPassable(HexCoord coord) const;
    [[nodiscard]] bool isExplored(HexCoord coord) const;
    [[nodiscard]] std::vector<HexCoord> herdFootprint() const;

private:
    void generateMap();
    void placeHerd();
    void step();
    void moveHerd();
    void feedHerd();
    void applyFoodRatio(float foodRatio);
    void regenerateTiles();
    void updateResult();
    void markExplored(const std::vector<HexCoord>& coords);

    [[nodiscard]] Tile makeTile(HexCoord coord);
    [[nodiscard]] float maxVegetation(const Tile& tile) const;
    [[nodiscard]] float growthRate(const Tile& tile) const;
    [[nodiscard]] float foodAccessibility(const Tile& tile) const;
    [[nodiscard]] float currentTickSeconds() const;
    [[nodiscard]] std::vector<HexCoord> footprintFor(const sf::Vector2f& position) const;
    [[nodiscard]] float bodyRadiusFor(const Herd& herd) const;
    [[nodiscard]] float tileCoverageWeight(HexCoord coord, const sf::Vector2f& position, float bodyRadius) const;
    [[nodiscard]] static float normalizeAngle(float angle);
    [[nodiscard]] sf::Vector2f headingVector() const;

    SimulationConfig config_;
    HexGrid grid_;
    std::vector<Tile> tiles_;
    std::vector<std::uint8_t> explored_;
    Herd herd_;
    GameResult result_ = GameResult::Running;
    float accumulator_ = 0.0F;
    float lastFoodRatio_ = 1.0F;
    std::mt19937 rng_;
    bool forwardHeld_ = false;
    bool backwardHeld_ = false;
    bool turnLeftHeld_ = false;
    bool turnRightHeld_ = false;
};

} // namespace elephants
