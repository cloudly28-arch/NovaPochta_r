#include "application/BackendFacade.h"
#include <algorithm>
#include <random>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

namespace {
long long adjustedFoodStock(long long quantity, int shelfLifeDays) {
    return shelfLifeDays <= 7 ? (quantity * 110 + 99) / 100 : quantity;
}
int stockCoverDays(int shelfLifeDays) {
    if (shelfLifeDays <= 7) { return std::min(3, std::max(1, shelfLifeDays - 1)); }
    return shelfLifeDays <= 30 ? 5 : 7;
}
}

bool BackendFacade::initialize(const std::string& databasePath)
{
    lastError_.clear();
    ready_ = false;

    database_.close();
    warehouse_ = Warehouse{};

    if (!database_.open(databasePath)) {
        lastError_ = "Cannot open database: " + databasePath;
        return false;
    }

    if (!database_.loadWarehouse(warehouse_)) {
        lastError_ = "Cannot load warehouse: " + databasePath +
                     "\nSQLite: " + database_.getLastError();

        database_.close();
        return false;
    }

    ready_ = true;
    return true;
}

bool BackendFacade::resetDatabase(
    const std::string& databasePath,
    const std::string& schemaPath
)
{
    lastError_.clear();

    std::ifstream schemaFile(schemaPath, std::ios::binary);

    if (!schemaFile) {
        lastError_ = "Cannot open schema: " + schemaPath;
        return false;
    }

    std::ostringstream buffer;
    buffer << schemaFile.rdbuf();

    const std::string schema = buffer.str();

    if (schema.empty() || schemaFile.bad()) {
        lastError_ = "Schema is empty or unreadable: " + schemaPath;
        return false;
    }

    Database prepared;

    if (!prepared.open(":memory:")) {
        lastError_ = "Cannot create temporary database";
        return false;
    }

    if (!prepared.executeSql(schema)) {
        lastError_ = "Invalid schema: " + schemaPath +
                     "\nSQLite: " + prepared.getLastError();
        return false;
    }

    // Настраиваем магазины и товары в новой базе.
    const std::string configureSql =
        "BEGIN TRANSACTION;"

        "DELETE FROM stores WHERE id > " +
        std::to_string(storeCount_) + ";"

        "DELETE FROM products WHERE id > " +
        std::to_string(productCount_) + ";"

        "COMMIT;";

    if (!prepared.executeSql(configureSql)) {
        lastError_ = "Cannot configure experiment:\n" +
                     prepared.getLastError();
        return false;
    }

    Warehouse initialWarehouse;

    if (!prepared.loadWarehouse(initialWarehouse)) {
        lastError_ = "Schema does not contain a valid warehouse";
        return false;
    }

    // Keep a moderate reserve; a small surplus can still expire naturally.
    const std::vector<WarehouseStockRow> initialStock =
        prepared.getWarehouseInventory(initialWarehouse.getId());
    std::string initialStockSql = "BEGIN TRANSACTION;";
    std::mt19937 initialGenerator(static_cast<unsigned int>(storeCount_ * 7919 + productCount_ * 101));
    std::uniform_int_distribution<int> surplusDistribution(5, 10);
    for (const WarehouseStockRow& product : initialStock) {
        long long dailyDemand = 0;
        for (const Store& store : initialWarehouse.getStores()) {
            for (const StoreStockRow& item : prepared.getStoreInventory(store.getId())) {
                if (item.productId == product.productId) { dailyDemand += std::min(12, item.capacity); }
            }
        }
        const long long pack = std::max(1, product.unitsPerPackage);
        const int coverDays = stockCoverDays(product.shelfLifeDays);
        const long long target = adjustedFoodStock(
            (dailyDemand * coverDays * (100 + surplusDistribution(initialGenerator)) + 99) / 100, product.shelfLifeDays);
        const long long quantity = std::min(((target + pack - 1) / pack) * pack,
            (static_cast<long long>(product.capacity) / pack) * pack);
        const long long minimum = std::min(quantity,
            ((dailyDemand * std::min(3,coverDays) + pack - 1) / pack) * pack);
        initialStockSql += "UPDATE warehouse_inventory SET quantity=" +
            std::to_string(quantity) + ", min_stock=" + std::to_string(minimum) +
            " WHERE warehouse_id=" + std::to_string(initialWarehouse.getId()) +
            " AND product_id=" + std::to_string(product.productId) + ";";
        initialStockSql += "UPDATE warehouse_batches SET quantity=" +
            std::to_string(quantity) +
            " WHERE warehouse_id=" + std::to_string(initialWarehouse.getId()) +
            " AND product_id=" + std::to_string(product.productId) + ";";
    }
    initialStockSql += "COMMIT;";
    if (!prepared.executeSql(initialStockSql)) {
        lastError_ = "Cannot configure initial stock: " + prepared.getLastError();
        return false;
    }

    const std::filesystem::path parent =
        std::filesystem::path(databasePath).parent_path();

    if (!parent.empty()) {
        std::error_code error;
        std::filesystem::create_directories(parent, error);

        if (error) {
            lastError_ = "Cannot create database directory: " +
                         error.message();
            return false;
        }
    }

    Database destination;

    if (!destination.open(databasePath)) {
        lastError_ = "Cannot open destination database: " +
                     databasePath;
        return false;
    }

    if (!destination.copyFrom(prepared)) {
        lastError_ = "Cannot reset database: " +
                     destination.getLastError();
        return false;
    }

    destination.close();

    return initialize(databasePath);
}
const std::string& BackendFacade::getLastError() const
{
    return lastError_;
}
bool BackendFacade::isReady() const
{
    return ready_;
}

