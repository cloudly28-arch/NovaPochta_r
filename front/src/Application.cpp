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
        io.FontDefault = cyrillicFont;

        if (!ImGui::SFML::UpdateFontTexture()) {
            std::cerr << "Failed to update font texture\n";
        }

        std::cout
            << "Cyrillic font loaded successfully\n";
    }
    else
    {
        std::cerr
            << "Failed to load Cyrillic font\n";
    }

    databasePath_ =
        exeDirectory /
        "database" /
        "nova_poshta_warehouse.db";

    schemaPath_ =
        exeDirectory /
        "database" /
        "schema.sql";

    std::cout << "Database path: " << databasePath_ << '\n';
    std::cout << "Schema path: " << schemaPath_ << '\n';

    bool databaseReady = false;

    if (std::filesystem::exists(databasePath_)) {
        databaseReady = backend_.initialize(databasePath_.string());

        if (!databaseReady) {
            std::cerr
                << "Existing database could not be loaded:\n"
                << backend_.getLastError()
                << '\n';
        }
    }

    if (!databaseReady) {
        databaseReady = backend_.resetDatabase(
            databasePath_.string(),
            schemaPath_.string()
        );
    }

    if (!databaseReady) {
        std::cerr
            << "Database initialization failed:\n"
            << backend_.getLastError()
            << '\n';

        window_.close();
        return;
    }

    std::cout << "Database loaded successfully\n";
    simulationScreen_.setBackend(
        &backend_
    );

    simulationScreen_.setDatabasePaths(
        databasePath_,
        schemaPath_
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
    const sf::Time deltaTime = deltaClock_.restart();

    ImGui::SFML::Update(window_, deltaTime);

    if (currentScreen_ == Screen::Start) {
        if (startScreen_.draw(settings_)) {
            const bool started = backend_.startExperiment(
                databasePath_.string(),
                schemaPath_.string(),
                settings_.stores,
                settings_.products
            );

            if (started) {
                simulationScreen_.configure(settings_);
                currentScreen_ = Screen::Simulation;
            } else {
                ImGui::OpenPopup("Experiment initialization error");
            }
        }
    } else {
        simulationScreen_.update(deltaTime.asSeconds());
    }

    if (ImGui::BeginPopupModal(
            "Experiment initialization error",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped(
            "%s",
            backend_.getLastError().c_str()
        );

        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
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
