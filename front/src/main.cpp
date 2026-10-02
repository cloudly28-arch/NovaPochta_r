#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({1280, 720}),
        "Nova Poshta Warehouse"
    );

    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window))
    {
        return 1;
    }

    sf::Clock deltaClock;

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("Nova Warehouse");

        ImGui::Text("Frontend is working!");
        ImGui::Separator();
        ImGui::Text("Dashboard");

        ImGui::Text("Products: 0");
        ImGui::Text("Orders: 0");
        ImGui::Text("Workers: 0");
        ImGui::Text("Tasks: 0");

        ImGui::End();

        window.clear(sf::Color(45, 45, 45));

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    return 0;
}