std::vector<StoreInfo>
BackendFacade::getStores() const
{
    std::vector<StoreInfo> result;

    if (!ready_)
    {
        return result;
    }

    result.reserve(
        warehouse_.getStores().size()
    );

    for (const Store& store :
         warehouse_.getStores())
    {
        result.push_back(
            StoreInfo{
                store.getId(),
                store.getName()
            }
        );
    }

    return result;
}

std::string
BackendFacade::getStoreName(
    int storeId
) const
{
    if (!ready_)
    {
        return {};
    }

    for (const Store& store :
         warehouse_.getStores())
    {
        if (store.getId() == storeId)
        {
            return store.getName();
        }
    }

    return {};
}

std::vector<ProductStockInfo>
BackendFacade::getStoreInventory(
    int storeId
) const
{
    std::vector<ProductStockInfo> result;

    if (!ready_)
    {
        return result;
    }

    const std::vector<StoreStockRow> rows =
        database_.getStoreInventory(
            storeId
        );

    result.reserve(rows.size());

    for (const StoreStockRow& row : rows)
    {
        result.push_back(
            ProductStockInfo{
                row.productId,
                row.productName,
                row.quantity,
                row.capacity,
                row.minStock
            }
        );
    }

    return result;
}
std::vector<WarehouseStockInfo>
BackendFacade::getWarehouseInventory() const
{
    std::vector<WarehouseStockInfo> result;

    if (!ready_)
    {
        return result;
    }

    const std::vector<WarehouseStockRow> rows =
        database_.getWarehouseInventory(
            warehouse_.getId()
        );

    result.reserve(rows.size());

    for (const WarehouseStockRow& row : rows)
    {
        result.push_back(
            WarehouseStockInfo{
                row.productId,
                row.productName,
                row.quantity,
                row.capacity,
                row.minStock,
                row.unitPriceCents,
                row.shelfLifeDays,
                row.unitsPerPackage
            }
        );
    }

    return result;
}
std::vector<SupplierRequestInfo>
BackendFacade::getSupplierRequests() const
{
    std::vector<SupplierRequestInfo> result;

    if (!ready_)
    {
        return result;
    }

    const std::vector<SupplierRequestRow> rows =
        database_.getSupplierRequests();

    result.reserve(rows.size());

    for (const SupplierRequestRow& row : rows)
    {
        result.push_back(
            SupplierRequestInfo{
                row.id,
                row.productId,
                row.productName,
                row.requestedQuantity,
                row.createdDay,
                row.deliveryDay,
                row.status
            }
        );
    }

    return result;
}

bool BackendFacade::createSupplierRequest(
    int productId,
    int quantity,
    int currentDay,
    int deliveryDay
)
{
    if (!ready_)
    {
        return false;
    }

    return database_.createSupplierRequest(
        productId,
        quantity,
        currentDay,
        deliveryDay
    );
}

