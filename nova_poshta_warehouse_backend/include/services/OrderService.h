#pragma once

#include "Warehouse.h"

class InventoryService;
class Database;

class OrderService {
private:
    Warehouse& warehouse_;
    InventoryService& inventoryService_;
    Database* database_{nullptr};

public:
    OrderService(Warehouse& warehouse,
                 InventoryService& inventoryService,
                 Database* database = nullptr);

    bool createOrder(const Order& order);
    bool canReserveOrder(int orderId) const;
    bool reserveOrder(int orderId);
    bool cancelOrder(int orderId);
    bool shipOrder(int orderId);
    bool completeOrder(int orderId);
};
