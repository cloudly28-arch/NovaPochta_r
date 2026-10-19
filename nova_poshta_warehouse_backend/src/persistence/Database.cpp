#include "persistence/Database.h"
#include <sqlite3.h>
#include <iostream>
#include <utility>

namespace {
bool exec(sqlite3* db, const char* sql) {
  char* error = nullptr;
  const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &error);
  if (rc != SQLITE_OK) {
    if (error) {
      std::cerr << "SQLite: " << error << '\n';
      sqlite3_free(error);
    }
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
  if (!exec(db_, "PRAGMA foreign_keys = ON;") || !executeSql(R"SQL(
      CREATE TABLE IF NOT EXISTS experiment_settings (
        id INTEGER PRIMARY KEY CHECK(id = 1),
        initial_fill_percent INTEGER NOT NULL CHECK(initial_fill_percent BETWEEN -1 AND 100),
        demand_percent INTEGER NOT NULL CHECK(demand_percent BETWEEN 50 AND 150),
        stock_cover_days INTEGER NOT NULL CHECK(stock_cover_days BETWEEN 0 AND 14),
        purchase_cost_percent INTEGER NOT NULL CHECK(purchase_cost_percent BETWEEN 1 AND 100)
      );
      INSERT OR IGNORE INTO experiment_settings VALUES(1, -1, 100, 0, 70);
  )SQL")) {
    close();
    return false;
  }
  return true;
}
void Database::close() {
  if (db_) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}
bool Database::isOpen() const { return db_ != nullptr; }
bool Database::executeSql(const std::string& sql) {
  if (!db_ || sql.empty()) {
    return false;
  }

  char* errorMessage = nullptr;

  const int result =
      sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errorMessage);

  if (result != SQLITE_OK) {
    if (errorMessage != nullptr) {
      std::cerr << "SQLite error: " << errorMessage << '\n';

      sqlite3_free(errorMessage);
    }

    return false;
  }

  return true;
}

bool Database::getExperimentSettings(ExperimentSettings& settings) const {
  if (!db_) return false;
  sqlite3_stmt* stmt = nullptr;
  const char* sql =
      "SELECT initial_fill_percent, demand_percent, "
      "stock_cover_days, purchase_cost_percent "
      "FROM experiment_settings WHERE id=1";
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;
  ExperimentSettings loaded;
  const bool found = sqlite3_step(stmt) == SQLITE_ROW;
  if (found) {
    loaded.initialFillPercent = sqlite3_column_int(stmt, 0);
    loaded.demandPercent = sqlite3_column_int(stmt, 1);
    loaded.stockCoverDays = sqlite3_column_int(stmt, 2);
    loaded.purchaseCostPercent = sqlite3_column_int(stmt, 3);
  }
  const int finalized = sqlite3_finalize(stmt);
  if (!found || !loaded.isValid() || finalized != SQLITE_OK) return false;
  settings = loaded;
  return true;
}

bool Database::setExperimentSettings(const ExperimentSettings& settings) {
  if (!db_ || !settings.isValid()) return false;
  sqlite3_stmt* stmt = nullptr;
  const char* sql =
      "UPDATE experiment_settings SET initial_fill_percent=?1, "
      "demand_percent=?2, stock_cover_days=?3, "
      "purchase_cost_percent=?4 WHERE id=1";
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;
  const bool bound =
      sqlite3_bind_int(stmt, 1, settings.initialFillPercent) == SQLITE_OK &&
      sqlite3_bind_int(stmt, 2, settings.demandPercent) == SQLITE_OK &&
      sqlite3_bind_int(stmt, 3, settings.stockCoverDays) == SQLITE_OK &&
      sqlite3_bind_int(stmt, 4, settings.purchaseCostPercent) == SQLITE_OK;
  const bool updated =
      bound && sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) == 1;
  return sqlite3_finalize(stmt) == SQLITE_OK && updated;
}

bool Database::loadWarehouse(Warehouse& warehouse) const {
  if (!db_) return false;

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          db_, "SELECT id,name,address FROM warehouses ORDER BY id LIMIT 1", -1,
          &stmt, nullptr) != SQLITE_OK)
    return false;
  if (sqlite3_step(stmt) != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    return false;
  }
  const auto* warehouseName = sqlite3_column_text(stmt, 1);
  const auto* warehouseAddress = sqlite3_column_text(stmt, 2);
  warehouse = Warehouse(
      sqlite3_column_int(stmt, 0),
      warehouseName ? reinterpret_cast<const char*>(warehouseName)
                    : "Warehouse",
      warehouseAddress ? reinterpret_cast<const char*>(warehouseAddress) : "");
  sqlite3_finalize(stmt);

  if (sqlite3_prepare_v2(db_,
                         "SELECT id,name,category FROM products ORDER BY id",
                         -1, &stmt, nullptr) != SQLITE_OK)
    return false;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    warehouse.addProduct(
        Product(sqlite3_column_int(stmt, 0),
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))));
  }
  sqlite3_finalize(stmt);

  if (sqlite3_prepare_v2(db_, "SELECT id,name FROM stores ORDER BY id", -1,
                         &stmt, nullptr) != SQLITE_OK)
    return false;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    warehouse.addStore(
        Store(sqlite3_column_int(stmt, 0),
              reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))));
  }
  sqlite3_finalize(stmt);

  if (sqlite3_prepare_v2(
          db_,
          "SELECT product_id,quantity FROM warehouse_inventory WHERE warehouse_id=? ORDER BY product_id",
          -1, &stmt, nullptr) != SQLITE_OK)
    return false;
  sqlite3_bind_int(stmt, 1, warehouse.getId());
  int recordId = 1;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    warehouse.addInventoryRecord(
        InventoryRecord(recordId++, sqlite3_column_int(stmt, 0), -1,
                        sqlite3_column_int(stmt, 1)));
  }
  sqlite3_finalize(stmt);
  return true;
}

