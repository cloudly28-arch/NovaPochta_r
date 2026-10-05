#include "domain/OrderItem.h"
#include <stdexcept>

OrderItem::OrderItem(int productId, int quantity) : productId_(productId), quantity_(quantity) {
    if (productId_ <= 0 || quantity_ <= 0) throw std::invalid_argument("OrderItem values must be positive");
}
int OrderItem::getProductId() const { return productId_; }
int OrderItem::getQuantity() const { return quantity_; }
void OrderItem::setQuantity(int quantity) { if (quantity <= 0) throw std::invalid_argument("Quantity must be positive"); quantity_ = quantity; }
