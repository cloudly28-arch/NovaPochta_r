#pragma once

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <string>
#include <vector>

#include "application/BackendFacade.h"
#include "ui/StartScreen.h"

class SimulationScreen {
 public:
  void update(float deltaTime);
  bool consumeSettingsRequest();

  void draw(sf::RenderWindow& window, const SimulationSettings& settings);

  void setBackend(BackendFacade* backend);

  void setAssetsPath(const std::filesystem::path& path);

  void configure(const SimulationSettings& settings);

  void setDatabasePaths(const std::filesystem::path& databasePath,
                        const std::filesystem::path& schemaPath);

 private:
  enum class SelectedObjectType { None, Warehouse, Store, Supplier };

  enum class SimulationState { Stopped, Running, Paused, Finished };

  enum class VehicleDirection { ToStore, ToWarehouse };
  enum class VehicleType { StoreDelivery, SupplierDelivery };

  struct StoreView {
    int id = 0;
    sf::Vector2f position;
    sf::FloatRect bounds;
  };

  struct VehicleView {
    sf::Vector2f position;

    int targetStoreId = 1;

    float progress = 0.0f;

    VehicleDirection direction = VehicleDirection::ToStore;

    VehicleType type = VehicleType::StoreDelivery;

    bool active = false;

    float startDelay = 0.0f;
    int scheduleDay = -1;
    int nextDeparture = 0;
    float departureHours[2] = {8.0f, 16.0f};
    float speedMultiplier = 1.0f;
    bool unloading = false;
    float unloadingHours = 0.0f;
  };

  struct SupplierTruckView {
    sf::Vector2f position;

    float progress = 0.0f;

    bool active = false;

    bool returning = false;
  };

 private:
  void loadTextures();
  void ensureStoresCreated(int storeCount);

  void drawTopBar(const SimulationSettings& settings);

  void drawBottomBar();

  void drawRoutes(sf::RenderWindow& window);

  void drawWarehouse(sf::RenderWindow& window);
  void drawSupplier(sf::RenderWindow& window);

  void drawSupplierRoute(sf::RenderWindow& window);

  void drawSupplierTruck(sf::RenderWindow& window);

  void drawStores(sf::RenderWindow& window);

  void drawVehicles(sf::RenderWindow& window);

  void handleMouseClick(sf::RenderWindow& window);

  void drawInfoPanel();
  void drawSupplySummary();
  long long currentSupplierCargoUnits(int productId) const;
  std::vector<WarehouseSupplySummary> supplySummary_;
  bool supplySummaryLoaded_ = false;

  void refreshSelectedStoreData();
  void refreshWarehouseData();
  void refreshSupplierData();
  void updateVehicles(float deltaTime);
  void updateSupplierTruck(float deltaTime);

  sf::Vector2f getStorePosition(int storeId) const;