std::vector<StoreStockRow> Database::getStoreInventory(int storeId) const {
  std::vector<StoreStockRow> result;
  if (!db_) return result;
  const char* sql =
      "SELECT store_id,store_name,product_id,product_name,quantity,capacity,min_stock FROM store_inventory_view WHERE store_id=? ORDER BY product_id";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return result;
  sqlite3_bind_int(stmt, 1, storeId);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    StoreStockRow row;
    row.storeId = sqlite3_column_int(stmt, 0);
    row.storeName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    row.productId = sqlite3_column_int(stmt, 2);
    row.productName =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    row.quantity = sqlite3_column_int(stmt, 4);
    row.capacity = sqlite3_column_int(stmt, 5);
    row.minStock = sqlite3_column_int(stmt, 6);
    result.push_back(std::move(row));
  }
  sqlite3_finalize(stmt);
  return result;
}
bool Database::getActiveStoreOrder(int storeId, StoreOrderRow& order) const {
  if (!db_ || storeId <= 0) {
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

  if (sqlite3_prepare_v2(db_, orderSql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_int(stmt, 1, storeId);

  if (sqlite3_step(stmt) != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    return false;
  }

  order.id = sqlite3_column_int(stmt, 0);

  order.storeId = sqlite3_column_int(stmt, 1);

  order.createdDay = sqlite3_column_int(stmt, 2);

  order.deliveryDay = sqlite3_column_int(stmt, 3);

  const auto* status = sqlite3_column_text(stmt, 4);

  order.status = status ? reinterpret_cast<const char*>(status) : "";

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

  if (sqlite3_prepare_v2(db_, itemSql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_int(stmt, 1, order.id);

  order.items.clear();

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    OrderItemRow item;

    item.productId = sqlite3_column_int(stmt, 0);

    const auto* productName = sqlite3_column_text(stmt, 1);

    item.productName =
        productName ? reinterpret_cast<const char*>(productName) : "";

    item.requestedQuantity = sqlite3_column_int(stmt, 2);

    item.allocatedQuantity = sqlite3_column_int(stmt, 3);

    order.items.push_back(std::move(item));
  }

  sqlite3_finalize(stmt);

  return true;
}
std::vector<WarehouseStockRow> Database::getWarehouseInventory(
    int warehouseId) const {
  std::vector<WarehouseStockRow> result;

  if (!db_) {
    return result;
  }

  const char* sql =
      "SELECT wi.warehouse_id, wi.product_id, p.name, "
      "wi.quantity, wi.capacity, wi.min_stock, "
      "p.unit_price_cents, p.shelf_life_days, "
      "p.units_per_package "
      "FROM warehouse_inventory AS wi "
      "JOIN products AS p ON p.id = wi.product_id "
      "WHERE wi.warehouse_id = ? "
      "ORDER BY wi.product_id;";

  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return result;
  }

  sqlite3_bind_int(stmt, 1, warehouseId);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    WarehouseStockRow row;
    row.unitsPerPackage = sqlite3_column_int(stmt, 8);
    row.warehouseId = sqlite3_column_int(stmt, 0);

    row.productId = sqlite3_column_int(stmt, 1);

    const auto* productName = sqlite3_column_text(stmt, 2);

    row.productName =
        productName ? reinterpret_cast<const char*>(productName) : "";

    row.quantity = sqlite3_column_int(stmt, 3);

    row.capacity = sqlite3_column_int(stmt, 4);

    row.minStock = sqlite3_column_int(stmt, 5);
    row.unitPriceCents = sqlite3_column_int(stmt, 6);
    row.shelfLifeDays = sqlite3_column_int(stmt, 7);

    result.push_back(std::move(row));
  }

  sqlite3_finalize(stmt);

  return result;
}

bool Database::createSupplierRequest(int productId, int requestedQuantity,
                                     int createdDay, int deliveryDay) {
  if (!db_ || productId <= 0 || requestedQuantity <= 0 || createdDay <= 0 ||
      deliveryDay <= createdDay) {
    return false;
  }
  if (!exec(db_, "SAVEPOINT merge_supplier_request;")) {
    return false;
  }
  const auto cancel = [this]() {
    exec(db_, "ROLLBACK TO merge_supplier_request;");
    exec(db_, "RELEASE merge_supplier_request;");
    return false;
  };
  sqlite3_stmt* find = nullptr;
  const char* findSql =
      "SELECT id FROM supplier_requests WHERE product_id=?1 "
      "AND status='Created' ORDER BY created_day,id LIMIT 1;";
  if (sqlite3_prepare_v2(db_, findSql, -1, &find, nullptr) != SQLITE_OK) {
    sqlite3_finalize(find);
    return cancel();
  }
  if (sqlite3_bind_int(find, 1, productId) != SQLITE_OK) {
    sqlite3_finalize(find);
    return cancel();
  }
  const int state = sqlite3_step(find);
  const int existingId = state == SQLITE_ROW ? sqlite3_column_int(find, 0) : 0;
  const int finalized = sqlite3_finalize(find);
  if ((state != SQLITE_ROW && state != SQLITE_DONE) || finalized != SQLITE_OK) {
    return cancel();
  }
  sqlite3_stmt* statement = nullptr;
  const char* sql =
      existingId > 0
          ? "UPDATE supplier_requests SET requested_quantity=requested_quantity+?1 "
            "WHERE id=?2 AND status='Created' AND requested_quantity<=2147483647-?1;"
          : "INSERT INTO supplier_requests(product_id,requested_quantity,created_day,delivery_day,status) "
            "VALUES(?1,?2,?3,?4,'Created');";
  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    sqlite3_finalize(statement);
    return cancel();
  }
  int bound = SQLITE_OK;
  if (existingId > 0) {
    bound = sqlite3_bind_int(statement, 1, requestedQuantity);
    if (bound == SQLITE_OK) {
      bound = sqlite3_bind_int(statement, 2, existingId);
    }
  } else {
    bound = sqlite3_bind_int(statement, 1, productId);
    if (bound == SQLITE_OK) {
      bound = sqlite3_bind_int(statement, 2, requestedQuantity);
    }
    if (bound == SQLITE_OK) {
      bound = sqlite3_bind_int(statement, 3, createdDay);
    }
    if (bound == SQLITE_OK) {
      bound = sqlite3_bind_int(statement, 4, deliveryDay);
    }
  }
  const bool done = bound == SQLITE_OK &&
                    sqlite3_step(statement) == SQLITE_DONE &&
                    sqlite3_changes(db_) == 1;
  const int finished = sqlite3_finalize(statement);
  if (!done || finished != SQLITE_OK) {
    return cancel();
  }
  if (!exec(db_, "RELEASE merge_supplier_request;")) {
    return cancel();
  }
  return true;
}

std::vector<SupplierRequestRow> Database::getSupplierRequests() const {
  std::vector<SupplierRequestRow> result;

  if (!db_) {
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

  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return result;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    SupplierRequestRow row;

    row.id = sqlite3_column_int(stmt, 0);

    row.productId = sqlite3_column_int(stmt, 1);

    const auto* productName = sqlite3_column_text(stmt, 2);

    row.productName =
        productName ? reinterpret_cast<const char*>(productName) : "";

    row.requestedQuantity = sqlite3_column_int(stmt, 3);

    row.createdDay = sqlite3_column_int(stmt, 4);

    row.deliveryDay = sqlite3_column_int(stmt, 5);

    const auto* status = sqlite3_column_text(stmt, 6);

    row.status = status ? reinterpret_cast<const char*>(status) : "";

    result.push_back(std::move(row));
  }

  sqlite3_finalize(stmt);

  return result;
}
bool Database::hasActiveSupplierRequest(int productId) const {
  if (!db_ || productId <= 0) {
    return false;
  }

  const char* sql =
      "SELECT 1 "
      "FROM supplier_requests "
      "WHERE product_id=? "
      "AND status IN ('Created', 'InTransit') "
      "LIMIT 1";

  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_int(stmt, 1, productId);

  const bool exists = sqlite3_step(stmt) == SQLITE_ROW;

  sqlite3_finalize(stmt);

  return exists;
}
bool Database::setSupplierRequestStatus(int requestId,
                                        const std::string& status) {
  if (!db_ || requestId <= 0 || status.empty()) {
    return false;
  }

  const char* sql =
      "UPDATE supplier_requests "
      "SET status=? "
      "WHERE id=?";

  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);

  sqlite3_bind_int(stmt, 2, requestId);

  const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;

  sqlite3_finalize(stmt);

  return ok;
}

