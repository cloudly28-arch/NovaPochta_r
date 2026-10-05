#include "services/SimulationService.h"
#include <algorithm>

SimulationService::SimulationService(Warehouse& warehouse, int totalDays)
    : warehouse_(warehouse), totalDays_(std::max(1, totalDays)) {}
int SimulationService::getCurrentDay() const { return currentDay_; }
int SimulationService::getTotalDays() const { return totalDays_; }
bool SimulationService::isRunning() const { return running_; }
void SimulationService::start() { if (currentDay_ <= totalDays_) running_ = true; }
void SimulationService::pause() { running_ = false; }
void SimulationService::reset() { currentDay_ = 1; running_ = false; }
bool SimulationService::nextDay() { if (!running_ || currentDay_ >= totalDays_) { running_ = false; return false; } ++currentDay_; if (currentDay_ >= totalDays_) running_ = false; return true; }
