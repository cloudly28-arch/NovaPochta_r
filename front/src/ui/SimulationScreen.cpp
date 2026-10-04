#include "ui/SimulationScreen.h"

#include <imgui.h>
#include <iostream>
#include <cmath>
#include <algorithm>

void SimulationScreen::update(float deltaTime)
{
    if (
        simulationState_ ==
        SimulationState::Running
    )
    {
        updateVehicles(deltaTime);
    }
}

void SimulationScreen::draw(
    sf::RenderWindow& window,
    const SimulationSettings& settings
)
{
    loadTextures();
    ensureStoresCreated(settings.stores);

    handleMouseClick(window);

    drawRoutes(window);
    drawWarehouse(window);
    drawStores(window);
    drawVehicles(window);

    drawTopBar(settings);
    drawBottomBar();
    drawInfoPanel();
}

void SimulationScreen::ensureStoresCreated(
    int storeCount
)
{
    if (
        static_cast<int>(stores_.size())
        == storeCount
    )
    {
        return;
    }

    stores_.clear();

    const float radiusX = 330.0f;
    const float radiusY = 220.0f;

    for (int i = 0; i < storeCount; ++i)
    {
        const float angle =
            static_cast<float>(i) /
            static_cast<float>(storeCount) *
            2.0f *
            3.14159265f;

        StoreView store;

        store.id = i + 1;

        store.position =
            sf::Vector2f(
                warehouseCenter_.x +
                    std::cos(angle) *
                    radiusX,

                warehouseCenter_.y +
                    std::sin(angle) *
                    radiusY
            );

        store.bounds =
            sf::FloatRect(
                sf::Vector2f(
                    store.position.x - 40.0f,
                    store.position.y - 30.0f
                ),
                sf::Vector2f(
                    80.0f,
                    60.0f
                )
            );

        stores_.push_back(store);
    }

    vehicles_.clear();

    for (const StoreView& store : stores_)
    {
        VehicleView vehicle;

        vehicle.targetStoreId = store.id;
        vehicle.position = warehouseCenter_;
        vehicle.progress = 0.0f;

        vehicle.direction =
            VehicleDirection::ToStore;

        vehicle.active = true;

        vehicle.startDelay =
            static_cast<float>(
                store.id - 1
            ) * 0.7f;

        vehicles_.push_back(vehicle);
    }
}



void SimulationScreen::drawTopBar(
    const SimulationSettings& settings
)
{
    const ImVec2 displaySize =
        ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowPos(
        ImVec2(0.0f, 0.0f),
        ImGuiCond_Always
    );

    ImGui::SetNextWindowSize(
        ImVec2(
            displaySize.x,
            70.0f
        ),
        ImGuiCond_Always
    );

    ImGui::Begin(
        "TopBar",
        nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse
    );

    ImGui::Text("NOVA WAREHOUSE");

    ImGui::SameLine(280.0f);

    ImGui::Text(
        "Day: 1 / %d",
        settings.days
    );

    ImGui::SameLine(450.0f);

    ImGui::Text(
        "Stores: %d",
        settings.stores
    );

    ImGui::SameLine(600.0f);

    ImGui::Text(
        "Products: %d",
        settings.products
    );

    ImGui::SameLine(800.0f);

    if (
        simulationState_ ==
        SimulationState::Running
    )
    {
        ImGui::Text("Status: RUNNING");
    }
    else if (
        simulationState_ ==
        SimulationState::Paused
    )
    {
        ImGui::Text("Status: PAUSED");
    }
    else
    {
        ImGui::Text("Status: STOPPED");
    }

    ImGui::End();
}

