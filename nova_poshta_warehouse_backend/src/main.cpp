#include "Warehouse.h"

#include <iostream>

int main() {
    try {
        Warehouse warehouse(1, "Nova Poshta Wholesale Warehouse", "Main warehouse");

        warehouse.addWorker(Worker(1, "Ivan Petrenko", WorkerRole::Manager));
        warehouse.addWorker(Worker(2, "Oleh Kovalenko", WorkerRole::Storekeeper));
        warehouse.addWorker(Worker(3, "Andrii Bondar", WorkerRole::Loader));

        warehouse.addStore(Store(1, "Store #1", "Kyiv", "+380000000001"));
        warehouse.addStore(Store(2, "Store #2", "Lviv", "+380000000002"));

        warehouse.addProduct(Product(1, "SKU-001", "Cardboard box M", 0.35, 28.50));
        warehouse.addProduct(Product(2, "SKU-002", "Packing tape", 0.12, 42.00));
        warehouse.addProduct(Product(3, "SKU-003", "Stretch film", 1.80, 195.00));

        warehouse.receiveProduct(1, 100, "A-01-01");
        warehouse.receiveProduct(2, 250, "A-01-02");
        warehouse.receiveProduct(3, 50, "B-02-01");

        Order order(1, 1);
        order.addItem(OrderItem(1, 10, 28.50));
        order.addItem(OrderItem(2, 5, 42.00));
        warehouse.addOrder(order);

        std::cout << warehouse.toString() << "\n\n";

        std::cout << "Workers:\n";
        for (const auto& worker : warehouse.getWorkers()) {
            std::cout << "  " << worker.toString() << '\n';
        }

        std::cout << "\nStores:\n";
        for (const auto& store : warehouse.getStores()) {
            std::cout << "  " << store.toString() << '\n';
        }

        std::cout << "\nProducts:\n";
        for (const auto& product : warehouse.getProducts()) {
            std::cout << "  " << product.toString() << '\n';
        }

        std::cout << "\nInventory before order confirmation:\n";
        for (const auto& item : warehouse.getInventory()) {
            std::cout << "  " << item.toString() << '\n';
        }

        const bool confirmed = warehouse.reserveAndConfirmOrder(1);
        std::cout << "\nOrder confirmation: " << (confirmed ? "success" : "failed") << '\n';

        if (const auto* savedOrder = warehouse.findOrderById(1)) {
            std::cout << savedOrder->toString() << '\n';
        }

        std::cout << "\nInventory after order confirmation:\n";
        for (const auto& item : warehouse.getInventory()) {
            std::cout << "  " << item.toString() << '\n';
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
