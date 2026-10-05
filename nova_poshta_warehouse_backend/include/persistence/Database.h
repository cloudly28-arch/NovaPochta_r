#pragma once

#include <string>
#include <vector>
#include "Warehouse.h"

struct StoreStockRow {
    int storeId{};
    std::string storeName;
    int productId{};
    std::string productName;
    int quantity{};
    int capacity{};
    int minStock{};
};

class Database {
private:
    struct sqlite3* db_{nullptr};

public:
    Database() = default;
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool open(const std::string& path);
    void close();
    bool isOpen() const;

    bool loadWarehouse(Warehouse& warehouse) const;
    std::vector<StoreStockRow> getStoreInventory(int storeId) const;

    bool setWarehouseQuantity(int warehouseId, int productId, int quantity);
    bool changeWarehouseQuantity(int warehouseId, int productId, int delta);
    bool setStoreQuantity(int storeId, int productId, int quantity);
    bool changeStoreQuantity(int storeId, int productId, int delta);

    bool beginTransaction();
    bool commit();
    bool rollback();
};
