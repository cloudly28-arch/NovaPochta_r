#include "domain/Vehicle.h"
#include <stdexcept>
#include <utility>

Vehicle::Vehicle(int id, std::string model, double maxLoad)
    : id_(id), model_(std::move(model)), maxLoad_(maxLoad) {
    if (id_ <= 0) throw std::invalid_argument("Vehicle id must be positive");
    if (maxLoad_ < 0) throw std::invalid_argument("Vehicle max load cannot be negative");
}
int Vehicle::getId() const { return id_; }
const std::string& Vehicle::getModel() const { return model_; }
double Vehicle::getMaxLoad() const { return maxLoad_; }
bool Vehicle::isAvailable() const { return available_; }
const Position& Vehicle::getPosition() const { return position_; }
void Vehicle::setAvailable(bool available) { available_ = available; }
void Vehicle::setPosition(const Position& position) { position_ = position; }
