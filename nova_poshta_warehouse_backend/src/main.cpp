#include <iostream>
#include <string>
#include "Warehouse.h"
#include "persistence/Database.h"
#include "services/InventoryService.h"
#include "services/OrderService.h"
#include "services/TaskService.h"

int main(int argc, char** argv) {
    const std::string dbPath = argc > 1 ? argv[1] : "database/nova_poshta_warehouse.db";

    Database database;
    if (!database.open(dbPath)) {
        std::cerr << "Cannot open database: " << dbPath << '\n';
        return 1;
    }

    Warehouse warehouse;
    if (!database.loadWarehouse(warehouse)) {
        std::cerr << "Cannot load warehouse from database\n";
        return 1;
    }

    InventoryService inventoryService(warehouse, &database);
    OrderService orderService(warehouse, inventoryService, &database);
    TaskService taskService(warehouse);

    std::cout << "Warehouse: " << warehouse.getName() << '\n';
    std::cout << "Stores: " << warehouse.getStores().size() << '\n';
    std::cout << "Products: " << warehouse.getProducts().size() << '\n';
    std::cout << "Inventory records: " << warehouse.getInventory().size() << "\n\n";

    std::cout << "Warehouse stock:\n";
    for (const auto& record : warehouse.getInventory()) {
        const auto* product = warehouse.findProductById(record.getProductId());
        std::cout << "  " << (product ? product->getName() : "Unknown")
                  << ": " << record.getQuantity() << '\n';
    }

    std::cout << "\nFreshMart stock:\n";
    for (const auto& row : database.getStoreInventory(1)) {
        std::cout << "  " << row.productName << ": " << row.quantity << '/' << row.capacity << '\n';
    }

    // Example order. Uncomment to test real stock reservation/shipping.
    // Order order(1, 1, "2026-10-04");
    // order.addItem(OrderItem(1, 5)); // Milk
    // order.addItem(OrderItem(2, 3)); // Bread
    // if (orderService.createOrder(order) && orderService.reserveOrder(1)) {
    //     taskService.createPickingTasksForOrder(1);
    //     orderService.shipOrder(1);
    //     orderService.completeOrder(1);
    // }

    return 0;
}
