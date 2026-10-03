#pragma once

#include <SFML/Graphics.hpp>

#include "ui/StartScreen.h"

class SimulationScreen
{
public:
    void draw(
        sf::RenderWindow& window,
        const SimulationSettings& settings
    );

private:
    void drawTopBar(const SimulationSettings& settings);
    void drawBottomBar();

    void drawWarehouse(sf::RenderWindow& window);
    void drawStores(
        sf::RenderWindow& window,
        int storeCount
    );
};