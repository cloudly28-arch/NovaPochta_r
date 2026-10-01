#include "OrderItem.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

OrderItem::OrderItem(int productId, int quantity, double unitPrice)
    : productId_(productId), quantity_(quantity), unitPrice_(unitPrice) {
    if (productId_ <= 0) throw std::invalid_argument("Product id must be positive");
    if (quantity_ <= 0) throw std::invalid_argument("Order item quantity must be positive");
    if (unitPrice_ < 0) throw std::invalid_argument("Unit price cannot be negative");
}

int OrderItem::getProductId() const { return productId_; }
int OrderItem::getQuantity() const { return quantity_; }
double OrderItem::getUnitPrice() const { return unitPrice_; }
double OrderItem::getTotalPrice() const { return unitPrice_ * quantity_; }

std::string OrderItem::toString() const {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2)
        << "OrderItem{productId=" << productId_
        << ", quantity=" << quantity_
        << ", unitPrice=" << unitPrice_
        << ", total=" << getTotalPrice()
        << "}";
    return out.str();
}
