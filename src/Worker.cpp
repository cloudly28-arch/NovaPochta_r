#include "Worker.h"

#include <sstream>
#include <stdexcept>
#include <utility>

Worker::Worker(int id, std::string fullName, WorkerRole role)
    : id_(id), fullName_(std::move(fullName)), role_(role), active_(true) {
    if (id_ <= 0) {
        throw std::invalid_argument("Worker id must be positive");
    }
    if (fullName_.empty()) {
        throw std::invalid_argument("Worker name cannot be empty");
    }
}

int Worker::getId() const { return id_; }
const std::string& Worker::getFullName() const { return fullName_; }
WorkerRole Worker::getRole() const { return role_; }
bool Worker::isActive() const { return active_; }

void Worker::setFullName(const std::string& fullName) {
    if (fullName.empty()) {
        throw std::invalid_argument("Worker name cannot be empty");
    }
    fullName_ = fullName;
}

void Worker::setRole(WorkerRole role) { role_ = role; }
void Worker::setActive(bool active) { active_ = active; }

std::string Worker::roleToString() const {
    switch (role_) {
        case WorkerRole::Manager: return "Manager";
        case WorkerRole::Storekeeper: return "Storekeeper";
        case WorkerRole::Loader: return "Loader";
        case WorkerRole::Driver: return "Driver";
        case WorkerRole::Operator: return "Operator";
    }
    return "Unknown";
}

std::string Worker::toString() const {
    std::ostringstream out;
    out << "Worker{id=" << id_
        << ", name='" << fullName_ << "'"
        << ", role=" << roleToString()
        << ", active=" << (active_ ? "true" : "false")
        << "}";
    return out.str();
}
