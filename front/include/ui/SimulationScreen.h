#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

#include "ui/StartScreen.h"

class SimulationScreen
{
public:
    void update(float deltaTime);
    
    void draw(
        sf::RenderWindow& window,
        const SimulationSettings& settings
    );

private:
    enum class SelectedObjectType
    {
        None,
        Warehouse,
        Store
    };

    enum class SimulationState
    {
        Stopped,
        Running,
        Paused
    };

    enum class VehicleDirection
    {
        ToStore,
        ToWarehouse
    };

    struct StoreView
    {
        int id = 0;
        sf::Vector2f position;
        sf::FloatRect bounds;
    };

    struct VehicleView
    {
        sf::Vector2f position;

        int targetStoreId = 1;

        float progress = 0.0f;

        VehicleDirection direction =
            VehicleDirection::ToStore;
    };

private:
    void ensureStoresCreated(int storeCount);

    void drawTopBar(
        const SimulationSettings& settings
    );

    void drawBottomBar();

    void drawRoutes(sf::RenderWindow& window);

    void drawWarehouse(sf::RenderWindow& window);

    void drawStores(sf::RenderWindow& window);

    void drawVehicle(sf::RenderWindow& window);

    void handleMouseClick(sf::RenderWindow& window);

    void drawInfoPanel();

    void updateVehicle(float deltaTime);

    sf::Vector2f getStorePosition(int storeId) const;

private:
    std::vector<StoreView> stores_;

    sf::FloatRect warehouseBounds_;

    const sf::Vector2f warehouseCenter_{
        500.0f,
        360.0f
    };

    SelectedObjectType selectedType_ =
        SelectedObjectType::None;

    int selectedStoreId_ = -1;

    bool mouseWasPressed_ = false;

    SimulationState simulationState_ =
        SimulationState::Stopped;

    VehicleView vehicle_;

    float simulationSpeed_ = 1.0f;
};