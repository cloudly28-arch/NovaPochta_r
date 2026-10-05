#include "services/TaskService.h"

TaskService::TaskService(Warehouse& warehouse) : warehouse_(warehouse) {
    for (const auto& task : warehouse_.getTasks()) if (task.getId() >= nextTaskId_) nextTaskId_ = task.getId() + 1;
}
void TaskService::createPickingTasksForOrder(int orderId) {
    auto* order = warehouse_.findOrderById(orderId); if (!order) return;
    for (const auto& item : order->getItems()) {
        WarehouseTask task(nextTaskId_++, TaskType::PickGoods, item.getProductId(), item.getQuantity());
        task.setOrderId(orderId); warehouse_.addTask(task);
    }
    if (order->getStatus() == OrderStatus::Reserved) order->setStatus(OrderStatus::InProgress);
}
bool TaskService::assignWorker(int taskId, int workerId) {
    auto* task = warehouse_.findTaskById(taskId); auto* worker = warehouse_.findWorkerById(workerId);
    if (!task || !worker || worker->getStatus() != WorkerStatus::Free) return false;
    task->assignWorker(workerId); worker->assignTask(taskId); return true;
}
bool TaskService::completeTask(int taskId) {
    auto* task = warehouse_.findTaskById(taskId); if (!task) return false;
    task->setStatus(TaskStatus::Completed);
    if (task->getWorkerId() >= 0) if (auto* worker = warehouse_.findWorkerById(task->getWorkerId())) worker->clearTask();
    return true;
}