bool BackendFacade::completeSupplierRequest(int requestId, int receivedDay) {
    if (!ready_) {
        return false;
    }

    const std::vector<SupplierRequestRow> requests =
        database_.getSupplierRequests();

    for (const SupplierRequestRow& request : requests) {
        if (request.id != requestId) {
            continue;
        }

        if (request.status == "Delivered") {
            return true;
        }

        if (request.status != "InTransit"){
            lastError_ = "Supplier request cannot be delivered";
            return false;
        }

        if (!database_.beginTransaction()) {
            lastError_ = database_.getLastError();
            return false;
        }

        const bool stockUpdated =
            database_.changeWarehouseQuantity(
                warehouse_.getId(),
                request.productId,
                request.requestedQuantity
            );

        const bool batchCreated =
            stockUpdated &&
            database_.createWarehouseBatch(
                warehouse_.getId(),
                request.productId,
                request.requestedQuantity,
                receivedDay > 0 ? receivedDay : request.deliveryDay
            );

        const bool statusUpdated =
            batchCreated &&
            database_.setSupplierRequestStatus(
                request.id,
                "Delivered"
            );

        if (!statusUpdated) {
            lastError_ =
                "Failed to receive supplier delivery: " +
                database_.getLastError();

            database_.rollback();
            return false;
        }

        if (!database_.commit()) {
            lastError_ = database_.getLastError();
            database_.rollback();
            return false;
        }

        lastError_.clear();
        return true;
    }

    lastError_ = "Supplier request not found";
    return false;
}
bool BackendFacade::getActiveStoreOrder(
    int storeId,
    StoreOrderInfo& order
) const
{
    if (!ready_)
    {
        return false;
    }

    StoreOrderRow row;

    if (
        !database_.getActiveStoreOrder(
            storeId,
            row
        )
    )
    {
        return false;
    }

    order.id =
        row.id;

    order.storeId =
        row.storeId;

    order.createdDay =
        row.createdDay;

    order.deliveryDay =
        row.deliveryDay;

    order.status =
        row.status;

    order.items.clear();

    for (
        const OrderItemRow& item :
        row.items
    )
    {
        order.items.push_back(
            OrderItemInfo{
                item.productId,
                item.productName,
                item.requestedQuantity,
                item.allocatedQuantity
            }
        );
    }

    return true;
}
bool BackendFacade::simulateStoreSales(int currentDay, int intervalIndex)
{
    if (!ready_ || currentDay <= 0 || intervalIndex < 1 || intervalIndex > 8) {
        return false;
    }

    if (!database_.beginTransaction()) {
        lastError_ = "Cannot start sales transaction: " +
                     database_.getLastError();
        std::cerr << lastError_ << '\n';
        return false;
    }

    std::mt19937 generator(
        static_cast<unsigned int>(currentDay * 1009 + intervalIndex * 9176)
    );

    std::uniform_int_distribution<int> salesDistribution(1, 2);

    const std::vector<StoreInfo> stores = getStores();

    for (const StoreInfo& store : stores) {
        const std::vector<ProductStockInfo> inventory =
            getStoreInventory(store.id);

        for (const ProductStockInfo& product : inventory) {
            if (product.quantity <= 0) {
                continue;
            }

            const int sold = std::min(
                salesDistribution(generator),
                product.quantity
            );

            if (!database_.changeStoreQuantity(
                    store.id,
                    product.productId,
                    -sold)) {
                lastError_ = "Cannot update store stock: " +
                             database_.getLastError();

                database_.rollback();
                std::cerr << lastError_ << '\n';
                return false;
            }
        }
    }

    if (!database_.commit()) {
        lastError_ = "Cannot save store sales: " +
                     database_.getLastError();

        database_.rollback();
        std::cerr << lastError_ << '\n';
        return false;
    }
    lastError_.clear();
    return true;
}
void BackendFacade::processStoreDeliveries(int currentDay, int onlyStoreId) {
    if (!ready_ || currentDay <= 0) {
        return;
    }

    const std::vector<StoreInfo> stores = getStores();

    for (const StoreInfo& store : stores) {
        if (onlyStoreId != 0 && store.id != onlyStoreId) { continue; }
        StoreOrderRow order;

        if (!database_.getActiveStoreOrder(store.id, order)) {
            continue;
        }

        if (order.status != "Allocated" ||
            order.deliveryDay > currentDay) {
            continue;
        }

        if (!database_.isOrderStockValid(order.id, currentDay)) {
            lastError_ = "Delivery blocked: expired or inconsistent reservation, order #" +
                         std::to_string(order.id);
            std::cerr << lastError_ << '\n';
            continue;
        }

        bool hasAllocatedGoods = false;

        for (const OrderItemRow& item : order.items) {
            if (item.allocatedQuantity > 0) {
                hasAllocatedGoods = true;
                break;
            }
        }

        if (!hasAllocatedGoods) {
            continue;
        }

        if (!database_.beginTransaction()) {
            lastError_ = "Cannot begin store delivery";
            return;
        }

        bool delivered = true;

        for (const OrderItemRow& item : order.items) {
            if (item.allocatedQuantity <= 0) {
                continue;
            }

            if (!database_.changeStoreQuantity(
                    store.id,
                    item.productId,
                    item.allocatedQuantity
                )) {
                delivered = false;
                break;
            }
        }

        if (delivered) {
            delivered = database_.setOrderStatus(
                order.id,
                "Completed"
            );
        }

        if (!delivered) {
            lastError_ = "Cannot complete store delivery";
            database_.rollback();
            return;
        }

        if (!database_.commit()) {
            lastError_ = "Cannot commit store delivery";
            database_.rollback();
            return;
        }

        lastError_.clear();
    }
}
void BackendFacade::processStoreOrders(
    int currentDay
)
{
    if (
        !ready_ ||
        currentDay <= 0
    )
    {
        return;
    }

    std::mt19937 generator(
        static_cast<unsigned int>(
            currentDay * 7919
        )
    );


    const std::vector<StoreInfo> stores =
        getStores();

    for (const StoreInfo& store : stores) {
        if (database_.hasActiveStoreOrder(store.id)) {
            continue;
        }

        const std::vector<ProductStockInfo> inventory =
            getStoreInventory(store.id);

        const std::vector<WarehouseStockRow> warehouseStock =
            database_.getWarehouseInventory(warehouse_.getId());
        std::vector<WarehouseBatchRow> batches;

        if (!database_.getWarehouseBatches(
                warehouse_.getId(),
                batches
            )) {
            lastError_ = "Cannot load batches for order generation";
            return;
        }

        std::vector<OrderItemRow> candidates;
        std::vector<double> candidateWeights;

        for (const ProductStockInfo& product : inventory) {
            int discountPercent = 0;

            for (const WarehouseBatchRow& batch : batches) {
                if (batch.productId == product.productId &&
                    batch.quantity > 0 &&
                    batch.expiresDay > currentDay) {
                    discountPercent = batch.discountPercent;
                    break;
                }
            }

            if (product.quantity > product.minStock &&
                discountPercent == 0) {
                continue;
            }

            int packageSize = 0;

            for (const WarehouseStockRow& stock : warehouseStock) {
                if (stock.productId == product.productId) {
                    packageSize = stock.unitsPerPackage;
                    break;
                }
            }

            if (packageSize <= 0) {
                continue;
            }

            const int freeSpace =
                std::max(0, product.capacity - product.quantity);

            const int maxPackages = freeSpace / packageSize;

            if (maxPackages == 0) {
                continue;
            }

            std::uniform_int_distribution<int> orderVariation(-8, 8);
            // Order half of this shop's capacity, with relative variation of +/-8%.
            const long long requested = std::max(1LL,
                (static_cast<long long>(product.capacity) * (100 + orderVariation(generator)) + 199) / 200);
            const int packages = static_cast<int>(std::min(static_cast<long long>(maxPackages),
                (requested + packageSize - 1) / packageSize));
            if (packages == 0) { continue; }
            OrderItemRow item;
            item.productId = product.productId;
            item.productName = product.productName;
            item.requestedQuantity = packages * packageSize;
            item.allocatedQuantity = 0;

            candidates.push_back(item);
            const double weight = 1.0 + discountPercent / 20.0;
            candidateWeights.push_back(weight);
        }

        if (candidates.empty()) {
            continue;
        }

        // Include every depleted position, rather than randomly omitting some.
        const std::vector<OrderItemRow> selectedItems = candidates;

        if (!database_.createStoreOrder(
                store.id,
                selectedItems,
                currentDay,
                currentDay + 1
            )) {
            lastError_ =
                "Cannot create store order: " +
                database_.getLastError();
            return;
        }
    }

    for (
        const StoreInfo& store :
        stores
    )
    {
        StoreOrderRow order;

        if (
            !database_.getActiveStoreOrder(
                store.id,
                order
            )
        )
        {
            continue;
        }

        // A packed order must remain unchanged while its truck is travelling.
        if (order.status == "Allocated") { continue; }

        for (
            const OrderItemRow& item :
            order.items
        )
        {
            const int remaining =
                item.requestedQuantity -
                item.allocatedQuantity;

            if (
                remaining <= 0
            )
            {
                continue;
            }

            const std::vector<WarehouseStockRow>
                warehouseInventory =
                    database_.getWarehouseInventory(
                        warehouse_.getId()
                    );

            const WarehouseStockRow* warehouseProduct =
                nullptr;

            for (
                const WarehouseStockRow& stock :
                warehouseInventory
            )
            {
                if (
                    stock.productId ==
                    item.productId
                )
                {
                    warehouseProduct =
                        &stock;

                    break;
                }
            }



            const int plannedDeliveryDay = std::max(currentDay, order.deliveryDay);
            std::vector<WarehouseBatchRow> dispatchBatches;
            if (!database_.getWarehouseBatches(warehouse_.getId(), dispatchBatches)) {
                lastError_ = "Cannot check batches for delivery";
                return;
            }
            long long validQuantity = 0;
            for (const WarehouseBatchRow& batch : dispatchBatches) {
                if (batch.productId == item.productId && batch.expiresDay > plannedDeliveryDay)
                    validQuantity += batch.quantity;
            }
            const int available = static_cast<int>(std::min(
                validQuantity, static_cast<long long>(warehouseProduct->quantity)));

            const int packageSize = warehouseProduct->unitsPerPackage;
            if (packageSize <= 0) {
                lastError_ = "Invalid package size";
                return;
            }



            int storeFreeSpace = 0;

            const std::vector<StoreStockRow> storeStock =
                database_.getStoreInventory(store.id);

            for (const StoreStockRow& stock : storeStock) {
                if (stock.productId == item.productId) {
                    storeFreeSpace = std::max(
                        0,
                        stock.capacity -
                        stock.quantity -
                        item.allocatedQuantity
                    );
                    break;
                }
            }

            const int allocationLimit =
                std::min(
                    remaining,
                    std::min(available, storeFreeSpace)
                );

            int allocatedNow =
                (allocationLimit / packageSize) * packageSize;

            if (allocatedNow > 0) {
                if (!database_.beginTransaction()) {
                    lastError_ = "Cannot begin order allocation";
                    return;
                }

                const bool stockChanged =
                    database_.changeWarehouseQuantity(
                        warehouse_.getId(),
                        item.productId,
                        -allocatedNow,
                        currentDay,
                        plannedDeliveryDay,
                        order.id
                    );

                const bool orderChanged =
                    stockChanged &&
                    database_.setOrderItemAllocated(
                        order.id,
                        item.productId,
                        item.allocatedQuantity + allocatedNow
                    );

                if (!orderChanged) {
                    lastError_ = "Cannot allocate packages to order";
                    database_.rollback();
                    return;
                }

                if (!database_.commit()) {
                    lastError_ = "Cannot commit order allocation";
                    database_.rollback();
                    return;
                }
            }

            // Supplier requests are calculated centrally once per day,
            // after all stores have reserved their goods.


        }

        StoreOrderRow updatedOrder;

        if (!database_.getActiveStoreOrder(store.id, updatedOrder)) {
            lastError_ = "Cannot reload store order";
            return;
        }

        bool hasAllocatedGoods = false;

        for (const OrderItemRow& item : updatedOrder.items) {
            if (item.allocatedQuantity > 0) {
                hasAllocatedGoods = true;
                break;
            }
        }

        const std::string nextStatus =
            hasAllocatedGoods ? "Allocated" : "WaitingSupply";

        if (!database_.setOrderStatus(order.id, nextStatus)) {
            lastError_ = "Cannot update store order status";
            return;
        }
    }
}

