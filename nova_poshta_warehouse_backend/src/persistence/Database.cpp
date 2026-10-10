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
bool Database::executeSql(
    const std::string& sql
)
{
    if (
        !db_ ||
        sql.empty()
    )
    {
        return false;
    }

    char* errorMessage = nullptr;

    const int result =
        sqlite3_exec(
            db_,
            sql.c_str(),
            nullptr,
            nullptr,
            &errorMessage
        );

    if (
        result != SQLITE_OK
    )
    {
        if (errorMessage != nullptr)
        {
            std::cerr
                << "SQLite error: "
                << errorMessage
                << '\n';

            sqlite3_free(
                errorMessage
            );
        }

        return false;
    }

    return true;
}

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
        "wi.warehouse_id, "
        "wi.product_id, "
        "p.name, "
        "wi.quantity, "
        "wi.capacity, "
        "wi.min_stock, "
        "p.unit_price_cents, "
        "p.shelf_life_days "
        "FROM warehouse_inventory wi "
        "JOIN products p ON p.id = wi.product_id "
        "WHERE wi.warehouse_id=? "
        "ORDER BY wi.product_id";

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
        row.unitPriceCents = sqlite3_column_int(stmt, 6);
        row.shelfLifeDays = sqlite3_column_int(stmt, 7);  

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
bool Database::hasActiveSupplierRequest(
    int productId
) const
{
    if (
        !db_ ||
        productId <= 0
    )
    {
        return false;
    }

    const char* sql =
        "SELECT 1 "
        "FROM supplier_requests "
        "WHERE product_id=? "
        "AND status IN ('Created', 'InTransit') "
        "LIMIT 1";

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

    const bool exists =
        sqlite3_step(stmt) ==
        SQLITE_ROW;

    sqlite3_finalize(stmt);

    return exists;
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
    sqlite3_bind_int(stmt,1,quantity);
     sqlite3_bind_int(stmt,2,warehouseId);
      sqlite3_bind_int(stmt,3,productId);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;
    sqlite3_finalize(stmt); return ok;
}
bool Database::changeWarehouseQuantityRaw(
    int warehouseId,
    int productId,
    int delta
)
{
    if (!db_)
    {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "UPDATE warehouse_inventory "
        "SET quantity=quantity+? "
        "WHERE warehouse_id=? "
        "AND product_id=? "
        "AND quantity+?>=0 "
        "AND quantity+?<=capacity";

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
        delta
    );

    sqlite3_bind_int(
        stmt,
        2,
        warehouseId
    );

    sqlite3_bind_int(
        stmt,
        3,
        productId
    );

    sqlite3_bind_int(
        stmt,
        4,
        delta
    );

    sqlite3_bind_int(
        stmt,
        5,
        delta
    );

    const bool ok =
        sqlite3_step(stmt) ==
            SQLITE_DONE &&
        sqlite3_changes(db_) > 0;

    sqlite3_finalize(stmt);

    return ok;
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

bool Database::hasActiveStoreOrder(
    int storeId
) const
{
    if (
        !db_ ||
        storeId <= 0
    )
    {
        return false;
    }

    const char* sql =
        "SELECT 1 "
        "FROM orders "
        "WHERE store_id = ? "
        "AND status NOT IN ('Completed', 'Cancelled') "
        "LIMIT 1";

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
        storeId
    );

    const bool exists =
        sqlite3_step(stmt) ==
        SQLITE_ROW;

    sqlite3_finalize(stmt);

    return exists;
}

