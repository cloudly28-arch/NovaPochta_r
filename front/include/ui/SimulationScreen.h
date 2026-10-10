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

    void setDatabasePaths(
        const std::filesystem::path& databasePath,
        const std::filesystem::path& schemaPath
    );
private:
    enum class SelectedObjectType
    {
        None,
        Warehouse,
        Store,
        Supplier
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
    enum class VehicleType
    {
        StoreDelivery,
        SupplierDelivery
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

        VehicleType type =
            VehicleType::StoreDelivery;

        bool active = false;

        float startDelay = 0.0f;
    };

    struct SupplierTruckView
    {
        sf::Vector2f position;

        float progress = 0.0f;

        bool active = false;

        bool returning = false;
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
    void drawSupplier(
        sf::RenderWindow& window
    );

    void drawSupplierRoute(
        sf::RenderWindow& window
    );

    void drawSupplierTruck(
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
    void refreshWarehouseData();
    void refreshSupplierData();
    void updateVehicles(
        float deltaTime
    );
    void updateSupplierTruck(
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
    const sf::Vector2f supplierCenter_{
        120.0f,
        150.0f
    };

    sf::FloatRect supplierBounds_;

    SelectedObjectType selectedType_ =
        SelectedObjectType::None;

    int selectedStoreId_ = -1;

    bool mouseWasPressed_ = false;

    SimulationState simulationState_ =
        SimulationState::Stopped;

    std::vector<VehicleView> vehicles_;
    SupplierTruckView supplierTruck_;
    int activeSupplierRequestId_ = -1;

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
    std::vector<SupplierRequestInfo>
        supplierRequests_;
    std::vector<WarehouseStockInfo>
        warehouseInventory_;
    StoreOrderInfo selectedStoreOrder_;

    bool selectedStoreHasOrder_ =
        false;

    std::filesystem::path
        databasePath_;

    std::filesystem::path
        schemaPath_;
    };
