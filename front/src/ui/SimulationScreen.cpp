#include "ui/SimulationScreen.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <iostream>

void SimulationScreen::update(
    float deltaTime
)
{
    refreshWarehouseData();
    refreshSupplierData();
    if (
        simulationState_ !=
        SimulationState::Running
    )
    {
        return;
    }

    updateVehicles(deltaTime);
    updateSupplierTruck(deltaTime);

    dayTimer_ +=
        deltaTime * simulationSpeed_;

    if (
        dayTimer_ >=
        secondsPerDay_
    )
    {
        dayTimer_ -=
            secondsPerDay_;

        advanceDay();
    }
}

void SimulationScreen::draw(
    sf::RenderWindow& window,
    const SimulationSettings& settings
)
{
    loadTextures();

    ensureStoresCreated(
        settings.stores
    );

    handleMouseClick(window);

    drawRoutes(window);
    drawSupplierRoute(window);

    drawWarehouse(window);
    drawSupplier(window);

    drawStores(window);

    drawVehicles(window);
    drawSupplierTruck(window);

    drawTopBar(settings);
    drawBottomBar();
    drawInfoPanel();
}

void SimulationScreen::ensureStoresCreated(
    int storeCount
)
{
    if (
        static_cast<int>(
            stores_.size()
        ) == storeCount
    )
    {
        return;
    }

    stores_.clear();

    const float radiusX = 330.0f;
    const float radiusY = 220.0f;

    for (
        int i = 0;
        i < storeCount;
        ++i
    )
    {
        const float angle =
            static_cast<float>(i) /
            static_cast<float>(
                storeCount
            ) *
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
                    store.position.x -
                        40.0f,

                    store.position.y -
                        30.0f
                ),
                sf::Vector2f(
                    80.0f,
                    60.0f
                )
            );

        stores_.push_back(
            store
        );
    }

    vehicles_.clear();

    for (
        const StoreView& store :
        stores_
    )
    {
        VehicleView vehicle;

        vehicle.targetStoreId =
            store.id;

        vehicle.position =
            warehouseCenter_;

        vehicle.progress = 0.0f;

        vehicle.direction =
            VehicleDirection::ToStore;

        vehicle.active = true;

        vehicle.startDelay =
            static_cast<float>(
                store.id - 1
            ) * 0.7f;

        vehicles_.push_back(
            vehicle
        );
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

    ImGui::Text(
        "NOVA WAREHOUSE"
    );

    ImGui::SameLine(280.0f);

    ImGui::Text(
        "Day: %d / %d",
        currentDay_,
        totalDays_
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
        ImGui::Text(
            "Status: RUNNING"
        );
    }
    else if (
        simulationState_ ==
        SimulationState::Paused
    )
    {
        ImGui::Text(
            "Status: PAUSED"
        );
    }
    else if (
        simulationState_ ==
        SimulationState::Finished
    )
    {
        ImGui::Text(
            "Status: FINISHED"
        );
    }
    else
    {
        ImGui::Text(
            "Status: STOPPED"
        );
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
            displaySize.y -
                height
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
            ImVec2(
                100.0f,
                40.0f
            )
        )
    )
    {
        if (
            simulationState_ ==
            SimulationState::Finished
        )
        {
            currentDay_ = 1;
            dayTimer_ = 0.0f;

            for (
                VehicleView& vehicle :
                vehicles_
            )
            {
                vehicle.progress =
                    0.0f;

                vehicle.position =
                    warehouseCenter_;

                vehicle.direction =
                    VehicleDirection::ToStore;

                vehicle.startDelay =
                    static_cast<float>(
                        vehicle.targetStoreId -
                        1
                    ) * 0.7f;
            }
        }

        simulationState_ =
            SimulationState::Running;
    }

    ImGui::SameLine();

    if (
        ImGui::Button(
            "PAUSE",
            ImVec2(
                100.0f,
                40.0f
            )
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
            ImVec2(
                100.0f,
                40.0f
            )
        )
    )
    {
        simulationState_ =
            SimulationState::Stopped;

        currentDay_ = 1;
        dayTimer_ = 0.0f;

        for (
            VehicleView& vehicle :
            vehicles_
        )
        {
            vehicle.progress = 0.0f;

            vehicle.direction =
                VehicleDirection::ToStore;

            vehicle.position =
                warehouseCenter_;

            vehicle.startDelay =
                static_cast<float>(
                    vehicle.targetStoreId -
                    1
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

    ImGui::SameLine(900.0f);

    float dayProgress =
        dayTimer_ /
        secondsPerDay_;

    dayProgress =
        std::clamp(
            dayProgress,
            0.0f,
            1.0f
        );

    ImGui::ProgressBar(
        dayProgress,
        ImVec2(
            200.0f,
            20.0f
        )
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
void SimulationScreen::drawSupplierRoute(
    sf::RenderWindow& window
)
{
    sf::VertexArray route(
        sf::PrimitiveType::Lines,
        2
    );

    route[0].position =
        supplierCenter_;

    route[0].color =
        sf::Color(
            220,
            150,
            40
        );

    route[1].position =
        warehouseCenter_;

    route[1].color =
        sf::Color(
            220,
            150,
            40
        );

    window.draw(route);
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
        warehouseCenter_.x -
            size.x / 2.0f,

        warehouseCenter_.y -
            size.y / 2.0f
    );

    if (warehouseTextureLoaded_)
    {
        sf::Sprite warehouse(
            warehouseTexture_
        );

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
                    static_cast<float>(
                        textureSize.x
                    ),

                size.y /
                    static_cast<float>(
                        textureSize.y
                    )
            )
        );

        window.draw(warehouse);

        warehouseBounds_ =
            warehouse.getGlobalBounds();
    }
    else
    {
        sf::RectangleShape warehouse(
            size
        );

        warehouse.setPosition(
            position
        );

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
void SimulationScreen::drawSupplier(
    sf::RenderWindow& window
)
{
    const sf::Vector2f size(
        140.0f,
        90.0f
    );

    sf::RectangleShape supplier(
        size
    );

    supplier.setOrigin(
        sf::Vector2f(
            size.x / 2.0f,
            size.y / 2.0f
        )
    );

    supplier.setPosition(
        supplierCenter_
    );

    supplier.setFillColor(
        sf::Color(
            230,
            140,
            30
        )
    );

    supplier.setOutlineThickness(
        4.0f
    );

    if (
        selectedType_ ==
        SelectedObjectType::Supplier
    )
    {
        supplier.setOutlineColor(
            sf::Color(
                255,
                255,
                100
            )
        );
    }
    else
    {
        supplier.setOutlineColor(
            sf::Color::White
        );
    }

    window.draw(supplier);

    supplierBounds_ =
        supplier.getGlobalBounds();
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
    for (
        const VehicleView& vehicle :
        vehicles_
    )
    {
        if (!vehicle.active)
        {
            continue;
        }

        if (
            vehicle.startDelay >
            0.0f
        )
        {
            continue;
        }

        const sf::Vector2f
            storePosition =
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

            const sf::Vector2u
                textureSize =
                    truckTexture_.getSize();

            truck.setOrigin(
                sf::Vector2f(
                    textureSize.x /
                        2.0f,

                    textureSize.y /
                        2.0f
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
void SimulationScreen::drawSupplierTruck(
    sf::RenderWindow& window
)
{
    if (!supplierTruck_.active)
    {
        return;
    }

    sf::Vector2f direction;

    if (!supplierTruck_.returning)
    {
        direction =
            warehouseCenter_ -
            supplierCenter_;
    }
    else
    {
        direction =
            supplierCenter_ -
            warehouseCenter_;
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
            supplierTruck_.position
        );

        // Грузовик поставщика специально больше обычного.
        const float desiredWidth =
            75.0f;

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

        // Отдельный цвет поставщика.
        truck.setColor(
            sf::Color(
                255,
                170,
                60
            )
        );

        window.draw(truck);
    }
    else
    {
        sf::RectangleShape truck(
            sf::Vector2f(
                65.0f,
                30.0f
            )
        );

        truck.setOrigin(
            sf::Vector2f(
                32.5f,
                15.0f
            )
        );

        truck.setPosition(
            supplierTruck_.position
        );

        truck.setRotation(
            sf::degrees(angle)
        );

        truck.setFillColor(
            sf::Color(
                255,
                140,
                20
            )
        );

        truck.setOutlineThickness(
            3.0f
        );

        truck.setOutlineColor(
            sf::Color::White
        );

        window.draw(truck);
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

    for (
        VehicleView& vehicle :
        vehicles_
    )
    {
        if (!vehicle.active)
        {
            continue;
        }

        if (
            vehicle.startDelay >
            0.0f
        )
        {
            vehicle.startDelay -=
                deltaTime *
                simulationSpeed_;

            continue;
        }

        const sf::Vector2f
            storePosition =
                getStorePosition(
                    vehicle.targetStoreId
                );

        const float movementSpeed =
            0.20f *
            simulationSpeed_;

        vehicle.progress +=
            movementSpeed *
            deltaTime;

        if (
            vehicle.progress >=
            1.0f
        )
        {
            vehicle.progress =
                0.0f;

            if (
                vehicle.direction ==
                VehicleDirection::ToStore
            )
            {
                vehicle.direction =
                    VehicleDirection::
                        ToWarehouse;
            }
            else
            {
                vehicle.direction =
                    VehicleDirection::
                        ToStore;

                vehicle.startDelay =
                    1.0f;
            }
        }

        sf::Vector2f start;
        sf::Vector2f end;

        if (
            vehicle.direction ==
            VehicleDirection::ToStore
        )
        {
            start =
                warehouseCenter_;

            end =
                storePosition;
        }
        else
        {
            start =
                storePosition;

            end =
                warehouseCenter_;
        }

        vehicle.position =
            start +
            (end - start) *
            vehicle.progress;
    }
}
void SimulationScreen::updateSupplierTruck(
    float deltaTime
)
{
    if (!supplierTruck_.active)
    {
        return;
    }

    const float movementSpeed =
        0.12f *
        simulationSpeed_;

    supplierTruck_.progress +=
        movementSpeed *
        deltaTime;

    if (
        supplierTruck_.progress >=
        1.0f
    )
    {
        supplierTruck_.progress =
            0.0f;

        if (!supplierTruck_.returning)
        {
            // Грузовик прибыл на склад.
            supplierTruck_.returning =
                true;
        }
        else
        {
            // Грузовик вернулся поставщику.
            supplierTruck_.returning =
                false;

            supplierTruck_.active =
                false;

            supplierTruck_.position =
                supplierCenter_;
        }
    }

    sf::Vector2f start;
    sf::Vector2f end;

    if (!supplierTruck_.returning)
    {
        start =
            supplierCenter_;

        end =
            warehouseCenter_;
    }
    else
    {
        start =
            warehouseCenter_;

        end =
            supplierCenter_;
    }

    supplierTruck_.position =
        start +
        (end - start) *
        supplierTruck_.progress;
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
        if (
            store.id ==
            storeId
        )
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
        const sf::Vector2i
            pixelPosition =
                sf::Mouse::getPosition(
                    window
                );

        const sf::Vector2f
            mousePosition =
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

            selectedStoreId_ =
                -1;

            selectedStoreName_.clear();
            selectedStoreInventory_.clear();
        }
        else if (
            supplierBounds_.contains(
                mousePosition
            )
        )
        {
            selectedType_ =
                SelectedObjectType::Supplier;

            selectedStoreId_ =
                -1;

            selectedStoreName_.clear();
            selectedStoreInventory_.clear();
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
                        SelectedObjectType::
                            Store;

                    selectedStoreId_ =
                        store.id;

                    refreshSelectedStoreData();

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
        380.0f;

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
        ImGui::Text(
            "WAREHOUSE"
        );

        ImGui::Separator();

        if (
            backend_ == nullptr ||
            !backend_->isReady()
        )
        {
            ImGui::TextColored(
                ImVec4(
                    1.0f,
                    0.35f,
                    0.35f,
                    1.0f
                ),
                "Backend database is not available"
            );
        }else if (
        selectedType_ ==
        SelectedObjectType::Store
    )
    {
        ImGui::Text(
            "STORE"
        );

        ImGui::Separator();

        ImGui::Text(
            "Name: %s",
            selectedStoreName_.c_str()
        );

        ImGui::Text(
            "ID: %d",
            selectedStoreId_
        );

        ImGui::Spacing();

        ImGui::Text(
            "Inventory:"
        );

        ImGui::Separator();

        if (
            selectedStoreInventory_.empty()
        )
        {
            ImGui::Text(
                "No products."
            );
        }
        else
        {
            const ImGuiTableFlags inventoryFlags =
                ImGuiTableFlags_Borders |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_Resizable;

            if (
                ImGui::BeginTable(
                    "StoreInventoryTable",
                    5,
                    inventoryFlags
                )
            )
            {
                ImGui::TableSetupColumn(
                    "Product"
                );

                ImGui::TableSetupColumn(
                    "Qty"
                );

                ImGui::TableSetupColumn(
                    "Capacity"
                );

                ImGui::TableSetupColumn(
                    "Min"
                );

                ImGui::TableSetupColumn(
                    "Status"
                );

                ImGui::TableHeadersRow();

                for (
                    const ProductStockInfo& item :
                    selectedStoreInventory_
                )
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);

                    ImGui::TextUnformatted(
                        item.productName.c_str()
                    );

                    ImGui::TableSetColumnIndex(1);

                    ImGui::Text(
                        "%d",
                        item.quantity
                    );

                    ImGui::TableSetColumnIndex(2);

                    ImGui::Text(
                        "%d",
                        item.capacity
                    );

                    ImGui::TableSetColumnIndex(3);

                    ImGui::Text(
                        "%d",
                        item.minStock
                    );

                    ImGui::TableSetColumnIndex(4);

                    if (
                        item.quantity <=
                        item.minStock
                    )
                    {
                        ImGui::TextColored(
                            ImVec4(
                                1.0f,
                                0.35f,
                                0.35f,
                                1.0f
                            ),
                            "LOW"
                        );
                    }
                    else
                    {
                        ImGui::TextColored(
                            ImVec4(
                                0.35f,
                                1.0f,
                                0.45f,
                                1.0f
                            ),
                            "OK"
                        );
                    }
                }

                ImGui::EndTable();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text(
            "Current order:"
        );

        if (!selectedStoreHasOrder_)
        {
            ImGui::TextColored(
                ImVec4(
                    0.65f,
                    0.65f,
                    0.65f,
                    1.0f
                ),
                "No active order"
            );
        }
        else
        {
            ImGui::Text(
                "Order #%d",
                selectedStoreOrder_.id
            );

            ImGui::Text(
                "Created day: %d",
                selectedStoreOrder_.createdDay
            );

            ImGui::Text(
                "Delivery day: %d",
                selectedStoreOrder_.deliveryDay
            );

            ImGui::Text(
                "Status: %s",
                selectedStoreOrder_.status.c_str()
            );

            ImGui::Spacing();

            const ImGuiTableFlags orderFlags =
                ImGuiTableFlags_Borders |
                ImGuiTableFlags_RowBg;

            if (
                ImGui::BeginTable(
                    "StoreOrderItems",
                    3,
                    orderFlags
                )
            )
                {
                    ImGui::TableSetupColumn(
                        "Product"
                    );

                    ImGui::TableSetupColumn(
                        "Requested"
                    );

                    ImGui::TableSetupColumn(
                        "Allocated"
                    );

                    ImGui::TableHeadersRow();

                    for (
                        const OrderItemInfo& item :
                        selectedStoreOrder_.items
                    )
                    {
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);

                        ImGui::TextUnformatted(
                            item.productName.c_str()
                        );

                        ImGui::TableSetColumnIndex(1);

                        ImGui::Text(
                            "%d",
                            item.requestedQuantity
                        );

                        ImGui::TableSetColumnIndex(2);

                        ImGui::Text(
                            "%d",
                            item.allocatedQuantity
                        );
                    }

                    ImGui::EndTable();
                }
            }
        }
        else
        {
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
                // =========================
                // PRODUCTS
                // =========================
                if (
                    ImGui::BeginTabItem(
                        "Products"
                    )
                )
                {
                    const ImGuiTableFlags flags =
                        ImGuiTableFlags_Borders |
                        ImGuiTableFlags_RowBg |
                        ImGuiTableFlags_Resizable |
                        ImGuiTableFlags_SizingStretchProp;

                    if (
                        ImGui::BeginTable(
                            "WarehouseInventoryTable",
                            5,
                            flags
                        )
                    )
                    {
                        ImGui::TableSetupColumn(
                            "Product",
                            ImGuiTableColumnFlags_None,
                            2.2f
                        );

                        ImGui::TableSetupColumn(
                            "Qty"
                        );

                        ImGui::TableSetupColumn(
                            "Capacity"
                        );

                        ImGui::TableSetupColumn(
                            "Min"
                        );

                        ImGui::TableSetupColumn(
                            "Status"
                        );

                        ImGui::TableHeadersRow();

                        for (
                            const WarehouseStockInfo& item :
                            warehouseInventory_
                        )
                        {
                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);

                            ImGui::TextUnformatted(
                                item.productName.c_str()
                            );

                            ImGui::TableSetColumnIndex(1);

                            ImGui::Text(
                                "%d",
                                item.quantity
                            );

                            ImGui::TableSetColumnIndex(2);

                            ImGui::Text(
                                "%d",
                                item.capacity
                            );

                            ImGui::TableSetColumnIndex(3);

                            ImGui::Text(
                                "%d",
                                item.minStock
                            );

                            ImGui::TableSetColumnIndex(4);

                            if (
                                item.quantity <=
                                item.minStock
                            )
                            {
                                ImGui::TextColored(
                                    ImVec4(
                                        1.0f,
                                        0.35f,
                                        0.35f,
                                        1.0f
                                    ),
                                    "LOW"
                                );
                            }
                            else
                            {
                                ImGui::TextColored(
                                    ImVec4(
                                        0.35f,
                                        1.0f,
                                        0.45f,
                                        1.0f
                                    ),
                                    "OK"
                                );
                            }
                        }

                        ImGui::EndTable();
                    }

                    if (
                        warehouseInventory_.empty()
                    )
                    {
                        ImGui::Spacing();

                        ImGui::Text(
                            "No warehouse products found."
                        );
                    }

                    ImGui::EndTabItem();
                }

                // =========================
                // SUPPLY
                // =========================
                if (
                    ImGui::BeginTabItem(
                        "Supply"
                    )
                )
                {
                    int activeRequests = 0;

                    for (
                        const SupplierRequestInfo& request :
                        supplierRequests_
                    )
                    {
                        if (
                            request.status !=
                            "Delivered"
                        )
                        {
                            activeRequests++;
                        }
                    }

                    ImGui::Text(
                        "Active requests: %d",
                        activeRequests
                    );

                    ImGui::Spacing();

                    if (
                        supplierRequests_.empty()
                    )
                    {
                        ImGui::Text(
                            "No supplier requests."
                        );
                    }
                    else
                    {
                        const ImGuiTableFlags flags =
                            ImGuiTableFlags_Borders |
                            ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_Resizable;

                        if (
                            ImGui::BeginTable(
                                "SupplierRequestsTable",
                                5,
                                flags
                            )
                        )
                        {
                            ImGui::TableSetupColumn(
                                "Product"
                            );

                            ImGui::TableSetupColumn(
                                "Qty"
                            );

                            ImGui::TableSetupColumn(
                                "Created"
                            );

                            ImGui::TableSetupColumn(
                                "Delivery"
                            );

                            ImGui::TableSetupColumn(
                                "Status"
                            );

                            ImGui::TableHeadersRow();

                            for (
                                const SupplierRequestInfo& request :
                                supplierRequests_
                            )
                            {
                                ImGui::TableNextRow();

                                ImGui::TableSetColumnIndex(0);

                                ImGui::TextUnformatted(
                                    request.productName.c_str()
                                );

                                ImGui::TableSetColumnIndex(1);

                                ImGui::Text(
                                    "%d",
                                    request.requestedQuantity
                                );

                                ImGui::TableSetColumnIndex(2);

                                ImGui::Text(
                                    "%d",
                                    request.createdDay
                                );

                                ImGui::TableSetColumnIndex(3);

                                ImGui::Text(
                                    "%d",
                                    request.deliveryDay
                                );

                                ImGui::TableSetColumnIndex(4);

                                ImGui::TextUnformatted(
                                    request.status.c_str()
                                );
                            }

                            ImGui::EndTable();
                        }
                    }

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
    }else if (
        selectedType_ ==
        SelectedObjectType::Supplier
    )
    {
        ImGui::Text(
            "SUPPLIER COMPANY"
        );

        ImGui::Separator();

        int activeRequests = 0;

        for (
            const SupplierRequestInfo& request :
            supplierRequests_
        )
        {
            if (
                request.status !=
                "Delivered"
            )
            {
                activeRequests++;
            }
        }

        ImGui::Text(
            "Active requests: %d",
            activeRequests
        );

        ImGui::Spacing();

        if (
            supplierRequests_.empty()
        )
        {
            ImGui::TextColored(
                ImVec4(
                    0.65f,
                    0.65f,
                    0.65f,
                    1.0f
                ),
                "No supplier requests"
            );
        }
        else
        {
            const ImGuiTableFlags flags =
                ImGuiTableFlags_Borders |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_Resizable;

            if (
                ImGui::BeginTable(
                    "SupplierInfoTable",
                    5,
                    flags
                )
            )
            {
                ImGui::TableSetupColumn(
                    "Product"
                );

                ImGui::TableSetupColumn(
                    "Qty"
                );

                ImGui::TableSetupColumn(
                    "Created"
                );

                ImGui::TableSetupColumn(
                    "Delivery"
                );

                ImGui::TableSetupColumn(
                    "Status"
                );

                ImGui::TableHeadersRow();

                for (
                    const SupplierRequestInfo& request :
                    supplierRequests_
                )
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);

                    ImGui::TextUnformatted(
                        request.productName.c_str()
                    );

                    ImGui::TableSetColumnIndex(1);

                    ImGui::Text(
                        "%d",
                        request.requestedQuantity
                    );

                    ImGui::TableSetColumnIndex(2);

                    ImGui::Text(
                        "%d",
                        request.createdDay
                    );

                    ImGui::TableSetColumnIndex(3);

                    ImGui::Text(
                        "%d",
                        request.deliveryDay
                    );

                    ImGui::TableSetColumnIndex(4);

                    ImGui::TextUnformatted(
                        request.status.c_str()
                    );
                }

                ImGui::EndTable();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text(
            "Supplier truck:"
        );

        if (
            supplierTruck_.active
        )
        {
            if (
                !supplierTruck_.returning
            )
            {
                ImGui::TextColored(
                    ImVec4(
                        1.0f,
                        0.65f,
                        0.20f,
                        1.0f
                    ),
                    "Delivering products to warehouse"
                );
            }
            else
            {
                ImGui::TextColored(
                    ImVec4(
                        0.50f,
                        0.80f,
                        1.0f,
                        1.0f
                    ),
                    "Returning to supplier"
                );
            }
        }
        else
        {
            ImGui::Text(
                "Waiting"
            );
        }
    }else if (
        selectedType_ ==
        SelectedObjectType::Store
    )
    {
        ImGui::Text(
            "STORE"
        );

        ImGui::Separator();

        if (
            backend_ == nullptr ||
            !backend_->isReady()
        )
        {
            ImGui::TextColored(
                ImVec4(
                    1.0f,
                    0.35f,
                    0.35f,
                    1.0f
                ),
                "Backend database is not available"
            );
        }
        else
        {
            ImGui::Text(
                "ID: %d",
                selectedStoreId_
            );

            ImGui::Text(
                "Name: %s",
                selectedStoreName_.c_str()
            );

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text(
                "Inventory"
            );

            ImGui::Spacing();

            if (
                selectedStoreInventory_.empty()
            )
            {
                ImGui::TextColored(
                    ImVec4(
                        0.65f,
                        0.65f,
                        0.65f,
                        1.0f
                    ),
                    "No products in store"
                );
            }
            else
            {
                const ImGuiTableFlags flags =
                    ImGuiTableFlags_Borders |
                    ImGuiTableFlags_RowBg |
                    ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_SizingStretchProp;

                if (
                    ImGui::BeginTable(
                        "StoreInventoryTable",
                        5,
                        flags
                    )
                )
                {
                    ImGui::TableSetupColumn(
                        "Product",
                        ImGuiTableColumnFlags_None,
                        2.0f
                    );

                    ImGui::TableSetupColumn(
                        "Qty"
                    );

                    ImGui::TableSetupColumn(
                        "Capacity"
                    );

                    ImGui::TableSetupColumn(
                        "Min"
                    );

                    ImGui::TableSetupColumn(
                        "Status"
                    );

                    ImGui::TableHeadersRow();

                    for (
                        const ProductStockInfo& item :
                        selectedStoreInventory_
                    )
                    {
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);

                        ImGui::TextUnformatted(
                            item.productName.c_str()
                        );

                        ImGui::TableSetColumnIndex(1);

                        ImGui::Text(
                            "%d",
                            item.quantity
                        );

                        ImGui::TableSetColumnIndex(2);

                        ImGui::Text(
                            "%d",
                            item.capacity
                        );

                        ImGui::TableSetColumnIndex(3);

                        ImGui::Text(
                            "%d",
                            item.minStock
                        );

                        ImGui::TableSetColumnIndex(4);

                        if (
                            item.quantity <=
                            item.minStock
                        )
                        {
                            ImGui::TextColored(
                                ImVec4(
                                    1.0f,
                                    0.35f,
                                    0.35f,
                                    1.0f
                                ),
                                "LOW"
                            );
                        }
                        else
                        {
                            ImGui::TextColored(
                                ImVec4(
                                    0.35f,
                                    1.0f,
                                    0.45f,
                                    1.0f
                                ),
                                "OK"
                            );
                        }
                    }

                    ImGui::EndTable();
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text(
                "Current order"
            );

            if (!selectedStoreHasOrder_)
            {
                ImGui::TextColored(
                    ImVec4(
                        0.65f,
                        0.65f,
                        0.65f,
                        1.0f
                    ),
                    "No active order"
                );
            }
            else
            {
                ImGui::Text(
                    "Order #%d",
                    selectedStoreOrder_.id
                );

                ImGui::Text(
                    "Created day: %d",
                    selectedStoreOrder_.createdDay
                );

                ImGui::Text(
                    "Delivery day: %d",
                    selectedStoreOrder_.deliveryDay
                );

                ImGui::Text(
                    "Status: %s",
                    selectedStoreOrder_.status.c_str()
                );

                ImGui::Spacing();

                if (
                    ImGui::BeginTable(
                        "StoreOrderItems",
                        3,
                        ImGuiTableFlags_Borders |
                        ImGuiTableFlags_RowBg
                    )
                )
                {
                    ImGui::TableSetupColumn(
                        "Product"
                    );

                    ImGui::TableSetupColumn(
                        "Requested"
                    );

                    ImGui::TableSetupColumn(
                        "Allocated"
                    );

                    ImGui::TableHeadersRow();

                    for (
                        const OrderItemInfo& item :
                        selectedStoreOrder_.items
                    )
                    {
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);

                        ImGui::TextUnformatted(
                            item.productName.c_str()
                        );

                        ImGui::TableSetColumnIndex(1);

                        ImGui::Text(
                            "%d",
                            item.requestedQuantity
                        );

                        ImGui::TableSetColumnIndex(2);

                        ImGui::Text(
                            "%d",
                            item.allocatedQuantity
                        );
                    }

                    ImGui::EndTable();
                }
            }
        }
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
        assetsPath_ /
        "textures" /
        "warehouse.png";

    const auto storePath =
        assetsPath_ /
        "textures" /
        "store.png";

    const auto truckPath =
        assetsPath_ /
        "textures" /
        "truck.png";

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

void SimulationScreen::setBackend(
    BackendFacade* backend
)
{
    backend_ = backend;
}

void SimulationScreen::setAssetsPath(
    const std::filesystem::path& path
)
{
    assetsPath_ = path;
}

void SimulationScreen::
refreshSelectedStoreData()
{
    selectedStoreName_.clear();

    selectedStoreInventory_.clear();
    selectedStoreOrder_ =
        StoreOrderInfo{};

    selectedStoreHasOrder_ =
        false;

    if (
        backend_ == nullptr ||
        !backend_->isReady() ||
        selectedStoreId_ < 0
    )
    {
        return;
    }

    selectedStoreName_ =
        backend_->getStoreName(
            selectedStoreId_
        );

    selectedStoreInventory_ =
        backend_->getStoreInventory(
            selectedStoreId_
        );
    selectedStoreHasOrder_ =
    backend_->getActiveStoreOrder(
        selectedStoreId_,
        selectedStoreOrder_
    );
}
void SimulationScreen::refreshWarehouseData()
{
    warehouseInventory_.clear();

    if (
        backend_ == nullptr ||
        !backend_->isReady()
    )
    {
        return;
    }

    warehouseInventory_ =
        backend_->getWarehouseInventory();
}
void SimulationScreen::refreshSupplierData()
{
    supplierRequests_.clear();

    if (
        backend_ == nullptr ||
        !backend_->isReady()
    )
    {
        supplierTruck_.active = false;
        return;
    }

    supplierRequests_ =
        backend_->getSupplierRequests();

    bool hasActiveRequest = false;

    for (
        const SupplierRequestInfo& request :
        supplierRequests_
    )
    {
        if (
            request.status == "Created" ||
            request.status == "InTransit"
        )
        {
            hasActiveRequest = true;
            break;
        }
    }

    if (
        hasActiveRequest &&
        !supplierTruck_.active
    )
    {
        supplierTruck_.active = true;

        supplierTruck_.position =
            supplierCenter_;

        supplierTruck_.progress =
            0.0f;

        supplierTruck_.returning =
            false;
    }

    if (!hasActiveRequest)
    {
        supplierTruck_.active = false;

        supplierTruck_.position =
            supplierCenter_;

        supplierTruck_.progress =
            0.0f;

        supplierTruck_.returning =
            false;
    }
}
void SimulationScreen::configure(
    const SimulationSettings& settings
)
{
    totalDays_ =
        settings.days;

    currentDay_ = 1;
    dayTimer_ = 0.0f;

    simulationSpeed_ = 1.0f;
    supplierTruck_.position =
    supplierCenter_;

    supplierTruck_.progress =
        0.0f;

    supplierTruck_.returning =
        false;

    supplierTruck_.active =
        false;
    simulationState_ =
        SimulationState::Stopped;
}

void SimulationScreen::advanceDay()
{
    if (
        currentDay_ >=
        totalDays_
    )
    {
        simulationState_ =
            SimulationState::Finished;

        dayTimer_ = 0.0f;

        return;
    }

    currentDay_++;
    if (
        backend_ != nullptr &&
        backend_->isReady()
    )
    {   
        backend_->processStoreOrders(
            currentDay_
        );
    }
}
