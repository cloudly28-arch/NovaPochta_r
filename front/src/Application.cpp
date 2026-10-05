#include "Application.h"

#include <imgui.h>
#include <imgui-SFML.h>

#include <filesystem>
#include <iostream>
#include <windows.h>

static std::filesystem::path
getExecutableDirectory()
{
    wchar_t buffer[MAX_PATH];

    GetModuleFileNameW(
        nullptr,
        buffer,
        MAX_PATH
    );

    return std::filesystem::path(
        buffer
    ).parent_path();
}

Application::Application()
    : window_(
        sf::VideoMode({1280, 720}),
        "Nova Poshta Warehouse"
    )
{
    window_.setFramerateLimit(60);

    const auto exeDirectory =
        getExecutableDirectory();

    simulationScreen_.setAssetsPath(
        exeDirectory / "assets"
    );

    if (!ImGui::SFML::Init(window_))
    {
        window_.close();
        return;
    }

    // Кириллица для названий товаров из SQLite.
    ImGuiIO& io = ImGui::GetIO();

    ImFont* cyrillicFont =
        io.Fonts->AddFontFromFileTTF(
            "C:/Windows/Fonts/arial.ttf",
            18.0f,
            nullptr,
            io.Fonts->GetGlyphRangesCyrillic()
        );

    if (cyrillicFont != nullptr)
    {
        ImGui::SFML::UpdateFontTexture();
    }

    const auto databasePath =
        exeDirectory /
        "database" /
        "nova_poshta_warehouse.db";

    if (!backend_.initialize(
            databasePath.string()
        ))
    {
        std::cerr
            << "Failed to open database: "
            << databasePath
            << '\n';
    }

    simulationScreen_.setBackend(
        &backend_
    );

    ImGui::StyleColorsDark();

    ImGuiStyle& style =
        ImGui::GetStyle();

    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;

    style.WindowPadding =
        ImVec2(20.0f, 20.0f);

    style.FramePadding =
        ImVec2(10.0f, 8.0f);

    style.Colors[ImGuiCol_Button] =
        ImVec4(
            0.75f,
            0.05f,
            0.08f,
            1.0f
        );

    style.Colors[ImGuiCol_ButtonHovered] =
        ImVec4(
            0.90f,
            0.08f,
            0.10f,
            1.0f
        );

    style.Colors[ImGuiCol_ButtonActive] =
        ImVec4(
            0.60f,
            0.03f,
            0.05f,
            1.0f
        );

    style.Colors[ImGuiCol_SliderGrab] =
        ImVec4(
            0.85f,
            0.05f,
            0.08f,
            1.0f
        );

    style.Colors[
        ImGuiCol_SliderGrabActive
    ] =
        ImVec4(
            1.0f,
            0.10f,
            0.12f,
            1.0f
        );
}

Application::~Application()
{
    ImGui::SFML::Shutdown();
}

void Application::run()
{
    while (window_.isOpen())
    {
        processEvents();
        update();
        render();
    }
}

void Application::processEvents()
{
    while (
        const auto event =
            window_.pollEvent()
    )
    {
        ImGui::SFML::ProcessEvent(
            window_,
            *event
        );

        if (
            event->is<
                sf::Event::Closed
            >()
        )
        {
            window_.close();
        }
    }
}

void Application::update()
{
    const sf::Time deltaTime =
        deltaClock_.restart();

    ImGui::SFML::Update(
        window_,
        deltaTime
    );

    if (
        currentScreen_ ==
        Screen::Start
    )
    {
        if (
            startScreen_.draw(
                settings_
            )
        )
        {
            simulationScreen_.configure(
                settings_
            );

            currentScreen_ =
                Screen::Simulation;
        }
    }
    else
    {
        simulationScreen_.update(
            deltaTime.asSeconds()
        );
    }
}

void Application::render()
{
    window_.clear(
        sf::Color(25, 25, 28)
    );

    if (
        currentScreen_ ==
        Screen::Simulation
    )
    {
        simulationScreen_.draw(
            window_,
            settings_
        );
    }

    ImGui::SFML::Render(
        window_
    );

    window_.display();
}
