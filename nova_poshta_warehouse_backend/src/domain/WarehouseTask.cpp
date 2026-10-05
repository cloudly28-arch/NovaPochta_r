#include "domain/WarehouseTask.h"
#include <stdexcept>

WarehouseTask::WarehouseTask(int id, TaskType type, int productId, int quantity, Position sourcePosition, Position targetPosition)
    : id_(id), type_(type), productId_(productId), quantity_(quantity), sourcePosition_(sourcePosition), targetPosition_(targetPosition) {
    if (id_ <= 0 || productId_ <= 0 || quantity_ <= 0) throw std::invalid_argument("WarehouseTask values must be positive");
}
int WarehouseTask::getId() const { return id_; }
TaskType WarehouseTask::getType() const { return type_; }
TaskStatus WarehouseTask::getStatus() const { return status_; }
int WarehouseTask::getWorkerId() const { return workerId_; }
int WarehouseTask::getOrderId() const { return orderId_; }
int WarehouseTask::getProductId() const { return productId_; }
int WarehouseTask::getQuantity() const { return quantity_; }
const Position& WarehouseTask::getSourcePosition() const { return sourcePosition_; }
const Position& WarehouseTask::getTargetPosition() const { return targetPosition_; }
void WarehouseTask::setStatus(TaskStatus status) { status_ = status; }
void WarehouseTask::assignWorker(int workerId) { workerId_ = workerId; status_ = TaskStatus::Assigned; }
void WarehouseTask::setOrderId(int orderId) { orderId_ = orderId; }
