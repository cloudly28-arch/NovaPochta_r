#include "Warehouse.h"

#include <sstream>
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

void Warehouse::addWorker(const Worker& worker) {
    if (findWorkerById(worker.getId())) throw std::invalid_argument("Worker with this id already exists");
    workers_.push_back(worker);
}

void Warehouse::addStore(const Store& store) {
    if (findStoreById(store.getId())) throw std::invalid_argument("Store with this id already exists");
    stores_.push_back(store);
}

void Warehouse::addProduct(const Product& product) {
    if (findProductById(product.getId())) throw std::invalid_argument("Product with this id already exists");
    products_.push_back(product);
}

void Warehouse::addInventoryItem(const InventoryItem& item) {
    if (!findProductById(item.getProductId())) {
        throw std::invalid_argument("Cannot add inventory for unknown product");
    }
    if (findInventoryByProductId(item.getProductId())) {
        throw std::invalid_argument("Inventory item for this product already exists");
    }
    inventory_.push_back(item);
}

void Warehouse::addOrder(const Order& order) {
    if (findOrderById(order.getId())) throw std::invalid_argument("Order with this id already exists");
    if (!findStoreById(order.getStoreId())) throw std::invalid_argument("Order references unknown store");
    orders_.push_back(order);
}

Worker* Warehouse::findWorkerById(int id) {
    for (auto& worker : workers_) if (worker.getId() == id) return &worker;
    return nullptr;
}

Store* Warehouse::findStoreById(int id) {
    for (auto& store : stores_) if (store.getId() == id) return &store;
    return nullptr;
}

Product* Warehouse::findProductById(int id) {
    for (auto& product : products_) if (product.getId() == id) return &product;
    return nullptr;
}

InventoryItem* Warehouse::findInventoryByProductId(int productId) {
    for (auto& item : inventory_) if (item.getProductId() == productId) return &item;
    return nullptr;
}

Order* Warehouse::findOrderById(int id) {
    for (auto& order : orders_) if (order.getId() == id) return &order;
    return nullptr;
}

const std::vector<Worker>& Warehouse::getWorkers() const { return workers_; }
const std::vector<Store>& Warehouse::getStores() const { return stores_; }
const std::vector<Product>& Warehouse::getProducts() const { return products_; }
const std::vector<InventoryItem>& Warehouse::getInventory() const { return inventory_; }
const std::vector<Order>& Warehouse::getOrders() const { return orders_; }

bool Warehouse::receiveProduct(int productId, int quantity, const std::string& locationCode) {
    if (quantity <= 0 || !findProductById(productId)) return false;

    auto* inventoryItem = findInventoryByProductId(productId);
    if (inventoryItem) {
        inventoryItem->addQuantity(quantity);
        if (!locationCode.empty()) inventoryItem->setLocationCode(locationCode);
    } else {
        inventory_.emplace_back(productId, quantity, locationCode);
    }
    return true;
}

bool Warehouse::reserveAndConfirmOrder(int orderId) {
    auto* order = findOrderById(orderId);
    if (!order || order->getStatus() != OrderStatus::Created) return false;

    for (const auto& item : order->getItems()) {
        auto* stock = findInventoryByProductId(item.getProductId());
        if (!stock || stock->getQuantity() < item.getQuantity()) return false;
    }

    for (const auto& item : order->getItems()) {
        auto* stock = findInventoryByProductId(item.getProductId());
        stock->removeQuantity(item.getQuantity());
    }

    order->setStatus(OrderStatus::Confirmed);
    return true;
}

std::string Warehouse::toString() const {
    std::ostringstream out;
    out << "Warehouse{id=" << id_
        << ", name='" << name_ << "'"
        << ", address='" << address_ << "'"
        << ", workers=" << workers_.size()
        << ", stores=" << stores_.size()
        << ", products=" << products_.size()
        << ", inventoryPositions=" << inventory_.size()
        << ", orders=" << orders_.size()
        << "}";
    return out.str();
}
