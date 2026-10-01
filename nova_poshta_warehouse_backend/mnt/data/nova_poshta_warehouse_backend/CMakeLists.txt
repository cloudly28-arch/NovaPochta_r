cmake_minimum_required(VERSION 3.16)
project(NovaPoshtaWarehouseBackend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(warehouse_backend
    src/main.cpp
    src/Worker.cpp
    src/Store.cpp
    src/Product.cpp
    src/InventoryItem.cpp
    src/OrderItem.cpp
    src/Order.cpp
    src/Warehouse.cpp
)

target_include_directories(warehouse_backend PRIVATE include)
