#pragma once

#include <string>
#include "domain/Position.h"

class StorageCell {
private:
    int id_{};
    std::string code_;
    Position position_{};
    int capacity_{};

public:
    StorageCell() = default;
    StorageCell(int id, std::string code, Position position, int capacity);

    int getId() const;
    const std::string& getCode() const;
    const Position& getPosition() const;
    int getCapacity() const;

    void setPosition(const Position& position);
};