  void advanceDay();
  bool processThreeHourSales(int intervalsDue);
  int salesIntervalsProcessed_ = 0;
  void drawAmbientClouds(sf::RenderWindow& window);
  void drawMapBackground(sf::RenderWindow& window);
  sf::VertexArray roadMesh_{sf::PrimitiveType::Triangles};
  sf::VertexArray yellowMarkingMesh_{sf::PrimitiveType::Triangles};
  int cachedRoadStoreCount_ = -1;
  sf::VertexArray backgroundMesh_{sf::PrimitiveType::Triangles};
  int cachedBackgroundStoreCount_ = -1;
  sf::Vector2f cachedBackgroundSize_;
  sf::Vector2f cachedBackgroundOrigin_;
  void drawRoadSegment(sf::RenderWindow& window, sf::Vector2f start,
                       sf::Vector2f end);
  struct RoadSample {
    sf::Vector2f position;
    sf::Vector2f direction;
  };
  RoadSample sampleStoreRoad(int storeId, float progress, bool returning) const;
  void drawAmbientPlanes(sf::RenderWindow& window);
  double ambientTime_ = 0.0;
  enum class FinaleState { None, Approach, Explosion, Done };
  FinaleState finaleState_ = FinaleState::None;
  bool finaleEnabled_ = false;
  float finaleTimer_ = 0.0f;
  sf::Vector2f lastPlanePosition_{100.0f, 150.0f};
  sf::Vector2f finaleStart_;
  sf::Vector2f finaleTarget_;
  sf::Vector2f lastSecondPlanePosition_{250.0f, 250.0f};
  sf::Vector2f finaleSecondStart_;
  sf::Vector2f finaleSecondTarget_;
  void drawDecorativeTowers(sf::RenderWindow& window);
  void beginFinalAnimation();
  void updateFinalAnimation(float deltaTime);
  void drawFinalExplosion(sf::RenderWindow& window);
  void approveSupplierRequestsAutomatically();
  bool automaticSupplierApproval_ = false;
  bool settingsRequested_ = false;
  void drawFinalReport();
  void recordDailyFinance();
  void drawDailyFinanceChart();
  struct DailyFinance {
    long long revenueCents = 0;
    long long writeoffCents = 0;
    bool recorded = false;
  };
  std::vector<DailyFinance> dailyFinance_;
  void loadFinalReport();
  ExperimentReport finalReport_;
  bool finalReportVisible_ = false;
  bool finalReportLoaded_ = false;

 private:
  std::vector<StoreView> stores_;

  sf::FloatRect warehouseBounds_;

  const sf::Vector2f warehouseCenter_{640.0f, 540.0f};
  const sf::Vector2f supplierCenter_{140.0f, 540.0f};

  sf::FloatRect supplierBounds_;

  SelectedObjectType selectedType_ = SelectedObjectType::None;

  int selectedStoreId_ = -1;

  bool mouseWasPressed_ = false;

  SimulationState simulationState_ = SimulationState::Stopped;

  std::vector<VehicleView> vehicles_;
  SupplierTruckView supplierTruck_;
  struct SupplierFlight {
    SupplierTruckView truck;
    std::vector<int> requestIds;
    std::vector<int> productIds;
  };
  std::vector<SupplierFlight> supplierFlights_;
  double nextSupplierDepartureHour_ = 0.0;
  void syncSupplierFleet();
  void drawSupplierTruckSprite(sf::RenderWindow& window);
  int activeSupplierRequestId_ = -1;
  std::vector<int> activeSupplierRequestIds_;
  std::vector<int> activeSupplierProductIds_;

  float simulationSpeed_ = 1.0f;
  int currentDay_ = 1;
  int totalDays_ = 20;

  float dayTimer_ = 0.0f;
  float dataRefreshTimer_ = 0.25f;

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

  std::vector<ProductStockInfo> selectedStoreInventory_;
  std::vector<SupplierRequestInfo> supplierRequests_;
  std::vector<WarehouseStockInfo> warehouseInventory_;
  StoreOrderInfo selectedStoreOrder_;

  bool selectedStoreHasOrder_ = false;

  std::filesystem::path databasePath_;

  std::filesystem::path schemaPath_;

  std::vector<int> animatedSupplierRequestIds_;

  sf::Texture supplierTexture_;
  sf::Texture supplierTruckTexture_;

  bool supplierTextureLoaded_ = false;
  bool supplierTruckTextureLoaded_ = false;

  std::vector<WarehouseBatchRow> warehouseBatches_;
  bool warehouseBatchesLoaded_ = false;

  void drawWarehouseBatches();

  std::vector<WarehouseWriteoffRow> warehouseWriteoffs_;
  bool warehouseWriteoffsLoaded_ = false;

  void drawWarehouseWriteoffs();
  int batchDiscountPercent_ = 20;
  bool batchDiscountFailed_ = false;

  WarehouseAllocationStats warehouseAllocationStats_;
  bool warehouseAllocationStatsLoaded_ = false;

  void drawWarehouseAllocationStats();

  bool firstDayProcessed_ = false;

  bool processCurrentDay();

  bool supplierApprovalFailed_ = false;

  void drawSupplierApprovals();
};
