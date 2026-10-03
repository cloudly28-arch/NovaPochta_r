#include "ui/StartScreen.h"

#include <imgui.h>

bool StartScreen::draw(SimulationSettings& settings)
{
    bool startPressed = false;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    const float windowWidth = 500.0f;
    const float windowHeight = 420.0f;

    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->WorkPos.x + (viewport->WorkSize.x - windowWidth) * 0.5f,
            viewport->WorkPos.y + (viewport->WorkSize.y - windowHeight) * 0.5f
        ),
        ImGuiCond_Always
    );

    ImGui::SetNextWindowSize(
        ImVec2(windowWidth, windowHeight),
        ImGuiCond_Always
    );

    ImGui::Begin(
        "Simulation Settings",
        nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove
    );

    ImGui::Spacing();

    ImGui::SetWindowFontScale(1.4f);
    ImGui::Text("NOVA WAREHOUSE");
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Simulation period");
    ImGui::SliderInt(
        "Days",
        &settings.days,
        10,
        30
    );

    ImGui::Spacing();

    ImGui::Text("Number of stores");
    ImGui::SliderInt(
        "Stores",
        &settings.stores,
        3,
        9
    );

    ImGui::Spacing();

    ImGui::Text("Number of product types");
    ImGui::SliderInt(
        "Products",
        &settings.products,
        12,
        20
    );

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Selected parameters:");

    ImGui::BulletText(
        "Simulation: %d days",
        settings.days
    );

    ImGui::BulletText(
        "Stores: %d",
        settings.stores
    );

    ImGui::BulletText(
        "Products: %d",
        settings.products
    );

    ImGui::Spacing();
    ImGui::Spacing();

    const float buttonWidth = 200.0f;

    ImGui::SetCursorPosX(
        (ImGui::GetWindowWidth() - buttonWidth) * 0.5f
    );

    if (ImGui::Button(
            "START SIMULATION",
            ImVec2(buttonWidth, 50.0f)
        ))
    {
        startPressed = true;
    }

    ImGui::End();

    return startPressed;
}