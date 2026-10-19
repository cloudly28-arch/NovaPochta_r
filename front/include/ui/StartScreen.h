#pragma once

#include "domain/ExperimentSettings.h"

struct SimulationSettings {
  int days = 20;
  int stores = 9;
  int products = 20;
  ExperimentSettings experiment;
  bool automaticSupplierApproval = false;
};

class StartScreen {
 public:
  bool draw(SimulationSettings& settings);
};
