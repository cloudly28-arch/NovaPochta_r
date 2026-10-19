#include "ui/SimulationScreen.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <initializer_list>
#include <random>

void SimulationScreen::update(float deltaTime) {
  // Decorative motion uses real time, independently of simulation speed.
  if (std::isfinite(deltaTime) && deltaTime > 0.0f) {
    ambientTime_ += std::min(deltaTime, 0.1f);
    updateFinalAnimation(std::min(deltaTime, 0.1f));
  }
  dataRefreshTimer_ += deltaTime;

  if (dataRefreshTimer_ >= 0.25f) {
    dataRefreshTimer_ = 0.0f;

    refreshWarehouseData();
    refreshSupplierData();

    if (selectedType_ == SelectedObjectType::Store) {
      refreshSelectedStoreData();
    }
  }
  if (simulationState_ != SimulationState::Running) {
    return;
  }
  if (!firstDayProcessed_) {
    if (!processCurrentDay()) {
      return;
    }

    firstDayProcessed_ = true;
  }

  const float dayLength = std::max(secondsPerDay_, 0.001f);
  float remaining = std::max(0.0f, deltaTime * simulationSpeed_);
  while (remaining > 0.000001f &&
         simulationState_ == SimulationState::Running) {
    const float slice = std::min(remaining, dayLength - dayTimer_);
    updateVehicles(slice / std::max(simulationSpeed_, 0.001f));
    updateSupplierTruck(slice / std::max(simulationSpeed_, 0.001f));
    dayTimer_ += slice;
    remaining -= slice;
    const int due =
        std::clamp(static_cast<int>(dayTimer_ * 8.0f / dayLength), 0, 8);
    if (!processThreeHourSales(due)) {
      return;
    }
    if (dayTimer_ >= dayLength - 0.000001f) {
      recordDailyFinance();
      dayTimer_ = 0.0f;
      advanceDay();
    }
  }
}

void SimulationScreen::draw(sf::RenderWindow& window,
                            const SimulationSettings& settings) {
  loadTextures();

  ensureStoresCreated(settings.stores);

  handleMouseClick(window);

  drawMapBackground(window);
  drawAmbientClouds(window);
  drawRoutes(window);

  drawWarehouse(window);
  drawSupplier(window);

  drawStores(window);

  drawVehicles(window);
  drawSupplierTruck(window);
  drawDecorativeTowers(window);
  drawAmbientPlanes(window);
  drawFinalExplosion(window);

  drawTopBar(settings);
  drawBottomBar();
  if (finaleState_ != FinaleState::Approach &&
      finaleState_ != FinaleState::Explosion) {
    drawInfoPanel();
  }
  drawFinalReport();
}

void SimulationScreen::ensureStoresCreated(int storeCount) {
  if (backend_ == nullptr || !backend_->isReady()) {
    return;
  }

  const std::vector<StoreInfo> actualStores = backend_->getStores();

  storeCount = static_cast<int>(actualStores.size());
  if (static_cast<int>(stores_.size()) == storeCount) {
    return;
  }

  stores_.clear();

  for (int i = 0; i < storeCount; ++i) {
    StoreView store;
    store.id = actualStores[i].id;
    const float columns[] = {380.0f, 760.0f, 1010.0f};
    store.position = {columns[i % 3], 170.0f + (i / 3) * 130.0f};

    store.bounds = sf::FloatRect(sf::Vector2f(store.position.x - 40.0f,

                                              store.position.y - 30.0f),
                                 sf::Vector2f(80.0f, 60.0f));

    stores_.push_back(store);
  }

  vehicles_.clear();

  for (const StoreView& store : stores_) {
    VehicleView vehicle;

    vehicle.targetStoreId = store.id;

    vehicle.position = warehouseCenter_;

    vehicle.progress = 0.0f;

    vehicle.direction = VehicleDirection::ToStore;

    vehicle.active = false;

    vehicle.startDelay = 0.0f;
    vehicle.active = false;
    vehicle.unloading = false;
    vehicle.scheduleDay = -1;

    vehicles_.push_back(vehicle);
  }
}

void SimulationScreen::drawTopBar(const SimulationSettings& settings) {
  const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);

  ImGui::SetNextWindowSize(ImVec2(displaySize.x, 70.0f), ImGuiCond_Always);

  ImGui::Begin("TopBar", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

  ImGui::Text("NOVA WAREHOUSE");

  ImGui::SameLine(280.0f);

  ImGui::Text("Day: %d / %d", currentDay_, totalDays_);

  ImGui::SameLine(450.0f);

  ImGui::Text("Stores: %d", settings.stores);

  ImGui::SameLine(600.0f);

  ImGui::Text("Products: %d", settings.products);

  ImGui::SameLine(800.0f);

  if (simulationState_ == SimulationState::Running) {
    ImGui::Text("Status: RUNNING");
  } else if (simulationState_ == SimulationState::Paused) {
    ImGui::Text("Status: PAUSED");
  } else if (simulationState_ == SimulationState::Finished) {
    ImGui::Text("Status: FINISHED");
  } else {
    ImGui::Text("Status: STOPPED");
  }

  ImGui::End();
}

