BEGIN TRANSACTION;

CREATE TABLE stores (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE
);

INSERT INTO "stores" VALUES(1,'FreshMart');
INSERT INTO "stores" VALUES(2,'Green Basket');
INSERT INTO "stores" VALUES(3,'Bakery House');
INSERT INTO "stores" VALUES(4,'MeatPoint');
INSERT INTO "stores" VALUES(5,'Sweet Corner');
INSERT INTO "stores" VALUES(6,'Daily Food');
INSERT INTO "stores" VALUES(7,'FrozenFood');
INSERT INTO "stores" VALUES(8,'TeaCorner');
INSERT INTO "stores" VALUES(9,'GrainMarket');

CREATE TABLE warehouses (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    address TEXT
);

INSERT INTO "warehouses" VALUES(
    1,
    'Центральный склад Nova Poshta',
    'Основной учебный склад'
);

CREATE TABLE products (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    category TEXT NOT NULL
);

INSERT INTO "products" VALUES(1,'Молоко','Пищевой');
INSERT INTO "products" VALUES(2,'Хлеб','Пищевой');
INSERT INTO "products" VALUES(3,'Яблоки','Пищевой');
INSERT INTO "products" VALUES(4,'Бананы','Пищевой');
INSERT INTO "products" VALUES(5,'Картофель','Пищевой');
INSERT INTO "products" VALUES(6,'Куриное филе','Пищевой');
INSERT INTO "products" VALUES(7,'Сосиски','Пищевой');
INSERT INTO "products" VALUES(8,'Шоколад','Пищевой');
INSERT INTO "products" VALUES(9,'Печенье','Пищевой');
INSERT INTO "products" VALUES(10,'Сок','Пищевой');
INSERT INTO "products" VALUES(11,'Сыр','Пищевой');
INSERT INTO "products" VALUES(12,'Йогурт','Пищевой');
INSERT INTO "products" VALUES(13,'Мука','Пищевой');
INSERT INTO "products" VALUES(14,'Замороженная рыба','Замороженные продукты');
INSERT INTO "products" VALUES(15,'Пельмени','Замороженные продукты');
INSERT INTO "products" VALUES(16,'Чай','Напитки');
INSERT INTO "products" VALUES(17,'Кофе','Напитки');
INSERT INTO "products" VALUES(18,'Рис','Крупы');
INSERT INTO "products" VALUES(19,'Гречка','Крупы');
INSERT INTO "products" VALUES(20,'Мороженое','Замороженные продукты');
CREATE TABLE store_inventory (
    store_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    quantity INTEGER NOT NULL DEFAULT 0 CHECK(quantity >= 0),
    capacity INTEGER NOT NULL CHECK(capacity >= 0),
    min_stock INTEGER NOT NULL DEFAULT 0 CHECK(min_stock >= 0),
    PRIMARY KEY (store_id, product_id),
    FOREIGN KEY (store_id) REFERENCES stores(id) ON DELETE CASCADE,
    FOREIGN KEY (product_id) REFERENCES products(id) ON DELETE CASCADE
);
INSERT INTO "store_inventory" VALUES(1,1,70,120,20);
INSERT INTO "store_inventory" VALUES(1,2,35,80,15);
INSERT INTO "store_inventory" VALUES(1,11,30,60,10);
INSERT INTO "store_inventory" VALUES(2,3,90,150,30);
INSERT INTO "store_inventory" VALUES(2,4,85,130,25);
INSERT INTO "store_inventory" VALUES(2,5,140,200,40);
INSERT INTO "store_inventory" VALUES(3,2,55,100,20);
INSERT INTO "store_inventory" VALUES(3,13,50,90,15);
INSERT INTO "store_inventory" VALUES(3,1,40,75,15);
INSERT INTO "store_inventory" VALUES(4,6,65,110,20);
INSERT INTO "store_inventory" VALUES(4,7,50,95,20);
INSERT INTO "store_inventory" VALUES(4,11,35,70,15);
INSERT INTO "store_inventory" VALUES(5,8,95,140,25);
INSERT INTO "store_inventory" VALUES(5,9,110,160,30);
INSERT INTO "store_inventory" VALUES(5,10,75,120,20);
INSERT INTO "store_inventory" VALUES(6,1,55,100,20);
INSERT INTO "store_inventory" VALUES(6,12,45,90,20);
INSERT INTO "store_inventory" VALUES(6,2,40,85,15);
INSERT INTO "store_inventory" VALUES(7,14,45,70,10);
INSERT INTO "store_inventory" VALUES(7,15,35,65,10);
INSERT INTO "store_inventory" VALUES(7,20,80,120,20);
INSERT INTO "store_inventory" VALUES(8,16,65,100,20);
INSERT INTO "store_inventory" VALUES(8,17,45,75,15);
INSERT INTO "store_inventory" VALUES(8,10,60,90,15);
INSERT INTO "store_inventory" VALUES(9,18,85,130,25);
INSERT INTO "store_inventory" VALUES(9,19,120,180,30);
INSERT INTO "store_inventory" VALUES(9,13,70,100,20);

