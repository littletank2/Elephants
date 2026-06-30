#include "Elephants/HexGrid.hpp"
#include "Elephants/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool nearlyEqual(float left, float right, float epsilon = 0.001F) {
    return std::abs(left - right) <= epsilon;
}

elephants::HexCoord directionBetween(elephants::HexCoord from, elephants::HexCoord to) {
    return {to.q - from.q, to.r - from.r};
}

bool choosePassableMovementAxis(elephants::Simulation& simulation, elephants::HexCoord& direction) {
    const elephants::HexCoord start = simulation.herd().center;
    for (elephants::HexCoord candidate : elephants::HexGrid::directions()) {
        const elephants::HexCoord forward = start + candidate;
        const elephants::HexCoord backward = start + candidate * -1;
        if (simulation.isPassable(forward) && simulation.isPassable(backward)) {
            direction = candidate;
            simulation.setDirection(candidate);
            return true;
        }
    }

    return false;
}

void testHexGridRadiusAndNeighbors() {
    const elephants::HexGrid grid(2, 10.0F);

    require(grid.radius() == 2, "grid radius is preserved");
    require(nearlyEqual(grid.hexSize(), 10.0F), "grid hex size is preserved");
    require(grid.coords().size() == 19, "radius 2 hex map has 19 cells");
    require(grid.contains({0, 0}), "origin is inside the grid");
    require(grid.contains({2, -2}), "edge coordinate is inside the grid");
    require(!grid.contains({3, 0}), "outside coordinate is rejected");

    const auto neighbors = grid.neighbors({0, 0});
    require(neighbors[0] == elephants::HexCoord{1, 0}, "east neighbor is correct");
    require(neighbors[2] == elephants::HexCoord{0, -1}, "north-west neighbor is correct");
    require(neighbors[5] == elephants::HexCoord{0, 1}, "south-east neighbor is correct");

    const sf::Vector2f origin = grid.toPixel({0, 0});
    require(nearlyEqual(origin.x, 0.0F) && nearlyEqual(origin.y, 0.0F), "origin converts to zero pixel offset");
    require(grid.toPixel({1, 0}).x > origin.x, "positive q moves right on screen");
    require(grid.fromPixel(grid.toPixel({1, -1})) == elephants::HexCoord{1, -1}, "pixel coordinates round back to the original hex");
}

void testSimulationInitializesValidWorld() {
    elephants::SimulationConfig config{};
    config.mapRadius = 5;
    config.hexSize = 12.0F;

    const elephants::Simulation simulation(config);
    const elephants::SimulationStats stats = simulation.stats();

    require(simulation.tiles().size() == simulation.grid().coords().size(), "every grid coordinate has a tile");
    require(stats.passableTiles > 0, "generated map has passable land");
    require(stats.forestTiles >= 0 && stats.forestTiles <= stats.passableTiles, "forest count is within land count");
    require(stats.forestRatio >= 0.0F && stats.forestRatio <= 1.0F, "forest ratio is normalized");
    require(stats.averageVegetation >= 0.0F && stats.averageVegetation <= 1.0F, "average vegetation is normalized");
    require(simulation.isPassable(simulation.herd().center), "herd starts on passable land");
    require(!simulation.statusText().empty(), "status text is available");

    bool variedVegetation = false;
    const auto& tiles = simulation.tiles();
    for (std::size_t index = 1; index < tiles.size(); ++index) {
        if (!nearlyEqual(tiles[0].vegetation, tiles[index].vegetation)) {
            variedVegetation = true;
            break;
        }
    }

    require(variedVegetation, "initial vegetation varies across tiles");
}

void testDirectionValidation() {
    elephants::Simulation simulation;

    const float initialHeading = simulation.herd().heading;
    simulation.setDirection({2, 0});
    require(nearlyEqual(simulation.herd().heading, initialHeading), "invalid direction is ignored");

    simulation.setDirection({0, 1});
    require(!nearlyEqual(simulation.herd().heading, initialHeading), "valid direction updates heading");
}

void testUpdateConsumesFoodAndKeepsInvariants() {
    elephants::SimulationConfig config{};
    config.mapRadius = 6;
    config.tickSeconds = 0.01F;
    config.initialHerdSize = 8.0F;

    elephants::Simulation simulation(config);
    const auto beforeTiles = simulation.tiles();
    const int passableBefore = simulation.stats().passableTiles;

    simulation.update(config.tickSeconds);

    bool tileChanged = false;
    for (std::size_t index = 0; index < beforeTiles.size(); ++index) {
        const elephants::Tile& before = beforeTiles[index];
        const elephants::Tile& after = simulation.tiles()[index];
        if (!nearlyEqual(before.vegetation, after.vegetation) || !nearlyEqual(before.manure, after.manure) || !nearlyEqual(before.fertility, after.fertility)) {
            tileChanged = true;
            break;
        }
    }

    require(tileChanged, "one simulation tick changes ecological tile state");
    require(simulation.isPassable(simulation.herd().center), "herd remains on passable land after update");
    require(simulation.herd().hunger >= 0.0F && simulation.herd().hunger <= 1.0F, "herd hunger stays normalized");
    require(simulation.herd().size >= 0.0F && simulation.herd().size <= config.maxHerdSize, "herd size stays within configured bounds");
    require(simulation.stats().passableTiles == passableBefore, "normal updates do not change passable tile count");
}

