cmake_minimum_required(VERSION 3.22)
project(Licenta)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build SFML from source using its bundled dependencies (Graphics only)
set(SFML_USE_SYSTEM_DEPS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
add_subdirectory(include/SFML)

# Configure imgui-sfml subdirectory options before adding it
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/include/imgui CACHE PATH "" FORCE)
set(IMGUI_SFML_USE_DEFAULT_CONFIG OFF CACHE BOOL "" FORCE)
set(IMGUI_SFML_CONFIG_DIR ${CMAKE_SOURCE_DIR} CACHE PATH "" FORCE)
set(IMGUI_SFML_CONFIG_NAME "imconfig-custom.h" CACHE STRING "" FORCE)

add_subdirectory(include/imgui-sfml)

add_executable(sfml-app main.cpp)

target_link_libraries(sfml-app PRIVATE
    ImGui-SFML::ImGui-SFML
)
