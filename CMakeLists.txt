cmake_minimum_required(VERSION 3.16)
project(Licenta)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SFML 3 REQUIRED COMPONENTS Graphics)
find_package(OpenGL REQUIRED)

add_executable(sfml-app
    main.cpp
    include/imgui/imgui.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_demo.cpp
    include/imgui-sfml/imgui-SFML.cpp
)

target_include_directories(sfml-app PRIVATE
    include/imgui
    include/imgui-sfml
    ${CMAKE_SOURCE_DIR}
)

target_compile_definitions(sfml-app PRIVATE
    IMGUI_USER_CONFIG="imconfig-custom.h"
)

target_link_libraries(sfml-app PRIVATE
    SFML::Graphics
    OpenGL::GL
)