void SimulationScreen::drawBottomBar() {
  const float height = 80.0f;

  const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

  ImGui::SetNextWindowPos(ImVec2(0.0f, displaySize.y - height),
                          ImGuiCond_Always);

  ImGui::SetNextWindowSize(ImVec2(displaySize.x, height), ImGuiCond_Always);

  ImGui::Begin("BottomBar", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

  if (ImGui::Button("START", ImVec2(100.0f, 40.0f))) {
    bool canStart = true;

    if (simulationState_ == SimulationState::Finished) {
      const bool resetOk =
          backend_ != nullptr && !databasePath_.empty() &&
          !schemaPath_.empty() &&
          backend_->resetDatabase(databasePath_.string(), schemaPath_.string());

      if (!resetOk) {
        canStart = false;
        ImGui::OpenPopup("Database error");
      } else {
        currentDay_ = 1;
        dayTimer_ = 0.0f;
        firstDayProcessed_ = false;
        salesIntervalsProcessed_ = 0;
        dailyFinance_.assign(static_cast<std::size_t>(totalDays_),
                             DailyFinance{});
        finaleState_ = FinaleState::None;
        finaleTimer_ = 0.0f;
        finalReport_ = ExperimentReport{};
        finalReportVisible_ = false;
        finalReportLoaded_ = false;

        batchDiscountPercent_ = 20;
        batchDiscountFailed_ = false;

        animatedSupplierRequestIds_.clear();
        activeSupplierRequestId_ = -1;
        activeSupplierRequestIds_.clear();
        activeSupplierProductIds_.clear();
        supplierFlights_.clear();
        nextSupplierDepartureHour_ = 0.0;

        supplierTruck_.active = false;
        supplierTruck_.returning = false;
        supplierTruck_.progress = 0.0f;
        supplierTruck_.position = supplierCenter_;

        for (VehicleView& vehicle : vehicles_) {
          vehicle.progress = 0.0f;
          vehicle.position = warehouseCenter_;
          vehicle.direction = VehicleDirection::ToStore;
          vehicle.startDelay = 0.0f;
          vehicle.active = false;
          vehicle.unloading = false;
          vehicle.scheduleDay = -1;
        }

        refreshWarehouseData();
        refreshSupplierData();

        if (selectedType_ == SelectedObjectType::Store) {
          refreshSelectedStoreData();
        }
      }
    }

    if (canStart) {
      simulationState_ = SimulationState::Running;
    }
  }
  ImGui::SameLine();

  if (ImGui::Button("PAUSE", ImVec2(100.0f, 40.0f))) {
    if (simulationState_ == SimulationState::Running) {
      simulationState_ = SimulationState::Paused;
    }
  }

  ImGui::SameLine();

  if (ImGui::Button("STOP", ImVec2(100.0f, 40.0f))) {
    bool resetOk = false;

    if (backend_ != nullptr && !databasePath_.empty() && !schemaPath_.empty()) {
      resetOk =
          backend_->resetDatabase(databasePath_.string(), schemaPath_.string());
    }

    if (!resetOk) {
      simulationState_ = SimulationState::Paused;
      ImGui::OpenPopup("Database error");
    } else {
      simulationState_ = SimulationState::Stopped;

      currentDay_ = 1;
      dayTimer_ = 0.0f;
      firstDayProcessed_ = false;
      salesIntervalsProcessed_ = 0;
      dailyFinance_.assign(static_cast<std::size_t>(totalDays_),
                           DailyFinance{});
      finaleState_ = FinaleState::None;
      finaleTimer_ = 0.0f;
      finalReport_ = ExperimentReport{};
      finalReportVisible_ = false;
      finalReportLoaded_ = false;
      animatedSupplierRequestIds_.clear();

      supplierTruck_.active = false;
      supplierTruck_.returning = false;
      supplierTruck_.progress = 0.0f;
      supplierTruck_.position = supplierCenter_;

      activeSupplierRequestId_ = -1;
      activeSupplierRequestIds_.clear();
      activeSupplierProductIds_.clear();
      supplierFlights_.clear();
      nextSupplierDepartureHour_ = 0.0;

      for (VehicleView& vehicle : vehicles_) {
        vehicle.progress = 0.0f;
        vehicle.direction = VehicleDirection::ToStore;
        vehicle.position = warehouseCenter_;

        vehicle.startDelay = 0.0f;
        vehicle.active = false;
        vehicle.unloading = false;
        vehicle.scheduleDay = -1;
      }

      refreshWarehouseData();
      refreshSupplierData();

      if (selectedType_ == SelectedObjectType::Store) {
        refreshSelectedStoreData();
      }
    }
  }
  if (ImGui::BeginPopupModal("Database error", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted("Failed to reset database.");

    if (backend_ != nullptr) {
      ImGui::TextWrapped("%s", backend_->getLastError().c_str());
    } else {
      ImGui::TextUnformatted("Backend is not connected.");
    }

    if (ImGui::Button("OK")) {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
  if (simulationState_ == SimulationState::Finished &&
      finaleState_ != FinaleState::Approach &&
      finaleState_ != FinaleState::Explosion) {
    ImGui::SameLine();
    if (ImGui::Button("Итоговый отчёт")) {
      loadFinalReport();
      finalReportVisible_ = true;
    }
  }
  ImGui::SameLine();

  ImGui::Text("Speed:");

  ImGui::SameLine();

  if (ImGui::Button("1x")) {
    simulationSpeed_ = 1.0f;
  }

  ImGui::SameLine();

  if (ImGui::Button("2x")) {
    simulationSpeed_ = 2.0f;
  }

  ImGui::SameLine();

  if (ImGui::Button("5x")) {
    simulationSpeed_ = 5.0f;
  }

  ImGui::SameLine();

  if (ImGui::Button("10x")) {
    simulationSpeed_ = 10.0f;
  }

  ImGui::SameLine(750.0f);

  ImGui::Text("Current speed: %.0fx", simulationSpeed_);

  ImGui::SameLine(900.0f);

  float dayProgress = dayTimer_ / secondsPerDay_;

  dayProgress = std::clamp(dayProgress, 0.0f, 1.0f);

  ImGui::ProgressBar(dayProgress, ImVec2(200.0f, 20.0f));

  ImGui::End();
}

void SimulationScreen::drawRoutes(sf::RenderWindow& window) {
  if (cachedRoadStoreCount_ == static_cast<int>(stores_.size())) {
    window.draw(roadMesh_);
    window.draw(
        yellowMarkingMesh_);  // Yellow markings are the final road layer.
    return;
  }
  roadMesh_.clear();
  yellowMarkingMesh_.clear();
  bool markings = false;
  struct Segment {
    sf::Vector2f start;
    sf::Vector2f end;
  };
  struct Rect {
    float left;
    float top;
    float right;
    float bottom;
  };
  std::vector<Segment> segments;
  std::vector<sf::Vector2f> junctions;
  const auto add = [&segments, &junctions](sf::Vector2f start,
                                           sf::Vector2f end) {
    if (std::abs(start.x - end.x) + std::abs(start.y - end.y) < 0.01f) {
      return;
    }
    for (const Segment& segment : segments) {
      if ((segment.start == start && segment.end == end) ||
          (segment.start == end && segment.end == start)) {
        return;
      }
    }
    segments.push_back({start, end});
    junctions.push_back(start);
    junctions.push_back(end);
  };
  add(supplierCenter_, warehouseCenter_);
  for (const StoreView& store : stores_) {
    const sf::Vector2f junction(warehouseCenter_.x, store.position.y + 55.0f);
    const sf::Vector2f entrance(store.position.x, store.position.y + 55.0f);
    add(warehouseCenter_, junction);
    add(junction, entrance);
    add(entrance, store.position);
  }
  const auto bounds = [](const Segment& segment, float halfWidth) {
    return Rect{std::min(segment.start.x, segment.end.x) - halfWidth,
                std::min(segment.start.y, segment.end.y) - halfWidth,
                std::max(segment.start.x, segment.end.x) + halfWidth,
                std::max(segment.start.y, segment.end.y) + halfWidth};
  };
  const auto fill = [this, &markings](const Rect& rect, sf::Color color) {
    sf::VertexArray& mesh = markings ? yellowMarkingMesh_ : roadMesh_;
    const sf::Vector2f points[] = {
        {rect.left, rect.top},     {rect.right, rect.top},
        {rect.right, rect.bottom}, {rect.left, rect.top},
        {rect.right, rect.bottom}, {rect.left, rect.bottom}};
    for (const sf::Vector2f point : points) {
      sf::Vertex vertex;
      vertex.position = point;
      vertex.color = color;
      mesh.append(vertex);
    }
  };
  // First draw ALL shoulders, then ALL asphalt. No shoulder can cut across
  // asphalt.
  for (const Segment& segment : segments) {
    fill(bounds(segment, 34.0f), sf::Color(146, 147, 139));
  }
  std::vector<Rect> asphalt;
  for (const Segment& segment : segments) {
    asphalt.push_back(bounds(segment, 28.0f));
    fill(asphalt.back(), sf::Color(52, 57, 63));
  }
  // Outline only the outer boundary of the combined asphalt surface.
  // Split edges at every rectangle boundary and discard internal portions.
  for (std::size_t index = 0; index < asphalt.size(); ++index) {
    const Rect& rect = asphalt[index];
    for (int side = 0; side < 4; ++side) {
      const bool horizontal = side < 2;
      const float fixed = horizontal ? (side == 0 ? rect.top : rect.bottom)
                                     : (side == 2 ? rect.left : rect.right);
      const float from = horizontal ? rect.left : rect.top;
      const float to = horizontal ? rect.right : rect.bottom;
      std::vector<float> cuts{from, to};
      for (const Rect& other : asphalt) {
        const float values[] = {horizontal ? other.left : other.top,
                                horizontal ? other.right : other.bottom};
        for (const float value : values) {
          if (value > from && value < to) {
            cuts.push_back(value);
          }
        }
      }
      std::sort(cuts.begin(), cuts.end());
      for (std::size_t part = 1; part < cuts.size(); ++part) {
        if (cuts[part] - cuts[part - 1] < 0.01f) {
          continue;
        }
        const float middle = (cuts[part] + cuts[part - 1]) * 0.5f;
        const sf::Vector2f sample = horizontal ? sf::Vector2f(middle, fixed)
                                               : sf::Vector2f(fixed, middle);
        bool internal = false;
        for (std::size_t otherIndex = 0; otherIndex < asphalt.size();
             ++otherIndex) {
          if (otherIndex == index) {
            continue;
          }
          const Rect& other = asphalt[otherIndex];
          if (sample.x > other.left + 0.01f && sample.x < other.right - 0.01f &&
              sample.y > other.top + 0.01f && sample.y < other.bottom - 0.01f) {
            internal = true;
            break;
          }
        }
        if (!internal) {
          const Rect line = horizontal ? Rect{cuts[part - 1], fixed - 0.75f,
                                              cuts[part], fixed + 0.75f}
                                       : Rect{fixed - 0.75f, cuts[part - 1],
                                              fixed + 0.75f, cuts[part]};
          fill(line, sf::Color(205, 209, 199));
        }
      }
    }
  }
  markings = true;
  for (const Segment& segment : segments) {
    const sf::Vector2f delta = segment.end - segment.start;
    const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    const sf::Vector2f direction = delta / length;
    for (float offset = 10.0f; offset < length - 8.0f; offset += 32.0f) {
      const float extent = std::min(17.0f, length - offset);
      const sf::Vector2f first = segment.start + direction * offset;
      const sf::Vector2f last = first + direction * extent;
      bool nearJunction = false;
      for (const sf::Vector2f junction : junctions) {
        const sf::Vector2f difference = junction - first;
        const float along =
            difference.x * direction.x + difference.y * direction.y;
        const float across =
            std::abs(difference.x * direction.y - difference.y * direction.x);
        if (across < 1.0f && along >= -36.0f && along <= extent + 36.0f) {
          nearJunction = true;
          break;
        }
      }
      if (!nearJunction) {
        fill(Rect{std::min(first.x, last.x) - 1.0f,
                  std::min(first.y, last.y) - 1.0f,
                  std::max(first.x, last.x) + 1.0f,
                  std::max(first.y, last.y) + 1.0f},
             sf::Color(239, 217, 137));
      }
    }
  }
  cachedRoadStoreCount_ = static_cast<int>(stores_.size());
  window.draw(roadMesh_);
  window.draw(yellowMarkingMesh_);  // After asphalt, shoulders and white lines.
}

void SimulationScreen::drawSupplierRoute(sf::RenderWindow& window) {
  drawRoadSegment(window, supplierCenter_, warehouseCenter_);
}

void SimulationScreen::drawWarehouse(sf::RenderWindow& window) {
  const sf::Vector2f size(180.0f, 120.0f);

  const sf::Vector2f position(warehouseCenter_.x - size.x / 2.0f,

                              warehouseCenter_.y - size.y / 2.0f);

  if (warehouseTextureLoaded_) {
    sf::Sprite warehouse(warehouseTexture_);

    const sf::Vector2u textureSize = warehouseTexture_.getSize();

    warehouse.setOrigin(
        sf::Vector2f(textureSize.x / 2.0f, textureSize.y / 2.0f));

    warehouse.setPosition(warehouseCenter_);

    warehouse.setScale(
        sf::Vector2f(size.x / static_cast<float>(textureSize.x),

                     size.y / static_cast<float>(textureSize.y)));

    window.draw(warehouse);

    warehouseBounds_ = warehouse.getGlobalBounds();
  } else {
    sf::RectangleShape warehouse(size);

    warehouse.setPosition(position);

    warehouse.setFillColor(sf::Color(170, 20, 30));

    warehouse.setOutlineThickness(4.0f);

    if (selectedType_ == SelectedObjectType::Warehouse) {
      warehouse.setOutlineColor(sf::Color(255, 200, 40));
    } else {
      warehouse.setOutlineColor(sf::Color::White);
    }

    window.draw(warehouse);

    warehouseBounds_ = warehouse.getGlobalBounds();
  }
}
void SimulationScreen::drawSupplier(sf::RenderWindow& window) {
  if (supplierTextureLoaded_) {
    sf::Sprite supplier(supplierTexture_);

    const sf::Vector2u size = supplierTexture_.getSize();

    const float scale = std::min(160.0f / static_cast<float>(size.x),
                                 105.0f / static_cast<float>(size.y));

    supplier.setOrigin(sf::Vector2f(size.x / 2.0f, size.y / 2.0f));

    supplier.setPosition(supplierCenter_);
    supplier.setScale(sf::Vector2f(scale, scale));

    supplierBounds_ = supplier.getGlobalBounds();
    window.draw(supplier);
  } else {
    sf::RectangleShape supplier(sf::Vector2f(140.0f, 90.0f));

    supplier.setOrigin(sf::Vector2f(70.0f, 45.0f));
    supplier.setPosition(supplierCenter_);
    supplier.setFillColor(sf::Color(230, 140, 30));

    supplierBounds_ = supplier.getGlobalBounds();
    window.draw(supplier);
  }

  if (selectedType_ == SelectedObjectType::Supplier) {
    sf::RectangleShape outline(supplierBounds_.size);

    outline.setPosition(supplierBounds_.position);
    outline.setFillColor(sf::Color::Transparent);
    outline.setOutlineThickness(2.0f);
    outline.setOutlineColor(sf::Color(255, 220, 100));

    window.draw(outline);
  }
}
void SimulationScreen::drawStores(sf::RenderWindow& window) {
  for (const StoreView& storeView : stores_) {
    const sf::Vector2f size(80.0f, 60.0f);

    if (storeTextureLoaded_) {
      sf::Sprite store(storeTexture_);

      const sf::Vector2u textureSize = storeTexture_.getSize();

      store.setOrigin(sf::Vector2f(textureSize.x / 2.0f, textureSize.y / 2.0f));

      store.setPosition(storeView.position);

      store.setScale(sf::Vector2f(size.x / static_cast<float>(textureSize.x),

                                  size.y / static_cast<float>(textureSize.y)));

      if (selectedType_ == SelectedObjectType::Store &&
          selectedStoreId_ == storeView.id) {
        store.setColor(sf::Color(255, 220, 120));
      }

      window.draw(store);
    } else {
      sf::RectangleShape store(size);

      store.setPosition(sf::Vector2f(storeView.position.x - size.x / 2.0f,

                                     storeView.position.y - size.y / 2.0f));

      if (selectedType_ == SelectedObjectType::Store &&
          selectedStoreId_ == storeView.id) {
        store.setFillColor(sf::Color(230, 180, 40));
      } else {
        store.setFillColor(sf::Color(220, 220, 220));
      }

      store.setOutlineThickness(2.0f);

      store.setOutlineColor(sf::Color(120, 120, 120));

      window.draw(store);
    }
  }
}

void SimulationScreen::drawVehicles(sf::RenderWindow& window) {
  for (const VehicleView& vehicle : vehicles_) {
    if (!vehicle.active) {
      continue;
    }

    if (vehicle.startDelay > 0.0f) {
      continue;
    }

    const sf::Vector2f storePosition = getStorePosition(vehicle.targetStoreId);

    const RoadSample road =
        sampleStoreRoad(vehicle.targetStoreId, vehicle.progress,
                        vehicle.direction == VehicleDirection::ToWarehouse);
    const sf::Vector2f direction = road.direction;

    const float angle =
        std::atan2(direction.y, direction.x) * 180.0f / 3.14159265f;

    if (truckTextureLoaded_) {
      sf::Sprite truck(truckTexture_);

      const sf::Vector2u textureSize = truckTexture_.getSize();

      truck.setOrigin(sf::Vector2f(textureSize.x / 2.0f,

                                   textureSize.y / 2.0f));

      truck.setPosition(vehicle.position);

      const float desiredWidth = 58.0f;

      const float scale = desiredWidth / static_cast<float>(textureSize.x);

      truck.setScale(sf::Vector2f(scale, scale));

      truck.setRotation(sf::degrees(angle));

      truck.setColor(sf::Color::White);

      window.draw(truck);
    } else {
      sf::RectangleShape shape(sf::Vector2f(35.0f, 20.0f));

      shape.setOrigin(sf::Vector2f(17.5f, 10.0f));

      shape.setPosition(vehicle.position);

      shape.setRotation(sf::degrees(angle));

      if (vehicle.direction == VehicleDirection::ToStore) {
        shape.setFillColor(sf::Color(40, 190, 90));
      } else {
        shape.setFillColor(sf::Color(70, 130, 230));
      }

      window.draw(shape);
    }
  }
}
void SimulationScreen::drawSupplierTruckSprite(sf::RenderWindow& window) {
  if (!supplierTruck_.active) {
    return;
  }

  sf::Vector2f direction;

  if (!supplierTruck_.returning) {
    direction = warehouseCenter_ - supplierCenter_;
  } else {
    direction = supplierCenter_ - warehouseCenter_;
  }

  const float angle =
      std::atan2(direction.y, direction.x) * 180.0f / 3.14159265f;

  if (supplierTruckTextureLoaded_) {
    sf::Sprite truck(supplierTruckTexture_);

    const sf::Vector2u textureSize = supplierTruckTexture_.getSize();

    truck.setOrigin(sf::Vector2f(textureSize.x / 2.0f, textureSize.y / 2.0f));

    truck.setPosition(supplierTruck_.position);

    // Грузовик поставщика специально больше обычного.
    const float desiredWidth = 85.0f;

    const float scale = desiredWidth / static_cast<float>(textureSize.x);

    truck.setScale(sf::Vector2f(scale, scale));

    truck.setRotation(sf::degrees(angle));

    truck.setColor(sf::Color::White);

    window.draw(truck);
  } else {
    sf::RectangleShape truck(sf::Vector2f(65.0f, 30.0f));

    truck.setOrigin(sf::Vector2f(32.5f, 15.0f));

    truck.setPosition(supplierTruck_.position);

    truck.setRotation(sf::degrees(angle));

    truck.setFillColor(sf::Color(255, 140, 20));

    truck.setOutlineThickness(3.0f);

    truck.setOutlineColor(sf::Color::White);

    window.draw(truck);
  }
}
void SimulationScreen::updateVehicles(float deltaTime) {
  if (backend_ == nullptr) {
    return;
  }
  const float hours = dayTimer_ / std::max(secondsPerDay_, 0.001f) * 24.0f;
  const float stepHours =
      deltaTime * simulationSpeed_ / std::max(secondsPerDay_, 0.001f) * 24.0f;
  for (VehicleView& vehicle : vehicles_) {
    std::mt19937 random(static_cast<unsigned int>(currentDay_ * 7919 +
                                                  vehicle.targetStoreId * 101));
    if (vehicle.scheduleDay != currentDay_) {
      vehicle.scheduleDay = currentDay_;
      vehicle.nextDeparture = 0;
      std::uniform_real_distribution<float> morning(8.0f, 10.0f),
          evening(16.0f, 18.0f);
      vehicle.departureHours[0] = morning(random);
      vehicle.departureHours[1] = evening(random);
    }
    float travelHours = vehicle.active ? stepHours : 0.0f;
    while (vehicle.nextDeparture < 2 &&
           hours + stepHours >= vehicle.departureHours[vehicle.nextDeparture]) {
      const int slot = vehicle.nextDeparture;
      const float windowEnd = slot == 0 ? 10.0f : 18.0f;
      if (vehicle.active || hours > windowEnd) {
        ++vehicle.nextDeparture;
        continue;
      }
      StoreOrderInfo order;
      if (!backend_->getActiveStoreOrder(vehicle.targetStoreId, order) ||
          order.status != "Allocated" || order.deliveryDay > currentDay_ ||
          !backend_->canDispatchStoreOrder(vehicle.targetStoreId,
                                           currentDay_)) {
        break;
      }
      ++vehicle.nextDeparture;
      std::mt19937 tripRandom(static_cast<unsigned int>(
          currentDay_ * 7919 + vehicle.targetStoreId * 101 + slot * 1009));
      std::uniform_real_distribution<float> speed(0.9f, 1.1f),
          unload(0.5f, 1.0f);
      vehicle.speedMultiplier = speed(tripRandom);
      vehicle.unloadingHours = unload(tripRandom);
      vehicle.unloading = false;
      vehicle.active = true;
      vehicle.direction = VehicleDirection::ToStore;
      vehicle.progress = 0.0f;
      travelHours = std::min(
          stepHours,
          std::max(0.0f, hours + stepHours - vehicle.departureHours[slot]));
    }
    if (!vehicle.active) {
      continue;
    }
    // Restore the former pacing: six game hours for each one-way leg.
    const float rate = 1.0f / 6.0f;
    while (travelHours > 0.000001f && vehicle.active) {
      if (vehicle.unloading) {
        const float used = std::min(travelHours, vehicle.unloadingHours);
        vehicle.unloadingHours -= used;
        travelHours -= used;
        if (vehicle.unloadingHours <= 0.000001f) {
          vehicle.unloading = false;
          vehicle.direction = VehicleDirection::ToWarehouse;
          vehicle.progress = 0.0f;
        }
        continue;
      }
      const float used =
          std::min(travelHours, (1.0f - vehicle.progress) / rate);
      vehicle.progress += used * rate;
      travelHours -= used;
      if (vehicle.progress >= 1.0f - 0.000001f) {
        vehicle.progress = 1.0f;
        if (vehicle.direction == VehicleDirection::ToStore) {
          backend_->processStoreDeliveries(currentDay_, vehicle.targetStoreId);
          vehicle.unloading = true;
          refreshWarehouseData();
          if (selectedStoreId_ == vehicle.targetStoreId) {
            refreshSelectedStoreData();
          }
        } else {
          vehicle.active = false;
          vehicle.progress = 0.0f;
          vehicle.position = warehouseCenter_;
        }
      }
    }
    if (vehicle.active) {
      vehicle.position =
          sampleStoreRoad(vehicle.targetStoreId, vehicle.progress,
                          vehicle.direction == VehicleDirection::ToWarehouse)
              .position;
    }
  }
}
void SimulationScreen::updateSupplierTruck(float deltaTime) {
  const float amount =
      deltaTime * simulationSpeed_ * (4.0f / std::max(secondsPerDay_, 0.001f));
  bool arrived = false;
  for (SupplierFlight& flight : supplierFlights_) {
    flight.truck.progress += amount;
    while (flight.truck.progress >= 1.0f && flight.truck.active) {
      flight.truck.progress -= 1.0f;
      if (!flight.truck.returning) {
        for (const int id : flight.requestIds) {
          if (backend_ != nullptr &&
              backend_->completeSupplierRequest(id, currentDay_)) {
            continue;
          }
          if (backend_ != nullptr) {
            std::cerr << backend_->getLastError() << '\n';
          }
          animatedSupplierRequestIds_.erase(
              std::remove(animatedSupplierRequestIds_.begin(),
                          animatedSupplierRequestIds_.end(), id),
              animatedSupplierRequestIds_.end());
        }
        flight.truck.returning = true;
        arrived = true;
      } else {
        flight.truck.active = false;
      }
    }
    const sf::Vector2f start =
        flight.truck.returning ? warehouseCenter_ : supplierCenter_;
    const sf::Vector2f end =
        flight.truck.returning ? supplierCenter_ : warehouseCenter_;
    flight.truck.position =
        start + (end - start) * flight.truck.progress +
        sf::Vector2f(0.0f, flight.truck.returning ? -9.0f : 9.0f);
  }
  supplierFlights_.erase(
      std::remove_if(
          supplierFlights_.begin(), supplierFlights_.end(),
          [](const SupplierFlight& flight) { return !flight.truck.active; }),
      supplierFlights_.end());
  if (arrived && backend_ != nullptr) {
    backend_->processStoreOrders(currentDay_);
    refreshWarehouseData();
  }
  syncSupplierFleet();
}

sf::Vector2f SimulationScreen::getStorePosition(int storeId) const {
  for (const StoreView& store : stores_) {
    if (store.id == storeId) {
      return store.position;
    }
  }

  return warehouseCenter_;
}

void SimulationScreen::handleMouseClick(sf::RenderWindow& window) {
  const bool mousePressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

  if (mousePressed && !mouseWasPressed_) {
    const sf::Vector2i pixelPosition = sf::Mouse::getPosition(window);

    const sf::Vector2f mousePosition = window.mapPixelToCoords(pixelPosition);

    if (warehouseBounds_.contains(mousePosition)) {
      selectedType_ = SelectedObjectType::Warehouse;

      selectedStoreId_ = -1;

      selectedStoreName_.clear();
      selectedStoreInventory_.clear();
    } else if (supplierBounds_.contains(mousePosition)) {
      selectedType_ = SelectedObjectType::Supplier;

      selectedStoreId_ = -1;

      selectedStoreName_.clear();
      selectedStoreInventory_.clear();
    } else {
      for (const StoreView& store : stores_) {
        if (store.bounds.contains(mousePosition)) {
          selectedType_ = SelectedObjectType::Store;

          selectedStoreId_ = store.id;

          refreshSelectedStoreData();

          break;
        }
      }
    }
  }

  mouseWasPressed_ = mousePressed;
}

void SimulationScreen::drawInfoPanel() {
  if (selectedType_ == SelectedObjectType::None) {
    return;
  }

  const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

  const float panelWidth = 620.0f;

  const float topHeight = 70.0f;

  const float bottomHeight = 80.0f;

  ImGui::SetNextWindowPos(ImVec2(displaySize.x - panelWidth,

                                 topHeight),
                          ImGuiCond_FirstUseEver);

  ImGui::SetNextWindowSize(ImVec2(panelWidth,

                                  displaySize.y - topHeight - bottomHeight),
                           ImGuiCond_FirstUseEver);

  bool panelOpen = true;
  const bool panelVisible =
      ImGui::Begin("InfoPanel", &panelOpen, ImGuiWindowFlags_NoCollapse);
  if (!panelOpen) {
    selectedType_ = SelectedObjectType::None;
    selectedStoreId_ = -1;
  }
  if (!panelVisible || !panelOpen) {
    ImGui::End();
    return;
  }
  if (selectedType_ == SelectedObjectType::Warehouse) {
    ImGui::Text("WAREHOUSE");

    ImGui::Separator();

    if (backend_ == nullptr || !backend_->isReady()) {
      ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f),
                         "Backend database is not available");
    } else if (selectedType_ == SelectedObjectType::Store) {
      ImGui::Text("STORE");

      ImGui::Separator();

      ImGui::Text("Name: %s", selectedStoreName_.c_str());

      ImGui::Text("ID: %d", selectedStoreId_);

      ImGui::Spacing();

      ImGui::Text("Inventory:");

      ImGui::Separator();

      if (selectedStoreInventory_.empty()) {
        ImGui::Text("No products.");
      } else {
        const ImGuiTableFlags inventoryFlags = ImGuiTableFlags_Borders |
                                               ImGuiTableFlags_RowBg |
                                               ImGuiTableFlags_Resizable;

        if (ImGui::BeginTable("StoreInventoryTable", 7, inventoryFlags)) {
          ImGui::TableSetupColumn("Product");

          ImGui::TableSetupColumn("Qty");

          ImGui::TableSetupColumn("Capacity");

          ImGui::TableSetupColumn("Min");

          ImGui::TableSetupColumn("Status");
          ImGui::TableSetupColumn("Price, RUB");
          ImGui::TableSetupColumn("Shelf life, days");
          ImGui::TableHeadersRow();

          for (const ProductStockInfo& item : selectedStoreInventory_) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            ImGui::TextUnformatted(item.productName.c_str());

            ImGui::TableSetColumnIndex(1);

            ImGui::Text("%d", item.quantity);

            ImGui::TableSetColumnIndex(2);

            ImGui::Text("%d", item.capacity);

            ImGui::TableSetColumnIndex(3);

            ImGui::Text("%d", item.minStock);

            ImGui::TableSetColumnIndex(4);

            if (item.quantity <= item.minStock) {
              ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "LOW");
            } else {
              ImGui::TextColored(ImVec4(0.35f, 1.0f, 0.45f, 1.0f), "OK");
            }
          }

          ImGui::EndTable();
        }
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::Text("Current order:");

      if (!selectedStoreHasOrder_) {
        ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.65f, 1.0f),
                           "No active order");
      } else {
        ImGui::Text("Order #%d", selectedStoreOrder_.id);

        ImGui::Text("Created day: %d", selectedStoreOrder_.createdDay);

        ImGui::Text("Delivery day: %d", selectedStoreOrder_.deliveryDay);

        ImGui::Text("Status: %s", selectedStoreOrder_.status.c_str());

        ImGui::Spacing();

        const ImGuiTableFlags orderFlags =
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

        if (ImGui::BeginTable("StoreOrderItems", 3, orderFlags)) {
          ImGui::TableSetupColumn("Product");

          ImGui::TableSetupColumn("Requested");

          ImGui::TableSetupColumn("Allocated");

          ImGui::TableHeadersRow();

          for (const OrderItemInfo& item : selectedStoreOrder_.items) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            ImGui::TextUnformatted(item.productName.c_str());

            ImGui::TableSetColumnIndex(1);

            ImGui::Text("%d", item.requestedQuantity);

            ImGui::TableSetColumnIndex(2);

            ImGui::Text("%d", item.allocatedQuantity);
          }

          ImGui::EndTable();
        }
      }
    } else {
      ImGui::Text("Status: Working");

      ImGui::Spacing();

      if (ImGui::BeginTabBar("WarehouseTabs")) {
        if (ImGui::BeginTabItem("Потребность и поставки")) {
          drawSupplySummary();
          ImGui::EndTabItem();
        }
        // =========================
        // PRODUCTS
        // =========================
        if (ImGui::BeginTabItem("Products")) {
          drawWarehouseBatches();
          drawWarehouseWriteoffs();
          drawWarehouseAllocationStats();
          const ImGuiTableFlags warehouseTableFlags =
              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
              ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX |
              ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

          if (ImGui::BeginTable("WarehouseInventoryTable", 8,
                                warehouseTableFlags, ImVec2(0.0f, 280.0f),
                                1100.0f)) {
            ImGui::TableSetupColumn("Товар", ImGuiTableColumnFlags_WidthFixed,
                                    220.0f);
            ImGui::TableSetupColumn("Количество, шт.",
                                    ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Вместимость, шт.",
                                    ImGuiTableColumnFlags_WidthFixed, 130.0f);
            ImGui::TableSetupColumn("Минимум, шт.",
                                    ImGuiTableColumnFlags_WidthFixed, 110.0f);
            ImGui::TableSetupColumn("Статус", ImGuiTableColumnFlags_WidthFixed,
                                    90.0f);
            ImGui::TableSetupColumn("Цена, руб.",
                                    ImGuiTableColumnFlags_WidthFixed, 110.0f);
            ImGui::TableSetupColumn("Срок хранения, дней",
                                    ImGuiTableColumnFlags_WidthFixed, 160.0f);
            ImGui::TableSetupColumn("Остаток в упаковках",
                                    ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (const WarehouseStockInfo& item : warehouseInventory_) {
              ImGui::TableNextRow();
              ImGui::TableSetColumnIndex(5);
              ImGui::Text("%.2f", item.unitPriceCents / 100.0);

              ImGui::TableSetColumnIndex(6);
              ImGui::Text("%d", item.shelfLifeDays);

              ImGui::TableSetColumnIndex(0);

              ImGui::TextUnformatted(item.productName.c_str());

              ImGui::TableSetColumnIndex(1);

              ImGui::Text("%d", item.quantity);

              ImGui::TableSetColumnIndex(2);

              ImGui::Text("%d", item.capacity);

              ImGui::TableSetColumnIndex(3);

              ImGui::Text("%d", item.minStock);

              ImGui::TableSetColumnIndex(4);

              if (item.quantity <= item.minStock) {
                ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "LOW");
              } else {
                ImGui::TextColored(ImVec4(0.35f, 1.0f, 0.45f, 1.0f), "OK");
              }

              ImGui::TableSetColumnIndex(7);

              const int packageSize =
                  item.unitsPerPackage > 0 ? item.unitsPerPackage : 1;

              const int fullPackages = item.quantity / packageSize;
              const int looseUnits = item.quantity % packageSize;

              ImGui::Text("%d уп. + %d шт.", fullPackages, looseUnits);

              if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("В одной упаковке: %d шт.", packageSize);
              }
            }
            ImGui::EndTable();
          }

          if (warehouseInventory_.empty()) {
            ImGui::Spacing();

            ImGui::Text("No warehouse products found.");
          }

          ImGui::EndTabItem();
        }

        // =========================
        // SUPPLY
        // =========================
        if (ImGui::BeginTabItem("Supply")) {
          drawSupplierApprovals();
          int activeRequests = 0;

          for (const SupplierRequestInfo& request : supplierRequests_) {
            if (request.status != "Delivered") {
              activeRequests++;
            }
          }

          ImGui::Text("Active requests: %d", activeRequests);

          ImGui::Spacing();

          if (supplierRequests_.empty()) {
            ImGui::Text("No supplier requests.");
          } else {
            const ImGuiTableFlags flags = ImGuiTableFlags_Borders |
                                          ImGuiTableFlags_RowBg |
                                          ImGuiTableFlags_Resizable;

            if (ImGui::BeginTable("SupplierRequestsTable", 5, flags)) {
              ImGui::TableSetupColumn("Product");

              ImGui::TableSetupColumn("Qty");

              ImGui::TableSetupColumn("Created");

              ImGui::TableSetupColumn("Delivery");

              ImGui::TableSetupColumn("Status");

              ImGui::TableHeadersRow();

              for (const SupplierRequestInfo& request : supplierRequests_) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);

                ImGui::TextUnformatted(request.productName.c_str());

                ImGui::TableSetColumnIndex(1);

                ImGui::Text("%d", request.requestedQuantity);

                ImGui::TableSetColumnIndex(2);

                ImGui::Text("%d", request.createdDay);

                ImGui::TableSetColumnIndex(3);

                ImGui::Text("%d", request.deliveryDay);

                ImGui::TableSetColumnIndex(4);

                ImGui::TextUnformatted(request.status.c_str());
              }

              ImGui::EndTable();
            }
          }

          ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
      }
    }
  } else if (selectedType_ == SelectedObjectType::Supplier) {
    ImGui::Text("SUPPLIER COMPANY");

    ImGui::Separator();

    if (ImGui::BeginTabBar("SupplierInfoTabs")) {
      if (ImGui::BeginTabItem("Все товары и груз")) {
        drawSupplySummary();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("История заявок")) {
        int activeRequests = 0;

        for (const SupplierRequestInfo& request : supplierRequests_) {
          if (request.status != "Delivered") {
            activeRequests++;
          }
        }

        ImGui::Text("Active requests: %d", activeRequests);

        ImGui::Spacing();

        if (supplierRequests_.empty()) {
          ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.65f, 1.0f),
                             "No supplier requests");
        } else {
          const ImGuiTableFlags flags = ImGuiTableFlags_Borders |
                                        ImGuiTableFlags_RowBg |
                                        ImGuiTableFlags_Resizable;

          if (ImGui::BeginTable("SupplierInfoTable", 5, flags)) {
            ImGui::TableSetupColumn("Product");

            ImGui::TableSetupColumn("Qty");

            ImGui::TableSetupColumn("Created");

            ImGui::TableSetupColumn("Delivery");

            ImGui::TableSetupColumn("Status");

            ImGui::TableHeadersRow();

            for (const SupplierRequestInfo& request : supplierRequests_) {
              ImGui::TableNextRow();

              ImGui::TableSetColumnIndex(0);

              ImGui::TextUnformatted(request.productName.c_str());

              ImGui::TableSetColumnIndex(1);

              ImGui::Text("%d", request.requestedQuantity);

              ImGui::TableSetColumnIndex(2);

              ImGui::Text("%d", request.createdDay);

              ImGui::TableSetColumnIndex(3);

              ImGui::Text("%d", request.deliveryDay);

              ImGui::TableSetColumnIndex(4);

              ImGui::TextUnformatted(request.status.c_str());
            }

            ImGui::EndTable();
          }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Supplier truck:");

        if (supplierTruck_.active) {
          if (!supplierTruck_.returning) {
            ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.20f, 1.0f),
                               "Delivering products to warehouse");
          } else {
            ImGui::TextColored(ImVec4(0.50f, 0.80f, 1.0f, 1.0f),
                               "Returning to supplier");
          }
        } else {
          ImGui::Text("Waiting");
        }
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
  } else if (selectedType_ == SelectedObjectType::Store) {
    ImGui::Text("STORE");

    ImGui::Separator();

    if (backend_ == nullptr || !backend_->isReady()) {
      ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f),
                         "Backend database is not available");
    } else {
      ImGui::Text("ID: %d", selectedStoreId_);

      ImGui::Text("Name: %s", selectedStoreName_.c_str());

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::Text("Inventory");

      ImGui::Spacing();

      if (selectedStoreInventory_.empty()) {
        ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.65f, 1.0f),
                           "No products in store");
      } else {
        const ImGuiTableFlags flags =
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;

        if (ImGui::BeginTable("StoreInventoryTable", 5, flags)) {
          ImGui::TableSetupColumn("Product", ImGuiTableColumnFlags_None, 2.0f);

          ImGui::TableSetupColumn("Qty");

          ImGui::TableSetupColumn("Capacity");

          ImGui::TableSetupColumn("Min");

          ImGui::TableSetupColumn("Status");

          ImGui::TableHeadersRow();

          for (const ProductStockInfo& item : selectedStoreInventory_) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            ImGui::TextUnformatted(item.productName.c_str());

            ImGui::TableSetColumnIndex(1);

            ImGui::Text("%d", item.quantity);

            ImGui::TableSetColumnIndex(2);

            ImGui::Text("%d", item.capacity);

            ImGui::TableSetColumnIndex(3);

            ImGui::Text("%d", item.minStock);

            ImGui::TableSetColumnIndex(4);

            if (item.quantity <= item.minStock) {
              ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "LOW");
            } else {
              ImGui::TextColored(ImVec4(0.35f, 1.0f, 0.45f, 1.0f), "OK");
            }
          }

          ImGui::EndTable();
        }
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::Text("Current order");

      if (!selectedStoreHasOrder_) {
        ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.65f, 1.0f),
                           "No active order");
      } else {
        ImGui::Text("Order #%d", selectedStoreOrder_.id);

        ImGui::Text("Created day: %d", selectedStoreOrder_.createdDay);

        ImGui::Text("Delivery day: %d", selectedStoreOrder_.deliveryDay);

        ImGui::Text("Status: %s", selectedStoreOrder_.status.c_str());

        ImGui::Spacing();

        if (ImGui::BeginTable(
                "StoreOrderItems", 3,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
          ImGui::TableSetupColumn("Product");

          ImGui::TableSetupColumn("Requested");

          ImGui::TableSetupColumn("Allocated");

          ImGui::TableHeadersRow();

          for (const OrderItemInfo& item : selectedStoreOrder_.items) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            ImGui::TextUnformatted(item.productName.c_str());

            ImGui::TableSetColumnIndex(1);

            ImGui::Text("%d", item.requestedQuantity);

            ImGui::TableSetColumnIndex(2);

            ImGui::Text("%d", item.allocatedQuantity);
          }

          ImGui::EndTable();
        }
      }
    }
  }

  ImGui::End();
}

