#include "application/BackendFacade.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}

long long storeUnits(const BackendFacade& backend) {
  long long total = 0;
  for (const auto& store : backend.getStores()) {
    for (const auto& item : backend.getStoreInventory(store.id)) {
      total += item.quantity;
    }
  }
  return total;
}

void checkInventory(const BackendFacade& backend) {
  std::vector<WarehouseBatchRow> batches;
  check(backend.getWarehouseBatches(batches), "Cannot read batches");
  for (const auto& item : backend.getWarehouseInventory()) {
    check(item.quantity >= 0 && item.quantity <= item.capacity,
          "Warehouse capacity violated");
    long long batchUnits = 0;
    for (const auto& batch : batches) {
      if (batch.productId == item.productId) batchUnits += batch.quantity;
    }
    check(batchUnits == item.quantity, "Batch and inventory quantities differ");
  }
  for (const auto& store : backend.getStores()) {
    for (const auto& item : backend.getStoreInventory(store.id)) {
      check(item.quantity >= 0 && item.quantity <= item.capacity,
            "Store capacity violated");
    }
  }
}

void testFill(const std::string& schema, const std::string& path) {
  BackendFacade backend;
  for (int fill : {0, 50, 100, -1}) {
    ExperimentSettings settings;
    settings.initialFillPercent = fill;
    settings.demandPercent = 150;
    settings.stockCoverDays = 4;
    settings.purchaseCostPercent = 63;
    check(backend.startExperiment(path, schema, 9, 20, settings),
          backend.getLastError());
    check(backend.getStores().size() == 9, "Store assortment was lost");
    checkInventory(backend);
    for (const auto& item : backend.getWarehouseInventory()) {
      check(item.quantity % item.unitsPerPackage == 0,
            "Partial initial package");
      if (fill >= 0) {
        check(item.quantity ==
                  (item.capacity * fill / 100 / item.unitsPerPackage) *
                      item.unitsPerPackage,
              "Incorrect initial fill");
      }
      if (fill == 0)
        check(item.minStock > 0, "Empty warehouse lost reorder point");
    }
    ExperimentReport report;
    check(backend.getExperimentReport(report), "Cannot read experiment report");
    check(report.revenueCents == 0 && report.deliveredCostCents == 0,
          "Opening inventory incorrectly charged as an expense");
    check(backend.initialize(path), "Cannot reopen experiment");
    check(backend.getExperimentReport(report), "Cannot read reopened settings");
    check(report.settings.initialFillPercent == fill &&
              report.settings.demandPercent == 150 &&
              report.settings.stockCoverDays == 4 &&
              report.settings.purchaseCostPercent == 63,
          "Settings did not survive reopen");
    check(backend.resetDatabase(path, schema), "Cannot repeat experiment");
    check(backend.getExperimentReport(report) &&
              report.settings.purchaseCostPercent == 63 &&
              report.settings.initialFillPercent == fill,
          "Settings did not survive reset");
  }
  for (int field = 0; field < 4; ++field) {
    ExperimentSettings invalid;
    if (field == 0) invalid.initialFillPercent = 101;
    if (field == 1) invalid.demandPercent = 49;
    if (field == 2) invalid.stockCoverDays = 15;
    if (field == 3) invalid.purchaseCostPercent = 0;
    check(!backend.startExperiment(path, schema, 9, 20, invalid),
          "Invalid setting was accepted");
  }
}

void testDemandAndReserve(const std::string& schema, const std::string& path) {
  BackendFacade backend;
  std::vector<long long> sales;
  for (int percent : {50, 100, 150}) {
    ExperimentSettings settings;
    settings.demandPercent = percent;
    check(backend.startExperiment(path, schema, 9, 20, settings),
          backend.getLastError());
    const long long before = storeUnits(backend);
    for (int interval = 1; interval <= 8; ++interval) {
      check(backend.simulateStoreSales(1, interval), backend.getLastError());
    }
    sales.push_back(before - storeUnits(backend));
  }
  check(sales[0] < sales[1] && sales[1] < sales[2],
        "Demand does not scale sales");
  std::vector<long long> reserves;
  std::vector<long long> orders;
  for (int days : {2, 7}) {
    ExperimentSettings settings;
    settings.stockCoverDays = days;
    check(backend.startExperiment(path, schema, 9, 20, settings),
          backend.getLastError());
    long long units = 0;
    for (const auto& item : backend.getWarehouseInventory())
      units += item.quantity;
    reserves.push_back(units);
    settings.initialFillPercent = 0;
    check(backend.startExperiment(path, schema, 9, 20, settings),
          backend.getLastError());
    backend.processSupplierRequests(1);
    check(backend.getLastError().empty(), backend.getLastError());
    units = 0;
    for (const auto& request : backend.getSupplierRequests()) {
      units += request.requestedQuantity;
    }
    orders.push_back(units);
  }
  check(reserves[1] > reserves[0],
        "Cover days do not affect automatic reserve");
  check(orders[1] > orders[0], "Cover days do not affect replenishment");
  std::cout << "Sales at 50/100/150%: " << sales[0] << '/' << sales[1] << '/'
            << sales[2] << "; reserve at 2/7 days: " << reserves[0] << '/'
            << reserves[1] << '\n';
}

