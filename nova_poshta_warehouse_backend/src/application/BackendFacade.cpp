#include "application/BackendFacade.h"
#include <algorithm>
#include <random>

bool BackendFacade::initialize(
    const std::string& databasePath
)
{
    ready_ = false;

    database_.close();
    warehouse_ = Warehouse{};

    if (!database_.open(databasePath))
    {
        return false;
    }

    if (!database_.loadWarehouse(warehouse_))
    {
        database_.close();
        return false;
    }

    ready_ = true;
    return true;
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
void BackendFacade::simulateStoreSales(
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
            currentDay * 1009
        )
    );

    std::uniform_int_distribution<int>
        salesDistribution(
            5,
            15
        );

    const std::vector<StoreInfo> stores =
        getStores();

    for (
        const StoreInfo& store :
        stores
    )
    {
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
                product.quantity <= 0
            )
            {
                continue;
            }

            int sold =
                salesDistribution(
                    generator
                );

            sold =
                std::min(
                    sold,
                    product.quantity
                );

            database_.changeStoreQuantity(
                store.id,
                product.productId,
                -sold
            );
        }
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