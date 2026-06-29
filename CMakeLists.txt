cmake_minimum_required(VERSION 3.25)

project(Elephants
    VERSION 0.1.0
    DESCRIPTION "Hex ecology game about an elephant herd"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 3 CONFIG REQUIRED COMPONENTS Graphics Window System)

enable_testing()

function(elephants_apply_warnings target_name)
    if (MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(elephants_core
    src/HexGrid.cpp
    src/Simulation.cpp
)

target_include_directories(elephants_core PUBLIC include)
target_link_libraries(elephants_core PUBLIC SFML::System)
elephants_apply_warnings(elephants_core)

add_executable(elephants
    src/Game.cpp
    src/Main.cpp
    src/Renderer.cpp
)

target_link_libraries(elephants PRIVATE
    elephants_core
    SFML::Graphics
    SFML::Window
)
elephants_apply_warnings(elephants)

add_executable(elephants_tests
    tests/SimulationTests.cpp
)

target_link_libraries(elephants_tests PRIVATE elephants_core)
elephants_apply_warnings(elephants_tests)

add_test(NAME elephants_tests COMMAND elephants_tests)