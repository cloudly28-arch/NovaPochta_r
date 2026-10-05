#include "domain/StorageCell.h"
#include <stdexcept>
#include <utility>

StorageCell::StorageCell(int id, std::string code, Position position, int capacity)
    : id_(id), code_(std::move(code)), position_(position), capacity_(capacity) {
    if (id_ <= 0) throw std::invalid_argument("StorageCell id must be positive");
    if (capacity_ < 0) throw std::invalid_argument("StorageCell capacity cannot be negative");
}
int StorageCell::getId() const { return id_; }
const std::string& StorageCell::getCode() const { return code_; }
const Position& StorageCell::getPosition() const { return position_; }
int StorageCell::getCapacity() const { return capacity_; }
void StorageCell::setPosition(const Position& position) { position_ = position; }