void SimulationScreen::drawBottomBar()
{
    const float height = 80.0f;

    const ImVec2 displaySize =
        ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowPos(
        ImVec2(
            0.0f,
            displaySize.y - height
        ),
        ImGuiCond_Always
    );

    ImGui::SetNextWindowSize(
        ImVec2(
            displaySize.x,
            height
        ),
        ImGuiCond_Always
    );

    ImGui::Begin(
        "BottomBar",
        nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse
    );

    if (
        ImGui::Button(
            "START",
            ImVec2(100.0f, 40.0f)
        )
    )
    {
        simulationState_ =
            SimulationState::Running;
    }

    ImGui::SameLine();

    if (
        ImGui::Button(
            "PAUSE",
            ImVec2(100.0f, 40.0f)
        )
    )
    {
        if (
            simulationState_ ==
            SimulationState::Running
        )
        {
            simulationState_ =
                SimulationState::Paused;
        }
    }

    ImGui::SameLine();

    if (
        ImGui::Button(
            "STOP",
            ImVec2(100.0f, 40.0f)
        )
    )
    {
        simulationState_ =
            SimulationState::Stopped;

        for (VehicleView& vehicle : vehicles_)
        {
            vehicle.progress = 0.0f;

            vehicle.direction =
                VehicleDirection::ToStore;

            vehicle.position =
                warehouseCenter_;

            vehicle.startDelay =
                static_cast<float>(
                    vehicle.targetStoreId - 1
                ) * 0.7f;
        }
    }

    ImGui::SameLine(400.0f);

    ImGui::Text("Speed:");

    ImGui::SameLine();

    if (ImGui::Button("1x"))
    {
        simulationSpeed_ = 1.0f;
    }

    ImGui::SameLine();

    if (ImGui::Button("2x"))
    {
        simulationSpeed_ = 2.0f;
    }

    ImGui::SameLine();

    if (ImGui::Button("5x"))
    {
        simulationSpeed_ = 5.0f;
    }

    ImGui::SameLine();

    if (ImGui::Button("10x"))
    {
        simulationSpeed_ = 10.0f;
    }

    ImGui::SameLine(750.0f);

    ImGui::Text(
        "Current speed: %.0fx",
        simulationSpeed_
    );

    ImGui::End();
}

void SimulationScreen::drawRoutes(
    sf::RenderWindow& window
)
{
    for (
        const StoreView& store :
        stores_
    )
    {
        sf::VertexArray route(
            sf::PrimitiveType::Lines,
            2
        );

        route[0].position =
            warehouseCenter_;

        route[0].color =
            sf::Color(
                90,
                90,
                100
            );

        route[1].position =
            store.position;

        route[1].color =
            sf::Color(
                90,
                90,
                100
            );

        window.draw(route);
    }
}

void SimulationScreen::drawWarehouse(
    sf::RenderWindow& window
)
{
    const sf::Vector2f size(
        180.0f,
        120.0f
    );

    const sf::Vector2f position(
        warehouseCenter_.x - size.x / 2.0f,
        warehouseCenter_.y - size.y / 2.0f
    );

    if (warehouseTextureLoaded_)
    {
        sf::Sprite warehouse(warehouseTexture_);

        const sf::Vector2u textureSize =
            warehouseTexture_.getSize();

        warehouse.setOrigin(
            sf::Vector2f(
                textureSize.x / 2.0f,
                textureSize.y / 2.0f
            )
        );

        warehouse.setPosition(
            warehouseCenter_
        );

        warehouse.setScale(
            sf::Vector2f(
                size.x /
                    static_cast<float>(textureSize.x),

                size.y /
                    static_cast<float>(textureSize.y)
            )
        );

        window.draw(warehouse);

        warehouseBounds_ =
            warehouse.getGlobalBounds();
    }
    else
    {
        sf::RectangleShape warehouse(size);

        warehouse.setPosition(position);

        warehouse.setFillColor(
            sf::Color(
                170,
                20,
                30
            )
        );

        warehouse.setOutlineThickness(
            4.0f
        );

        if (
            selectedType_ ==
            SelectedObjectType::Warehouse
        )
        {
            warehouse.setOutlineColor(
                sf::Color(
                    255,
                    200,
                    40
                )
            );
        }
        else
        {
            warehouse.setOutlineColor(
                sf::Color::White
            );
        }

        window.draw(warehouse);

        warehouseBounds_ =
            warehouse.getGlobalBounds();
    }
}

void SimulationScreen::drawStores(
    sf::RenderWindow& window
)
{
    for (
        const StoreView& storeView :
        stores_
    )
    {
        const sf::Vector2f size(
            80.0f,
            60.0f
        );

        if (storeTextureLoaded_)
        {
            sf::Sprite store(
                storeTexture_
            );

            const sf::Vector2u textureSize =
                storeTexture_.getSize();

            store.setOrigin(
                sf::Vector2f(
                    textureSize.x / 2.0f,
                    textureSize.y / 2.0f
                )
            );

            store.setPosition(
                storeView.position
            );

            store.setScale(
                sf::Vector2f(
                    size.x /
                        static_cast<float>(
                            textureSize.x
                        ),

                    size.y /
                        static_cast<float>(
                            textureSize.y
                        )
                )
            );

            if (
                selectedType_ ==
                    SelectedObjectType::Store &&
                selectedStoreId_ ==
                    storeView.id
            )
            {
                store.setColor(
                    sf::Color(
                        255,
                        220,
                        120
                    )
                );
            }

            window.draw(store);
        }
        else
        {
            sf::RectangleShape store(
                size
            );

            store.setPosition(
                sf::Vector2f(
                    storeView.position.x -
                        size.x / 2.0f,

                    storeView.position.y -
                        size.y / 2.0f
                )
            );

            if (
                selectedType_ ==
                    SelectedObjectType::Store &&
                selectedStoreId_ ==
                    storeView.id
            )
            {
                store.setFillColor(
                    sf::Color(
                        230,
                        180,
                        40
                    )
                );
            }
            else
            {
                store.setFillColor(
                    sf::Color(
                        220,
                        220,
                        220
                    )
                );
            }

            store.setOutlineThickness(
                2.0f
            );

            store.setOutlineColor(
                sf::Color(
                    120,
                    120,
                    120
                )
            );

            window.draw(store);
        }
    }
}

