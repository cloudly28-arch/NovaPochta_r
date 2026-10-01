#pragma once

#include <string>
#include <vector>
#include "Types.h"
#include "Position.h"

class WarehouseZone {
private:
    int id_{};
    std::string name_;
    ZoneType type_{ZoneType::Storage};
    Position position_;
    double width_{};
    double height_{};
    std::vector<int> storageCellIds_;

public:
    WarehouseZone() = default;
    WarehouseZone(int id,
                  const std::string& name,
                  ZoneType type,
                  const Position& position,
                  double width,
                  double height);

    int getId() const;
    const std::string& getName() const;
    ZoneType getType() const;
    const Position& getPosition() const;
    double getWidth() const;
    double getHeight() const;
    const std::vector<int>& getStorageCellIds() const;

    void addStorageCell(int storageCellId);
    void removeStorageCell(int storageCellId);
};
