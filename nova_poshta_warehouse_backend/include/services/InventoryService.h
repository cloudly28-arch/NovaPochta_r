#pragma once

#include <string>
#include "Warehouse.h"

class Database;

class InventoryService {
private:
    Warehouse& warehouse_;
    Database* database_{nullptr};

public:
    explicit InventoryService(Warehouse& warehouse, Database* database = nullptr);

    int getTotalQuantity(int productId) const;
    int getAvailableQuantity(int productId) const;

    bool receiveProduct(int productId,
                        int quantity,
                        int storageCellId = -1,
                        const std::string& batchNumber = {},
                        const std::string& expirationDate = {});

    bool reserveProduct(int productId, int quantity);
    bool releaseReservation(int productId, int quantity);
    bool shipReservedProduct(int productId, int quantity, bool persist = true);
    bool writeOffProduct(int productId, int quantity);
};
