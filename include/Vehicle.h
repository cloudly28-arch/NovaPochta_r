#pragma once

#include <string>
#include "Position.h"

class Vehicle {
private:
    int id_{};
    std::string model_;
    double maxLoad_{};
    bool available_{true};
    Position position_;

public:
    Vehicle() = default;
    Vehicle(int id, const std::string& model, double maxLoad);

    int getId() const;
    const std::string& getModel() const;
    double getMaxLoad() const;
    bool isAvailable() const;
    const Position& getPosition() const;

    void setAvailable(bool available);
    void setPosition(const Position& position);
};
