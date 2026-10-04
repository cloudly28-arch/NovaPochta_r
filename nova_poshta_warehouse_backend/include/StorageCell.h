#pragma once

#include <string>
#include "Position.h"

class StorageCell {
private:
    int id_{};
    std::string code_;
    Position position_;
    int capacity_{};
    int productId_{-1};
    int quantity_{};

public:
    StorageCell() = default;
    StorageCell(int id,
                const std::string& code,
                const Position& position,
                int capacity);

    int getId() const;
    const std::string& getCode() const;
    const Position& getPosition() const;
    int getCapacity() const;
    int getProductId() const;
    int getQuantity() const;

    bool isEmpty() const;
    int getFreeCapacity() const;

    void setPosition(const Position& position);
    bool putProduct(int productId, int quantity);
    bool takeProduct(int quantity);
    void clear();
};
