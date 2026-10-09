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
    if (sqlite3_step(stmt) != SQLITE_ROW) { sqlite3_finalize(stmt); return false; }
    const auto* warehouseName = sqlite3_column_text(stmt, 1);
    const auto* warehouseAddress = sqlite3_column_text(stmt, 2);
    warehouse = Warehouse(sqlite3_column_int(stmt, 0), warehouseName ? reinterpret_cast<const char*>(warehouseName) : "Warehouse", warehouseAddress ? reinterpret_cast<const char*>(warehouseAddress) : "");
    sqlite3_finalize(stmt);

    const char* productSql = "SELECT id,name,category,unit_name,units_per_package,price_per_unit,shelf_life_days FROM products ORDER BY id";
    if (sqlite3_prepare_v2(db_, productSql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* name = sqlite3_column_text(stmt,1);
        const auto* category = sqlite3_column_text(stmt,2);
        const auto* unit = sqlite3_column_text(stmt,3);
        warehouse.addProduct(Product(sqlite3_column_int(stmt,0),
                                     name ? reinterpret_cast<const char*>(name) : "",
                                     category ? reinterpret_cast<const char*>(category) : "",
                                     "", 0.0, 0.0,
                                     unit ? reinterpret_cast<const char*>(unit) : "шт",
                                     sqlite3_column_int(stmt,4),
                                     sqlite3_column_double(stmt,5),
                                     sqlite3_column_int(stmt,6)));
    }
    sqlite3_finalize(stmt);

    if (sqlite3_prepare_v2(db_, "SELECT id,name FROM stores ORDER BY id", -1, &stmt, nullptr) != SQLITE_OK) return false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        warehouse.addStore(Store(sqlite3_column_int(stmt,0), reinterpret_cast<const char*>(sqlite3_column_text(stmt,1))));
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
    const char* sql = "UPDATE warehouse_inventory SET quantity=quantity+? WHERE warehouse_id=? AND product_id=? AND quantity+?>=0 AND quantity+?<=capacity";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt,1,delta); sqlite3_bind_int(stmt,2,warehouseId); sqlite3_bind_int(stmt,3,productId); sqlite3_bind_int(stmt,4,delta); sqlite3_bind_int(stmt,5,delta);
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