void SimulationScreen::loadTextures() {
  if (texturesLoaded_) {
    return;
  }

  texturesLoaded_ = true;

  const auto warehousePath = assetsPath_ / "textures" / "warehouse.png";

  const auto storePath = assetsPath_ / "textures" / "store.png";

  const auto truckPath = assetsPath_ / "textures" / "truck.png";

  warehouseTextureLoaded_ =
      warehouseTexture_.loadFromFile(warehousePath.string());

  storeTextureLoaded_ = storeTexture_.loadFromFile(storePath.string());

  truckTextureLoaded_ = truckTexture_.loadFromFile(truckPath.string());
  if (truckTextureLoaded_) {
    truckTexture_.setSmooth(true);
  }
  supplierTextureLoaded_ =
      supplierTexture_.loadFromFile(assetsPath_ / "textures" / "supplier.png");

  supplierTruckTextureLoaded_ = supplierTruckTexture_.loadFromFile(
      assetsPath_ / "textures" / "supplier_truck.png");

  if (supplierTextureLoaded_) {
    supplierTexture_.setSmooth(true);
  }

  if (supplierTruckTextureLoaded_) {
    supplierTruckTexture_.setSmooth(true);
  }
}

void SimulationScreen::setBackend(BackendFacade* backend) {
  backend_ = backend;
}