bool Database::setWarehouseQuantity(int warehouseId, int productId,
                                    int quantity) {
  if (!db_ || quantity < 0) return false;
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          db_,
          "UPDATE warehouse_inventory SET quantity=? WHERE warehouse_id=? AND product_id=?",
          -1, &stmt, nullptr) != SQLITE_OK)
    return false;
  sqlite3_bind_int(stmt, 1, quantity);
  sqlite3_bind_int(stmt, 2, warehouseId);
  sqlite3_bind_int(stmt, 3, productId);
  const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;
  sqlite3_finalize(stmt);
  return ok;
}
bool Database::changeWarehouseQuantityRaw(int warehouseId, int productId,
                                          int delta) {
  if (!db_) {
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

  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_int(stmt, 1, delta);

  sqlite3_bind_int(stmt, 2, warehouseId);

  sqlite3_bind_int(stmt, 3, productId);

  sqlite3_bind_int(stmt, 4, delta);

  sqlite3_bind_int(stmt, 5, delta);

  const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;

  sqlite3_finalize(stmt);

  return ok;
}
bool Database::setStoreQuantity(int storeId, int productId, int quantity) {
  if (!db_ || quantity < 0) return false;
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          db_,
          "UPDATE store_inventory SET quantity=? WHERE store_id=? AND product_id=?",
          -1, &stmt, nullptr) != SQLITE_OK)
    return false;
  sqlite3_bind_int(stmt, 1, quantity);
  sqlite3_bind_int(stmt, 2, storeId);
  sqlite3_bind_int(stmt, 3, productId);
  const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;
  sqlite3_finalize(stmt);
  return ok;
}
bool Database::changeStoreQuantity(int storeId, int productId, int delta) {
  if (!db_) return false;
  sqlite3_stmt* stmt = nullptr;
  const char* sql =
      "UPDATE store_inventory SET quantity=quantity+? WHERE store_id=? AND product_id=? AND quantity+?>=0 AND quantity+?<=capacity";
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;
  sqlite3_bind_int(stmt, 1, delta);
  sqlite3_bind_int(stmt, 2, storeId);
  sqlite3_bind_int(stmt, 3, productId);
  sqlite3_bind_int(stmt, 4, delta);
  sqlite3_bind_int(stmt, 5, delta);
  const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;
  sqlite3_finalize(stmt);
  return ok;
}
bool Database::beginTransaction() {
  return db_ && exec(db_, "BEGIN TRANSACTION;");
}
bool Database::commit() { return db_ && exec(db_, "COMMIT;"); }
bool Database::rollback() { return db_ && exec(db_, "ROLLBACK;"); }