void testCosts(const std::string& schema, const std::string& path) {
  BackendFacade backend;
  check(backend.startExperiment(path, schema, 9, 20), backend.getLastError());
  {
    Database db;
    check(db.open(path), "Cannot open ledger fixture");
    check(db.executeSql(R"SQL(
      UPDATE warehouse_batches SET unit_price_cents=10001 WHERE id=1;
      INSERT INTO orders(id,store_id,created_day,delivery_day,status)
      VALUES(1,1,1,2,'Completed'),(2,1,2,3,'Completed'),(3,1,3,4,'Allocated');
      INSERT INTO warehouse_allocations(batch_id,order_id,allocation_day,quantity,
          base_unit_price_cents,actual_unit_price_cents,discount_percent)
      VALUES(1,1,1,4,10001,7001,30),(1,2,2,6,10001,7001,30),
            (1,3,3,3,10001,7001,30);
      INSERT INTO warehouse_writeoffs(batch_id,warehouse_id,product_id,
          writeoff_day,quantity,loss_cents) VALUES(1,1,1,4,2,20002);
      INSERT INTO supplier_requests(product_id,requested_quantity,created_day,
          delivery_day,status) VALUES(1,12,1,2,'Delivered');
    )SQL"),
          "Cannot create ledger fixture");
  }
  ExperimentReport report;
  check(backend.getExperimentReport(report), "Cannot calculate costs");
  check(report.deliveredUnits == 10 && report.revenueCents == 70010 &&
            report.deliveredCostCents == 70010 &&
            report.writeoffCostCents == 14002 &&
            report.discountLossCents == 30000 && report.suppliedUnits == 12,
        "Wrong cost, discount, pending-order or supplied-stock accounting");
  check(report.revenueCents - report.deliveredCostCents -
                report.writeoffCostCents ==
            -14002,
        "Discount or purchase cost was charged twice");
}

void testLegacyDatabase(const std::string& schema, const std::string& path) {
  BackendFacade backend;
  check(backend.startExperiment(path, schema, 9, 20), backend.getLastError());
  const long long before = storeUnits(backend);
  {
    Database db;
    check(db.open(path) && db.executeSql("DROP TABLE experiment_settings"),
          "Cannot prepare a legacy database");
  }
  check(backend.initialize(path), "Legacy database cannot be opened");
  ExperimentReport report;
  check(backend.getExperimentReport(report), "Legacy report cannot be read");
  check(report.settings.demandPercent == 100 &&
            report.settings.purchaseCostPercent == 70 &&
            storeUnits(backend) == before,
        "Legacy database migration altered inventory");
}

void testExperiment(const std::string& schema, const std::string& path,
                    int days, const ExperimentSettings& settings) {
  BackendFacade backend;
  check(backend.startExperiment(path, schema, 9, 20, settings),
        backend.getLastError());
  for (int day = 1; day <= days; ++day) {
    check(backend.writeOffExpiredBatches(day), backend.getLastError());
    backend.processSupplierDeliveries(day);
    check(backend.getLastError().empty(), backend.getLastError());
    backend.processStoreDeliveries(day);
    check(backend.getLastError().empty(), backend.getLastError());
    backend.processStoreOrders(day);
    check(backend.getLastError().empty(), backend.getLastError());
    backend.processSupplierRequests(day);
    check(backend.getLastError().empty(), backend.getLastError());
    for (const auto& request : backend.getSupplierRequests()) {
      if (request.status == "Created") {
        check(backend.approveSupplierRequest(request.id, day),
              backend.getLastError());
      }
    }
    for (int interval = 1; interval <= 8; ++interval) {
      check(backend.simulateStoreSales(day, interval), backend.getLastError());
    }
    checkInventory(backend);
    ExperimentReport report;
    check(backend.getExperimentReport(report), "Daily report failed");
    long long revenue = 0, costs = 0, writeoffs = 0;
    for (const auto& item : report.products) {
      revenue += item.revenueCents;
      costs += item.deliveredCostCents;
      writeoffs += item.writeoffCostCents;
    }
    check(revenue == report.revenueCents &&
              costs == report.deliveredCostCents &&
              writeoffs == report.writeoffCostCents,
          "Product totals do not match the report");
  }
  std::cout << "Passed " << days << " days; demand=" << settings.demandPercent
            << ", fill=" << settings.initialFillPercent << '\n';
}
}

int main(int argc, char** argv) {
  try {
    check(argc == 3, "Usage: test SCHEMA_SQL TEMP_DB");
    const std::string schema = argv[1], path = argv[2];
    testFill(schema, path);
    testDemandAndReserve(schema, path);
    testCosts(schema, path);
    testLegacyDatabase(schema, path);
    ExperimentSettings settings;
    testExperiment(schema, path, 10, settings);
    settings.initialFillPercent = 0;
    settings.demandPercent = 150;
    settings.stockCoverDays = 2;
    testExperiment(schema, path, 30, settings);
    settings.initialFillPercent = 100;
    settings.demandPercent = 50;
    settings.stockCoverDays = 14;
    settings.purchaseCostPercent = 100;
    testExperiment(schema, path, 30, settings);
    std::cout << "All experiment settings tests passed\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "FAILED: " << error.what() << '\n';
    return 1;
  }
}