void SimulationScreen::setAssetsPath(const std::filesystem::path& path) {
  assetsPath_ = path;
}
void SimulationScreen::setDatabasePaths(
    const std::filesystem::path& databasePath,
    const std::filesystem::path& schemaPath) {
  databasePath_ = databasePath;

  schemaPath_ = schemaPath;
}
void SimulationScreen::refreshSelectedStoreData() {
  selectedStoreName_.clear();

  selectedStoreInventory_.clear();
  selectedStoreOrder_ = StoreOrderInfo{};

  selectedStoreHasOrder_ = false;

  if (backend_ == nullptr || !backend_->isReady() || selectedStoreId_ < 0) {
    return;
  }

  selectedStoreName_ = backend_->getStoreName(selectedStoreId_);

  selectedStoreInventory_ = backend_->getStoreInventory(selectedStoreId_);
  selectedStoreHasOrder_ =
      backend_->getActiveStoreOrder(selectedStoreId_, selectedStoreOrder_);
}
void SimulationScreen::refreshWarehouseData() {
  warehouseAllocationStats_ = WarehouseAllocationStats{};
  warehouseAllocationStatsLoaded_ = false;
  warehouseWriteoffs_.clear();
  warehouseWriteoffsLoaded_ = false;
  warehouseBatches_.clear();
  warehouseBatchesLoaded_ = false;
  warehouseInventory_.clear();
  supplySummary_.clear();
  supplySummaryLoaded_ = false;

  if (backend_ == nullptr || !backend_->isReady()) {
    return;
  }

  warehouseInventory_ = backend_->getWarehouseInventory();
  supplySummaryLoaded_ = backend_->getWarehouseSupplySummary(supplySummary_);
  warehouseBatchesLoaded_ = backend_->getWarehouseBatches(warehouseBatches_);
  warehouseWriteoffsLoaded_ =
      backend_->getWarehouseWriteoffs(warehouseWriteoffs_);
  warehouseAllocationStatsLoaded_ =
      backend_->getWarehouseAllocationStats(warehouseAllocationStats_);
}
void SimulationScreen::refreshSupplierData() {
  supplierRequests_.clear();
  if (backend_ == nullptr || !backend_->isReady()) {
    return;
  }
  supplierRequests_ = backend_->getSupplierRequests();
  const double hour = (currentDay_ - 1) * 24.0 +
                      dayTimer_ / std::max(secondsPerDay_, 0.001f) * 24.0;
  if (simulationState_ != SimulationState::Running ||
      supplierFlights_.size() >= 2 || hour < nextSupplierDepartureHour_) {
    return;
  }
  std::stable_sort(
      supplierRequests_.begin(), supplierRequests_.end(),
      [](const SupplierRequestInfo& first, const SupplierRequestInfo& second) {
        return first.deliveryDay != second.deliveryDay
                   ? first.deliveryDay < second.deliveryDay
                   : first.id < second.id;
      });
  SupplierFlight flight;
  for (const SupplierRequestInfo& request : supplierRequests_) {
    if (request.status != "InTransit" || request.deliveryDay > currentDay_ ||
        std::find(animatedSupplierRequestIds_.begin(),
                  animatedSupplierRequestIds_.end(),
                  request.id) != animatedSupplierRequestIds_.end()) {
      continue;
    }
    flight.requestIds.push_back(request.id);
    animatedSupplierRequestIds_.push_back(request.id);
    if (std::find(flight.productIds.begin(), flight.productIds.end(),
                  request.productId) == flight.productIds.end()) {
      flight.productIds.push_back(request.productId);
    }
  }
  if (flight.requestIds.empty()) {
    return;
  }
  flight.truck.active = true;
  flight.truck.position = supplierCenter_;
  supplierFlights_.push_back(std::move(flight));
  nextSupplierDepartureHour_ = hour + 8.0;
  syncSupplierFleet();
}