bool Database::hasActiveStoreOrder(int storeId) const {
  if (!db_ || storeId <= 0) {
    return false;
  }

  const char* sql =
      "SELECT 1 "
      "FROM orders "
      "WHERE store_id = ? "
      "AND status NOT IN ('Completed', 'Cancelled') "
      "LIMIT 1";

  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_int(stmt, 1, storeId);

  const bool exists = sqlite3_step(stmt) == SQLITE_ROW;

  sqlite3_finalize(stmt);

  return exists;
}

bool Database::createStoreOrder(int storeId, int productId,
                                int requestedQuantity, int createdDay,
                                int deliveryDay) {
  if (!db_ || storeId <= 0 || productId <= 0 || requestedQuantity <= 0 ||
      createdDay <= 0 || deliveryDay <= createdDay) {
    return false;
  }

  if (sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) !=
      SQLITE_OK) {
    return false;
  }

  const char* orderSql =
      "INSERT INTO orders "
      "(store_id, created_day, delivery_day, status) "
      "VALUES (?, ?, ?, 'Created')";

  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db_, orderSql, -1, &stmt, nullptr) != SQLITE_OK) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);

    return false;
  }

  sqlite3_bind_int(stmt, 1, storeId);

  sqlite3_bind_int(stmt, 2, createdDay);

  sqlite3_bind_int(stmt, 3, deliveryDay);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);

    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);

    return false;
  }

  sqlite3_finalize(stmt);

  const int orderId = static_cast<int>(sqlite3_last_insert_rowid(db_));

  const char* itemSql =
      "INSERT INTO order_items "
      "("
      "order_id, "
      "product_id, "
      "requested_quantity, "
      "allocated_quantity"
      ") "
      "VALUES (?, ?, ?, 0)";

  if (sqlite3_prepare_v2(db_, itemSql, -1, &stmt, nullptr) != SQLITE_OK) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);

    return false;
  }

  sqlite3_bind_int(stmt, 1, orderId);

  sqlite3_bind_int(stmt, 2, productId);

  sqlite3_bind_int(stmt, 3, requestedQuantity);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);

    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);

    return false;
  }

  sqlite3_finalize(stmt);

  if (sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);

    return false;
  }

  return true;
}
bool Database::setOrderItemAllocated(int orderId, int productId,
                                     int allocatedQuantity) {
  if (!db_ || orderId <= 0 || productId <= 0 || allocatedQuantity < 0) {
    return false;
  }

  const char* sql =
      "UPDATE order_items "
      "SET allocated_quantity=? "
      "WHERE order_id=? "
      "AND product_id=?";

  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_int(stmt, 1, allocatedQuantity);

  sqlite3_bind_int(stmt, 2, orderId);

  sqlite3_bind_int(stmt, 3, productId);

  const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;

  sqlite3_finalize(stmt);

  return ok;
}

bool Database::setOrderStatus(int orderId, const std::string& status) {
  if (!db_ || orderId <= 0 || status.empty()) {
    return false;
  }

  const char* sql =
      "UPDATE orders "
      "SET status=? "
      "WHERE id=?";

  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);

  sqlite3_bind_int(stmt, 2, orderId);

  const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;

  sqlite3_finalize(stmt);

  return ok;
}

std::string Database::getLastError() const {
  if (db_ == nullptr) {
    return "Database is not open";
  }

  return sqlite3_errmsg(db_);
}

bool Database::copyFrom(const Database& source) {
  if (db_ == nullptr || source.db_ == nullptr) {
    return false;
  }

  sqlite3_backup* backup = sqlite3_backup_init(db_, "main", source.db_, "main");

  if (backup == nullptr) {
    return false;
  }

  const int stepResult = sqlite3_backup_step(backup, -1);
  const int finishResult = sqlite3_backup_finish(backup);

  return stepResult == SQLITE_DONE && finishResult == SQLITE_OK;
}

bool Database::getWarehouseBatches(
    int warehouseId, std::vector<WarehouseBatchRow>& batches) const {
  batches.clear();

  if (db_ == nullptr) {
    return false;
  }

  const char* sql =
      "SELECT b.id, b.warehouse_id, b.product_id, p.name, "
      "b.quantity, b.received_day, b.expires_day, "
      "b.unit_price_cents, b.discount_percent "
      "FROM warehouse_batches AS b "
      "JOIN products AS p ON p.id = b.product_id "
      "WHERE b.warehouse_id = ? AND b.quantity > 0 "
      "ORDER BY b.expires_day, b.id;";

  sqlite3_stmt* statement = nullptr;

  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    sqlite3_finalize(statement);
    return false;
  }

  if (sqlite3_bind_int(statement, 1, warehouseId) != SQLITE_OK) {
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

    const unsigned char* name = sqlite3_column_text(statement, 3);

    if (name != nullptr) {
      batch.productName = reinterpret_cast<const char*>(name);
    }

    batch.quantity = sqlite3_column_int(statement, 4);
    batch.receivedDay = sqlite3_column_int(statement, 5);
    batch.expiresDay = sqlite3_column_int(statement, 6);
    batch.unitPriceCents = sqlite3_column_int(statement, 7);

    batch.discountPercent = sqlite3_column_int(statement, 8);
    result.push_back(batch);
  }

  const int finalizeStatus = sqlite3_finalize(statement);

  if (status != SQLITE_DONE || finalizeStatus != SQLITE_OK) {
    return false;
  }

  batches.swap(result);
  return true;
}