void testHerdFootprintScalesWithSize() {
    elephants::SimulationConfig smallConfig{};
    smallConfig.mapRadius = 6;
    smallConfig.initialHerdSize = 6.0F;
    const elephants::Simulation smallSimulation(smallConfig);
    require(smallSimulation.herdFootprint().size() == 1, "small herd occupies only its center hex");

    elephants::SimulationConfig largeConfig{};
    largeConfig.mapRadius = 6;
    largeConfig.initialHerdSize = 40.0F;
    const elephants::Simulation largeSimulation(largeConfig);
    require(largeSimulation.herdFootprint().size() > 1, "large herd affects a wider front");
}

void testForwardMovementAdvancesAndEatsGrass() {
    elephants::SimulationConfig config{};
    config.mapRadius = 6;
    config.tickSeconds = 0.01F;
    config.initialHerdSize = 8.0F;
    config.victoryForestRatio = -1.0F;

    elephants::Simulation simulation(config);
    elephants::HexCoord direction{};
    require(choosePassableMovementAxis(simulation, direction), "test map has a passable movement axis");

    const elephants::HexCoord start = simulation.herd().center;
    const auto beforeTiles = simulation.tiles();
    const float hungerBefore = simulation.herd().hunger;

    simulation.setMovementInput(true, false, false, false);
    simulation.setSpeedMode(elephants::HerdSpeedMode::Slow);
    simulation.update(config.tickSeconds);

    require(simulation.herd().previousCenter == start, "forward movement records the previous center");
    require(simulation.herd().center == start + direction, "forward movement advances along the heading");
    require(simulation.herd().moveProgress >= 0.0F && simulation.herd().moveProgress <= 1.0F, "forward movement progress is normalized");
    require(simulation.herd().hunger <= hungerBefore, "forward movement does not increase hunger");

    bool tileChanged = false;
    for (std::size_t index = 0; index < beforeTiles.size(); ++index) {
        const elephants::Tile& before = beforeTiles[index];
        const elephants::Tile& after = simulation.tiles()[index];
        if (!nearlyEqual(before.vegetation, after.vegetation) || !nearlyEqual(before.manure, after.manure) || !nearlyEqual(before.fertility, after.fertility)) {
            tileChanged = true;
            break;
        }
    }

    require(tileChanged, "forward movement changes ecological tile state");
}

void testBackwardMovementAdvancesOppositeHeading() {
    elephants::SimulationConfig config{};
    config.mapRadius = 6;
    config.tickSeconds = 0.01F;
    config.initialHerdSize = 8.0F;
    config.victoryForestRatio = -1.0F;

    elephants::Simulation simulation(config);
    elephants::HexCoord direction{};
    require(choosePassableMovementAxis(simulation, direction), "test map has a passable movement axis");

    const elephants::HexCoord start = simulation.herd().center;
    simulation.setMovementInput(false, true, false, false);
    simulation.setSpeedMode(elephants::HerdSpeedMode::Slow);
    simulation.update(config.tickSeconds);

    require(simulation.herd().previousCenter == start, "backward movement records the previous center");
    require(simulation.herd().center == start + direction * -1, "backward movement advances opposite the heading");
}

void testFastModeMovesWithoutEatingGrass() {
    elephants::SimulationConfig config{};
    config.mapRadius = 6;
    config.fastTickSeconds = 0.01F;
    config.initialHerdSize = 8.0F;
    config.victoryForestRatio = -1.0F;

    elephants::Simulation simulation(config);
    elephants::HexCoord direction{};
    require(choosePassableMovementAxis(simulation, direction), "test map has a passable movement axis");

    const elephants::HexCoord start = simulation.herd().center;
    const auto beforeTiles = simulation.tiles();
    const float hungerBefore = simulation.herd().hunger;
    const float sizeBefore = simulation.herd().size;

    simulation.setMovementInput(true, false, false, false);
    simulation.setSpeedMode(elephants::HerdSpeedMode::Fast);
    require(simulation.speedMode() == elephants::HerdSpeedMode::Fast, "fast speed mode is accepted");

    simulation.update(config.fastTickSeconds);

    require(simulation.herd().previousCenter == start, "fast movement records the previous center");
    require(simulation.herd().center == start + direction, "fast movement advances to the selected passable hex");
    require(simulation.herd().moveProgress >= 0.0F && simulation.herd().moveProgress <= 1.0F, "fast movement progress is normalized");
    require(nearlyEqual(simulation.stats().herdFoodRatio, 0.0F), "fast movement reports no eaten food");
    require(simulation.herd().hunger < hungerBefore, "fast movement lowers hunger when no food is eaten");
    require(simulation.herd().size < sizeBefore, "fast movement applies starvation pressure");

    for (std::size_t index = 0; index < beforeTiles.size(); ++index) {
        const elephants::Tile& before = beforeTiles[index];
        const elephants::Tile& after = simulation.tiles()[index];
        require(after.vegetation + 0.001F >= before.vegetation, "fast movement does not consume vegetation");
        require(after.manure <= before.manure + 0.001F, "fast movement does not add manure from feeding");
    }
}

