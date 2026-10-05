#include "domain/WarehouseZone.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

WarehouseZone::WarehouseZone(int id, std::string name, ZoneType type, Position position, double width, double height)
    : id_(id), name_(std::move(name)), type_(type), position_(position), width_(width), height_(height) {
    if (id_ <= 0) throw std::invalid_argument("WarehouseZone id must be positive");
    if (name_.empty()) throw std::invalid_argument("WarehouseZone name cannot be empty");
    if (width_ < 0 || height_ < 0) throw std::invalid_argument("Zone size cannot be negative");
}
int WarehouseZone::getId() const { return id_; }
const std::string& WarehouseZone::getName() const { return name_; }
ZoneType WarehouseZone::getType() const { return type_; }
const Position& WarehouseZone::getPosition() const { return position_; }
double WarehouseZone::getWidth() const { return width_; }
double WarehouseZone::getHeight() const { return height_; }
const std::vector<int>& WarehouseZone::getStorageCellIds() const { return storageCellIds_; }
void WarehouseZone::addStorageCell(int storageCellId) { if (std::find(storageCellIds_.begin(), storageCellIds_.end(), storageCellId) == storageCellIds_.end()) storageCellIds_.push_back(storageCellId); }
void WarehouseZone::removeStorageCell(int storageCellId) { storageCellIds_.erase(std::remove(storageCellIds_.begin(), storageCellIds_.end(), storageCellId), storageCellIds_.end()); }