void BackendFacade::processSupplierRequests(int currentDay) {
    if (!ready_ || currentDay <= 0) { return; }
    const std::vector<StoreInfo> stores = getStores();
    const std::vector<WarehouseStockRow> inventory = database_.getWarehouseInventory(warehouse_.getId());
    const std::vector<SupplierRequestRow> pendingRequests = database_.getSupplierRequests();
    std::mt19937 generator(static_cast<unsigned int>(currentDay * 7919));
    std::uniform_int_distribution<int> delayDistribution(1, 5), bufferDistribution(5, 10);
    for (const WarehouseStockRow& product : inventory) {
        if (product.unitsPerPackage <= 0) { continue; }
        const int delay = delayDistribution(generator);
        long long alreadyOrdered = 0;
        for (const SupplierRequestRow& request : pendingRequests) {
            if (request.productId == product.productId &&
                (request.status == "Created" || request.status == "InTransit")) {
                alreadyOrdered += request.requestedQuantity;
            }
        }
        const int buffer = bufferDistribution(generator);
        // Sum each store's shortage separately: one shop's excess cannot feed another shop.
        long long totalShortage = 0, outstandingDemand = 0, dailyDemand = 0;
        const int horizonTenths = std::min(delay * 10 + 9, std::max(1, product.shelfLifeDays - 1) * 10);
        for (const StoreInfo& store : stores) {
            const std::vector<StoreStockRow> stock = database_.getStoreInventory(store.id);
            for (const StoreStockRow& item : stock) {
                if (item.productId != product.productId) { continue; }
                const int consumption = std::min(12, item.capacity);
                dailyDemand += consumption;
                long long allocated = 0;
                StoreOrderRow order;
                if (database_.getActiveStoreOrder(store.id, order)) {
                    for (const OrderItemRow& orderItem : order.items) {
                        if (orderItem.productId == product.productId) {
                            allocated += orderItem.allocatedQuantity;
                            outstandingDemand += std::max(0, orderItem.requestedQuantity - orderItem.allocatedQuantity);
                        }
                    }
                }
                const long long target = std::min(static_cast<long long>(item.capacity),
                    (static_cast<long long>(consumption) * horizonTenths * (100 + buffer) + 999) / 1000);
                totalShortage += std::max(0LL, target - item.quantity - allocated);
            }
        }
        const long long stockPosition = product.quantity + alreadyOrdered;
        const int coverDays = stockCoverDays(product.shelfLifeDays);
        const long long reserveTarget = adjustedFoodStock(
            (dailyDemand * coverDays * (100 + buffer) + 99) / 100, product.shelfLifeDays);
        const long long demand = std::max(totalShortage, outstandingDemand);
        const long long trigger = dailyDemand * std::min(coverDays, delay + 1);
        if (stockPosition >= trigger && stockPosition >= demand) { continue; }
        const long long shelfLimit = adjustedFoodStock(
            (dailyDemand * std::max(1,product.shelfLifeDays - 1) * (100 + buffer) + 99) / 100, product.shelfLifeDays);
        const long long target = std::min(std::min(static_cast<long long>(product.capacity), shelfLimit),
            std::max(reserveTarget, demand));
        const long long needed = target - stockPosition;
        if (needed <= 0) { continue; }
        const long long pack = product.unitsPerPackage;
        const long long space = std::max(0LL, static_cast<long long>(product.capacity) - stockPosition);
        const long long rounded = ((needed + pack - 1) / pack) * pack;
        const int quantity = static_cast<int>(std::min(rounded, (space / pack) * pack));
        if (quantity > 0 && !database_.createSupplierRequest(product.productId, quantity, currentDay, currentDay + delay)) {
            lastError_ = "Cannot order combined store shortages";
            return;
        }
    }
}

