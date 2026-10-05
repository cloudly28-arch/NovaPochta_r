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

class BackendFacade
{
public:
    bool initialize(const std::string& databasePath);
    bool isReady() const;

    std::vector<StoreInfo> getStores() const;
    std::string getStoreName(int storeId) const;

    std::vector<ProductStockInfo>
    getStoreInventory(int storeId) const;

private:
    Database database_;
    Warehouse warehouse_;
    bool ready_{false};
};
