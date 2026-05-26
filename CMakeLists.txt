cmake_minimum_required(VERSION 3.22)
project(Licenta)

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

if(NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "nogit")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Warnings
add_compile_options(-Wall -Wextra -Wpedantic)

# Simulation

add_library(simcore STATIC
    src/core/behaviour.cpp
    src/core/network.cpp
    src/core/car.cpp
    src/core/simulation.cpp
    src/core/routing.cpp
    src/core/traffic_light.cpp
    src/io/network_loader.cpp
    src/io/metrics_writer.cpp
    src/rng/prng.cpp
    src/sim/spawner.cpp
)


target_include_directories(simcore PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(simcore PUBLIC GIT_COMMIT_HASH="${GIT_COMMIT_HASH}")

option(SIM_SANITIZE "Enable ASan/UBSan" OFF)
if(SIM_SANITIZE)
    target_compile_options(simcore PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(simcore PUBLIC -fsanitize=address,undefined)
endif()

# SFML
set(SFML_USE_SYSTEM_DEPS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
add_subdirectory(include/SFML)

# IMGUI
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/include/imgui CACHE PATH "" FORCE)
set(IMGUI_SFML_USE_DEFAULT_CONFIG OFF CACHE BOOL "" FORCE)
set(IMGUI_SFML_CONFIG_DIR ${CMAKE_SOURCE_DIR} CACHE PATH "" FORCE)
set(IMGUI_SFML_CONFIG_NAME "imconfig-custom.h" CACHE STRING "" FORCE)
add_subdirectory(include/imgui-sfml)

# Executabil
add_executable(sim src/main.cpp)
target_link_libraries(sim PRIVATE simcore ImGui-SFML::ImGui-SFML)

# Tests

include (FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch.git
    GIT_TAG v3.15.0
)

FetchContent_MakeAvailable(Catch2)

enable_testing()
add_executable(sim_tests
    tests/test_main.cpp
    tests/test_geometry.cpp
    tests/test_simulation_skeleton.cpp
    tests/test_network_loader.cpp
    tests/test_rng.cpp
    tests/test_idm.cpp
    tests/test_routing.cpp
    tests/test_e2e.cpp
    tests/test_intended_lane.cpp
    tests/test_traffic_light.cpp
    tests/test_lane_change.cpp
    tests/test_metrics.cpp
    tests/test_metrics_writer.cpp
)

target_include_directories(sim_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
target_link_libraries(sim_tests PRIVATE simcore Catch2::Catch2WithMain)
add_test(NAME sim_tests COMMAND sim_tests)