bool Database::createWarehouseBatch(int warehouseId, int productId,
                                    int quantity, int receivedDay) {
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

  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    sqlite3_finalize(statement);
    return false;
  }

  const bool bound = sqlite3_bind_int(statement, 1, warehouseId) == SQLITE_OK &&
                     sqlite3_bind_int(statement, 2, quantity) == SQLITE_OK &&
                     sqlite3_bind_int(statement, 3, receivedDay) == SQLITE_OK &&
                     sqlite3_bind_int(statement, 4, receivedDay) == SQLITE_OK &&
                     sqlite3_bind_int(statement, 5, productId) == SQLITE_OK;

  const bool inserted = bound && sqlite3_step(statement) == SQLITE_DONE &&
                        sqlite3_changes(db_) == 1;

  const int finalizeStatus = sqlite3_finalize(statement);

  return inserted && finalizeStatus == SQLITE_OK;
}

bool Database::changeWarehouseQuantity(int warehouseId, int productId,
                                       int delta, int currentDay,
                                       int deliveryDay, int orderId) {
  if (db_ == nullptr || currentDay < 1) {
    return false;
  }

  const int requiredDay = deliveryDay > currentDay ? deliveryDay : currentDay;

  if (delta >= 0) {
    return changeWarehouseQuantityRaw(warehouseId, productId, delta);
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
    if (batch.productId == productId && batch.expiresDay > requiredDay) {
      available += batch.quantity;
    }
  }

  long long remaining = -static_cast<long long>(delta);

  if (available < remaining) {
    return cancel();
  }

  for (const WarehouseBatchRow& batch : batches) {
    if (remaining == 0) {
      break;
    }

    if (batch.productId != productId || batch.expiresDay <= requiredDay) {
      continue;
    }

    const int taken = remaining < batch.quantity ? static_cast<int>(remaining)
                                                 : batch.quantity;

    const std::string sql =
        "UPDATE warehouse_batches "
        "SET quantity = quantity - " +
        std::to_string(taken) + " WHERE id = " + std::to_string(batch.id) +
        " AND quantity >= " + std::to_string(taken) + ";";

    if (!executeSql(sql) || sqlite3_changes(db_) != 1) {
      return cancel();
    }
    const long long actualUnitPriceCents =
        (static_cast<long long>(batch.unitPriceCents) *
             (100 - batch.discountPercent) +
         50) /
        100;

    const std::string allocationSql =
        "INSERT INTO warehouse_allocations ("
        "batch_id, order_id, allocation_day, quantity, "
        "base_unit_price_cents, actual_unit_price_cents, "
        "discount_percent"
        ") VALUES (" +
        std::to_string(batch.id) + "," +
        (orderId > 0 ? std::to_string(orderId) : "NULL") + "," +
        std::to_string(currentDay) + "," + std::to_string(taken) + "," +
        std::to_string(batch.unitPriceCents) + "," +
        std::to_string(actualUnitPriceCents) + "," +
        std::to_string(batch.discountPercent) + ");";

    if (!executeSql(allocationSql)) {
      return cancel();
    }
    remaining -= taken;
  }

  if (!changeWarehouseQuantityRaw(warehouseId, productId, delta)) {
    return cancel();
  }

  if (!executeSql("RELEASE warehouse_dispatch;")) {
    return cancel();
  }

  return true;
}

bool Database::writeOffExpiredBatches(int warehouseId, int currentDay) {
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
        static_cast<long long>(batch.quantity) * batch.unitPriceCents;

    const std::string insertSql =
        "INSERT INTO warehouse_writeoffs ("
        "batch_id, warehouse_id, product_id, "
        "writeoff_day, quantity, loss_cents"
        ") VALUES (" +
        std::to_string(batch.id) + "," + std::to_string(warehouseId) + "," +
        std::to_string(batch.productId) + "," + std::to_string(currentDay) +
        "," + std::to_string(batch.quantity) + "," + std::to_string(lossCents) +
        ");";

    if (!executeSql(insertSql)) {
      return cancel();
    }

    if (!changeWarehouseQuantityRaw(warehouseId, batch.productId,
                                    -batch.quantity)) {
      return cancel();
    }

    const std::string updateSql =
        "UPDATE warehouse_batches SET quantity = 0 "
        "WHERE id = " +
        std::to_string(batch.id) + ";";

    if (!executeSql(updateSql) || sqlite3_changes(db_) != 1) {
      return cancel();
    }
  }

  if (!executeSql("RELEASE expired_writeoff;")) {
    return cancel();
  }

  return true;
}