void testSlowModeMovesForwardAndEatsGrass() {
    elephants::SimulationConfig config{};
    config.mapRadius = 6;
    config.tickSeconds = 0.01F;
    config.initialHerdSize = 8.0F;
    config.victoryForestRatio = -1.0F;

    elephants::Simulation simulation(config);
    elephants::HexCoord direction{};
    require(choosePassableMovementAxis(simulation, direction), "test map has a passable movement axis");

    const elephants::HexCoord start = simulation.herd().center;
    const auto beforeTiles = simulation.tiles();
    const float hungerBefore = simulation.herd().hunger;

    simulation.setMovementInput(true, false, false, false);
    simulation.setSpeedMode(elephants::HerdSpeedMode::Slow);
    require(simulation.speedMode() == elephants::HerdSpeedMode::Slow, "slow speed mode is accepted");

    simulation.update(config.tickSeconds);

    require(simulation.herd().previousCenter == start, "slow movement records the previous center");
    require(simulation.herd().center == start + direction, "slow movement advances to the selected passable hex");
    require(simulation.herd().moveProgress >= 0.0F && simulation.herd().moveProgress <= 1.0F, "slow movement progress is normalized");
    require(simulation.herd().hunger <= hungerBefore, "slow movement does not increase hunger");

    bool tileChanged = false;
    for (std::size_t index = 0; index < beforeTiles.size(); ++index) {
        const elephants::Tile& before = beforeTiles[index];
        const elephants::Tile& after = simulation.tiles()[index];
        if (!nearlyEqual(before.vegetation, after.vegetation) || !nearlyEqual(before.manure, after.manure) || !nearlyEqual(before.fertility, after.fertility)) {
            tileChanged = true;
            break;
        }
    }

    require(tileChanged, "slow movement changes ecological tile state");
}

void testHerdMovementProgressInterpolatesBetweenSteps() {
    elephants::SimulationConfig config{};
    config.mapRadius = 6;
    config.tickSeconds = 0.50F;
    config.initialHerdSize = 8.0F;
    config.victoryForestRatio = -1.0F;

    elephants::Simulation simulation(config);
    elephants::HexCoord direction{};
    require(choosePassableMovementAxis(simulation, direction), "test map has a passable movement axis");

    simulation.setMovementInput(true, false, false, false);
    const elephants::HexCoord start = simulation.herd().center;
    simulation.update(config.tickSeconds);

    require(simulation.herd().previousCenter == start, "movement records the previous center");
    require(simulation.herd().center == start + direction, "movement advances to the selected passable hex");
    require(nearlyEqual(simulation.herd().moveProgress, 0.0F), "movement progress resets after a logical step");

    simulation.update(config.tickSeconds * 0.5F);
    require(simulation.herd().center == start + direction, "partial update does not advance another logical step");
    require(nearlyEqual(simulation.herd().moveProgress, 0.5F), "movement progress advances between logical steps");
}

void testVictoryThresholdCanEndGame() {
    elephants::SimulationConfig config{};
    config.mapRadius = 4;
    config.tickSeconds = 0.01F;
    config.victoryForestRatio = 1.0F;

    elephants::Simulation simulation(config);
    require(simulation.result() == elephants::GameResult::Running, "new simulation starts running");

    simulation.update(config.tickSeconds);
    require(simulation.result() == elephants::GameResult::Won, "victory threshold ends the game on update");
}

struct TestCase {
    const char* name;
    void (*run)();
};

} // namespace

int main() {
    const std::vector<TestCase> tests = {
        {"HexGrid radius and neighbors", testHexGridRadiusAndNeighbors},
        {"Simulation initializes valid world", testSimulationInitializesValidWorld},
        {"Direction validation", testDirectionValidation},
        {"Update consumes food and keeps invariants", testUpdateConsumesFoodAndKeepsInvariants},
        {"Herd footprint scales with size", testHerdFootprintScalesWithSize},
        {"Forward movement advances and eats grass", testForwardMovementAdvancesAndEatsGrass},
        {"Backward movement advances opposite heading", testBackwardMovementAdvancesOppositeHeading},
        {"Fast mode moves without eating grass", testFastModeMovesWithoutEatingGrass},
        {"Slow mode moves forward and eats grass", testSlowModeMovesForwardAndEatsGrass},
        {"Herd movement progress interpolates between steps", testHerdMovementProgressInterpolatesBetweenSteps},
        {"Victory threshold can end game", testVictoryThresholdCanEndGame},
    };

    int failures = 0;
    for (const TestCase& test : tests) {
        try {
            test.run();
            std::cout << "[PASS] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << error.what() << '\n';
        }
    }

    if (failures > 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " test(s) passed\n";
    return 0;
}

