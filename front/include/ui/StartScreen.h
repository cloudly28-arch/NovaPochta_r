#pragma once

struct SimulationSettings {
  int days = 20;
  int stores = 5;
  int products = 16;
  bool automaticSupplierApproval = false;
};

class StartScreen {
 public:
  bool draw(SimulationSettings& settings);
};
