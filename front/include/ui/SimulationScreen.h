#pragma once

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <string>
#include <vector>

#include "application/BackendFacade.h"
#include "ui/StartScreen.h"

class SimulationScreen
{
public:
    void update(float deltaTime);

    void draw(
        sf::RenderWindow& window,
        const SimulationSettings& settings
    );

    void setBackend(
        BackendFacade* backend
    );

    void setAssetsPath(
        const std::filesystem::path& path
    );

    void configure(
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
        Paused,
        Finished
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

        bool active = false;

        float startDelay = 0.0f;
    };

private:
    void loadTextures();
    void ensureStoresCreated(
        int storeCount
    );

    void drawTopBar(
        const SimulationSettings& settings
    );

    void drawBottomBar();

    void drawRoutes(
        sf::RenderWindow& window
    );

    void drawWarehouse(
        sf::RenderWindow& window
    );

    void drawStores(
        sf::RenderWindow& window
    );

    void drawVehicles(
        sf::RenderWindow& window
    );

    void handleMouseClick(
        sf::RenderWindow& window
    );

    void drawInfoPanel();

    void refreshSelectedStoreData();

    void updateVehicles(
        float deltaTime
    );

    sf::Vector2f getStorePosition(
        int storeId
    ) const;

    void advanceDay();

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

    std::vector<VehicleView> vehicles_;

    float simulationSpeed_ = 1.0f;
    int currentDay_ = 1;
    int totalDays_ = 20;

    float dayTimer_ = 0.0f;

    float secondsPerDay_ = 8.0f;

    sf::Texture warehouseTexture_;
    sf::Texture storeTexture_;
    sf::Texture truckTexture_;

    bool warehouseTextureLoaded_ = false;
    bool storeTextureLoaded_ = false;
    bool truckTextureLoaded_ = false;

    bool texturesLoaded_ = false;

    std::filesystem::path assetsPath_;

    BackendFacade* backend_ = nullptr;

    std::string selectedStoreName_;

    std::vector<ProductStockInfo>
        selectedStoreInventory_;
};