void SimulationScreen::configure(const SimulationSettings& settings) {
  automaticSupplierApproval_ = settings.automaticSupplierApproval;
  batchDiscountPercent_ = 20;
  batchDiscountFailed_ = false;
  totalDays_ = settings.days;

  currentDay_ = 1;
  dayTimer_ = 0.0f;
  firstDayProcessed_ = false;
  salesIntervalsProcessed_ = 0;
  dailyFinance_.assign(static_cast<std::size_t>(totalDays_), DailyFinance{});
  finaleState_ = FinaleState::None;
  finaleTimer_ = 0.0f;
  finalReport_ = ExperimentReport{};
  finalReportVisible_ = false;
  finalReportLoaded_ = false;
  animatedSupplierRequestIds_.clear();

  simulationSpeed_ = 1.0f;
  supplierTruck_.position = supplierCenter_;

  supplierTruck_.progress = 0.0f;

  supplierTruck_.returning = false;

  supplierTruck_.active = false;
  activeSupplierRequestId_ = -1;
  activeSupplierRequestIds_.clear();
  activeSupplierProductIds_.clear();
  supplierFlights_.clear();
  nextSupplierDepartureHour_ = 0.0;
  simulationState_ = SimulationState::Stopped;
}

void SimulationScreen::advanceDay() {
  if (currentDay_ >= totalDays_) {
    simulationState_ = SimulationState::Finished;
    dayTimer_ = 0.0f;
    loadFinalReport();
    beginFinalAnimation();
    return;
  }

  ++currentDay_;
  processCurrentDay();
}
void SimulationScreen::drawWarehouseBatches() {
  ImGui::Separator();
  ImGui::TextUnformatted("Партии товаров");

  if (!warehouseBatchesLoaded_) {
    ImGui::TextUnformatted(
        "Не удалось загрузить партии. "
        "Начните новый эксперимент по обновлённой схеме.");
    return;
  }

  if (warehouseBatches_.empty()) {
    ImGui::TextUnformatted("На складе нет партий.");
    return;
  }

  const ImGuiTableFlags flags =
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX;
  ImGui::SliderInt("Скидка для выбранной партии, %", &batchDiscountPercent_, 0,
                   90);

  if (batchDiscountFailed_) {
    ImGui::TextUnformatted(
        "Не удалось сохранить скидку. "
        "Проверьте срок и остаток партии.");
  }
  if (ImGui::BeginTable("WarehouseBatchesTable", 9, flags)) {
    ImGui::TableSetupColumn("Партия");
    ImGui::TableSetupColumn("Товар");
    ImGui::TableSetupColumn("Количество");
    ImGui::TableSetupColumn("Поступление");
    ImGui::TableSetupColumn("Годен до дня");
    ImGui::TableSetupColumn("Цена, руб.");
    ImGui::TableSetupColumn("Состояние");
    ImGui::TableSetupColumn("Скидка");
    ImGui::TableSetupColumn("Уценка");
    ImGui::TableHeadersRow();

    for (const WarehouseBatchRow& batch : warehouseBatches_) {
      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%d", batch.id);

      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted(batch.productName.c_str());

      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%d", batch.quantity);

      ImGui::TableSetColumnIndex(3);
      ImGui::Text("%d", batch.receivedDay);

      ImGui::TableSetColumnIndex(4);
      ImGui::Text("%d", batch.expiresDay - 1);

      ImGui::TableSetColumnIndex(5);

      const long long discountedPriceCents =
          (static_cast<long long>(batch.unitPriceCents) *
               (100 - batch.discountPercent) +
           50) /
          100;

      ImGui::Text("%.2f", discountedPriceCents / 100.0);

      ImGui::TableSetColumnIndex(6);

      if (currentDay_ >= batch.expiresDay) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Просрочена");
      } else {
        ImGui::Text("Осталось дней: %d", batch.expiresDay - currentDay_);
      }
      ImGui::TableSetColumnIndex(7);
      ImGui::Text("%d%%", batch.discountPercent);

      ImGui::TableSetColumnIndex(8);

      const int remainingDays = batch.expiresDay - currentDay_;

      if (remainingDays > 0 && remainingDays <= 3) {
        ImGui::PushID(batch.id);

        if (ImGui::Button("Применить")) {
          const bool saved = backend_ != nullptr &&
                             backend_->setBatchDiscount(
                                 batch.id, batchDiscountPercent_, currentDay_);

          batchDiscountFailed_ = !saved;

          if (saved) {
            dataRefreshTimer_ = 0.25f;
          }
        }

        ImGui::PopID();
      } else {
        ImGui::TextUnformatted("-");
      }
    }

    ImGui::EndTable();
  }
}

void SimulationScreen::drawWarehouseWriteoffs() {
  ImGui::Separator();
  ImGui::TextUnformatted("Списание просроченных товаров");

  if (!warehouseWriteoffsLoaded_) {
    ImGui::TextUnformatted("Не удалось загрузить историю списаний.");
    return;
  }

  long long totalLossCents = 0;
  long long totalQuantity = 0;

  for (const WarehouseWriteoffRow& row : warehouseWriteoffs_) {
    totalLossCents += row.lossCents;
    totalQuantity += row.quantity;
  }

  ImGui::Text("Списано единиц: %lld", totalQuantity);

  ImGui::Text("Убытки от списаний: %.2f руб.", totalLossCents / 100.0);

  if (warehouseWriteoffs_.empty()) {
    ImGui::TextUnformatted("Списаний пока нет.");
    return;
  }

  const ImGuiTableFlags flags =
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX;

  if (ImGui::BeginTable("WarehouseWriteoffsTable", 5, flags)) {
    ImGui::TableSetupColumn("День");
    ImGui::TableSetupColumn("Партия");
    ImGui::TableSetupColumn("Товар");
    ImGui::TableSetupColumn("Количество");
    ImGui::TableSetupColumn("Убытки, руб.");
    ImGui::TableHeadersRow();

    for (const WarehouseWriteoffRow& row : warehouseWriteoffs_) {
      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%d", row.writeoffDay);

      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%d", row.batchId);

      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(row.productName.c_str());

      ImGui::TableSetColumnIndex(3);
      ImGui::Text("%d", row.quantity);

      ImGui::TableSetColumnIndex(4);
      ImGui::Text("%.2f", row.lossCents / 100.0);
    }

    ImGui::EndTable();
  }
}

void SimulationScreen::drawWarehouseAllocationStats() {
  ImGui::Separator();
  ImGui::TextUnformatted("Товары, выделенные заказам");

  if (!warehouseAllocationStatsLoaded_) {
    ImGui::TextUnformatted("Не удалось загрузить статистику выделения.");
    return;
  }

  ImGui::Text("Выделено единиц: %lld",
              warehouseAllocationStats_.allocatedUnits);

  ImGui::Text("Стоимость с учётом скидок: %.2f руб.",
              warehouseAllocationStats_.allocatedValueCents / 100.0);

  ImGui::Text("Снижение стоимости из-за скидок: %.2f руб.",
              warehouseAllocationStats_.discountLossCents / 100.0);

  ImGui::TextWrapped(
      "Показатели учитывают выделение товара заказам. "
      "Доставка в магазин может ещё не завершиться.");
}

bool SimulationScreen::processCurrentDay() {
  if (backend_ == nullptr || !backend_->isReady()) {
    simulationState_ = SimulationState::Paused;
    std::cerr << "Database is not ready\n";
    return false;
  }

  if (!backend_->writeOffExpiredBatches(currentDay_)) {
    simulationState_ = SimulationState::Paused;
    std::cerr << backend_->getLastError() << '\n';
    return false;
  }

  salesIntervalsProcessed_ = 0;
  backend_->processStoreOrders(currentDay_);
  backend_->processSupplierRequests(currentDay_);
  approveSupplierRequestsAutomatically();

  refreshWarehouseData();
  refreshSupplierData();

  if (selectedType_ == SelectedObjectType::Store) {
    refreshSelectedStoreData();
  }

  return true;
}

