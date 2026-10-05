#pragma once

#include "Warehouse.h"

class SimulationService {
private:
    Warehouse& warehouse_;
    int currentDay_{1};
    int totalDays_{20};
    bool running_{false};

public:
    explicit SimulationService(Warehouse& warehouse, int totalDays = 20);

    int getCurrentDay() const;
    int getTotalDays() const;
    bool isRunning() const;

    void start();
    void pause();
    void reset();
    bool nextDay();
};
