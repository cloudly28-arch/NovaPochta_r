#pragma once

struct SimulationSettings
{
    int days = 20;
    int stores = 5;
    int products = 16;
};

class StartScreen
{
public:
    bool draw(SimulationSettings& settings);
};