void SimulationScreen::drawSupplierApprovals() {
  ImGui::Text("Грузовик поставщика: все готовые виды товаров. Сейчас: %d",
              supplierTruck_.active && !supplierTruck_.returning
                  ? static_cast<int>(activeSupplierProductIds_.size())
                  : 0);
  if (automaticSupplierApproval_) {
    ImGui::TextWrapped(
        "Автоматический режим: новые заявки поставщику "
        "подтверждаются в конце каждого дня.");
    if (supplierApprovalFailed_) {
      ImGui::TextWrapped(
          "Не удалось автоматически подтвердить заявку. "
          "Подробности выведены в консоль; повторная попытка будет в следующий день.");
    }
    return;
  }
  ImGui::Separator();
  ImGui::TextUnformatted("Подтверждение заявок поставщику");
  ImGui::TextWrapped("Срок доставки отсчитывается от дня подтверждения.");

  if (supplierApprovalFailed_) {
    ImGui::TextUnformatted("Не удалось подтвердить заявку.");
  }

  bool hasPendingRequests = false;

  for (const SupplierRequestInfo& request : supplierRequests_) {
    if (request.status != "Created") {
      continue;
    }

    hasPendingRequests = true;
    ImGui::PushID(request.id);

    ImGui::Text("Заявка №%d: %s — %d шт.", request.id,
                request.productName.c_str(), request.requestedQuantity);

    if (ImGui::Button("Подтвердить")) {
      const bool approved =
          backend_ != nullptr &&
          backend_->approveSupplierRequest(request.id, currentDay_);

      supplierApprovalFailed_ = !approved;

      if (approved) {
        dataRefreshTimer_ = 0.25f;
      }
    }

    ImGui::PopID();
    ImGui::Spacing();
  }

  if (!hasPendingRequests) {
    ImGui::TextUnformatted("Заявок, ожидающих подтверждения, нет.");
  }
}

void SimulationScreen::loadFinalReport() {
  finalReportLoaded_ =
      backend_ != nullptr && backend_->getExperimentReport(finalReport_);
}

void SimulationScreen::drawFinalReport() {
  if (!finalReportVisible_) {
    return;
  }
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(
      ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
             viewport->WorkPos.y + viewport->WorkSize.y * 0.5f),
      ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(
      ImVec2(viewport->WorkSize.x * 0.94f, viewport->WorkSize.y * 0.86f),
      ImGuiCond_Appearing);
  if (!ImGui::Begin("Итоговый отчёт за период", &finalReportVisible_)) {
    ImGui::End();
    return;
  }
  ImGui::Text("Моделирование завершено: %d дней", totalDays_);
  ImGui::Text("Подтверждение поставок: %s",
              automaticSupplierApproval_ ? "автоматическое" : "ручное");
  if (!finalReportLoaded_) {
    ImGui::TextWrapped("Не удалось получить итоговые данные из базы.");
    if (ImGui::Button("Повторить загрузку")) {
      loadFinalReport();
    }
    ImGui::End();
    return;
  }
  ImGui::Text("Завершённые заказы: %lld | Незавершённые: %lld",
              finalReport_.completedOrders, finalReport_.pendingOrders);
  ImGui::Text(
      "Доставлено в магазины: %lld шт. | Получено от поставщика: %lld шт.",
      finalReport_.deliveredUnits, finalReport_.suppliedUnits);
  ImGui::Text("Выручка по завершённым доставкам: %.2f руб.",
              finalReport_.revenueCents / 100.0);
  ImGui::Text("Снижение выручки из-за уценки: %.2f руб.",
              finalReport_.discountLossCents / 100.0);
  ImGui::Text("Списано: %lld шт. | Потери от списаний: %.2f руб.",
              finalReport_.writtenOffUnits,
              finalReport_.writeoffLossCents / 100.0);
  ImGui::Text(
      "Выручка за вычетом списаний: %.2f руб.",
      (finalReport_.revenueCents - finalReport_.writeoffLossCents) / 100.0);
  ImGui::TextWrapped(
      "Прибыль не рассчитана: закупочная стоимость и расходы не заданы. "
      "Уценка уже учтена в выручке и повторно не вычитается. "
      "Выручка отражает стоимость доставки со склада, а не продажи магазинов покупателям.");
  ImGui::Separator();
  if (!ImGui::BeginTabBar("FinalReportTabs")) {
    ImGui::End();
    return;
  }
  if (ImGui::BeginTabItem("График по дням")) {
    drawDailyFinanceChart();
    ImGui::EndTabItem();
  }
  if (ImGui::BeginTabItem("Товары")) {
    ImGui::TextWrapped(
        "Остатки склада показаны без резервов. Резерв относится к незавершённым "
        "заказам и не включён в доставку и выручку. Количества указаны в штуках.");
    const ImGuiTableFlags flags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
    if (ImGui::BeginTable("FinalProductResults", 11, flags,
                          ImVec2(0.0f, ImGui::GetContentRegionAvail().y),
                          1590.0f)) {
      ImGui::TableSetupColumn("Товар", ImGuiTableColumnFlags_WidthFixed,
                              200.0f);
      const char* headers[] = {"На складе, шт.",  "В магазинах, шт.",
                               "В резерве, шт.",  "Доставлено, шт.",
                               "Доставлено, уп.", "Выручка, руб.",
                               "Уценка, руб.",    "Списано, шт.",
                               "Списания, руб.",  "Поставлено, шт."};
      for (const char* header : headers) {
        ImGui::TableSetupColumn(header, ImGuiTableColumnFlags_WidthFixed,
                                135.0f);
      }
      ImGui::TableSetupScrollFreeze(1, 1);
      ImGui::TableHeadersRow();
      for (const ExperimentProductResult& item : finalReport_.products) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(item.productName.c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%lld", item.warehouseUnits);
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%lld", item.storeUnits);
        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%lld", item.pendingUnits);
        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%lld", item.deliveredUnits);
        const int pack = item.unitsPerPackage > 0 ? item.unitsPerPackage : 1;
        ImGui::TableSetColumnIndex(5);
        ImGui::Text("%lld + %lld шт.", item.deliveredUnits / pack,
                    item.deliveredUnits % pack);
        ImGui::TableSetColumnIndex(6);
        ImGui::Text("%.2f", item.revenueCents / 100.0);
        ImGui::TableSetColumnIndex(7);
        ImGui::Text("%.2f", item.discountLossCents / 100.0);
        ImGui::TableSetColumnIndex(8);
        ImGui::Text("%lld", item.writtenOffUnits);
        ImGui::TableSetColumnIndex(9);
        ImGui::Text("%.2f", item.writeoffLossCents / 100.0);
        ImGui::TableSetColumnIndex(10);
        ImGui::Text("%lld", item.suppliedUnits);
      }
      ImGui::EndTable();
    }
    ImGui::EndTabItem();
  }
  ImGui::EndTabBar();
  ImGui::End();
}

void SimulationScreen::recordDailyFinance() {
  if (currentDay_ < 1 || currentDay_ > totalDays_ || backend_ == nullptr) {
    return;
  }
  if (dailyFinance_.size() != static_cast<std::size_t>(totalDays_)) {
    dailyFinance_.assign(static_cast<std::size_t>(totalDays_), DailyFinance{});
  }
  ExperimentReport totals;
  if (!backend_->getExperimentReport(totals)) {
    return;
  }
  long long earlierRevenue = 0;
  long long earlierWriteoffs = 0;
  for (int day = 0; day < currentDay_ - 1; ++day) {
    if (!dailyFinance_[day].recorded) {
      // Do not invent daily values if an earlier snapshot is missing.
      return;
    }
    earlierRevenue += dailyFinance_[day].revenueCents;
    earlierWriteoffs += dailyFinance_[day].writeoffCents;
  }
  DailyFinance& today = dailyFinance_[currentDay_ - 1];
  today.revenueCents = totals.revenueCents - earlierRevenue;
  today.writeoffCents = totals.writeoffLossCents - earlierWriteoffs;
  today.recorded = true;
}

void SimulationScreen::drawDailyFinanceChart() {
  if (dailyFinance_.size() != static_cast<std::size_t>(totalDays_)) {
    ImGui::TextUnformatted("Нет истории дней. Начните новый эксперимент.");
    return;
  }
  double maximum = 1.0;
  long long revenueTotal = 0;
  long long writeoffTotal = 0;
  for (const DailyFinance& day : dailyFinance_) {
    if (!day.recorded) {
      ImGui::TextWrapped(
          "История дней загружена не полностью. "
          "График доступен после нового полного эксперимента.");
      return;
    }
    maximum = std::max(maximum,
                       std::max(day.revenueCents, day.writeoffCents) / 100.0);
    revenueTotal += day.revenueCents;
    writeoffTotal += day.writeoffCents;
  }
  if (revenueTotal != finalReport_.revenueCents ||
      writeoffTotal != finalReport_.writeoffLossCents) {
    ImGui::TextWrapped(
        "История дней не совпадает с итогами базы. "
        "Начните новый эксперимент для построения графика.");
    return;
  }
  maximum *= 1.1;
  ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.45f, 1.0f), "Выручка за день");
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.35f, 1.0f), "Списания за день");
  ImGui::TextWrapped(
      "Суммы в рублях. Наведите мышь на график для просмотра дня. "
      "Выручка учитывает только завершённые доставки; это не прибыль.");

  const float width = ImGui::GetContentRegionAvail().x;
  if (width < 130.0f) {
    ImGui::TextUnformatted("Расширьте окно для отображения графика.");
    return;
  }
  const float height = std::max(
      160.0f, std::min(280.0f, ImGui::GetContentRegionAvail().y - 10.0f));
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  ImGui::InvisibleButton("DailyFinancePlot", ImVec2(width, height));
  const bool hovered = ImGui::IsItemHovered();
  ImDrawList* list = ImGui::GetWindowDrawList();
  const float left = origin.x + 85.0f;
  const float right = origin.x + width - 18.0f;
  const float top = origin.y + 12.0f;
  const float bottom = origin.y + height - 30.0f;
  const ImU32 green = IM_COL32(77, 230, 115, 255);
  const ImU32 red = IM_COL32(255, 102, 89, 255);
  const ImU32 text = ImGui::GetColorU32(ImGuiCol_Text);
  const ImU32 grid = ImGui::GetColorU32(ImGuiCol_Border);
  for (int tick = 0; tick <= 4; ++tick) {
    const float y = bottom - (bottom - top) * tick / 4.0f;
    const std::string label =
        std::to_string(static_cast<long long>(maximum * tick / 4.0));
    list->AddLine(ImVec2(left, y), ImVec2(right, y), grid);
    list->AddText(ImVec2(origin.x + 2.0f, y - 8.0f), text, label.c_str());
  }
  const int count = static_cast<int>(dailyFinance_.size());
  const auto dayX = [=](int index) {
    return left + (right - left) * index / std::max(1, count - 1);
  };
  const auto moneyY = [=](long long cents) {
    return bottom -
           static_cast<float>(cents / 100.0 / maximum) * (bottom - top);
  };
  for (int tick = 0; tick <= 4; ++tick) {
    const int index = (count - 1) * tick / 4;
    const float x = dayX(index);
    list->AddLine(ImVec2(x, top), ImVec2(x, bottom), grid);
    const std::string label = "День " + std::to_string(index + 1);
    list->AddText(ImVec2(std::min(x, right - 60.0f), bottom + 7.0f), text,
                  label.c_str());
  }
  list->PushClipRect(ImVec2(left - 2.0f, top - 2.0f),
                     ImVec2(right + 2.0f, bottom + 2.0f), true);
  for (int i = 1; i < count; ++i) {
    list->AddLine(
        ImVec2(dayX(i - 1), moneyY(dailyFinance_[i - 1].revenueCents)),
        ImVec2(dayX(i), moneyY(dailyFinance_[i].revenueCents)), green, 2.0f);
    list->AddLine(
        ImVec2(dayX(i - 1), moneyY(dailyFinance_[i - 1].writeoffCents)),
        ImVec2(dayX(i), moneyY(dailyFinance_[i].writeoffCents)), red, 2.0f);
  }
  list->PopClipRect();
  if (hovered) {
    const float position = (ImGui::GetIO().MousePos.x - left) / (right - left);
    const int index = std::clamp(
        static_cast<int>(position * (count - 1) + 0.5f), 0, count - 1);
    list->AddLine(ImVec2(dayX(index), top), ImVec2(dayX(index), bottom), text);
    ImGui::BeginTooltip();
    ImGui::Text("День %d из %d", index + 1, count);
    ImGui::Text("Выручка: %.2f руб.",
                dailyFinance_[index].revenueCents / 100.0);
    ImGui::Text("Списания: %.2f руб.",
                dailyFinance_[index].writeoffCents / 100.0);
    ImGui::EndTooltip();
  }
}