bool BackendFacade::startExperiment(
    const std::string& databasePath,
    const std::string& schemaPath,
    int storeCount,
    int productCount
)
{
    if (storeCount < 3 || storeCount > 9 ||
        productCount < 12 || productCount > 20) {
        lastError_ =
            "Stores must be 3..9 and products must be 12..20";
        return false;
    }

    const int previousStoreCount = storeCount_;
    const int previousProductCount = productCount_;

    storeCount_ = storeCount;
    productCount_ = productCount;

    if (!resetDatabase(databasePath, schemaPath)) {
        storeCount_ = previousStoreCount;
        productCount_ = previousProductCount;
        return false;
    }

    return true;
}

void BackendFacade::processSupplierDeliveries(int currentDay) {
    if (!ready_ || currentDay < 1) {
        return;
    }

    const std::vector<SupplierRequestRow> requests =
        database_.getSupplierRequests();

    for (const SupplierRequestRow& request : requests) {
        if (request.status != "InTransit" ||
            request.deliveryDay > currentDay) {
            continue;
        }

        if (!completeSupplierRequest(request.id)) {
            std::cerr << getLastError() << '\n';
            return;
        }
    }
}
bool BackendFacade::getWarehouseBatches(
    std::vector<WarehouseBatchRow>& batches
) const {
    batches.clear();

    if (!ready_) {
        return false;
    }

    return database_.getWarehouseBatches(
        warehouse_.getId(),
        batches
    );
}

