#pragma once

#include <string>
#include <vector>

#include "Worker.h"
#include "Store.h"
#include "Product.h"
#include "StorageCell.h"
#include "WarehouseZone.h"
#include "InventoryRecord.h"
#include "Order.h"
#include "WarehouseTask.h"
#include "Vehicle.h"

class Warehouse {
private:
    int id_{};
    std::string name_;
    std::string address_;

    std::vector<Worker> workers_;
    std::vector<Store> stores_;
    std::vector<Product> products_;
    std::vector<StorageCell> storageCells_;
    std::vector<WarehouseZone> zones_;
    std::vector<InventoryRecord> inventory_;
    std::vector<Order> orders_;
    std::vector<WarehouseTask> tasks_;
    std::vector<Vehicle> vehicles_;

public:
    Warehouse() = default;

    Warehouse(
        int id,
        const std::string& name,
        const std::string& address
    );

    int getId() const;
    const std::string& getName() const;
    const std::string& getAddress() const;

    const std::vector<Worker>& getWorkers() const;
    const std::vector<Store>& getStores() const;
    const std::vector<Product>& getProducts() const;
    const std::vector<StorageCell>& getStorageCells() const;
    const std::vector<WarehouseZone>& getZones() const;
    const std::vector<InventoryRecord>& getInventory() const;
    const std::vector<Order>& getOrders() const;
    const std::vector<WarehouseTask>& getTasks() const;
    const std::vector<Vehicle>& getVehicles() const;

    void addWorker(const Worker& worker);
    void addStore(const Store& store);
    void addProduct(const Product& product);
    void addStorageCell(const StorageCell& storageCell);
    void addZone(const WarehouseZone& zone);
    void addInventoryRecord(const InventoryRecord& record);
    void addOrder(const Order& order);
    void addTask(const WarehouseTask& task);
    void addVehicle(const Vehicle& vehicle);

    Worker* findWorkerById(int id);
    Store* findStoreById(int id);
    Product* findProductById(int id);
    StorageCell* findStorageCellById(int id);
    WarehouseZone* findZoneById(int id);
    InventoryRecord* findInventoryRecordById(int id);
    Order* findOrderById(int id);
    WarehouseTask* findTaskById(int id);
    Vehicle* findVehicleById(int id);
};