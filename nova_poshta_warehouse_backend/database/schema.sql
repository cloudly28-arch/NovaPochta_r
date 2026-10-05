BEGIN TRANSACTION;
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
INSERT INTO "products" VALUES(14,'Средство для мытья посуды','Бытовая химия');
INSERT INTO "products" VALUES(15,'Стиральный порошок','Бытовая химия');
INSERT INTO "products" VALUES(16,'Батарейки','Электроника');
INSERT INTO "products" VALUES(17,'USB-кабель','Электроника');
INSERT INTO "products" VALUES(18,'Бумага A4','Канцелярия');
INSERT INTO "products" VALUES(19,'Ручки','Канцелярия');
INSERT INTO "products" VALUES(20,'Картонные коробки','Упаковка');
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
INSERT INTO "store_inventory" VALUES(8,20,60,90,15);
INSERT INTO "store_inventory" VALUES(9,18,85,130,25);
INSERT INTO "store_inventory" VALUES(9,19,120,180,30);
INSERT INTO "store_inventory" VALUES(9,20,70,100,20);
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
INSERT INTO "stores" VALUES(7,'CleanHome');
INSERT INTO "stores" VALUES(8,'TechBox');
INSERT INTO "stores" VALUES(9,'PaperLine');
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
CREATE TABLE warehouses (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    address TEXT
);
INSERT INTO "warehouses" VALUES(1,'Центральный склад Nova Poshta','Основной учебный склад');
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
COMMIT;