bool BackendFacade::writeOffExpiredBatches(int currentDay) {
    if (!ready_) {
        lastError_ = "Database is not ready";
        return false;
    }

    if (!database_.writeOffExpiredBatches(
            warehouse_.getId(), currentDay
        )) {
        lastError_ = "Failed to write off expired batches";
        return false;
    }

    std::vector<WarehouseBatchRow> batches;
    if (!database_.getWarehouseBatches(warehouse_.getId(), batches)) {
        lastError_ = "Cannot check batches for automatic discount";
        return false;
    }
    for (const WarehouseBatchRow& batch : batches) {
        const int remaining = batch.expiresDay - currentDay;
        const int discount = remaining == 1 ? 30 : (remaining == 2 ? 15 : 0);
        if (batch.quantity > 0 && discount > batch.discountPercent &&
            !database_.setBatchDiscount(warehouse_.getId(), batch.id, discount, currentDay)) {
            lastError_ = "Cannot apply automatic discount";
            return false;
        }
    }
    lastError_.clear();
    return true;
}

bool BackendFacade::getWarehouseWriteoffs(
    std::vector<WarehouseWriteoffRow>& writeoffs
) const {
    writeoffs.clear();

    if (!ready_) {
        return false;
    }

    return database_.getWarehouseWriteoffs(
        warehouse_.getId(),
        writeoffs
    );
}