bool Database::createStoreOrder(
    int storeId,
    int productId,
    int requestedQuantity,
    int createdDay,
    int deliveryDay
)
{
    if (
        !db_ ||
        storeId <= 0 ||
        productId <= 0 ||
        requestedQuantity <= 0 ||
        createdDay <= 0 ||
        deliveryDay <= createdDay
    )
    {
        return false;
    }

    if (
        sqlite3_exec(
            db_,
            "BEGIN TRANSACTION;",
            nullptr,
            nullptr,
            nullptr
        ) != SQLITE_OK
    )
    {
        return false;
    }

    const char* orderSql =
        "INSERT INTO orders "
        "(store_id, created_day, delivery_day, status) "
        "VALUES (?, ?, ?, 'Created')";

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
        sqlite3_exec(
            db_,
            "ROLLBACK;",
            nullptr,
            nullptr,
            nullptr
        );

        return false;
    }

    sqlite3_bind_int(
        stmt,
        1,
        storeId
    );

    sqlite3_bind_int(
        stmt,
        2,
        createdDay
    );

    sqlite3_bind_int(
        stmt,
        3,
        deliveryDay
    );

    if (
        sqlite3_step(stmt) !=
        SQLITE_DONE
    )
    {
        sqlite3_finalize(stmt);

        sqlite3_exec(
            db_,
            "ROLLBACK;",
            nullptr,
            nullptr,
            nullptr
        );

        return false;
    }

    sqlite3_finalize(stmt);

    const int orderId =
        static_cast<int>(
            sqlite3_last_insert_rowid(
                db_
            )
        );

    const char* itemSql =
        "INSERT INTO order_items "
        "("
        "order_id, "
        "product_id, "
        "requested_quantity, "
        "allocated_quantity"
        ") "
        "VALUES (?, ?, ?, 0)";

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
        sqlite3_exec(
            db_,
            "ROLLBACK;",
            nullptr,
            nullptr,
            nullptr
        );

        return false;
    }

    sqlite3_bind_int(
        stmt,
        1,
        orderId
    );

    sqlite3_bind_int(
        stmt,
        2,
        productId
    );

    sqlite3_bind_int(
        stmt,
        3,
        requestedQuantity
    );

    if (
        sqlite3_step(stmt) !=
        SQLITE_DONE
    )
    {
        sqlite3_finalize(stmt);

        sqlite3_exec(
            db_,
            "ROLLBACK;",
            nullptr,
            nullptr,
            nullptr
        );

        return false;
    }

    sqlite3_finalize(stmt);

    if (
        sqlite3_exec(
            db_,
            "COMMIT;",
            nullptr,
            nullptr,
            nullptr
        ) != SQLITE_OK
    )
    {
        sqlite3_exec(
            db_,
            "ROLLBACK;",
            nullptr,
            nullptr,
            nullptr
        );

        return false;
    }

    return true;
}
bool Database::setOrderItemAllocated(
    int orderId,
    int productId,
    int allocatedQuantity
)
{
    if (
        !db_ ||
        orderId <= 0 ||
        productId <= 0 ||
        allocatedQuantity < 0
    )
    {
        return false;
    }

    const char* sql =
        "UPDATE order_items "
        "SET allocated_quantity=? "
        "WHERE order_id=? "
        "AND product_id=?";

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
        allocatedQuantity
    );

    sqlite3_bind_int(
        stmt,
        2,
        orderId
    );

    sqlite3_bind_int(
        stmt,
        3,
        productId
    );

    const bool ok =
        sqlite3_step(stmt) ==
            SQLITE_DONE &&
        sqlite3_changes(db_) > 0;

    sqlite3_finalize(stmt);

    return ok;
}

bool Database::setOrderStatus(
    int orderId,
    const std::string& status
)
{
    if (
        !db_ ||
        orderId <= 0 ||
        status.empty()
    )
    {
        return false;
    }

    const char* sql =
        "UPDATE orders "
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
        orderId
    );

    const bool ok =
        sqlite3_step(stmt) ==
            SQLITE_DONE &&
        sqlite3_changes(db_) > 0;

    sqlite3_finalize(stmt);

    return ok;
}

std::string Database::getLastError() const
{
    if (db_ == nullptr) {
        return "Database is not open";
    }

    return sqlite3_errmsg(db_);
}

