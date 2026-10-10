#pragma once

#include <string>
#include <vector>

#include "Warehouse.h"
#include "persistence/Database.h"

struct StoreInfo
{
    int id{};
    std::string name;
};

struct ProductStockInfo
{
    int productId{};
    std::string productName;
    int quantity{};
    int capacity{};
    int minStock{};
};
struct WarehouseStockInfo
{
    int productId{};
    std::string productName;
    int quantity{};
    int capacity{};
    int minStock{};
};
struct SupplierRequestInfo
{
    int id{};
    int productId{};

    std::string productName;

    int requestedQuantity{};
    int createdDay{};
    int deliveryDay{};

    std::string status;
};
struct OrderItemInfo
{
    int productId{};
    std::string productName;

    int requestedQuantity{};
    int allocatedQuantity{};
};

struct StoreOrderInfo
{
    int id{};
    int storeId{};

    int createdDay{};
    int deliveryDay{};

    std::string status;

    std::vector<OrderItemInfo> items;
};
class BackendFacade
{
public:
    bool initialize(const std::string& databasePath);
    bool resetDatabase(
        const std::string& databasePath,
        const std::string& schemaPath
    );
    bool isReady() const;

    std::vector<StoreInfo> getStores() const;
    std::string getStoreName(int storeId) const;

    std::vector<ProductStockInfo>
    getStoreInventory(int storeId) const;
    std::vector<WarehouseStockInfo>
    getWarehouseInventory() const;
    std::vector<SupplierRequestInfo>
    getSupplierRequests() const;

    bool createSupplierRequest(
        int productId,
        int quantity,
        int currentDay,
        int deliveryDay
    );

    bool completeSupplierRequest(
        int requestId
    );

    bool getActiveStoreOrder(
    int storeId,
        StoreOrderInfo& order
    ) const;
    void simulateStoreSales(
        int currentDay
    );
    void processStoreDeliveries(
        int currentDay
    );
    void processStoreOrders(
        int currentDay
    );
    void processSupplierRequests(
        int currentDay
    );
    const std::string& getLastError() const;
    bool startExperiment(
        const std::string& databasePath,
        const std::string& schemaPath,
        int storeCount,
        int productCount
    );

private:
    Database database_;
    Warehouse warehouse_;
    bool ready_{false};
    
    std::string lastError_;
    int storeCount_ = 9;
    int productCount_ = 20;
};
