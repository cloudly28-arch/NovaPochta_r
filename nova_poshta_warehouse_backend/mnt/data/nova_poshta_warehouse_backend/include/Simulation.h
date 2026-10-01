#pragma once

#include "Warehouse.h"

class Simulation {
private:
    Warehouse* warehouse_{nullptr};
    double simulationTime_{};
    double speedMultiplier_{1.0};
    bool running_{false};

public:
    Simulation() = default;
    explicit Simulation(Warehouse* warehouse);

    Warehouse* getWarehouse() const;
    double getSimulationTime() const;
    double getSpeedMultiplier() const;
    bool isRunning() const;

    void setWarehouse(Warehouse* warehouse);
    void setSpeedMultiplier(double multiplier);

    void start();
    void pause();
    void reset();
    void update(double deltaTime);
};