bool Database::copyFrom(const Database& source)
{
    if (db_ == nullptr || source.db_ == nullptr) {
        return false;
    }

    sqlite3_backup* backup = sqlite3_backup_init(
        db_,
        "main",
        source.db_,
        "main"
    );

    if (backup == nullptr) {
        return false;
    }

    const int stepResult = sqlite3_backup_step(backup, -1);
    const int finishResult = sqlite3_backup_finish(backup);

    return stepResult == SQLITE_DONE &&
           finishResult == SQLITE_OK;
}

bool Database::getWarehouseBatches(
    int warehouseId,
    std::vector<WarehouseBatchRow>& batches
) const {
    batches.clear();

    if (db_ == nullptr) {
        return false;
    }

    const char* sql =
        "SELECT b.id, b.warehouse_id, b.product_id, p.name, "
        "b.quantity, b.received_day, b.expires_day, "
        "b.unit_price_cents "
        "FROM warehouse_batches AS b "
        "JOIN products AS p ON p.id = b.product_id "
        "WHERE b.warehouse_id = ? AND b.quantity > 0 "
        "ORDER BY b.expires_day, b.id;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db_, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return false;
    }

    if (sqlite3_bind_int(
            statement, 1, warehouseId
        ) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return false;
    }

    std::vector<WarehouseBatchRow> result;
    int status = SQLITE_OK;

    while ((status = sqlite3_step(statement)) == SQLITE_ROW) {
        WarehouseBatchRow batch;

        batch.id = sqlite3_column_int(statement, 0);
        batch.warehouseId = sqlite3_column_int(statement, 1);
        batch.productId = sqlite3_column_int(statement, 2);

        const unsigned char* name =
            sqlite3_column_text(statement, 3);

        if (name != nullptr) {
            batch.productName =
                reinterpret_cast<const char*>(name);
        }

        batch.quantity = sqlite3_column_int(statement, 4);
        batch.receivedDay = sqlite3_column_int(statement, 5);
        batch.expiresDay = sqlite3_column_int(statement, 6);
        batch.unitPriceCents =
            sqlite3_column_int(statement, 7);

        result.push_back(batch);
    }

    const int finalizeStatus = sqlite3_finalize(statement);

    if (status != SQLITE_DONE || finalizeStatus != SQLITE_OK) {
        return false;
    }

    batches.swap(result);
    return true;
}

bool Database::createWarehouseBatch(
    int warehouseId,
    int productId,
    int quantity,
    int receivedDay
) {
    if (db_ == nullptr || quantity <= 0 || receivedDay < 1) {
        return false;
    }

    const char* sql =
        "INSERT INTO warehouse_batches ("
        "warehouse_id, product_id, quantity, "
        "received_day, expires_day, unit_price_cents"
        ") "
        "SELECT ?, id, ?, ?, ? + shelf_life_days, "
        "unit_price_cents "
        "FROM products WHERE id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db_, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return false;
    }

    const bool bound =
        sqlite3_bind_int(statement, 1, warehouseId) == SQLITE_OK &&
        sqlite3_bind_int(statement, 2, quantity) == SQLITE_OK &&
        sqlite3_bind_int(statement, 3, receivedDay) == SQLITE_OK &&
        sqlite3_bind_int(statement, 4, receivedDay) == SQLITE_OK &&
        sqlite3_bind_int(statement, 5, productId) == SQLITE_OK;

    const bool inserted =
        bound &&
        sqlite3_step(statement) == SQLITE_DONE &&
        sqlite3_changes(db_) == 1;

    const int finalizeStatus = sqlite3_finalize(statement);

    return inserted && finalizeStatus == SQLITE_OK;
}