void SimulationScreen::drawVehicles(
    sf::RenderWindow& window
)
{
    for (const VehicleView& vehicle : vehicles_)
    {
        if (!vehicle.active)
        {
            continue;
        }

        if (vehicle.startDelay > 0.0f)
        {
            continue;
        }

        const sf::Vector2f storePosition =
            getStorePosition(
                vehicle.targetStoreId
            );

        sf::Vector2f direction;

        if (
            vehicle.direction ==
            VehicleDirection::ToStore
        )
        {
            direction =
                storePosition -
                warehouseCenter_;
        }
        else
        {
            direction =
                warehouseCenter_ -
                storePosition;
        }

        const float angle =
            std::atan2(
                direction.y,
                direction.x
            ) *
            180.0f /
            3.14159265f;

        if (truckTextureLoaded_)
        {
            sf::Sprite truck(
                truckTexture_
            );

            const sf::Vector2u textureSize =
                truckTexture_.getSize();

            truck.setOrigin(
                sf::Vector2f(
                    textureSize.x / 2.0f,
                    textureSize.y / 2.0f
                )
            );

            truck.setPosition(
                vehicle.position
            );

            const float desiredWidth =
                45.0f;

            const float scale =
                desiredWidth /
                static_cast<float>(
                    textureSize.x
                );

            truck.setScale(
                sf::Vector2f(
                    scale,
                    scale
                )
            );

            truck.setRotation(
                sf::degrees(angle)
            );

            if (
                vehicle.direction ==
                VehicleDirection::ToStore
            )
            {
                truck.setColor(
                    sf::Color(
                        120,
                        255,
                        150
                    )
                );
            }
            else
            {
                truck.setColor(
                    sf::Color(
                        140,
                        190,
                        255
                    )
                );
            }

            window.draw(truck);
        }
        else
        {
            sf::RectangleShape shape(
                sf::Vector2f(
                    35.0f,
                    20.0f
                )
            );

            shape.setOrigin(
                sf::Vector2f(
                    17.5f,
                    10.0f
                )
            );

            shape.setPosition(
                vehicle.position
            );

            shape.setRotation(
                sf::degrees(angle)
            );

            if (
                vehicle.direction ==
                VehicleDirection::ToStore
            )
            {
                shape.setFillColor(
                    sf::Color(
                        40,
                        190,
                        90
                    )
                );
            }
            else
            {
                shape.setFillColor(
                    sf::Color(
                        70,
                        130,
                        230
                    )
                );
            }

            window.draw(shape);
        }
    }
}

void SimulationScreen::updateVehicles(
    float deltaTime
)
{
    if (stores_.empty())
    {
        return;
    }

    for (VehicleView& vehicle : vehicles_)
    {
        if (!vehicle.active)
        {
            continue;
        }

        if (vehicle.startDelay > 0.0f)
        {
            vehicle.startDelay -=
                deltaTime * simulationSpeed_;

            continue;
        }

        const sf::Vector2f storePosition =
            getStorePosition(
                vehicle.targetStoreId
            );

        const float movementSpeed =
            0.20f * simulationSpeed_;

        vehicle.progress +=
            movementSpeed * deltaTime;

        if (vehicle.progress >= 1.0f)
        {
            vehicle.progress = 0.0f;

            if (
                vehicle.direction ==
                VehicleDirection::ToStore
            )
            {
                vehicle.direction =
                    VehicleDirection::ToWarehouse;
            }
            else
            {
                vehicle.direction =
                    VehicleDirection::ToStore;

                vehicle.startDelay = 1.0f;
            }
        }

        sf::Vector2f start;
        sf::Vector2f end;

        if (
            vehicle.direction ==
            VehicleDirection::ToStore
        )
        {
            start = warehouseCenter_;
            end = storePosition;
        }
        else
        {
            start = storePosition;
            end = warehouseCenter_;
        }

        vehicle.position =
            start +
            (end - start) *
            vehicle.progress;
    }
}

