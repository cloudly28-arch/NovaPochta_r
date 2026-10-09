#include "persistence/Database.h"
#include <sqlite3.h>
#include <iostream>
#include <utility>

namespace {
bool exec(sqlite3* db, const char* sql) {
    char* error = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        if (error) { std::cerr << "SQLite: " << error << '\n'; sqlite3_free(error); }
        return false;
    }
    return true;
}
}

Database::~Database() { close(); }
bool Database::open(const std::string& path) {
    close();
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Cannot open SQLite database: " << sqlite3_errmsg(db_) << '\n';
        close();
        return false;
    }
    exec(db_, "PRAGMA foreign_keys = ON;");
    return true;
}
void Database::close() { if (db_) { sqlite3_close(db_); db_ = nullptr; } }
bool Database::isOpen() const { return db_ != nullptr; }

bool Database::loadWarehouse(Warehouse& warehouse) const {
    if (!db_) return false;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT id,name,address FROM warehouses ORDER BY id LIMIT 1", -1, &stmt, nullptr) != SQLITE_OK) return false;
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false;
    }
    const auto* warehouseName = sqlite3_column_text(stmt, 1);
    const auto* warehouseAddress = sqlite3_column_text(stmt, 2);
    warehouse = Warehouse(sqlite3_column_int(stmt, 0),
                          warehouseName ? reinterpret_cast<const char*>(warehouseName) : "Warehouse",
                          warehouseAddress ? reinterpret_cast<const char*>(warehouseAddress) : "");
    sqlite3_finalize(stmt);

    if (sqlite3_prepare_v2(db_, "SELECT id,name,category FROM products ORDER BY id", -1, &stmt, nullptr) != SQLITE_OK) return false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        warehouse.addProduct(Product(sqlite3_column_int(stmt,0),
                                     reinterpret_cast<const char*>(sqlite3_column_text(stmt,1)),
                                     reinterpret_cast<const char*>(sqlite3_column_text(stmt,2))));
    }
    sqlite3_finalize(stmt);

    if (sqlite3_prepare_v2(db_, "SELECT id,name FROM stores ORDER BY id", -1, &stmt, nullptr) != SQLITE_OK) return false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        warehouse.addStore(Store(sqlite3_column_int(stmt,0),
                                 reinterpret_cast<const char*>(sqlite3_column_text(stmt,1))));
    }
    sqlite3_finalize(stmt);

    if (sqlite3_prepare_v2(db_, "SELECT product_id,quantity FROM warehouse_inventory WHERE warehouse_id=? ORDER BY product_id", -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, warehouse.getId());
    int recordId = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        warehouse.addInventoryRecord(InventoryRecord(recordId++, sqlite3_column_int(stmt,0), -1, sqlite3_column_int(stmt,1)));
    }
    sqlite3_finalize(stmt);
    return true;
}

