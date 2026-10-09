cmake_minimum_required(VERSION 3.16)

project(NovaPoshtaWarehouseBackend LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    sqlite3
    GIT_REPOSITORY https://github.com/underclocked/sqlite.git
    GIT_TAG master
)

FetchContent_MakeAvailable(sqlite3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_library(nova_warehouse_backend
    src/Warehouse.cpp

    src/domain/InventoryRecord.cpp
    src/domain/Order.cpp
    src/domain/OrderItem.cpp
    src/domain/Product.cpp
    src/domain/StorageCell.cpp
    src/domain/Store.cpp
    src/domain/Vehicle.cpp
    src/domain/WarehouseTask.cpp
    src/domain/WarehouseZone.cpp
    src/domain/Worker.cpp

    src/persistence/Database.cpp

    src/services/InventoryService.cpp
    src/services/OrderService.cpp
    src/services/SimulationService.cpp
    src/services/TaskService.cpp

    src/application/BackendFacade.cpp
)

target_include_directories(
    nova_warehouse_backend
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    nova_warehouse_backend
    PUBLIC
    SQLite3
)

add_executable(
    warehouse_backend
    src/main.cpp
)

target_link_libraries(
    warehouse_backend
    PRIVATE
    nova_warehouse_backend
)
