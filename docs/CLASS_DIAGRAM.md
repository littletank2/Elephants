# Диаграмма классов

Эта схема показывает основные типы кодовой базы и зависимости между игровым циклом, симуляцией, гекс-сеткой и рендером.

```mermaid
classDiagram
    direction LR

    namespace elephants {
        class Game {
            +Game()
            +run() void
            -handleEvents() void
            -handleKeyPressed(sf::Keyboard::Key key) void
            -update(float dt) void
            -render() void
            -refreshWindowTitle() void
            -sf::RenderWindow window_
            -Simulation simulation_
            -Renderer renderer_
            -bool paused_
            -float titleRefreshTimer_
        }

        class Renderer {
            +Renderer(const HexGrid& grid)
            +draw(sf::RenderWindow& window, const Simulation& simulation, bool paused) void
            -boardCenterOffset(sf::Vector2u windowSize) sf::Vector2f
            -makeHex(sf::Vector2f center, float radius) sf::ConvexShape
            -tileColor(const Tile& tile) sf::Color
            -vegetationOverlay(const Tile& tile) sf::Color
            -drawTiles(sf::RenderWindow& window, const Simulation& simulation, sf::Vector2f offset) void
            -drawHerd(sf::RenderWindow& window, const Simulation& simulation, sf::Vector2f offset) void
            -drawHud(sf::RenderWindow& window, const Simulation& simulation, bool paused) void
            -drawBar(sf::RenderWindow& window, sf::Vector2f position, sf::Vector2f size, float value, sf::Color fill) void
            -const HexGrid& grid_
        }

        class Simulation {
            +Simulation(SimulationConfig config)
            +reset() void
            +update(float dt) void
            +setDirection(HexCoord direction) void
            +grid() const HexGrid&
            +tiles() const vector~Tile~&
            +herd() const Herd&
            +result() GameResult
            +stats() SimulationStats
            +statusText() string
            +isPassable(HexCoord coord) bool
            +herdFootprint() vector~HexCoord~
            -generateMap() void
            -placeHerd() void
            -step() void
            -moveHerd() void
            -feedHerd() void
            -regenerateTiles() void
            -updateResult() void
            -makeTile(HexCoord coord) Tile
            -maxVegetation(const Tile& tile) float
            -growthRate(const Tile& tile) float
            -foodAccessibility(const Tile& tile) float
            -footprintFor(const Herd& herd) vector~HexCoord~
            -SimulationConfig config_
            -HexGrid grid_
            -vector~Tile~ tiles_
            -Herd herd_
            -GameResult result_
            -float accumulator_
            -float lastFoodRatio_
            -std::mt19937 rng_
        }

        class HexGrid {
            +HexGrid(int radius, float hexSize)
            +radius() int
            +hexSize() float
            +coords() const vector~HexCoord~&
            +contains(HexCoord coord) bool
            +indexOf(HexCoord coord) size_t
            +toPixel(HexCoord coord) sf::Vector2f
            +neighbors(HexCoord coord) array~HexCoord, 6~
            +directions() array~HexCoord, 6~
            -int radius_
            -float hexSize_
            -vector~HexCoord~ coords_
        }

        class HexCoord {
            +int q
            +int r
            +operator==(const HexCoord& other) bool
            +operator!=(const HexCoord& other) bool
        }

        class Tile {
            +TerrainType terrain
            +float rainfall
            +float fertility
            +float vegetation
            +float manure
        }

        class Herd {
            +HexCoord center
            +HexCoord direction
            +float size
            +float hunger
        }

        class SimulationConfig {
            +int mapRadius
            +float hexSize
            +float tickSeconds
            +float baseConsumptionPerElephant
            +float herdGrowthFoodThreshold
            +float herdStarvationThreshold
            +float herdGrowthRate
            +float herdStarvationRate
            +float minHerdSize
            +float initialHerdSize
            +float maxHerdSize
            +float manurePerFood
            +float manureDecay
            +float manureFertilityBoost
            +float fertilityDecay
            +float forestBreakBaseChance
            +float forestBreakChancePerSize
            +float victoryForestRatio
        }

        class SimulationStats {
            +float forestRatio
            +float averageVegetation
            +float herdFoodRatio
            +int passableTiles
            +int forestTiles
        }

        class TerrainType {
            <<enumeration>>
            Forest
            Grassland
            DryGrassland
            Water
            Desert
            Swamp
        }

        class GameResult {
            <<enumeration>>
            Running
            Won
            Lost
        }
    }

    Game *-- Simulation : owns
    Game *-- Renderer : owns
    Renderer --> HexGrid : reads layout
    Renderer --> Simulation : reads state
    Simulation *-- HexGrid : owns
    Simulation *-- Herd : owns
    Simulation o-- Tile : owns many
    Simulation --> SimulationConfig : configured by
    Simulation --> SimulationStats : reports
    Simulation --> GameResult : state
    HexGrid o-- HexCoord : stores coords
    Tile --> TerrainType : terrain
    Herd --> HexCoord : center/direction
```

## Как читать схему

- `Game` - runtime-обвязка: окно, ввод, кадры, пауза и рестарт.
- `Simulation` - единственный владелец игрового состояния и правил экологии.
- `HexGrid` - геометрия axial-гексов, соседи и перевод координат в пиксели.
- `Renderer` - read-only слой отрисовки, который читает `Simulation` и не меняет модель.
- `Tile`, `Herd`, `SimulationConfig` и `SimulationStats` - простые структуры данных для состояния и баланса.

Стрелки `owns` означают владение объектом по значению. Стрелки `reads` означают зависимость без владения: например, `Renderer` хранит ссылку на `HexGrid` и получает `Simulation` только на время `draw()`.