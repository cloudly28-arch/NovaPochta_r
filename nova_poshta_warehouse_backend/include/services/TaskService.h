#pragma once

#include "Warehouse.h"

class TaskService {
private:
    Warehouse& warehouse_;
    int nextTaskId_{1};

public:
    explicit TaskService(Warehouse& warehouse);

    void createPickingTasksForOrder(int orderId);
    bool assignWorker(int taskId, int workerId);
    bool completeTask(int taskId);
};
