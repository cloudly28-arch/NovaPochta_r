#pragma once

#include <SFML/Graphics.hpp>

#include "application/BackendFacade.h"
#include "ui/SimulationScreen.h"
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

private:
    sf::RenderWindow window_;
    sf::Clock deltaClock_;

    Screen currentScreen_ =
        Screen::Start;

    SimulationSettings settings_;
    StartScreen startScreen_;

    BackendFacade backend_;
    SimulationScreen simulationScreen_;
};
