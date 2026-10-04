#pragma once

#include <string>
#include "Types.h"
#include "Position.h"

class Worker {
private:
    int id_{};
    std::string fullName_;
    WorkerRole role_{WorkerRole::Loader};
    WorkerStatus status_{WorkerStatus::Free};
    Position position_;
    int currentTaskId_{-1};

public:
    Worker() = default;
    Worker(int id, const std::string& fullName, WorkerRole role);

    int getId() const;
    const std::string& getFullName() const;
    WorkerRole getRole() const;
    WorkerStatus getStatus() const;
    const Position& getPosition() const;
    int getCurrentTaskId() const;

    void setFullName(const std::string& fullName);
    void setRole(WorkerRole role);
    void setStatus(WorkerStatus status);
    void setPosition(const Position& position);
    void assignTask(int taskId);
    void clearTask();
};