bool Database::getWarehouseWriteoffs(
    int warehouseId, std::vector<WarehouseWriteoffRow>& writeoffs) const {
  writeoffs.clear();

  if (db_ == nullptr) {
    return false;
  }

  const char* sql =
      "SELECT w.id, w.batch_id, w.product_id, p.name, "
      "w.writeoff_day, w.quantity, w.loss_cents "
      "FROM warehouse_writeoffs AS w "
      "JOIN products AS p ON p.id = w.product_id "
      "WHERE w.warehouse_id = ? "
      "ORDER BY w.writeoff_day DESC, w.id DESC;";

  sqlite3_stmt* statement = nullptr;

  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    sqlite3_finalize(statement);
    return false;
  }

  if (sqlite3_bind_int(statement, 1, warehouseId) != SQLITE_OK) {
    sqlite3_finalize(statement);
    return false;
  }

  std::vector<WarehouseWriteoffRow> result;
  int status = SQLITE_OK;

  while ((status = sqlite3_step(statement)) == SQLITE_ROW) {
    WarehouseWriteoffRow row;

    row.id = sqlite3_column_int(statement, 0);
    row.batchId = sqlite3_column_int(statement, 1);
    row.productId = sqlite3_column_int(statement, 2);

    const unsigned char* name = sqlite3_column_text(statement, 3);

    if (name != nullptr) {
      row.productName = reinterpret_cast<const char*>(name);
    }

    row.writeoffDay = sqlite3_column_int(statement, 4);
    row.quantity = sqlite3_column_int(statement, 5);
    row.lossCents = sqlite3_column_int64(statement, 6);

    result.push_back(row);
  }

  const int finalizeStatus = sqlite3_finalize(statement);

  if (status != SQLITE_DONE || finalizeStatus != SQLITE_OK) {
    return false;
  }

  writeoffs.swap(result);
  return true;
}

bool Database::setBatchDiscount(int warehouseId, int batchId, int percent,
                                int currentDay) {
  if (db_ == nullptr || currentDay < 1 || percent < 0 || percent > 90) {
    return false;
  }

  const char* sql =
      "UPDATE warehouse_batches "
      "SET discount_percent = ? "
      "WHERE warehouse_id = ? AND id = ? "
      "AND quantity > 0 "
      "AND expires_day > ? "
      "AND expires_day - ? <= 3;";

  sqlite3_stmt* statement = nullptr;

  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    sqlite3_finalize(statement);
    return false;
  }

  const bool bound = sqlite3_bind_int(statement, 1, percent) == SQLITE_OK &&
                     sqlite3_bind_int(statement, 2, warehouseId) == SQLITE_OK &&
                     sqlite3_bind_int(statement, 3, batchId) == SQLITE_OK &&
                     sqlite3_bind_int(statement, 4, currentDay) == SQLITE_OK &&
                     sqlite3_bind_int(statement, 5, currentDay) == SQLITE_OK;

  const bool updated = bound && sqlite3_step(statement) == SQLITE_DONE &&
                       sqlite3_changes(db_) == 1;

  const int finalizeStatus = sqlite3_finalize(statement);

  return updated && finalizeStatus == SQLITE_OK;
}

bool Database::getWarehouseAllocationStats(
    int warehouseId, WarehouseAllocationStats& stats) const {
  stats = WarehouseAllocationStats{};

  if (db_ == nullptr) {
    return false;
  }

  const char* sql =
      "SELECT "
      "COALESCE(SUM(a.quantity), 0), "
      "COALESCE(SUM(a.quantity * a.actual_unit_price_cents), 0), "
      "COALESCE(SUM(a.quantity * "
      "(a.base_unit_price_cents - a.actual_unit_price_cents)), 0) "
      "FROM warehouse_allocations AS a "
      "JOIN warehouse_batches AS b ON b.id = a.batch_id "
      "WHERE b.warehouse_id = ?;";

  sqlite3_stmt* statement = nullptr;

  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    sqlite3_finalize(statement);
    return false;
  }

  if (sqlite3_bind_int(statement, 1, warehouseId) != SQLITE_OK) {
    sqlite3_finalize(statement);
    return false;
  }

  if (sqlite3_step(statement) != SQLITE_ROW) {
    sqlite3_finalize(statement);
    return false;
  }

  WarehouseAllocationStats result;

  result.allocatedUnits = sqlite3_column_int64(statement, 0);

  result.allocatedValueCents = sqlite3_column_int64(statement, 1);

  result.discountLossCents = sqlite3_column_int64(statement, 2);

  const int nextStatus = sqlite3_step(statement);
  const int finalizeStatus = sqlite3_finalize(statement);

  if (nextStatus != SQLITE_DONE || finalizeStatus != SQLITE_OK) {
    return false;
  }

  stats = result;
  return true;
}

bool Database::createStoreOrder(int storeId,
                                const std::vector<OrderItemRow>& items,
                                int createdDay, int deliveryDay) {
  if (db_ == nullptr || storeId <= 0 || items.empty() || createdDay <= 0 ||
      deliveryDay <= createdDay) {
    return false;
  }

  for (const OrderItemRow& item : items) {
    if (item.productId <= 0 || item.requestedQuantity <= 0) {
      return false;
    }
  }

  if (!executeSql("SAVEPOINT create_multi_order;")) {
    return false;
  }

  const auto cancel = [this]() {
    executeSql("ROLLBACK TO create_multi_order;");
    executeSql("RELEASE create_multi_order;");
    return false;
  };

  const std::string orderSql =
      "INSERT INTO orders ("
      "store_id, created_day, delivery_day, status"
      ") SELECT " +
      std::to_string(storeId) + "," + std::to_string(createdDay) + "," +
      std::to_string(deliveryDay) +
      ",'Created' "
      "WHERE NOT EXISTS ("
      "SELECT 1 FROM orders WHERE store_id = " +
      std::to_string(storeId) +
      " AND created_day = " + std::to_string(createdDay) + ");";

  if (!executeSql(orderSql) || sqlite3_changes(db_) != 1) {
    return cancel();
  }

  const long long orderId = sqlite3_last_insert_rowid(db_);

  for (const OrderItemRow& item : items) {
    const std::string itemSql =
        "INSERT INTO order_items ("
        "order_id, product_id, requested_quantity, "
        "allocated_quantity"
        ") VALUES (" +
        std::to_string(orderId) + "," + std::to_string(item.productId) + "," +
        std::to_string(item.requestedQuantity) + ",0);";

    if (!executeSql(itemSql)) {
      return cancel();
    }
  }

  if (!executeSql("RELEASE create_multi_order;")) {
    return cancel();
  }

  return true;
}

