PRAGMA foreign_keys = ON;
BEGIN TRANSACTION;
ALTER TABLE products ADD COLUMN unit_name TEXT NOT NULL DEFAULT 'шт';
ALTER TABLE products ADD COLUMN units_per_package INTEGER NOT NULL DEFAULT 1 CHECK(units_per_package > 0);
ALTER TABLE products ADD COLUMN price_per_unit REAL NOT NULL DEFAULT 0 CHECK(price_per_unit >= 0);
ALTER TABLE products ADD COLUMN shelf_life_days INTEGER NOT NULL DEFAULT 0 CHECK(shelf_life_days >= 0);
UPDATE products SET unit_name='пачка', units_per_package=12, price_per_unit=85.00, shelf_life_days=7 WHERE id=1;
UPDATE products SET unit_name='шт', units_per_package=10, price_per_unit=45.00, shelf_life_days=5 WHERE id=2;
UPDATE products SET unit_name='кг', units_per_package=10, price_per_unit=120.00, shelf_life_days=20 WHERE id=3;
UPDATE products SET unit_name='кг', units_per_package=10, price_per_unit=110.00, shelf_life_days=14 WHERE id=4;
UPDATE products SET unit_name='кг', units_per_package=25, price_per_unit=55.00, shelf_life_days=60 WHERE id=5;
UPDATE products SET unit_name='кг', units_per_package=10, price_per_unit=320.00, shelf_life_days=7 WHERE id=6;
UPDATE products SET unit_name='пачка', units_per_package=12, price_per_unit=180.00, shelf_life_days=14 WHERE id=7;
UPDATE products SET unit_name='шт', units_per_package=20, price_per_unit=95.00, shelf_life_days=180 WHERE id=8;
UPDATE products SET unit_name='пачка', units_per_package=16, price_per_unit=90.00, shelf_life_days=120 WHERE id=9;
UPDATE products SET unit_name='пачка', units_per_package=12, price_per_unit=130.00, shelf_life_days=180 WHERE id=10;
UPDATE products SET unit_name='кг', units_per_package=8, price_per_unit=650.00, shelf_life_days=30 WHERE id=11;
UPDATE products SET unit_name='шт', units_per_package=12, price_per_unit=75.00, shelf_life_days=10 WHERE id=12;
UPDATE products SET unit_name='пачка', units_per_package=10, price_per_unit=70.00, shelf_life_days=365 WHERE id=13;
UPDATE products SET unit_name='шт', units_per_package=12, price_per_unit=160.00, shelf_life_days=0 WHERE id=14;
UPDATE products SET unit_name='шт', units_per_package=8, price_per_unit=420.00, shelf_life_days=0 WHERE id=15;
UPDATE products SET unit_name='упаковка', units_per_package=10, price_per_unit=250.00, shelf_life_days=0 WHERE id=16;
UPDATE products SET unit_name='шт', units_per_package=20, price_per_unit=300.00, shelf_life_days=0 WHERE id=17;
UPDATE products SET unit_name='пачка', units_per_package=5, price_per_unit=350.00, shelf_life_days=0 WHERE id=18;
UPDATE products SET unit_name='шт', units_per_package=50, price_per_unit=35.00, shelf_life_days=0 WHERE id=19;
UPDATE products SET unit_name='шт', units_per_package=25, price_per_unit=40.00, shelf_life_days=0 WHERE id=20;
CREATE TABLE IF NOT EXISTS orders (
 id INTEGER PRIMARY KEY AUTOINCREMENT,
 store_id INTEGER NOT NULL,
 created_day INTEGER NOT NULL CHECK(created_day > 0),
 delivery_day INTEGER NOT NULL CHECK(delivery_day > created_day),
 status TEXT NOT NULL DEFAULT 'Created',
 FOREIGN KEY (store_id) REFERENCES stores(id) ON DELETE CASCADE
);
CREATE UNIQUE INDEX IF NOT EXISTS idx_one_order_per_store_per_day ON orders(store_id, created_day);
CREATE TABLE IF NOT EXISTS order_items (
 order_id INTEGER NOT NULL,
 product_id INTEGER NOT NULL,
 requested_quantity INTEGER NOT NULL CHECK(requested_quantity > 0),
 allocated_quantity INTEGER NOT NULL DEFAULT 0 CHECK(allocated_quantity >= 0),
 package_count INTEGER NOT NULL DEFAULT 0 CHECK(package_count >= 0),
 PRIMARY KEY (order_id, product_id),
 FOREIGN KEY (order_id) REFERENCES orders(id) ON DELETE CASCADE,
 FOREIGN KEY (product_id) REFERENCES products(id) ON DELETE RESTRICT
);
CREATE TABLE IF NOT EXISTS supplier_requests (
 id INTEGER PRIMARY KEY AUTOINCREMENT,
 product_id INTEGER NOT NULL,
 requested_quantity INTEGER NOT NULL CHECK(requested_quantity > 0),
 created_day INTEGER NOT NULL CHECK(created_day > 0),
 delivery_day INTEGER,
 status TEXT NOT NULL DEFAULT 'Created',
 FOREIGN KEY (product_id) REFERENCES products(id) ON DELETE RESTRICT
);
DROP VIEW IF EXISTS store_inventory_view;
DROP VIEW IF EXISTS warehouse_inventory_view;
CREATE VIEW store_inventory_view AS
SELECT s.id AS store_id,s.name AS store_name,p.id AS product_id,p.name AS product_name,p.category,p.unit_name,p.units_per_package,p.price_per_unit,p.shelf_life_days,si.quantity,si.capacity,si.min_stock,(si.capacity-si.quantity) AS free_capacity
FROM store_inventory si JOIN stores s ON s.id=si.store_id JOIN products p ON p.id=si.product_id;
CREATE VIEW warehouse_inventory_view AS
SELECT w.id AS warehouse_id,w.name AS warehouse_name,p.id AS product_id,p.name AS product_name,p.category,p.unit_name,p.units_per_package,p.price_per_unit,p.shelf_life_days,wi.quantity,wi.capacity,wi.min_stock,(wi.capacity-wi.quantity) AS free_capacity
FROM warehouse_inventory wi JOIN warehouses w ON w.id=wi.warehouse_id JOIN products p ON p.id=wi.product_id;
COMMIT;
