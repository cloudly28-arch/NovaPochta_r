#include "application/BackendFacade.h"
#include <algorithm>
#include <random>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

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
                row.minStock
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

bool BackendFacade::completeSupplierRequest(
    int requestId
)
{
    if (!ready_)
    {
        return false;
    }

    const std::vector<SupplierRequestRow> requests =
        database_.getSupplierRequests();

    for (const SupplierRequestRow& request : requests)
    {
        if (request.id != requestId)
        {
            continue;
        }

        if (request.status == "Delivered")
        {
            return true;
        }

        if (
            !database_.changeWarehouseQuantity(
                warehouse_.getId(),
                request.productId,
                request.requestedQuantity
            )
        )
        {
            return false;
        }

        return database_.setSupplierRequestStatus(
            request.id,
            "Delivered"
        );
    }

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
void BackendFacade::simulateStoreSales(int currentDay)
{
    if (!ready_ || currentDay <= 0) {
        return;
    }

    if (!database_.beginTransaction()) {
        lastError_ = "Cannot start sales transaction: " +
                     database_.getLastError();
        std::cerr << lastError_ << '\n';
        return;
    }

    std::mt19937 generator(
        static_cast<unsigned int>(currentDay * 1009)
    );

    std::uniform_int_distribution<int> salesDistribution(5, 15);

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
                return;
            }
        }
    }

    if (!database_.commit()) {
        lastError_ = "Cannot save store sales: " +
                     database_.getLastError();

        database_.rollback();
        std::cerr << lastError_ << '\n';
    }
}
void BackendFacade::processStoreDeliveries(
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

    const std::vector<StoreInfo> stores =
        getStores();

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

        if (
            order.status != "Allocated"
        )
        {
            continue;
        }

        if (
            order.deliveryDay >
            currentDay
        )
        {
            continue;
        }

        bool delivered = true;

        for (
            const OrderItemRow& item :
            order.items
        )
        {
            if (
                item.allocatedQuantity <= 0
            )
            {
                continue;
            }

            if (
                !database_.changeStoreQuantity(
                    store.id,
                    item.productId,
                    item.allocatedQuantity
                )
            )
            {
                delivered = false;
                break;
            }
        }

        if (delivered)
        {
            database_.setOrderStatus(
                order.id,
                "Completed"
            );
        }
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

    std::uniform_int_distribution<int>
        supplierDelay(
            1,
            5
        );

    const std::vector<StoreInfo> stores =
        getStores();

    // Сначала магазины создают новые заказы.
    for (
        const StoreInfo& store :
        stores
    )
    {
        if (
            database_.hasActiveStoreOrder(
                store.id
            )
        )
        {
            continue;
        }

        const std::vector<ProductStockInfo>
            inventory =
                getStoreInventory(
                    store.id
                );

        for (
            const ProductStockInfo& product :
            inventory
        )
        {
            if (
                product.quantity >
                product.minStock
            )
            {
                continue;
            }

            const int requestedQuantity =
                product.capacity -
                product.quantity;

            if (
                requestedQuantity <= 0
            )
            {
                continue;
            }

            database_.createStoreOrder(
                store.id,
                product.productId,
                requestedQuantity,
                currentDay,
                currentDay + 1
            );

            // Один заказ магазина за день.
            break;
        }
    }

    // Теперь склад обрабатывает активные заказы.
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

        bool completelyAllocated = true;

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

            if (
                warehouseProduct ==
                nullptr
            )
            {
                completelyAllocated =
                    false;

                continue;
            }

            const int available =
                warehouseProduct->quantity;

            const int allocatedNow =
                std::min(
                    remaining,
                    available
                );

            if (
                allocatedNow > 0
            )
            {
                if (
                    database_.changeWarehouseQuantity(
                        warehouse_.getId(),
                        item.productId,
                        -allocatedNow
                    )
                )
                {
                    database_.setOrderItemAllocated(
                        order.id,
                        item.productId,
                        item.allocatedQuantity +
                            allocatedNow
                    );
                }
            }

            const int stillMissing =
                remaining -
                allocatedNow;

            // Если склад не смог покрыть весь заказ,
            // создаём заявку поставщику.
            if (
                stillMissing > 0 &&
                !database_.hasActiveSupplierRequest(
                    item.productId
                )
            )
            {
                database_.createSupplierRequest(
                    item.productId,
                    stillMissing,
                    currentDay,
                    currentDay +
                        supplierDelay(
                            generator
                        )
                );
            }

            // Также пополняем склад, если после выдачи
            // остаток упал до минимального уровня.
            const int quantityAfterAllocation =
                available -
                allocatedNow;

            if (
                quantityAfterAllocation <=
                    warehouseProduct->minStock &&
                !database_.hasActiveSupplierRequest(
                    item.productId
                )
            )
            {
                const int replenishment =
                    warehouseProduct->capacity -
                    quantityAfterAllocation;

                if (
                    replenishment > 0
                )
                {
                    database_.createSupplierRequest(
                        item.productId,
                        replenishment,
                        currentDay,
                        currentDay +
                            supplierDelay(
                                generator
                            )
                    );
                }
            }

            if (
                stillMissing > 0
            )
            {
                completelyAllocated =
                    false;
            }
        }

        if (completelyAllocated)
        {
            database_.setOrderStatus(
                order.id,
                "Allocated"
            );
        }
        else
        {
            database_.setOrderStatus(
                order.id,
                "WaitingSupply"
            );
        }
    }
}

void BackendFacade::processSupplierRequests(
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

    std::uniform_int_distribution<int>
        delayDistribution(
            1,
            5
        );

    const std::vector<WarehouseStockRow>
        inventory =
            database_.getWarehouseInventory(
                warehouse_.getId()
            );

    for (
        const WarehouseStockRow& product :
        inventory
    )
    {
        if (
            product.quantity >
            product.minStock
        )
        {
            continue;
        }

        if (
            database_.hasActiveSupplierRequest(
                product.productId
            )
        )
        {
            continue;
        }

        const int quantity =
            product.capacity -
            product.quantity;

        if (
            quantity <= 0
        )
        {
            continue;
        }

        const int deliveryDay =
            currentDay +
            delayDistribution(
                generator
            );

        database_.createSupplierRequest(
            product.productId,
            quantity,
            currentDay,
            deliveryDay
        );
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

void BackendFacade::processSupplierDeliveries(int currentDay)
{
    if (!ready_ || currentDay <= 0) {
        return;
    }

    const std::vector<SupplierRequestInfo> requests =
        getSupplierRequests();

    for (const SupplierRequestInfo& request : requests) {
        const bool pending =
            request.status == "Created" ||
            request.status == "InTransit";

        if (pending && request.deliveryDay <= currentDay) {
            if (!completeSupplierRequest(request.id)) {
                lastError_ = "Cannot complete supplier request #" +
                             std::to_string(request.id);

                std::cerr << lastError_ << '\n';
            }
        }
    }
}