std::vector<StoreStockRow> Database::getStoreInventory(int storeId) const {
    std::vector<StoreStockRow> result;
    if (!db_) return result;
    const char* sql = "SELECT store_id,store_name,product_id,product_name,quantity,capacity,min_stock FROM store_inventory_view WHERE store_id=? ORDER BY product_id";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return result;
    sqlite3_bind_int(stmt, 1, storeId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        StoreStockRow row;
        row.storeId = sqlite3_column_int(stmt,0);
        row.storeName = reinterpret_cast<const char*>(sqlite3_column_text(stmt,1));
        row.productId = sqlite3_column_int(stmt,2);
        row.productName = reinterpret_cast<const char*>(sqlite3_column_text(stmt,3));
        row.quantity = sqlite3_column_int(stmt,4);
        row.capacity = sqlite3_column_int(stmt,5);
        row.minStock = sqlite3_column_int(stmt,6);
        result.push_back(std::move(row));
    }
    sqlite3_finalize(stmt);
    return result;
}
bool Database::getActiveStoreOrder(
    int storeId,
    StoreOrderRow& order
) const
{
    if (
        !db_ ||
        storeId <= 0
    )
    {
        return false;
    }

    const char* orderSql =
        "SELECT "
        "id, "
        "store_id, "
        "created_day, "
        "delivery_day, "
        "status "
        "FROM orders "
        "WHERE store_id=? "
        "AND status NOT IN ('Completed', 'Cancelled') "
        "ORDER BY id DESC "
        "LIMIT 1";

    sqlite3_stmt* stmt = nullptr;

    if (
        sqlite3_prepare_v2(
            db_,
            orderSql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK
    )
    {
        return false;
    }

    sqlite3_bind_int(
        stmt,
        1,
        storeId
    );

    if (
        sqlite3_step(stmt) !=
        SQLITE_ROW
    )
    {
        sqlite3_finalize(stmt);
        return false;
    }

    order.id =
        sqlite3_column_int(
            stmt,
            0
        );

    order.storeId =
        sqlite3_column_int(
            stmt,
            1
        );

    order.createdDay =
        sqlite3_column_int(
            stmt,
            2
        );

    order.deliveryDay =
        sqlite3_column_int(
            stmt,
            3
        );

    const auto* status =
        sqlite3_column_text(
            stmt,
            4
        );

    order.status =
        status
            ? reinterpret_cast<const char*>(
                status
            )
            : "";

    sqlite3_finalize(stmt);

    const char* itemSql =
        "SELECT "
        "oi.product_id, "
        "p.name, "
        "oi.requested_quantity, "
        "oi.allocated_quantity "
        "FROM order_items oi "
        "JOIN products p "
        "ON p.id = oi.product_id "
        "WHERE oi.order_id=? "
        "ORDER BY oi.product_id";

    if (
        sqlite3_prepare_v2(
            db_,
            itemSql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK
    )
    {
        return false;
    }

    sqlite3_bind_int(
        stmt,
        1,
        order.id
    );

    order.items.clear();

    while (
        sqlite3_step(stmt) ==
        SQLITE_ROW
    )
    {
        OrderItemRow item;

        item.productId =
            sqlite3_column_int(
                stmt,
                0
            );

        const auto* productName =
            sqlite3_column_text(
                stmt,
                1
            );

        item.productName =
            productName
                ? reinterpret_cast<const char*>(
                    productName
                )
                : "";

        item.requestedQuantity =
            sqlite3_column_int(
                stmt,
                2
            );

        item.allocatedQuantity =
            sqlite3_column_int(
                stmt,
                3
            );

        order.items.push_back(
            std::move(item)
        );
    }

    sqlite3_finalize(stmt);

    return true;
}
std::vector<WarehouseStockRow>
Database::getWarehouseInventory(
    int warehouseId
) const
{
    std::vector<WarehouseStockRow> result;

    if (!db_)
    {
        return result;
    }

    const char* sql =
        "SELECT "
        "warehouse_id, "
        "product_id, "
        "product_name, "
        "quantity, "
        "capacity, "
        "min_stock "
        "FROM warehouse_inventory_view "
        "WHERE warehouse_id=? "
        "ORDER BY product_id";

    sqlite3_stmt* stmt = nullptr;

    if (
        sqlite3_prepare_v2(
            db_,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK
    )
    {
        return result;
    }

    sqlite3_bind_int(
        stmt,
        1,
        warehouseId
    );

    while (
        sqlite3_step(stmt) ==
        SQLITE_ROW
    )
    {
        WarehouseStockRow row;

        row.warehouseId =
            sqlite3_column_int(
                stmt,
                0
            );

        row.productId =
            sqlite3_column_int(
                stmt,
                1
            );

        const auto* productName =
            sqlite3_column_text(
                stmt,
                2
            );

        row.productName =
            productName
                ? reinterpret_cast<const char*>(
                    productName
                )
                : "";

        row.quantity =
            sqlite3_column_int(
                stmt,
                3
            );

        row.capacity =
            sqlite3_column_int(
                stmt,
                4
            );

        row.minStock =
            sqlite3_column_int(
                stmt,
                5
            );

        result.push_back(
            std::move(row)
        );
    }

    sqlite3_finalize(stmt);

    return result;
}

bool Database::createSupplierRequest(
    int productId,
    int requestedQuantity,
    int createdDay,
    int deliveryDay
)
{
    if (
        !db_ ||
        productId <= 0 ||
        requestedQuantity <= 0 ||
        createdDay <= 0 ||
        deliveryDay <= createdDay
    )
    {
        return false;
    }

    const char* sql =
        "INSERT INTO supplier_requests "
        "("
        "product_id, "
        "requested_quantity, "
        "created_day, "
        "delivery_day, "
        "status"
        ") "
        "VALUES (?, ?, ?, ?, 'Created')";

    sqlite3_stmt* stmt = nullptr;

    if (
        sqlite3_prepare_v2(
            db_,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK
    )
    {
        return false;
    }

    sqlite3_bind_int(
        stmt,
        1,
        productId
    );

    sqlite3_bind_int(
        stmt,
        2,
        requestedQuantity
    );

    sqlite3_bind_int(
        stmt,
        3,
        createdDay
    );

    sqlite3_bind_int(
        stmt,
        4,
        deliveryDay
    );

    const bool ok =
        sqlite3_step(stmt) ==
        SQLITE_DONE;

    sqlite3_finalize(stmt);

    return ok;
}

std::vector<SupplierRequestRow>
Database::getSupplierRequests() const
{
    std::vector<SupplierRequestRow> result;

    if (!db_)
    {
        return result;
    }

    const char* sql =
        "SELECT "
        "sr.id, "
        "sr.product_id, "
        "p.name, "
        "sr.requested_quantity, "
        "sr.created_day, "
        "sr.delivery_day, "
        "sr.status "
        "FROM supplier_requests sr "
        "JOIN products p "
        "ON p.id = sr.product_id "
        "ORDER BY sr.id";

    sqlite3_stmt* stmt = nullptr;

    if (
        sqlite3_prepare_v2(
            db_,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK
    )
    {
        return result;
    }

    while (
        sqlite3_step(stmt) ==
        SQLITE_ROW
    )
    {
        SupplierRequestRow row;

        row.id =
            sqlite3_column_int(
                stmt,
                0
            );

        row.productId =
            sqlite3_column_int(
                stmt,
                1
            );

        const auto* productName =
            sqlite3_column_text(
                stmt,
                2
            );

        row.productName =
            productName
                ? reinterpret_cast<const char*>(
                    productName
                )
                : "";

        row.requestedQuantity =
            sqlite3_column_int(
                stmt,
                3
            );

        row.createdDay =
            sqlite3_column_int(
                stmt,
                4
            );

        row.deliveryDay =
            sqlite3_column_int(
                stmt,
                5
            );

        const auto* status =
            sqlite3_column_text(
                stmt,
                6
            );

        row.status =
            status
                ? reinterpret_cast<const char*>(
                    status
                )
                : "";

        result.push_back(
            std::move(row)
        );
    }

    sqlite3_finalize(stmt);

    return result;
}

bool Database::setSupplierRequestStatus(
    int requestId,
    const std::string& status
)
{
    if (
        !db_ ||
        requestId <= 0 ||
        status.empty()
    )
    {
        return false;
    }

    const char* sql =
        "UPDATE supplier_requests "
        "SET status=? "
        "WHERE id=?";

    sqlite3_stmt* stmt = nullptr;

    if (
        sqlite3_prepare_v2(
            db_,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK
    )
    {
        return false;
    }

    sqlite3_bind_text(
        stmt,
        1,
        status.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int(
        stmt,
        2,
        requestId
    );

    const bool ok =
        sqlite3_step(stmt) ==
            SQLITE_DONE &&
        sqlite3_changes(db_) > 0;

    sqlite3_finalize(stmt);

    return ok;
}



bool Database::setWarehouseQuantity(int warehouseId, int productId, int quantity) {
    if (!db_ || quantity < 0) return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "UPDATE warehouse_inventory SET quantity=? WHERE warehouse_id=? AND product_id=?", -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt,1,quantity); sqlite3_bind_int(stmt,2,warehouseId); sqlite3_bind_int(stmt,3,productId);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;
    sqlite3_finalize(stmt); return ok;
}
bool Database::changeWarehouseQuantity(int warehouseId, int productId, int delta) {
    if (!db_) return false;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE warehouse_inventory SET quantity=quantity+? WHERE warehouse_id=? AND product_id=? AND quantity+?>=0";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt,1,delta); sqlite3_bind_int(stmt,2,warehouseId); sqlite3_bind_int(stmt,3,productId); sqlite3_bind_int(stmt,4,delta);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;
    sqlite3_finalize(stmt); return ok;
}
bool Database::setStoreQuantity(int storeId, int productId, int quantity) {
    if (!db_ || quantity < 0) return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "UPDATE store_inventory SET quantity=? WHERE store_id=? AND product_id=?", -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt,1,quantity); sqlite3_bind_int(stmt,2,storeId); sqlite3_bind_int(stmt,3,productId);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;
    sqlite3_finalize(stmt); return ok;
}
bool Database::changeStoreQuantity(int storeId, int productId, int delta) {
    if (!db_) return false;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE store_inventory SET quantity=quantity+? WHERE store_id=? AND product_id=? AND quantity+?>=0 AND quantity+?<=capacity";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt,1,delta); sqlite3_bind_int(stmt,2,storeId); sqlite3_bind_int(stmt,3,productId); sqlite3_bind_int(stmt,4,delta); sqlite3_bind_int(stmt,5,delta);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;
    sqlite3_finalize(stmt); return ok;
}
bool Database::beginTransaction() { return db_ && exec(db_, "BEGIN TRANSACTION;"); }
bool Database::commit() { return db_ && exec(db_, "COMMIT;"); }
bool Database::rollback() { return db_ && exec(db_, "ROLLBACK;"); }
