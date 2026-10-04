#pragma once

#include <string>

enum class WorkerRole {
    Manager,
    Storekeeper,
    Loader,
    Picker,
    Driver,
    Operator
};

enum class WorkerStatus {
    Free,
    Busy,
    OnBreak,
    Offline
};

enum class OrderStatus {
    Created,
    Accepted,
    InProgress,
    ReadyForShipment,
    Shipped,
    Completed,
    Cancelled
};

enum class TaskType {
    ReceiveGoods,
    PutAway,
    PickGoods,
    MoveGoods,
    PackOrder,
    LoadVehicle
};

enum class TaskStatus {
    Created,
    Assigned,
    InProgress,
    Completed,
    Cancelled
};

enum class ZoneType {
    Receiving,
    Storage,
    Picking,
    Packing,
    Shipping
};
