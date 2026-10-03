#include "ui/SimulationScreen.h"

#include <imgui.h>

#include <cmath>
#include <string>

void SimulationScreen::draw(
    sf::RenderWindow& window,
    const SimulationSettings& settings
)
{
    drawTopBar(settings);

    drawWarehouse(window);
    drawStores(window, settings.stores);

    drawBottomBar();
}

void SimulationScreen::drawTopBar(
    const SimulationSettings& settings
)
{
    ImGui::SetNextWindowPos(
        ImVec2(0.0f, 0.0f),
        ImGuiCond_Always
    );

    ImGui::SetNextWindowSize(
        ImVec2(
            ImGui::GetIO().DisplaySize.x,
            80.0f
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

    ImGui::SameLine(300.0f);

    ImGui::Text(
        "Day: 1 / %d",
        settings.days
    );

    ImGui::SameLine(500.0f);

    ImGui::Text(
        "Stores: %d",
        settings.stores
    );

    ImGui::SameLine(650.0f);

    ImGui::Text(
        "Products: %d",
        settings.products
    );

    ImGui::End();
}

void SimulationScreen::drawBottomBar()
{
    const float height = 90.0f;

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

    ImGui::Button("START", ImVec2(100, 40));

    ImGui::SameLine();

    ImGui::Button("PAUSE", ImVec2(100, 40));

    ImGui::SameLine();

    ImGui::Button("STOP", ImVec2(100, 40));

    ImGui::SameLine(400.0f);

    ImGui::Text("Speed:");

    ImGui::SameLine();

    ImGui::Button("1x");

    ImGui::SameLine();

    ImGui::Button("2x");

    ImGui::SameLine();

    ImGui::Button("5x");

    ImGui::SameLine();

    ImGui::Button("10x");

    ImGui::End();
}

void SimulationScreen::drawWarehouse(
    sf::RenderWindow& window
)
{
    const sf::Vector2f size(180.0f, 120.0f);

    sf::RectangleShape warehouse(size);

    warehouse.setPosition(
        sf::Vector2f(550.0f, 300.0f)
    );

    warehouse.setFillColor(
        sf::Color(170, 20, 30)
    );

    warehouse.setOutlineThickness(4.0f);

    warehouse.setOutlineColor(
        sf::Color::White
    );

    window.draw(warehouse);
}

void SimulationScreen::drawStores(
    sf::RenderWindow& window,
    int storeCount
)
{
    const sf::Vector2f center(
        640.0f,
        360.0f
    );

    const float radius = 250.0f;

    for (int i = 0; i < storeCount; ++i)
    {
        const float angle =
            static_cast<float>(i) /
            static_cast<float>(storeCount) *
            2.0f *
            3.14159265f;

        const float x =
            center.x +
            std::cos(angle) *
            radius;

        const float y =
            center.y +
            std::sin(angle) *
            radius;

        sf::RectangleShape store(
            sf::Vector2f(80.0f, 60.0f)
        );

        store.setPosition(
            sf::Vector2f(
                x - 40.0f,
                y - 30.0f
            )
        );

        store.setFillColor(
            sf::Color(220, 220, 220)
        );

        store.setOutlineThickness(2.0f);

        store.setOutlineColor(
            sf::Color(120, 120, 120)
        );

        window.draw(store);
    }
}