sf::Vector2f
SimulationScreen::getStorePosition(
    int storeId
) const
{
    for (
        const StoreView& store :
        stores_
    )
    {
        if (store.id == storeId)
        {
            return store.position;
        }
    }

    return warehouseCenter_;
}

void SimulationScreen::handleMouseClick(
    sf::RenderWindow& window
)
{
    const bool mousePressed =
        sf::Mouse::isButtonPressed(
            sf::Mouse::Button::Left
        );

    if (
        mousePressed &&
        !mouseWasPressed_
    )
    {
        const sf::Vector2i pixelPosition =
            sf::Mouse::getPosition(
                window
            );

        const sf::Vector2f mousePosition =
            window.mapPixelToCoords(
                pixelPosition
            );

        if (
            warehouseBounds_.contains(
                mousePosition
            )
        )
        {
            selectedType_ =
                SelectedObjectType::Warehouse;

            selectedStoreId_ = -1;
        }
        else
        {
            for (
                const StoreView& store :
                stores_
            )
            {
                if (
                    store.bounds.contains(
                        mousePosition
                    )
                )
                {
                    selectedType_ =
                        SelectedObjectType::Store;

                    selectedStoreId_ =
                        store.id;

                    break;
                }
            }
        }
    }

    mouseWasPressed_ =
        mousePressed;
}

void SimulationScreen::drawInfoPanel()
{
    if (
        selectedType_ ==
        SelectedObjectType::None
    )
    {
        return;
    }

    const ImVec2 displaySize =
        ImGui::GetIO().DisplaySize;

    const float panelWidth =
        320.0f;

    const float topHeight =
        70.0f;

    const float bottomHeight =
        80.0f;

    ImGui::SetNextWindowPos(
        ImVec2(
            displaySize.x -
                panelWidth,

            topHeight
        ),
        ImGuiCond_Always
    );

    ImGui::SetNextWindowSize(
        ImVec2(
            panelWidth,

            displaySize.y -
                topHeight -
                bottomHeight
        ),
        ImGuiCond_Always
    );

    ImGui::Begin(
        "InfoPanel",
        nullptr,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse
    );

    if (
        selectedType_ ==
        SelectedObjectType::Warehouse
    )
    {
        ImGui::Text("WAREHOUSE");

        ImGui::Separator();

        ImGui::Text(
            "Status: Working"
        );

        ImGui::Spacing();

        if (
            ImGui::BeginTabBar(
                "WarehouseTabs"
            )
        )
        {
            if (
                ImGui::BeginTabItem(
                    "Products"
                )
            )
            {
                ImGui::Text(
                    "Product data will be here."
                );

                ImGui::EndTabItem();
            }

            if (
                ImGui::BeginTabItem(
                    "Orders"
                )
            )
            {
                ImGui::Text(
                    "Order data will be here."
                );

                ImGui::EndTabItem();
            }

            if (
                ImGui::BeginTabItem(
                    "Supply"
                )
            )
            {
                ImGui::Text(
                    "Supply data will be here."
                );

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    else if (
        selectedType_ ==
        SelectedObjectType::Store
    )
    {
        ImGui::Text(
            "STORE #%d",
            selectedStoreId_
        );

        ImGui::Separator();

        ImGui::Text(
            "Status: Working"
        );

        ImGui::Spacing();

        ImGui::Text(
            "Current stock:"
        );

        ImGui::BulletText(
            "Product A: 24"
        );

        ImGui::BulletText(
            "Product B: 12"
        );

        ImGui::BulletText(
            "Product C: 30"
        );

        ImGui::Spacing();

        ImGui::Text(
            "Current order:"
        );

        ImGui::Text(
            "No active order"
        );
    }

    ImGui::End();
}

void SimulationScreen::loadTextures()
{
    if (texturesLoaded_)
    {
        return;
    }

    texturesLoaded_ = true;

    const auto warehousePath =
        assetsPath_ / "textures" / "warehouse.png";

    const auto storePath =
        assetsPath_ / "textures" / "store.png";

    const auto truckPath =
        assetsPath_ / "textures" / "truck.png";

    warehouseTextureLoaded_ =
        warehouseTexture_.loadFromFile(
            warehousePath.string()
        );

    storeTextureLoaded_ =
        storeTexture_.loadFromFile(
            storePath.string()
        );

    truckTextureLoaded_ =
        truckTexture_.loadFromFile(
            truckPath.string()
        );
}

void SimulationScreen::setAssetsPath(
    const std::filesystem::path& path
)
{
    assetsPath_ = path;
}