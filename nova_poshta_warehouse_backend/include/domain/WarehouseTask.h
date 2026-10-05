#pragma once

#include "domain/Position.h"
#include "domain/Types.h"

class WarehouseTask {
private:
    int id_{};
    TaskType type_{TaskType::MoveGoods};
    TaskStatus status_{TaskStatus::Created};
    int workerId_{-1};
    int orderId_{-1};
    int productId_{-1};
    int quantity_{};
    Position sourcePosition_{};
    Position targetPosition_{};

public:
    WarehouseTask() = default;
    WarehouseTask(int id,
                  TaskType type,
                  int productId,
                  int quantity,
                  Position sourcePosition = {},
                  Position targetPosition = {});

    int getId() const;
    TaskType getType() const;
    TaskStatus getStatus() const;
    int getWorkerId() const;
    int getOrderId() const;
    int getProductId() const;
    int getQuantity() const;
    const Position& getSourcePosition() const;
    const Position& getTargetPosition() const;

    void setStatus(TaskStatus status);
    void assignWorker(int workerId);
    void setOrderId(int orderId);
};