CREATE TABLE warehouse_inventory (
    warehouse_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    quantity INTEGER NOT NULL DEFAULT 0 CHECK(quantity >= 0),
    capacity INTEGER NOT NULL CHECK(capacity >= 0),
    min_stock INTEGER NOT NULL DEFAULT 0 CHECK(min_stock >= 0),
    PRIMARY KEY (warehouse_id, product_id),
    FOREIGN KEY (warehouse_id) REFERENCES warehouses(id) ON DELETE CASCADE,
    FOREIGN KEY (product_id) REFERENCES products(id) ON DELETE CASCADE
);
INSERT INTO "warehouse_inventory" VALUES(1,1,325,575,100);
INSERT INTO "warehouse_inventory" VALUES(1,2,350,600,100);
INSERT INTO "warehouse_inventory" VALUES(1,3,375,625,100);
INSERT INTO "warehouse_inventory" VALUES(1,4,400,650,100);
INSERT INTO "warehouse_inventory" VALUES(1,5,300,550,100);
INSERT INTO "warehouse_inventory" VALUES(1,6,325,575,100);
INSERT INTO "warehouse_inventory" VALUES(1,7,350,600,100);
INSERT INTO "warehouse_inventory" VALUES(1,8,375,625,100);
INSERT INTO "warehouse_inventory" VALUES(1,9,400,650,100);
INSERT INTO "warehouse_inventory" VALUES(1,10,300,550,100);
INSERT INTO "warehouse_inventory" VALUES(1,11,325,575,100);
INSERT INTO "warehouse_inventory" VALUES(1,12,350,600,100);
INSERT INTO "warehouse_inventory" VALUES(1,13,375,625,100);
INSERT INTO "warehouse_inventory" VALUES(1,14,280,530,60);
INSERT INTO "warehouse_inventory" VALUES(1,15,180,430,60);
INSERT INTO "warehouse_inventory" VALUES(1,16,205,455,60);
INSERT INTO "warehouse_inventory" VALUES(1,17,230,480,60);
INSERT INTO "warehouse_inventory" VALUES(1,18,255,505,60);
INSERT INTO "warehouse_inventory" VALUES(1,19,280,530,60);
INSERT INTO "warehouse_inventory" VALUES(1,20,180,430,60);

CREATE VIEW store_inventory_view AS
SELECT
    s.id AS store_id,
    s.name AS store_name,
    p.id AS product_id,
    p.name AS product_name,
    p.category,
    si.quantity,
    si.capacity,
    si.min_stock,
    (si.capacity - si.quantity) AS free_capacity
FROM store_inventory si
JOIN stores s ON s.id = si.store_id
JOIN products p ON p.id = si.product_id;
CREATE VIEW warehouse_inventory_view AS
SELECT
    w.id AS warehouse_id,
    w.name AS warehouse_name,
    p.id AS product_id,
    p.name AS product_name,
    p.category,
    wi.quantity,
    wi.capacity,
    wi.min_stock,
    (wi.capacity - wi.quantity) AS free_capacity
FROM warehouse_inventory wi
JOIN warehouses w ON w.id = wi.warehouse_id
JOIN products p ON p.id = wi.product_id;
CREATE TABLE orders
(
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    store_id INTEGER NOT NULL,

    created_day INTEGER NOT NULL
        CHECK(created_day > 0),

    delivery_day INTEGER NOT NULL
        CHECK(delivery_day > created_day),

    status TEXT NOT NULL
        DEFAULT 'Created',

    FOREIGN KEY (store_id)
        REFERENCES stores(id)
        ON DELETE CASCADE
);


CREATE TABLE order_items
(
    order_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,

    requested_quantity INTEGER NOT NULL
        CHECK(requested_quantity > 0),

    allocated_quantity INTEGER NOT NULL
        DEFAULT 0
        CHECK(allocated_quantity >= 0),

    PRIMARY KEY
    (
        order_id,
        product_id
    ),

    FOREIGN KEY (order_id)
        REFERENCES orders(id)
        ON DELETE CASCADE,

    FOREIGN KEY (product_id)
        REFERENCES products(id)
        ON DELETE RESTRICT
);
CREATE TABLE supplier_requests
(
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    product_id INTEGER NOT NULL,

    requested_quantity INTEGER NOT NULL
        CHECK(requested_quantity > 0),

    created_day INTEGER NOT NULL
        CHECK(created_day > 0),

    delivery_day INTEGER NOT NULL
        CHECK(delivery_day > created_day),

    status TEXT NOT NULL
        DEFAULT 'Created',

    FOREIGN KEY (product_id)
        REFERENCES products(id)
        ON DELETE RESTRICT
);
ALTER TABLE products
ADD COLUMN unit_price_cents INTEGER NOT NULL DEFAULT 10000
CHECK(unit_price_cents > 0);

