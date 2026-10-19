#pragma once

struct ExperimentSettings {
  int initialFillPercent = -1;  // -1: calculate from demand, otherwise 0..100.
  int demandPercent = 100;
  int stockCoverDays = 0;  // 0: choose by shelf life, otherwise 1..14.
  int purchaseCostPercent = 70;

  bool isValid() const {
    return initialFillPercent >= -1 && initialFillPercent <= 100 &&
           demandPercent >= 50 && demandPercent <= 150 && stockCoverDays >= 0 &&
           stockCoverDays <= 14 && purchaseCostPercent >= 1 &&
           purchaseCostPercent <= 100;
  }
};
