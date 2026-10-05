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
    Reserved,
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
    LoadVehicle,
    WriteOff
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

inline std::string toString(OrderStatus status) {
    switch (status) {
        case OrderStatus::Created: return "Created";
        case OrderStatus::Reserved: return "Reserved";
        case OrderStatus::InProgress: return "InProgress";
        case OrderStatus::ReadyForShipment: return "ReadyForShipment";
        case OrderStatus::Shipped: return "Shipped";
        case OrderStatus::Completed: return "Completed";
        case OrderStatus::Cancelled: return "Cancelled";
    }
    return "Unknown";
}