void SimulationScreen::approveSupplierRequestsAutomatically() {
  if (!automaticSupplierApproval_ || backend_ == nullptr) {
    return;
  }
  supplierApprovalFailed_ = false;
  const std::vector<SupplierRequestInfo> requests =
      backend_->getSupplierRequests();
  for (const SupplierRequestInfo& request : requests) {
    if (request.status == "Created" &&
        !backend_->approveSupplierRequest(request.id, currentDay_)) {
      supplierApprovalFailed_ = true;
      std::cerr << "Automatic supplier approval failed: "
                << backend_->getLastError() << '\n';
    }
  }
}

void SimulationScreen::drawAmbientClouds(sf::RenderWindow& window) {
  const sf::View& view = window.getView();
  const sf::Vector2f size = view.getSize();
  const sf::Vector2f topLeft = view.getCenter() - size * 0.5f;
  if (size.x <= 0.0f || size.y <= 0.0f) {
    return;
  }
  for (int i = 0; i < 7; ++i) {
    const float span = size.x + 240.0f;
    const float x = topLeft.x - 120.0f +
                    static_cast<float>(std::fmod(
                        ambientTime_ * (6.0 + i * 0.7) + i * span / 7.0, span));
    const float y = topLeft.y + size.y * (0.20f + (i % 4) * 0.16f);
    for (int puff = 0; puff < 4; ++puff) {
      const float radius = (puff == 1 || puff == 2) ? 22.0f : 16.0f;
      sf::CircleShape cloud(radius, 24);
      cloud.setOrigin({radius, radius});
      cloud.setPosition({x + puff * 22.0f, y + (puff % 2 == 0 ? 5.0f : -3.0f)});
      cloud.setScale({1.7f, 0.75f});
      cloud.setFillColor(sf::Color(180, 199, 220, 15));
      window.draw(cloud);
    }
  }
}

void SimulationScreen::drawAmbientPlanes(sf::RenderWindow& window) {
  const sf::View& view = window.getView();
  const sf::Vector2f size = view.getSize();
  const sf::Vector2f topLeft = view.getCenter() - size * 0.5f;
  if (size.x <= 0.0f || size.y <= 0.0f) {
    return;
  }
  // Each polygon is convex so SFML can triangulate it correctly.
  const auto polygon = [&window](const sf::Transform& transform,
                                 std::initializer_list<sf::Vector2f> points,
                                 sf::Color color) {
    sf::ConvexShape shape(points.size());
    std::size_t index = 0;
    for (const sf::Vector2f point : points) {
      shape.setPoint(index++, transform.transformPoint(point));
    }
    shape.setFillColor(color);
    window.draw(shape);
  };
  for (int i = 0; i < 3; ++i) {
    const float speed = 36.0f + i * 11.0f;
    const float span = size.x + 240.0f;
    const float phase = static_cast<float>(
        std::fmod(ambientTime_ * speed + i * span / 3.0, span));
    const float direction = i == 1 ? -1.0f : 1.0f;
    float x = topLeft.x +
              (direction > 0.0f ? phase - 120.0f : size.x + 120.0f - phase);
    const double wave = ambientTime_ * 0.32 + i * 2.1;
    float y = topLeft.y + size.y * (0.22f + i * 0.23f) +
              static_cast<float>(std::sin(wave)) * 20.0f;
    const float verticalSpeed = static_cast<float>(std::cos(wave)) * 6.4f;
    float angle =
        std::atan2(verticalSpeed, direction * speed) * 180.0f / 3.14159265f;
    if (i < 2) {
      if (finaleState_ == FinaleState::Explosion ||
          finaleState_ == FinaleState::Done) {
        continue;
      }
      if (finaleState_ == FinaleState::Approach) {
        const float progress = std::clamp(finaleTimer_ / 2.5f, 0.0f, 1.0f);
        const sf::Vector2f start = i == 0 ? finaleStart_ : finaleSecondStart_;
        const sf::Vector2f target =
            i == 0 ? finaleTarget_ : finaleSecondTarget_;
        const sf::Vector2f directionToTower = target - start;
        const sf::Vector2f position = start + directionToTower * progress;
        x = position.x;
        y = position.y;
        angle = std::atan2(directionToTower.y, directionToTower.x) * 180.0f /
                3.14159265f;
      }
      if (i == 0) {
        lastPlanePosition_ = {x, y};
      } else {
        lastSecondPlanePosition_ = {x, y};
      }
    }
    sf::Transform transform;
    transform.translate({x, y});
    transform.rotate(sf::degrees(angle));
    transform.scale({0.60f + i * 0.07f, 0.60f + i * 0.07f});
    for (int segment = 0; segment < 8; ++segment) {
      const float start = -52.0f - segment * 14.0f;
      sf::Vertex trail[2];
      trail[0].position = transform.transformPoint({start, 0.0f});
      trail[1].position = transform.transformPoint({start - 9.0f, 0.0f});
      const sf::Color color(205, 225, 245,
                            static_cast<unsigned char>(55 - segment * 6));
      trail[0].color = color;
      trail[1].color = color;
      window.draw(trail, 2, sf::PrimitiveType::Lines);
    }
    // Wings, tail, fuselage and cockpit form a small aircraft silhouette.
    polygon(
        transform,
        {{8.0f, -3.0f}, {-14.0f, -30.0f}, {-23.0f, -30.0f}, {-12.0f, -3.0f}},
        sf::Color(170, 196, 219));
    polygon(transform,
            {{8.0f, 3.0f}, {-12.0f, 3.0f}, {-23.0f, 30.0f}, {-14.0f, 30.0f}},
            sf::Color(190, 215, 235));
    polygon(
        transform,
        {{-29.0f, -2.0f}, {-41.0f, -13.0f}, {-47.0f, -13.0f}, {-40.0f, -2.0f}},
        sf::Color(75, 160, 218));
    polygon(transform,
            {{-29.0f, 2.0f}, {-40.0f, 2.0f}, {-47.0f, 13.0f}, {-41.0f, 13.0f}},
            sf::Color(75, 160, 218));
    polygon(transform,
            {{42.0f, 0.0f},
             {28.0f, -5.0f},
             {-38.0f, -4.0f},
             {-46.0f, 0.0f},
             {-38.0f, 4.0f},
             {28.0f, 5.0f}},
            sf::Color(230, 240, 250));
    polygon(transform,
            {{24.0f, -3.5f}, {30.0f, -2.5f}, {30.0f, 2.5f}, {24.0f, 3.5f}},
            sf::Color(35, 90, 135));
    const bool flash = std::fmod(ambientTime_ + i * 0.3, 1.2) < 0.18;
    if (flash) {
      sf::CircleShape light(2.0f, 10);
      light.setOrigin({2.0f, 2.0f});
      light.setPosition(transform.transformPoint({-18.0f, -29.0f}));
      light.setFillColor(sf::Color(255, 90, 90));
      window.draw(light);
      light.setPosition(transform.transformPoint({-18.0f, 29.0f}));
      light.setFillColor(sf::Color(100, 245, 140));
      window.draw(light);
    }
  }
}

void SimulationScreen::beginFinalAnimation() {
  finalReportVisible_ = false;
  finaleTimer_ = 0.0f;
  finaleStart_ = lastPlanePosition_;
  finaleSecondStart_ = lastSecondPlanePosition_;
  finaleState_ = FinaleState::Approach;
}

void SimulationScreen::updateFinalAnimation(float deltaTime) {
  if (finaleState_ != FinaleState::Approach &&
      finaleState_ != FinaleState::Explosion) {
    return;
  }
  finaleTimer_ += deltaTime;
  if (finaleState_ == FinaleState::Approach && finaleTimer_ >= 2.5f) {
    finaleTimer_ -= 2.5f;
    finaleState_ = FinaleState::Explosion;
  }
  if (finaleState_ == FinaleState::Explosion && finaleTimer_ >= 3.2f) {
    finaleState_ = FinaleState::Done;
    finaleTimer_ = 0.0f;
    finalReportVisible_ = true;
  }
}

void SimulationScreen::drawFinalExplosion(sf::RenderWindow& window) {
  if (finaleState_ != FinaleState::Explosion) {
    return;
  }
  const float time = finaleTimer_;
  for (int tower = 0; tower < 2; ++tower) {
    const sf::Vector2f impact =
        tower == 0 ? finaleTarget_ : finaleSecondTarget_;
    const auto circle = [&window](sf::Vector2f position, float radius,
                                  sf::Color color) {
      sf::CircleShape shape(radius, 32);
      shape.setOrigin({radius, radius});
      shape.setPosition(position);
      shape.setFillColor(color);
      window.draw(shape);
    };
    // Expanding smoke puffs rise and fade away.
    for (int i = 0; i < 8; ++i) {
      const float phase = i * 2.39996f;
      const float alpha = std::max(0.0f, 1.0f - time / 3.2f);
      const sf::Vector2f position =
          impact +
          sf::Vector2f(std::cos(phase) * (10.0f + time * 14.0f),
                       std::sin(phase) * 12.0f - time * (20.0f + i * 2.0f));
      circle(
          position, 12.0f + time * 17.0f,
          sf::Color(110, 116, 130, static_cast<unsigned char>(alpha * 125.0f)));
    }
    if (time < 0.85f) {
      const float fade = 1.0f - time / 0.85f;
      circle(impact, 16.0f + time * 90.0f,
             sf::Color(255, 95, 20, static_cast<unsigned char>(fade * 210.0f)));
      circle(
          impact, 10.0f + time * 52.0f,
          sf::Color(255, 220, 95, static_cast<unsigned char>(fade * 245.0f)));
    }
    for (int i = 0; i < 28; ++i) {
      const float angle = i * 2.39996f;
      const float speed = 45.0f + (i % 6) * 17.0f;
      const sf::Vector2f position =
          impact +
          sf::Vector2f(std::cos(angle) * speed * time,
                       std::sin(angle) * speed * time + 27.0f * time * time);
      const float fade = std::max(0.0f, 1.0f - time / 2.0f);
      if (fade > 0.0f) {
        circle(position, 2.0f + (i % 3),
               sf::Color(255, static_cast<unsigned char>(140 + i % 100), 40,
                         static_cast<unsigned char>(fade * 255.0f)));
      }
    }
  }
}

void SimulationScreen::drawDecorativeTowers(sf::RenderWindow& window) {
  const sf::View& view = window.getView();
  const sf::Vector2f size = view.getSize();
  const sf::Vector2f topLeft = view.getCenter() - size * 0.5f;
  const float left = topLeft.x + size.x - 170.0f;
  const float ground = topLeft.y + 220.0f;
  const auto rectangle = [&window](sf::Vector2f position,
                                   sf::Vector2f dimensions, sf::Color color) {
    sf::RectangleShape shape(dimensions);
    shape.setPosition(position);
    shape.setFillColor(color);
    window.draw(shape);
  };
  rectangle({left - 10.0f, ground}, {145.0f, 7.0f}, sf::Color(70, 83, 100));
  for (int tower = 0; tower < 2; ++tower) {
    const float x = left + tower * 76.0f;
    const float height = tower == 0 ? 108.0f : 92.0f;
    const float y = ground - height;
    const sf::Vector2f impact(x + 24.0f, y + height * 0.40f);
    if (finaleState_ == FinaleState::None) {
      if (tower == 0) {
        finaleTarget_ = impact;
      } else {
        finaleSecondTarget_ = impact;
      }
    }
    const bool hit = finaleState_ == FinaleState::Explosion ||
                     finaleState_ == FinaleState::Done;
    rectangle({x + 5.0f, y + 5.0f}, {48.0f, height}, sf::Color(0, 0, 0, 65));
    rectangle({x, y}, {48.0f, height},
              hit ? sf::Color(91, 100, 113) : sf::Color(130, 153, 179));
    rectangle({x, y}, {48.0f, 5.0f}, sf::Color(180, 203, 225));
    rectangle({x + 38.0f, y + 5.0f}, {10.0f, height - 5.0f},
              sf::Color(93, 116, 142));
    for (int floor = 0; floor < 7; ++floor) {
      for (int column = 0; column < 3; ++column) {
        const float windowY = y + 12.0f + floor * 12.0f;
        if (windowY + 6.0f > ground - 12.0f) {
          continue;
        }
        const bool lit = (floor + column + tower) % 3 != 0;
        rectangle({x + 5.0f + column * 12.0f, windowY}, {7.0f, 6.0f},
                  lit ? sf::Color(238, 211, 130, 210) : sf::Color(49, 75, 104));
      }
    }
    rectangle({x + 19.0f, ground - 12.0f}, {10.0f, 12.0f},
              sf::Color(44, 59, 79));
    rectangle({x + 23.0f, y - 13.0f}, {2.0f, 13.0f}, sf::Color(184, 203, 220));
    if (std::fmod(ambientTime_ + tower * 0.4, 1.4) < 0.35) {
      rectangle({x + 22.0f, y - 15.0f}, {4.0f, 3.0f}, sf::Color(255, 85, 85));
    }
    if (hit) {
      sf::CircleShape mark(13.0f, 20);
      mark.setOrigin({13.0f, 13.0f});
      mark.setPosition(impact);
      mark.setScale({1.2f, 0.75f});
      mark.setFillColor(sf::Color(30, 32, 38, 190));
      window.draw(mark);
    }
  }
}