ALTER TABLE products
ADD COLUMN shelf_life_days INTEGER NOT NULL DEFAULT 30
CHECK(shelf_life_days > 0);

UPDATE products
SET unit_price_cents = CASE id
    WHEN 1 THEN 9000
    WHEN 2 THEN 5000
    WHEN 3 THEN 12000
    WHEN 4 THEN 11000
    WHEN 5 THEN 6000
    WHEN 6 THEN 35000
    WHEN 7 THEN 25000
    WHEN 8 THEN 10000
    WHEN 9 THEN 8000
    WHEN 10 THEN 12000
    WHEN 11 THEN 30000
    WHEN 12 THEN 7000
    WHEN 13 THEN 6000
    WHEN 14 THEN 28000
    WHEN 15 THEN 20000
    WHEN 16 THEN 15000
    WHEN 17 THEN 25000
    WHEN 18 THEN 9000
    WHEN 19 THEN 10000
    WHEN 20 THEN 8000
    ELSE 10000
END;

UPDATE products
SET shelf_life_days = CASE id
    WHEN 1 THEN 7
    WHEN 2 THEN 3
    WHEN 3 THEN 14
    WHEN 4 THEN 7
    WHEN 5 THEN 30
    WHEN 6 THEN 3
    WHEN 7 THEN 10
    WHEN 8 THEN 180
    WHEN 9 THEN 90
    WHEN 10 THEN 90
    WHEN 11 THEN 30
    WHEN 12 THEN 10
    WHEN 13 THEN 180
    WHEN 14 THEN 120
    WHEN 15 THEN 120
    WHEN 16 THEN 365
    WHEN 17 THEN 365
    WHEN 18 THEN 365
    WHEN 19 THEN 365
    WHEN 20 THEN 90
    ELSE 30
END;
CREATE TABLE warehouse_batches (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    warehouse_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    quantity INTEGER NOT NULL CHECK (quantity >= 0),
    received_day INTEGER NOT NULL CHECK (received_day >= 1),
    expires_day INTEGER NOT NULL,
    unit_price_cents INTEGER NOT NULL CHECK (unit_price_cents > 0),
    discount_percent INTEGER NOT NULL DEFAULT 0
        CHECK (discount_percent BETWEEN 0 AND 90),
    FOREIGN KEY (warehouse_id)
        REFERENCES warehouses(id) ON DELETE CASCADE,
    FOREIGN KEY (product_id)
        REFERENCES products(id) ON DELETE CASCADE,
    CHECK (expires_day > received_day)
);

CREATE INDEX idx_warehouse_batches_expiry
    ON warehouse_batches (
        warehouse_id,
        product_id,
        expires_day,
        id
    );

INSERT INTO warehouse_batches (
    warehouse_id,
    product_id,
    quantity,
    received_day,
    expires_day,
    unit_price_cents
)
SELECT
    wi.warehouse_id,
    wi.product_id,
    wi.quantity,
    1,
    1 + p.shelf_life_days,
    p.unit_price_cents
FROM warehouse_inventory AS wi
JOIN products AS p ON p.id = wi.product_id
WHERE wi.quantity > 0;
CREATE TABLE warehouse_writeoffs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    batch_id INTEGER NOT NULL,
    warehouse_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    writeoff_day INTEGER NOT NULL CHECK (writeoff_day >= 1),
    quantity INTEGER NOT NULL CHECK (quantity > 0),
    loss_cents INTEGER NOT NULL CHECK (loss_cents > 0),

    FOREIGN KEY (batch_id)
        REFERENCES warehouse_batches(id) ON DELETE CASCADE,
    FOREIGN KEY (warehouse_id)
        REFERENCES warehouses(id) ON DELETE CASCADE,
    FOREIGN KEY (product_id)
        REFERENCES products(id) ON DELETE CASCADE
);
CREATE TABLE warehouse_allocations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    batch_id INTEGER NOT NULL,
    allocation_day INTEGER NOT NULL CHECK (allocation_day >= 1),
    quantity INTEGER NOT NULL CHECK (quantity > 0),
    base_unit_price_cents INTEGER NOT NULL
        CHECK (base_unit_price_cents > 0),
    actual_unit_price_cents INTEGER NOT NULL
        CHECK (actual_unit_price_cents > 0),
    discount_percent INTEGER NOT NULL
        CHECK (discount_percent BETWEEN 0 AND 90),

    FOREIGN KEY (batch_id)
        REFERENCES warehouse_batches(id) ON DELETE CASCADE
);
COMMIT;
