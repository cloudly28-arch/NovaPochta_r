#include "domain/Order.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

Order::Order(int id, int storeId, std::string createdAt)
    : id_(id), storeId_(storeId), createdAt_(std::move(createdAt)) {
    if (id_ <= 0 || storeId_ <= 0) throw std::invalid_argument("Order ids must be positive");
}
int Order::getId() const { return id_; }
int Order::getStoreId() const { return storeId_; }
OrderStatus Order::getStatus() const { return status_; }
const std::string& Order::getCreatedAt() const { return createdAt_; }
const std::vector<OrderItem>& Order::getItems() const { return items_; }
void Order::setStatus(OrderStatus status) { status_ = status; }
void Order::addItem(const OrderItem& item) {
    for (auto& current : items_) {
        if (current.getProductId() == item.getProductId()) {
            current.setQuantity(current.getQuantity() + item.getQuantity());
            return;
        }
    }
    items_.push_back(item);
}
bool Order::removeItemByProductId(int productId) {
    auto oldSize = items_.size();
    items_.erase(std::remove_if(items_.begin(), items_.end(), [productId](const OrderItem& item){ return item.getProductId() == productId; }), items_.end());
    return oldSize != items_.size();
}
int Order::getTotalItemCount() const { int total = 0; for (const auto& item : items_) total += item.getQuantity(); return total; }
