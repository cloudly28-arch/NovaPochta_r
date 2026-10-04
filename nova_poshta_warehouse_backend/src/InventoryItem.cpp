#include "InventoryItem.h"

#include <sstream>
#include <stdexcept>
#include <utility>

InventoryItem::InventoryItem(int productId, int quantity, std::string locationCode)
    : productId_(productId), quantity_(quantity), locationCode_(std::move(locationCode)) {
    if (productId_ <= 0) throw std::invalid_argument("Product id must be positive");
    if (quantity_ < 0) throw std::invalid_argument("Quantity cannot be negative");
}

int InventoryItem::getProductId() const { return productId_; }
int InventoryItem::getQuantity() const { return quantity_; }
const std::string& InventoryItem::getLocationCode() const { return locationCode_; }

void InventoryItem::addQuantity(int amount) {
    if (amount <= 0) throw std::invalid_argument("Amount must be positive");
    quantity_ += amount;
}

bool InventoryItem::removeQuantity(int amount) {
    if (amount <= 0) throw std::invalid_argument("Amount must be positive");
    if (amount > quantity_) return false;
    quantity_ -= amount;
    return true;
}

void InventoryItem::setLocationCode(const std::string& locationCode) {
    locationCode_ = locationCode;
}

std::string InventoryItem::toString() const {
    std::ostringstream out;
    out << "InventoryItem{productId=" << productId_
        << ", quantity=" << quantity_
        << ", location='" << locationCode_ << "'"
        << "}";
    return out.str();
}
