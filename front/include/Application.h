#pragma once

#include <SFML/Graphics.hpp>

#include "ui/StartScreen.h"

class Application
{
public:
    Application();
    ~Application();

    void run();

private:
    enum class Screen
    {
        Start,
        Simulation
    };

private:
    void processEvents();
    void update();
    void render();

    void drawSimulationPlaceholder();

private:
    sf::RenderWindow window_;
    sf::Clock deltaClock_;

    Screen currentScreen_ = Screen::Start;

    SimulationSettings settings_;
    StartScreen startScreen_;
};