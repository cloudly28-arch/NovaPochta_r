#include "Warehouse.h"
#include <stdexcept>
#include <utility>

Warehouse::Warehouse(int id, std::string name, std::string address)
    : id_(id), name_(std::move(name)), address_(std::move(address)) {
    if (id_ <= 0) throw std::invalid_argument("Warehouse id must be positive");
    if (name_.empty()) throw std::invalid_argument("Warehouse name cannot be empty");
}

int Warehouse::getId() const { return id_; }
const std::string& Warehouse::getName() const { return name_; }
const std::string& Warehouse::getAddress() const { return address_; }

std::vector<Worker>& Warehouse::getWorkers() { return workers_; }
std::vector<Store>& Warehouse::getStores() { return stores_; }
std::vector<Product>& Warehouse::getProducts() { return products_; }
std::vector<StorageCell>& Warehouse::getStorageCells() { return storageCells_; }
std::vector<WarehouseZone>& Warehouse::getZones() { return zones_; }
std::vector<InventoryRecord>& Warehouse::getInventory() { return inventory_; }
std::vector<Order>& Warehouse::getOrders() { return orders_; }
std::vector<WarehouseTask>& Warehouse::getTasks() { return tasks_; }
std::vector<Vehicle>& Warehouse::getVehicles() { return vehicles_; }

const std::vector<Worker>& Warehouse::getWorkers() const { return workers_; }
const std::vector<Store>& Warehouse::getStores() const { return stores_; }
const std::vector<Product>& Warehouse::getProducts() const { return products_; }
const std::vector<StorageCell>& Warehouse::getStorageCells() const { return storageCells_; }
const std::vector<WarehouseZone>& Warehouse::getZones() const { return zones_; }
const std::vector<InventoryRecord>& Warehouse::getInventory() const { return inventory_; }
const std::vector<Order>& Warehouse::getOrders() const { return orders_; }
const std::vector<WarehouseTask>& Warehouse::getTasks() const { return tasks_; }
const std::vector<Vehicle>& Warehouse::getVehicles() const { return vehicles_; }

void Warehouse::addWorker(const Worker& v) { if (findWorkerById(v.getId())) throw std::invalid_argument("Duplicate worker id"); workers_.push_back(v); }
void Warehouse::addStore(const Store& v) { if (findStoreById(v.getId())) throw std::invalid_argument("Duplicate store id"); stores_.push_back(v); }
void Warehouse::addProduct(const Product& v) { if (findProductById(v.getId())) throw std::invalid_argument("Duplicate product id"); products_.push_back(v); }
void Warehouse::addStorageCell(const StorageCell& v) { if (findStorageCellById(v.getId())) throw std::invalid_argument("Duplicate cell id"); storageCells_.push_back(v); }
void Warehouse::addZone(const WarehouseZone& v) { if (findZoneById(v.getId())) throw std::invalid_argument("Duplicate zone id"); zones_.push_back(v); }
void Warehouse::addInventoryRecord(const InventoryRecord& v) { if (findInventoryRecordById(v.getId())) throw std::invalid_argument("Duplicate inventory id"); inventory_.push_back(v); }
void Warehouse::addOrder(const Order& v) { if (findOrderById(v.getId())) throw std::invalid_argument("Duplicate order id"); orders_.push_back(v); }
void Warehouse::addTask(const WarehouseTask& v) { if (findTaskById(v.getId())) throw std::invalid_argument("Duplicate task id"); tasks_.push_back(v); }
void Warehouse::addVehicle(const Vehicle& v) { if (findVehicleById(v.getId())) throw std::invalid_argument("Duplicate vehicle id"); vehicles_.push_back(v); }

Worker* Warehouse::findWorkerById(int id) { for (auto& v : workers_) if (v.getId() == id) return &v; return nullptr; }
Store* Warehouse::findStoreById(int id) { for (auto& v : stores_) if (v.getId() == id) return &v; return nullptr; }
Product* Warehouse::findProductById(int id) { for (auto& v : products_) if (v.getId() == id) return &v; return nullptr; }
StorageCell* Warehouse::findStorageCellById(int id) { for (auto& v : storageCells_) if (v.getId() == id) return &v; return nullptr; }
WarehouseZone* Warehouse::findZoneById(int id) { for (auto& v : zones_) if (v.getId() == id) return &v; return nullptr; }
InventoryRecord* Warehouse::findInventoryRecordById(int id) { for (auto& v : inventory_) if (v.getId() == id) return &v; return nullptr; }
Order* Warehouse::findOrderById(int id) { for (auto& v : orders_) if (v.getId() == id) return &v; return nullptr; }
WarehouseTask* Warehouse::findTaskById(int id) { for (auto& v : tasks_) if (v.getId() == id) return &v; return nullptr; }
Vehicle* Warehouse::findVehicleById(int id) { for (auto& v : vehicles_) if (v.getId() == id) return &v; return nullptr; }
