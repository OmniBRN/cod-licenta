#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

int main()
{
    sf::RenderWindow window(sf::VideoMode({1280, 720}), "SFML + ImGui");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    sf::CircleShape shape(50.f);
    shape.setFillColor(sf::Color::Green);
    shape.setPosition({100.f, 100.f});

    float color[3] = {0.f, 1.f, 0.f};
    float radius = 50.f;
    float pos[2] = {100.f, 100.f};

    sf::Clock deltaClock;
    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("Shape controls");
        if (ImGui::ColorEdit3("Color", color))
            shape.setFillColor(sf::Color(
                static_cast<uint8_t>(color[0] * 255),
                static_cast<uint8_t>(color[1] * 255),
                static_cast<uint8_t>(color[2] * 255)));
        if (ImGui::SliderFloat("Radius", &radius, 10.f, 200.f))
        {
            shape.setRadius(radius);
            shape.setOrigin({radius, radius});
        }
        if (ImGui::SliderFloat2("Position", pos, 0.f, 1280.f))
            shape.setPosition({pos[0], pos[1]});
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::End();

        window.clear(sf::Color(30, 30, 30));
        window.draw(shape);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}