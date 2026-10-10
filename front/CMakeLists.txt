cmake_minimum_required(VERSION 3.20)

project(NovaPoshtaWarehouseFrontend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../nova_poshta_warehouse_backend
    ${CMAKE_BINARY_DIR}/nova_poshta_backend
)

add_executable(NovaPoshtaWarehouseFrontend
    src/main.cpp
    src/Application.cpp
    src/ui/StartScreen.cpp
    src/ui/SimulationScreen.cpp
)

target_include_directories(
    NovaPoshtaWarehouseFrontend
    PRIVATE
    include
)

target_link_libraries(
    NovaPoshtaWarehouseFrontend
    PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    ImGui-SFML::ImGui-SFML
    nova_warehouse_backend
)

add_custom_command(
    TARGET NovaPoshtaWarehouseFrontend
    POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:NovaPoshtaWarehouseFrontend>/assets

    COMMAND ${CMAKE_COMMAND} -E make_directory
    	$<TARGET_FILE_DIR:NovaPoshtaWarehouseFrontend>/database

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    	${CMAKE_CURRENT_SOURCE_DIR}/../nova_poshta_warehouse_backend/database/schema.sql
    	$<TARGET_FILE_DIR:NovaPoshtaWarehouseFrontend>/database/schema.sql
)


