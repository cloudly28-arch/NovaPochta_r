#include "domain/InventoryRecord.h"
#include <stdexcept>
#include <utility>

InventoryRecord::InventoryRecord(int id, int productId, int storageCellId, int quantity, int reservedQuantity, std::string batchNumber, std::string expirationDate)
    : id_(id), productId_(productId), storageCellId_(storageCellId), quantity_(quantity), reservedQuantity_(reservedQuantity), batchNumber_(std::move(batchNumber)), expirationDate_(std::move(expirationDate)) {
    if (id_ <= 0 || productId_ <= 0) throw std::invalid_argument("InventoryRecord ids must be positive");
    if (quantity_ < 0 || reservedQuantity_ < 0 || reservedQuantity_ > quantity_) throw std::invalid_argument("Invalid inventory quantities");
}
int InventoryRecord::getId() const { return id_; }
int InventoryRecord::getProductId() const { return productId_; }
int InventoryRecord::getStorageCellId() const { return storageCellId_; }
int InventoryRecord::getQuantity() const { return quantity_; }
int InventoryRecord::getReservedQuantity() const { return reservedQuantity_; }
int InventoryRecord::getAvailableQuantity() const { return quantity_ - reservedQuantity_; }
const std::string& InventoryRecord::getBatchNumber() const { return batchNumber_; }
const std::string& InventoryRecord::getExpirationDate() const { return expirationDate_; }
void InventoryRecord::addQuantity(int quantity) { if (quantity <= 0) throw std::invalid_argument("Quantity must be positive"); quantity_ += quantity; }
bool InventoryRecord::removeQuantity(int quantity) { if (quantity <= 0 || quantity > getAvailableQuantity()) return false; quantity_ -= quantity; return true; }
bool InventoryRecord::reserve(int quantity) { if (quantity <= 0 || quantity > getAvailableQuantity()) return false; reservedQuantity_ += quantity; return true; }
bool InventoryRecord::releaseReservation(int quantity) { if (quantity <= 0 || quantity > reservedQuantity_) return false; reservedQuantity_ -= quantity; return true; }
bool InventoryRecord::shipReserved(int quantity) { if (quantity <= 0 || quantity > reservedQuantity_) return false; reservedQuantity_ -= quantity; quantity_ -= quantity; return true; }
