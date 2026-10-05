#include "services/InventoryService.h"
#include "persistence/Database.h"
#include <algorithm>
#include <vector>

InventoryService::InventoryService(Warehouse& warehouse, Database* database)
    : warehouse_(warehouse), database_(database) {}
int InventoryService::getTotalQuantity(int productId) const { int total = 0; for (const auto& r : warehouse_.getInventory()) if (r.getProductId() == productId) total += r.getQuantity(); return total; }
int InventoryService::getAvailableQuantity(int productId) const { int total = 0; for (const auto& r : warehouse_.getInventory()) if (r.getProductId() == productId) total += r.getAvailableQuantity(); return total; }

bool InventoryService::receiveProduct(int productId, int quantity, int storageCellId, const std::string& batchNumber, const std::string& expirationDate) {
    if (quantity <= 0 || !warehouse_.findProductById(productId)) return false;
    for (auto& r : warehouse_.getInventory()) {
        if (r.getProductId() == productId && r.getStorageCellId() == storageCellId && r.getBatchNumber() == batchNumber && r.getExpirationDate() == expirationDate) {
            r.addQuantity(quantity);
            if (database_ && !database_->changeWarehouseQuantity(warehouse_.getId(), productId, quantity)) { r.removeQuantity(quantity); return false; }
            return true;
        }
    }
    int nextId = 1;
    for (const auto& r : warehouse_.getInventory()) nextId = std::max(nextId, r.getId() + 1);
    warehouse_.addInventoryRecord(InventoryRecord(nextId, productId, storageCellId, quantity, 0, batchNumber, expirationDate));
    if (database_ && !database_->changeWarehouseQuantity(warehouse_.getId(), productId, quantity)) { warehouse_.getInventory().pop_back(); return false; }
    return true;
}

bool InventoryService::reserveProduct(int productId, int quantity) {
    if (quantity <= 0 || getAvailableQuantity(productId) < quantity) return false;
    std::vector<InventoryRecord*> records;
    for (auto& r : warehouse_.getInventory()) if (r.getProductId() == productId && r.getAvailableQuantity() > 0) records.push_back(&r);
    std::stable_sort(records.begin(), records.end(), [](const InventoryRecord* a, const InventoryRecord* b) {
        const auto& ea = a->getExpirationDate(); const auto& eb = b->getExpirationDate();
        if (ea.empty() != eb.empty()) return !ea.empty();
        return ea < eb;
    });
    int left = quantity;
    for (auto* r : records) { int take = std::min(left, r->getAvailableQuantity()); if (take > 0) { r->reserve(take); left -= take; } if (left == 0) break; }
    return left == 0;
}

bool InventoryService::releaseReservation(int productId, int quantity) {
    if (quantity <= 0) return false;
    int reserved = 0; for (const auto& r : warehouse_.getInventory()) if (r.getProductId() == productId) reserved += r.getReservedQuantity();
    if (reserved < quantity) return false;
    int left = quantity;
    for (auto& r : warehouse_.getInventory()) if (r.getProductId() == productId && r.getReservedQuantity() > 0) { int take = std::min(left, r.getReservedQuantity()); r.releaseReservation(take); left -= take; if (left == 0) break; }
    return true;
}

bool InventoryService::shipReservedProduct(int productId, int quantity, bool persist) {
    if (quantity <= 0) return false;
    int reserved = 0; for (const auto& r : warehouse_.getInventory()) if (r.getProductId() == productId) reserved += r.getReservedQuantity();
    if (reserved < quantity) return false;
    if (persist && database_ && !database_->changeWarehouseQuantity(warehouse_.getId(), productId, -quantity)) return false;
    int left = quantity;
    for (auto& r : warehouse_.getInventory()) if (r.getProductId() == productId && r.getReservedQuantity() > 0) { int take = std::min(left, r.getReservedQuantity()); r.shipReserved(take); left -= take; if (left == 0) break; }
    return true;
}

bool InventoryService::writeOffProduct(int productId, int quantity) {
    if (quantity <= 0 || getAvailableQuantity(productId) < quantity) return false;
    if (database_ && !database_->changeWarehouseQuantity(warehouse_.getId(), productId, -quantity)) return false;
    int left = quantity;
    for (auto& r : warehouse_.getInventory()) if (r.getProductId() == productId && r.getAvailableQuantity() > 0) { int take = std::min(left, r.getAvailableQuantity()); r.removeQuantity(take); left -= take; if (left == 0) break; }
    return true;
}