SimulationScreen::RoadSample SimulationScreen::sampleStoreRoad(
    int storeId, float progress, bool returning) const {
  const sf::Vector2f shop = getStorePosition(storeId);
  sf::Vector2f points[] = {warehouseCenter_,
                           {warehouseCenter_.x, shop.y + 55.0f},
                           {shop.x, shop.y + 55.0f},
                           shop};
  if (returning) {
    std::swap(points[0], points[3]);
    std::swap(points[1], points[2]);
  }
  float lengths[3] = {};
  float total = 0.0f;
  for (int i = 0; i < 3; ++i) {
    const sf::Vector2f delta = points[i + 1] - points[i];
    lengths[i] = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    total += lengths[i];
  }
  float remaining = std::clamp(progress, 0.0f, 1.0f) * total;
  for (int i = 0; i < 3; ++i) {
    if (lengths[i] < 0.001f) {
      continue;
    }
    if (remaining <= lengths[i] || i == 2) {
      const sf::Vector2f direction = (points[i + 1] - points[i]) / lengths[i];
      const sf::Vector2f lane(-direction.y * 9.0f, direction.x * 9.0f);
      return {points[i] + direction * std::min(remaining, lengths[i]) + lane,
              direction};
    }
    remaining -= lengths[i];
  }
  return {points[3], {1.0f, 0.0f}};
}

void SimulationScreen::drawRoadSegment(sf::RenderWindow& window,
                                       sf::Vector2f start, sf::Vector2f end) {
  const sf::Vector2f delta = end - start;
  const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
  if (length < 0.01f) {
    return;
  }
  const float angle = std::atan2(delta.y, delta.x) * 180.0f / 3.14159265f;
  const auto strip = [&window, start, angle](float width, float distance,
                                             float extent, float sideways,
                                             sf::Color color) {
    sf::RectangleShape shape({extent, width});
    shape.setOrigin({0.0f, width * 0.5f});
    sf::Transform transform;
    transform.translate(start);
    transform.rotate(sf::degrees(angle));
    shape.setPosition(transform.transformPoint({distance, sideways}));
    shape.setRotation(sf::degrees(angle));
    shape.setFillColor(color);
    window.draw(shape);
  };
  strip(68.0f, 0.0f, length, 0.0f, sf::Color(146, 147, 139));
  strip(56.0f, 0.0f, length, 0.0f, sf::Color(52, 57, 63));
  strip(1.5f, 0.0f, length, -25.0f, sf::Color(205, 209, 199));
  strip(1.5f, 0.0f, length, 25.0f, sf::Color(205, 209, 199));
  for (float distance = 10.0f; distance < length - 8.0f; distance += 32.0f) {
    strip(2.0f, distance, std::min(17.0f, length - distance), 0.0f,
          sf::Color(239, 217, 137));
  }
}

void SimulationScreen::drawMapBackground(sf::RenderWindow& window) {
  const sf::View& view = window.getView();
  const sf::Vector2f origin = view.getCenter() - view.getSize() * 0.5f;
  const sf::Vector2f size = view.getSize();
  const bool rebuild =
      cachedBackgroundStoreCount_ != static_cast<int>(stores_.size()) ||
      cachedBackgroundSize_ != size || cachedBackgroundOrigin_ != origin;
  if (rebuild) {
    backgroundMesh_.clear();
    const auto rectangle = [this](sf::Vector2f position,
                                  sf::Vector2f dimensions, sf::Color color) {
      const sf::Vector2f end = position + dimensions;
      const sf::Vector2f points[] = {position, {end.x, position.y},
                                     end,      position,
                                     end,      {position.x, end.y}};
      for (const sf::Vector2f point : points) {
        sf::Vertex vertex;
        vertex.position = point;
        vertex.color = color;
        backgroundMesh_.append(vertex);
      }
    };
    rectangle(origin, size, sf::Color(67, 88, 70));
    for (int row = 0; row < 14; ++row) {
      for (int column = 0; column < 25; ++column) {
        rectangle(origin + sf::Vector2f(column * 56.0f, row * 56.0f),
                  {54.0f, 54.0f},
                  (row + column) % 2 == 0 ? sf::Color(73, 94, 75, 80)
                                          : sf::Color(58, 80, 64, 65));
      }
    }
    // Concrete yards around warehouses and paved shop entrances.
    rectangle(warehouseCenter_ - sf::Vector2f(115.0f, 78.0f), {230.0f, 156.0f},
              sf::Color(128, 133, 126));
    rectangle(supplierCenter_ - sf::Vector2f(83.0f, 65.0f), {166.0f, 130.0f},
              sf::Color(123, 128, 122));
    for (const StoreView& store : stores_) {
      rectangle(store.position - sf::Vector2f(52.0f, 42.0f), {104.0f, 86.0f},
                sf::Color(145, 145, 130));
    }
    cachedBackgroundStoreCount_ = static_cast<int>(stores_.size());
    cachedBackgroundSize_ = size;
    cachedBackgroundOrigin_ = origin;
  }
  window.draw(backgroundMesh_);
  for (int i = 0; i < 15; ++i) {
    const sf::Vector2f position(origin.x + 30.0f + (i % 3) * 47.0f,
                                origin.y + 105.0f + (i / 3) * 67.0f);
    sf::CircleShape shadow(14.0f, 18);
    shadow.setOrigin({14.0f, 14.0f});
    shadow.setPosition(position + sf::Vector2f(4.0f, 5.0f));
    shadow.setFillColor(sf::Color(25, 40, 31, 110));
    window.draw(shadow);
    sf::CircleShape crown(14.0f, 18);
    crown.setOrigin({14.0f, 14.0f});
    crown.setPosition(position);
    crown.setFillColor(sf::Color(43, 107 + (i % 3) * 9, 58));
    window.draw(crown);
    crown.setRadius(8.0f);
    crown.setOrigin({8.0f, 8.0f});
    crown.setPosition(position - sf::Vector2f(3.0f, 3.0f));
    crown.setFillColor(sf::Color(74, 139, 74));
    window.draw(crown);
  }
}

bool SimulationScreen::processThreeHourSales(int intervalsDue) {
  const int before = salesIntervalsProcessed_;
  const int target = std::clamp(intervalsDue, 0, 8);
  while (salesIntervalsProcessed_ < target) {
    if (backend_ == nullptr || !backend_->simulateStoreSales(
                                   currentDay_, salesIntervalsProcessed_ + 1)) {
      simulationState_ = SimulationState::Paused;
      if (backend_ != nullptr) {
        std::cerr << "Store sales interval failed: " << backend_->getLastError()
                  << '\n';
      }
      return false;
    }
    ++salesIntervalsProcessed_;
  }
  if (salesIntervalsProcessed_ != before && backend_ != nullptr) {
    backend_->processStoreOrders(currentDay_);
    backend_->processSupplierRequests(currentDay_);
    approveSupplierRequestsAutomatically();
  }
  if (salesIntervalsProcessed_ != before &&
      selectedType_ == SelectedObjectType::Store) {
    refreshSelectedStoreData();
  }
  return true;
}

void SimulationScreen::drawSupplySummary() {
  ImGui::TextUnformatted("Все выбранные товары склада");
  ImGui::TextWrapped(
      "Перетащите InfoPanel за заголовок; размер можно менять за край. Закройте крестиком и откройте кликом по зданию. "
      "Нужно магазинам — нехватка до запаса 0.9 дня либо больший объём открытых заказов. "
      "Все количества в штуках. «Едет к складу» — суммарный груз оранжевых машин, направляющихся к складу. Возвращающиеся машины пустые.");
  if (!supplySummaryLoaded_) {
    ImGui::TextUnformatted("Не удалось загрузить сводку из базы.");
    return;
  }
  ImGui::Text("Видов товаров: %d", static_cast<int>(supplySummary_.size()));
  const ImGuiTableFlags flags =
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX |
      ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
  if (ImGui::BeginTable(
          "WarehouseSupplySummary", 5, flags,
          ImVec2(0.0f, std::max(140.0f, ImGui::GetContentRegionAvail().y)),
          1050.0f)) {
    ImGui::TableSetupColumn("Товар", ImGuiTableColumnFlags_WidthFixed, 220.0f);
    const char* headers[] = {"На складе", "Нужно магазинам",
                             "Подтверждено, ждёт выезда", "Едет к складу"};
    for (const char* header : headers) {
      ImGui::TableSetupColumn(header, ImGuiTableColumnFlags_WidthFixed, 150.0f);
    }
    ImGui::TableSetupScrollFreeze(1, 1);
    ImGui::TableHeadersRow();
    for (const WarehouseSupplySummary& row : supplySummary_) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(row.productName.c_str());
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%lld", row.warehouseUnits);
      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%lld", row.storeNeed);
      const long long cargo = currentSupplierCargoUnits(row.productId);
      ImGui::TableSetColumnIndex(3);
      ImGui::Text("%lld", std::max(0LL, row.inTransitUnits - cargo));
      ImGui::TableSetColumnIndex(4);
      ImGui::Text("%lld", cargo);
    }
    ImGui::EndTable();
  }
}

long long SimulationScreen::currentSupplierCargoUnits(int productId) const {
  long long quantity = 0;
  for (const SupplierFlight& flight : supplierFlights_) {
    if (!flight.truck.active || flight.truck.returning) {
      continue;
    }
    for (const SupplierRequestInfo& request : supplierRequests_) {
      if (request.productId == productId && request.status == "InTransit" &&
          std::find(flight.requestIds.begin(), flight.requestIds.end(),
                    request.id) != flight.requestIds.end()) {
        quantity += request.requestedQuantity;
      }
    }
  }
  return quantity;
}

void SimulationScreen::syncSupplierFleet() {
  supplierTruck_.active = false;
  activeSupplierRequestId_ = -1;
  activeSupplierRequestIds_.clear();
  activeSupplierProductIds_.clear();
  bool outboundFound = false;
  for (const SupplierFlight& flight : supplierFlights_) {
    if (!supplierTruck_.active || (!flight.truck.returning && !outboundFound)) {
      supplierTruck_ = flight.truck;
    }
    if (flight.truck.returning) {
      continue;
    }
    outboundFound = true;
    for (const int id : flight.requestIds) {
      activeSupplierRequestIds_.push_back(id);
    }
    for (const int id : flight.productIds) {
      if (std::find(activeSupplierProductIds_.begin(),
                    activeSupplierProductIds_.end(),
                    id) == activeSupplierProductIds_.end()) {
        activeSupplierProductIds_.push_back(id);
      }
    }
  }
  if (!activeSupplierRequestIds_.empty()) {
    activeSupplierRequestId_ = activeSupplierRequestIds_.front();
  }
}

void SimulationScreen::drawSupplierTruck(sf::RenderWindow& window) {
  const SupplierTruckView saved = supplierTruck_;
  for (const SupplierFlight& flight : supplierFlights_) {
    supplierTruck_ = flight.truck;
    drawSupplierTruckSprite(window);
  }
  supplierTruck_ = saved;
}