bool Database::isOrderStockValid(int orderId, int currentDay) const {
  if (!db_ || orderId <= 0 || currentDay <= 0) return false;
  const char* sql =
      "SELECT EXISTS (SELECT 1 FROM order_items WHERE order_id=?1 AND allocated_quantity>0) "
      "AND NOT EXISTS (SELECT 1 FROM warehouse_allocations a "
      "JOIN warehouse_batches b ON b.id=a.batch_id "
      "WHERE a.order_id=?1 AND b.expires_day<=?2) "
      "AND NOT EXISTS (SELECT 1 FROM order_items oi WHERE oi.order_id=?1 "
      "AND oi.allocated_quantity<>COALESCE((SELECT SUM(a.quantity) "
      "FROM warehouse_allocations a JOIN warehouse_batches b ON b.id=a.batch_id "
      "WHERE a.order_id=oi.order_id AND b.product_id=oi.product_id),0));";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    sqlite3_finalize(stmt);
    return false;
  }
  const bool bound = sqlite3_bind_int(stmt, 1, orderId) == SQLITE_OK &&
                     sqlite3_bind_int(stmt, 2, currentDay) == SQLITE_OK;
  const bool valid = bound && sqlite3_step(stmt) == SQLITE_ROW &&
                     sqlite3_column_int(stmt, 0) == 1;
  sqlite3_finalize(stmt);
  return valid;
}

bool Database::getExperimentReport(int warehouseId,
                                   ExperimentReport& report) const {
  report = ExperimentReport{};
  if (db_ == nullptr) {
    return false;
  }

  // Aggregate each ledger separately to avoid multiplying rows in joins.
  const char* sql = R"SQL(
        WITH costs AS (
            SELECT purchase_cost_percent AS percent FROM experiment_settings WHERE id=1
        ), shipped AS (
            SELECT b.product_id,
                   SUM(a.quantity) AS units,
                   SUM(a.quantity * a.actual_unit_price_cents) AS revenue,
                   SUM(a.quantity * (a.base_unit_price_cents -
                                     a.actual_unit_price_cents)) AS discount_loss,
                   SUM(a.quantity * ((a.base_unit_price_cents * (SELECT percent FROM costs) + 50) / 100)) AS cost
            FROM warehouse_allocations a
            JOIN warehouse_batches b ON b.id = a.batch_id
            JOIN orders o ON o.id = a.order_id AND o.status = 'Completed'
            WHERE b.warehouse_id = ?1
            GROUP BY b.product_id
        ), pending AS (
            SELECT b.product_id, SUM(a.quantity) AS units
            FROM warehouse_allocations a
            JOIN warehouse_batches b ON b.id = a.batch_id
            JOIN orders o ON o.id = a.order_id AND o.status <> 'Completed'
            WHERE b.warehouse_id = ?1
            GROUP BY b.product_id
        ), losses AS (
            SELECT w.product_id, SUM(w.quantity) AS units, SUM(w.loss_cents) AS loss,
                   SUM(w.quantity * ((b.unit_price_cents * (SELECT percent FROM costs) + 50) / 100)) AS cost
            FROM warehouse_writeoffs w JOIN warehouse_batches b ON b.id=w.batch_id
            WHERE w.warehouse_id = ?1
            GROUP BY w.product_id
        ), stock AS (
            SELECT product_id, SUM(quantity) AS units
            FROM warehouse_inventory WHERE warehouse_id = ?1
            GROUP BY product_id
        ), shops AS (
            SELECT product_id, SUM(quantity) AS units
            FROM store_inventory GROUP BY product_id
        ), supplied AS (
            SELECT product_id, SUM(requested_quantity) AS units
            FROM supplier_requests WHERE status = 'Delivered'
            GROUP BY product_id
        )
        SELECT p.id, p.name, p.units_per_package,
               COALESCE(stock.units, 0), COALESCE(shops.units, 0),
               COALESCE(pending.units, 0), COALESCE(shipped.units, 0),
               COALESCE(shipped.revenue, 0), COALESCE(shipped.discount_loss, 0),
               COALESCE(losses.units, 0), COALESCE(losses.loss, 0),
               COALESCE(supplied.units, 0),
               (SELECT COUNT(*) FROM orders WHERE status = 'Completed'),
               (SELECT COUNT(*) FROM orders WHERE status <> 'Completed'),
               COALESCE(shipped.cost, 0), COALESCE(losses.cost, 0)
        FROM products p
        LEFT JOIN stock ON stock.product_id = p.id
        LEFT JOIN shops ON shops.product_id = p.id
        LEFT JOIN pending ON pending.product_id = p.id
        LEFT JOIN shipped ON shipped.product_id = p.id
        LEFT JOIN losses ON losses.product_id = p.id
        LEFT JOIN supplied ON supplied.product_id = p.id
        ORDER BY p.id;
    )SQL";
  sqlite3_stmt* statement = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) {
    sqlite3_finalize(statement);
    return false;
  }
  if (sqlite3_bind_int(statement, 1, warehouseId) != SQLITE_OK) {
    sqlite3_finalize(statement);
    return false;
  }
  ExperimentReport result;
  if (!getExperimentSettings(result.settings)) {
    sqlite3_finalize(statement);
    return false;
  }
  int status = SQLITE_OK;
  while ((status = sqlite3_step(statement)) == SQLITE_ROW) {
    ExperimentProductResult item;
    item.productId = sqlite3_column_int(statement, 0);
    const unsigned char* name = sqlite3_column_text(statement, 1);
    item.productName = name ? reinterpret_cast<const char*>(name) : "";
    item.unitsPerPackage = sqlite3_column_int(statement, 2);
    item.warehouseUnits = sqlite3_column_int64(statement, 3);
    item.storeUnits = sqlite3_column_int64(statement, 4);
    item.pendingUnits = sqlite3_column_int64(statement, 5);
    item.deliveredUnits = sqlite3_column_int64(statement, 6);
    item.revenueCents = sqlite3_column_int64(statement, 7);
    item.discountLossCents = sqlite3_column_int64(statement, 8);
    item.writtenOffUnits = sqlite3_column_int64(statement, 9);
    item.writeoffLossCents = sqlite3_column_int64(statement, 10);
    item.suppliedUnits = sqlite3_column_int64(statement, 11);
    item.deliveredCostCents = sqlite3_column_int64(statement, 14);
    item.writeoffCostCents = sqlite3_column_int64(statement, 15);
    result.deliveredCostCents += item.deliveredCostCents;
    result.writeoffCostCents += item.writeoffCostCents;
    result.completedOrders = sqlite3_column_int64(statement, 12);
    result.pendingOrders = sqlite3_column_int64(statement, 13);
    result.deliveredUnits += item.deliveredUnits;
    result.revenueCents += item.revenueCents;
    result.discountLossCents += item.discountLossCents;
    result.writtenOffUnits += item.writtenOffUnits;
    result.writeoffLossCents += item.writeoffLossCents;
    result.suppliedUnits += item.suppliedUnits;
    result.products.push_back(std::move(item));
  }
  const int finalized = sqlite3_finalize(statement);
  if (status != SQLITE_DONE || finalized != SQLITE_OK) {
    return false;
  }
  report = std::move(result);
  return true;
}

