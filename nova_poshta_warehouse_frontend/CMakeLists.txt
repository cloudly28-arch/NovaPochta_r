cmake_minimum_required(VERSION 3.20)

project(NovaPoshtaWarehouseFrontend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

FetchContent_Declare(
    ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9
)

FetchContent_MakeAvailable(ImGui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})

FetchContent_Declare(
    ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG v3.0
)

set(IMGUI_SFML_FIND_SFML OFF)

FetchContent_MakeAvailable(ImGui-SFML)

add_executable(NovaPoshtaWarehouseFrontend
    src/main.cpp
)

target_link_libraries(NovaPoshtaWarehouseFrontend
    PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    ImGui-SFML::ImGui-SFML
)