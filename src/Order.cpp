#include "Order.h"

#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>

Order::Order(int id, int storeId)
    : id_(id), storeId_(storeId), status_(OrderStatus::Created) {
    if (id_ <= 0) throw std::invalid_argument("Order id must be positive");
    if (storeId_ <= 0) throw std::invalid_argument("Store id must be positive");
}

int Order::getId() const { return id_; }
int Order::getStoreId() const { return storeId_; }
const std::vector<OrderItem>& Order::getItems() const { return items_; }
OrderStatus Order::getStatus() const { return status_; }

void Order::addItem(const OrderItem& item) {
    items_.push_back(item);
}

void Order::setStatus(OrderStatus status) {
    status_ = status;
}

double Order::getTotalAmount() const {
    double total = 0.0;
    for (const auto& item : items_) total += item.getTotalPrice();
    return total;
}

int Order::getTotalUnits() const {
    int total = 0;
    for (const auto& item : items_) total += item.getQuantity();
    return total;
}

std::string Order::statusToString() const {
    switch (status_) {
        case OrderStatus::Created: return "Created";
        case OrderStatus::Confirmed: return "Confirmed";
        case OrderStatus::Picking: return "Picking";
        case OrderStatus::ReadyForShipment: return "ReadyForShipment";
        case OrderStatus::Shipped: return "Shipped";
        case OrderStatus::Delivered: return "Delivered";
        case OrderStatus::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

std::string Order::toString() const {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2)
        << "Order{id=" << id_
        << ", storeId=" << storeId_
        << ", status=" << statusToString()
        << ", items=" << items_.size()
        << ", totalUnits=" << getTotalUnits()
        << ", totalAmount=" << getTotalAmount()
        << "}";
    return out.str();
}
