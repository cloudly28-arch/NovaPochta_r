#include "application/BackendFacade.h"

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


