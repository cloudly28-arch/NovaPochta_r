#include "services/OrderService.h"
#include "services/InventoryService.h"
#include "persistence/Database.h"
#include <unordered_map>

OrderService::OrderService(Warehouse& warehouse, InventoryService& inventoryService, Database* database)
    : warehouse_(warehouse), inventoryService_(inventoryService), database_(database) {}

bool OrderService::createOrder(const Order& order) {
    if (!warehouse_.findStoreById(order.getStoreId()) || warehouse_.findOrderById(order.getId()) || order.getItems().empty()) return false;
    warehouse_.addOrder(order);
    return true;
}

bool OrderService::canReserveOrder(int orderId) const {
    auto* order = const_cast<Warehouse&>(warehouse_).findOrderById(orderId);
    if (!order || order->getStatus() != OrderStatus::Created) return false;
    for (const auto& item : order->getItems()) {
        if (inventoryService_.getAvailableQuantity(item.getProductId()) < item.getQuantity()) return false;
    }
    return true;
}

bool OrderService::reserveOrder(int orderId) {
    auto* order = warehouse_.findOrderById(orderId);
    if (!order || !canReserveOrder(orderId)) return false;

    std::vector<OrderItem> reservedItems;
    for (const auto& item : order->getItems()) {
        if (!inventoryService_.reserveProduct(item.getProductId(), item.getQuantity())) {
            for (const auto& rollbackItem : reservedItems) {
                inventoryService_.releaseReservation(rollbackItem.getProductId(), rollbackItem.getQuantity());
            }
            return false;
        }
        reservedItems.push_back(item);
    }

    order->setStatus(OrderStatus::Reserved);
    return true;
}

bool OrderService::cancelOrder(int orderId) {
    auto* order = warehouse_.findOrderById(orderId);
    if (!order) return false;

    if (order->getStatus() == OrderStatus::Shipped ||
        order->getStatus() == OrderStatus::Completed ||
        order->getStatus() == OrderStatus::Cancelled) {
        return false;
    }

    if (order->getStatus() == OrderStatus::Reserved ||
        order->getStatus() == OrderStatus::InProgress ||
        order->getStatus() == OrderStatus::ReadyForShipment) {
        for (const auto& item : order->getItems()) {
            inventoryService_.releaseReservation(item.getProductId(), item.getQuantity());
        }
    }

    order->setStatus(OrderStatus::Cancelled);
    return true;
}

bool OrderService::shipOrder(int orderId) {
    auto* order = warehouse_.findOrderById(orderId);
    if (!order || (order->getStatus() != OrderStatus::Reserved &&
                   order->getStatus() != OrderStatus::ReadyForShipment)) {
        return false;
    }

    // First validate that the target store has these products and enough free capacity.
    if (database_) {
        const auto stock = database_->getStoreInventory(order->getStoreId());
        std::unordered_map<int, StoreStockRow> byProduct;
        for (const auto& row : stock) byProduct[row.productId] = row;

        for (const auto& item : order->getItems()) {
            auto it = byProduct.find(item.getProductId());
            if (it == byProduct.end()) return false;
            if (it->second.quantity + item.getQuantity() > it->second.capacity) return false;
        }

        if (!database_->beginTransaction()) return false;

        for (const auto& item : order->getItems()) {
            if (!database_->changeWarehouseQuantity(warehouse_.getId(), item.getProductId(), -item.getQuantity()) ||
                !database_->changeStoreQuantity(order->getStoreId(), item.getProductId(), item.getQuantity())) {
                database_->rollback();
                return false;
            }
        }

        if (!database_->commit()) {
            database_->rollback();
            return false;
        }
    }

    // Update in-memory warehouse only after the DB transaction has succeeded.
    for (const auto& item : order->getItems()) {
        if (!inventoryService_.shipReservedProduct(item.getProductId(), item.getQuantity(), false)) {
            return false; // should not happen after a successful reservation
        }
    }

    order->setStatus(OrderStatus::Shipped);
    return true;
}

bool OrderService::completeOrder(int orderId) {
    auto* order = warehouse_.findOrderById(orderId);
    if (!order || order->getStatus() != OrderStatus::Shipped) return false;
    order->setStatus(OrderStatus::Completed);
    return true;
}