bool BackendFacade::setBatchDiscount(
    int batchId,
    int percent,
    int currentDay
) {
    if (!ready_) {
        lastError_ = "Database is not ready";
        return false;
    }

    if (!database_.setBatchDiscount(
            warehouse_.getId(),
            batchId,
            percent,
            currentDay
        )) {
        lastError_ =
            "Cannot discount batch: check quantity and expiry";
        return false;
    }

    lastError_.clear();
    return true;
}

bool BackendFacade::getWarehouseAllocationStats(
    WarehouseAllocationStats& stats
) const {
    stats = WarehouseAllocationStats{};

    if (!ready_) {
        return false;
    }

    return database_.getWarehouseAllocationStats(
        warehouse_.getId(),
        stats
    );
}

bool BackendFacade::approveSupplierRequest(
    int requestId,
    int currentDay
) {
    if (!ready_ || currentDay < 1) {
        lastError_ = "Invalid supplier approval";
        return false;
    }

    const std::vector<SupplierRequestRow> requests =
        database_.getSupplierRequests();

    for (const SupplierRequestRow& request : requests) {
        if (request.id != requestId) {
            continue;
        }

        if (request.status != "Created") {
            lastError_ = "Request is not awaiting approval";
            return false;
        }

        const int delay = std::clamp(
            request.deliveryDay - request.createdDay,
            1,
            5
        );

        const int deliveryDay = currentDay + delay;

        if (!database_.beginTransaction()) {
            lastError_ = "Cannot begin supplier approval";
            return false;
        }

        const std::string sql =
            "UPDATE supplier_requests "
            "SET delivery_day = " +
            std::to_string(deliveryDay) +
            ", status = 'InTransit' "
            "WHERE id = " + std::to_string(requestId) +
            " AND status = 'Created';";

        if (!database_.executeSql(sql)) {
            lastError_ = database_.getLastError();
            database_.rollback();
            return false;
        }

        if (!database_.commit()) {
            lastError_ = database_.getLastError();
            database_.rollback();
            return false;
        }

        lastError_.clear();
        return true;
    }

    lastError_ = "Supplier request not found";
    return false;
}


bool BackendFacade::getExperimentReport(ExperimentReport& report) const {
    report = ExperimentReport{};
    return ready_ && database_.getExperimentReport(warehouse_.getId(), report);
}


bool BackendFacade::canDispatchStoreOrder(int storeId, int deliveryDay) const {
    StoreOrderRow order;
    return ready_ && database_.getActiveStoreOrder(storeId, order) &&
        order.status == "Allocated" && order.deliveryDay <= deliveryDay &&
        database_.isOrderStockValid(order.id, deliveryDay);
}

bool BackendFacade::getWarehouseSupplySummary(std::vector<WarehouseSupplySummary>& rows) const {
    rows.clear();
    return ready_ && database_.getWarehouseSupplySummary(warehouse_.getId(), rows);
}