bool Database::getWarehouseSupplySummary(
    int warehouseId, std::vector<WarehouseSupplySummary>& rows) const {
  rows.clear();
  if (!db_ || warehouseId <= 0) {
    return false;
  }
  const char* sql = R"SQL(
        WITH active_orders AS (
            SELECT o.store_id, oi.product_id,
                SUM(oi.allocated_quantity) AS allocated,
                SUM(MAX(0, oi.requested_quantity-oi.allocated_quantity)) AS missing
            FROM orders o JOIN order_items oi ON oi.order_id=o.id
            WHERE o.status IN ('Created','WaitingSupply','Allocated')
            GROUP BY o.store_id,oi.product_id
        ), demand AS (
            SELECT si.product_id,
                SUM(MAX(COALESCE(a.missing,0),
                    MAX(0,MIN(si.capacity,(12*(SELECT demand_percent FROM experiment_settings WHERE id=1)+99)/100)-si.quantity-COALESCE(a.allocated,0)))) AS units
            FROM store_inventory si
            LEFT JOIN active_orders a ON a.store_id=si.store_id AND a.product_id=si.product_id
            GROUP BY si.product_id
        ), supply AS (
            SELECT product_id,
                SUM(CASE WHEN status='Created' THEN requested_quantity ELSE 0 END) AS ordered,
                SUM(CASE WHEN status='InTransit' THEN requested_quantity ELSE 0 END) AS transit
            FROM supplier_requests GROUP BY product_id
        )
        SELECT p.id,p.name,COALESCE(wi.quantity,0),COALESCE(d.units,0),
            COALESCE(s.ordered,0),COALESCE(s.transit,0)
        FROM products p
        LEFT JOIN warehouse_inventory wi ON wi.product_id=p.id AND wi.warehouse_id=?1
        LEFT JOIN demand d ON d.product_id=p.id
        LEFT JOIN supply s ON s.product_id=p.id
        ORDER BY p.id;
    )SQL";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    sqlite3_finalize(stmt);
    return false;
  }
  if (sqlite3_bind_int(stmt, 1, warehouseId) != SQLITE_OK) {
    sqlite3_finalize(stmt);
    return false;
  }
  std::vector<WarehouseSupplySummary> result;
  int status;
  while ((status = sqlite3_step(stmt)) == SQLITE_ROW) {
    WarehouseSupplySummary row;
    row.productId = sqlite3_column_int(stmt, 0);
    const unsigned char* name = sqlite3_column_text(stmt, 1);
    row.productName = name ? reinterpret_cast<const char*>(name) : "";
    row.warehouseUnits = sqlite3_column_int64(stmt, 2);
    row.storeNeed = sqlite3_column_int64(stmt, 3);
    row.orderedUnits = sqlite3_column_int64(stmt, 4);
    row.inTransitUnits = sqlite3_column_int64(stmt, 5);
    const long long shortage = row.storeNeed - row.warehouseUnits -
                               row.orderedUnits - row.inTransitUnits;
    row.missingUnits = shortage > 0 ? shortage : 0;
    result.push_back(std::move(row));
  }
  const int finalized = sqlite3_finalize(stmt);
  if (status != SQLITE_DONE || finalized != SQLITE_OK) {
    return false;
  }
  rows = std::move(result);
  return true;
}