bool Database::changeWarehouseQuantity(
    int warehouseId,
    int productId,
    int delta,
    int currentDay
) {
    if (db_ == nullptr || currentDay < 1) {
        return false;
    }

    // Поступление: создание партии выполняется отдельно
    // в транзакции completeSupplierRequest().
    if (delta >= 0) {
        return changeWarehouseQuantityRaw(
            warehouseId, productId, delta
        );
    }

    if (!executeSql("SAVEPOINT warehouse_dispatch;")) {
        return false;
    }

    const auto cancel = [this]() {
        executeSql("ROLLBACK TO warehouse_dispatch;");
        executeSql("RELEASE warehouse_dispatch;");
        return false;
    };

    std::vector<WarehouseBatchRow> batches;

    if (!getWarehouseBatches(warehouseId, batches)) {
        return cancel();
    }

    long long available = 0;

    for (const WarehouseBatchRow& batch : batches) {
        if (batch.productId == productId &&
            batch.expiresDay > currentDay) {
            available += batch.quantity;
        }
    }

    long long remaining = -static_cast<long long>(delta);

    if (available < remaining) {
        return cancel();
    }

    // getWarehouseBatches() уже сортирует партии
    // по ближайшему сроку годности.
    for (const WarehouseBatchRow& batch : batches) {
        if (remaining == 0) {
            break;
        }

        if (batch.productId != productId ||
            batch.expiresDay <= currentDay) {
            continue;
        }

        const int taken =
            remaining < batch.quantity
                ? static_cast<int>(remaining)
                : batch.quantity;

        const std::string sql =
            "UPDATE warehouse_batches "
            "SET quantity = quantity - " +
            std::to_string(taken) +
            " WHERE id = " + std::to_string(batch.id) +
            " AND quantity >= " + std::to_string(taken) + ";";

        if (!executeSql(sql) || sqlite3_changes(db_) != 1) {
            return cancel();
        }

        remaining -= taken;
    }

    if (!changeWarehouseQuantityRaw(
            warehouseId, productId, delta
        )) {
        return cancel();
    }

    if (!executeSql("RELEASE warehouse_dispatch;")) {
        return cancel();
    }

    return true;
}

bool Database::writeOffExpiredBatches(
    int warehouseId,
    int currentDay
) {
    if (db_ == nullptr || warehouseId <= 0 || currentDay < 1) {
        return false;
    }

    if (!executeSql("SAVEPOINT expired_writeoff;")) {
        return false;
    }

    const auto cancel = [this]() {
        executeSql("ROLLBACK TO expired_writeoff;");
        executeSql("RELEASE expired_writeoff;");
        return false;
    };

    std::vector<WarehouseBatchRow> batches;

    if (!getWarehouseBatches(warehouseId, batches)) {
        return cancel();
    }

    for (const WarehouseBatchRow& batch : batches) {
        if (batch.expiresDay > currentDay) {
            continue;
        }

        const long long lossCents =
            static_cast<long long>(batch.quantity) *
            batch.unitPriceCents;

        const std::string insertSql =
            "INSERT INTO warehouse_writeoffs ("
            "batch_id, warehouse_id, product_id, "
            "writeoff_day, quantity, loss_cents"
            ") VALUES (" +
            std::to_string(batch.id) + "," +
            std::to_string(warehouseId) + "," +
            std::to_string(batch.productId) + "," +
            std::to_string(currentDay) + "," +
            std::to_string(batch.quantity) + "," +
            std::to_string(lossCents) + ");";

        if (!executeSql(insertSql)) {
            return cancel();
        }

        if (!changeWarehouseQuantityRaw(
                warehouseId,
                batch.productId,
                -batch.quantity
            )) {
            return cancel();
        }

        const std::string updateSql =
            "UPDATE warehouse_batches SET quantity = 0 "
            "WHERE id = " + std::to_string(batch.id) + ";";

        if (!executeSql(updateSql) || sqlite3_changes(db_) != 1) {
            return cancel();
        }
    }

    if (!executeSql("RELEASE expired_writeoff;")) {
        return cancel();
    }

    return true;
}