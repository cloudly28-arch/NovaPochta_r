#include "domain/Worker.h"
#include <stdexcept>
#include <utility>

Worker::Worker(int id, std::string fullName, WorkerRole role)
    : id_(id), fullName_(std::move(fullName)), role_(role) {
    if (id_ <= 0) throw std::invalid_argument("Worker id must be positive");
    if (fullName_.empty()) throw std::invalid_argument("Worker name cannot be empty");
}
int Worker::getId() const { return id_; }
const std::string& Worker::getFullName() const { return fullName_; }
WorkerRole Worker::getRole() const { return role_; }
WorkerStatus Worker::getStatus() const { return status_; }
const Position& Worker::getPosition() const { return position_; }
int Worker::getCurrentTaskId() const { return currentTaskId_; }
void Worker::setFullName(const std::string& fullName) { if (fullName.empty()) throw std::invalid_argument("Worker name cannot be empty"); fullName_ = fullName; }
void Worker::setRole(WorkerRole role) { role_ = role; }
void Worker::setStatus(WorkerStatus status) { status_ = status; }
void Worker::setPosition(const Position& position) { position_ = position; }
void Worker::assignTask(int taskId) { currentTaskId_ = taskId; status_ = WorkerStatus::Busy; }
void Worker::clearTask() { currentTaskId_ = -1; status_ = WorkerStatus::Free; }
