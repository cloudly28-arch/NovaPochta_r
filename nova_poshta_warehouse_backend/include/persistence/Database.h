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
struct WarehouseStockRow
{
    int warehouseId{};
    int productId{};
    std::string productName;

    int quantity{};
    int capacity{};
    int minStock{};
    int unitPriceCents{};
    int shelfLifeDays{};
};

struct SupplierRequestRow
{
    int id{};
    int productId{};
    std::string productName;

    int requestedQuantity{};
    int createdDay{};
    int deliveryDay{};

    std::string status;
};
struct OrderItemRow
{
    int productId{};
    std::string productName;

    int requestedQuantity{};
    int allocatedQuantity{};
};

struct StoreOrderRow
{
    int id{};
    int storeId{};

    int createdDay{};
    int deliveryDay{};

    std::string status;

    std::vector<OrderItemRow> items;
};
struct WarehouseBatchRow {
    int id{};
    int warehouseId{};
    int productId{};
    std::string productName;
    int quantity{};
    int receivedDay{};
    int expiresDay{};
    int unitPriceCents{};
    int discountPercent{};
};
struct WarehouseWriteoffRow {
    int id{};
    int batchId{};
    int productId{};
    std::string productName;
    int writeoffDay{};
    int quantity{};
    long long lossCents{};
};
struct WarehouseAllocationStats {
    long long allocatedUnits{};
    long long allocatedValueCents{};
    long long discountLossCents{};
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
    bool executeSql(
            const std::string& sql
        );

    bool loadWarehouse(Warehouse& warehouse) const;
    std::vector<StoreStockRow> getStoreInventory(int storeId) const;
    std::vector<WarehouseStockRow>
    getWarehouseInventory(int warehouseId) const;

    bool createSupplierRequest(
        int productId,
        int requestedQuantity,
        int createdDay,
        int deliveryDay
    );

    std::vector<SupplierRequestRow>
    getSupplierRequests() const;
    bool hasActiveSupplierRequest(
        int productId
    ) const;
    bool setSupplierRequestStatus(
        int requestId,
        const std::string& status
    );

    bool setWarehouseQuantity(int warehouseId, int productId, int quantity);
    bool changeWarehouseQuantity(
        int warehouseId,
        int productId,
        int delta,
        int currentDay = 1
    );

    bool changeWarehouseQuantityRaw(
        int warehouseId,
        int productId,
        int delta
    );
    bool setStoreQuantity(int storeId, int productId, int quantity);
    bool changeStoreQuantity(int storeId, int productId, int delta);

    bool beginTransaction();
    bool commit();
    bool rollback();
    bool getActiveStoreOrder(
        int storeId,
        StoreOrderRow& order
    ) const;
    bool hasActiveStoreOrder(
        int storeId
    ) const;

    bool createStoreOrder(
        int storeId,
        int productId,
        int requestedQuantity,
        int createdDay,
        int deliveryDay
    );
    bool setOrderItemAllocated(
        int orderId,
        int productId,
        int allocatedQuantity
    );

    bool setOrderStatus(
        int orderId,
        const std::string& status
    );

    bool copyFrom(const Database& source);
    std::string getLastError() const;

    bool getWarehouseBatches(
        int warehouseId,
            std::vector<WarehouseBatchRow>& batches
        ) const;
        bool createWarehouseBatch(
        int warehouseId,
        int productId,
        int quantity,
        int receivedDay
    );
    bool writeOffExpiredBatches(
        int warehouseId,
        int currentDay
    );

    bool getWarehouseWriteoffs(
        int warehouseId,
        std::vector<WarehouseWriteoffRow>& writeoffs
    ) const;
    bool setBatchDiscount(
        int warehouseId,
        int batchId,
        int percent,
        int currentDay
    );
    bool getWarehouseAllocationStats(
        int warehouseId,
        WarehouseAllocationStats& stats
    ) const;
};
