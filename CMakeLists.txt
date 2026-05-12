cmake_minimum_required(VERSION 3.22)
project(Licenta)

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
    src/io/network_loader.cpp
    src/rng/prng.cpp
)

target_include_directories(simcore PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

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
)

target_link_libraries(sim_tests PRIVATE simcore Catch2::Catch2WithMain)
add_test(NAME sim_tests COMMAND sim_tests)
