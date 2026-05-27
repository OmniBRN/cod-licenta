#include "gui/app.hpp"
#include <imgui.h>
#include <imgui-SFML.h>
#include <stdexcept>

namespace sim::gui {

App::App(Simulation sim) 
    : m_sim(std::move(sim))
    , m_win(sf::VideoMode({1280u, 720u}), "Traffic Simulation")
    , m_renderer(m_win, m_sim)
{
    if (!ImGui::SFML::Init(m_win))
        throw std::runtime_error("ImGui::SFML::Init failed");
    m_win.setFramerateLimit(60);
}

void App::run() {
    sf::Clock clock;
    while(m_win.isOpen()) {
        process_events();
        update();

        m_win.clear(sf::Color(30, 30, 30));

        sf::Vector2i mp = sf::Mouse::getPosition(m_win);
        m_win.setView(m_renderer.camera().view);
        sf::Vector2f mw = m_win.mapPixelToCoords(mp);
        m_win.setView(m_win.getDefaultView());

        m_renderer.draw(mw);

        ImGui::SFML::Update(m_win, clock.restart());
        draw_imgui();
        ImGui::SFML::Render(m_win);

        m_win.display();
    }
    ImGui::SFML::Shutdown();
}

void App::process_events() {
    while(const auto ev = m_win.pollEvent()) {
        ImGui::SFML::ProcessEvent(m_win, *ev);
        m_renderer.camera().handle_event(*ev, m_win);

        if (const auto* e = ev->getIf<sf::Event::MouseButtonPressed>()) {
            if (e->button == sf::Mouse::Button::Left && !ImGui::GetIO().WantCaptureMouse) {
                m_win.setView(m_renderer.camera().view);
                sf::Vector2f mw = m_win.mapPixelToCoords(e->position);
                m_win.setView(m_win.getDefaultView());
                CarId picked = m_renderer.pick_car(mw);
                if (picked) m_selected = picked;
            }
        }
    }
}

void App::update() {
    if (m_play.paused) return;
    size_t before = m_sim.exited();
    for (int i=0; i<m_play.speed_mult; ++i) m_sim.tick();
    size_t throughput = m_sim.exited() - before;
    m_tput_window.push_back(throughput);
    if (m_tput_window.size() > 100) m_tput_window.pop_front();
}

void App::draw_imgui() {
    // Toolbar
    ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({420, 55}, ImGuiCond_Always);
    ImGui::Begin("##playback", nullptr, ImGuiWindowFlags_NoTitleBar |
                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    if (ImGui::Button(m_play.paused? "Resume [P]" : "Pause [P]"))
        m_play.paused = !m_play.paused;
    ImGui::SameLine();
    if (ImGui::Button("Step") && m_play.paused) {
        m_sim.tick();
        m_tput_window.push_back(0);
    }
    ImGui::SameLine();
    const char* speed_labels[] = {"1x", "2x", "10x"};
    const int speed_vals[] = {1, 2, 10};
    static int sidx = 0;
    ImGui::SetNextItemWidth(80);
    if (ImGui::Combo("Speed", &sidx, speed_labels, 3))
        m_play.speed_mult = speed_vals[sidx];
    ImGui::End();

    size_t total = 0;
    for (size_t t : m_tput_window) total += t;
    double avg_tput = m_tput_window.empty() ? 0.0 
    : (double) total / m_tput_window.size() / sim::TICK_DT;

    ImGui::SetNextWindowPos({0, 55}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({420, 80}, ImGuiCond_Always);
    ImGui::Begin("##metrics", nullptr, ImGuiWindowFlags_NoTitleBar |
                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::Text("Tick: %llu", (unsigned long long)m_sim.current_tick());
    ImGui::Text("In Network: %zu | Left Network: %zu | Spawned: %zu", 
                m_sim.in_network(), m_sim.exited(), m_sim.spawned());
    ImGui::Text("Throughput (100 tick average): %.2f cars", avg_tput);
    ImGui::End(); // ##metrics

    if (m_selected != 0) {
        const Car* found = nullptr;
        for (const auto& c : m_sim.cars())
            if(c.id == m_selected) {
                found = &c;
                break;
            }
        bool open = true;
        ImGui::SetNextWindowPos({860, 0}, ImGuiCond_Once);
        ImGui::SetNextWindowSize({420, 210}, ImGuiCond_Once);
        ImGui::Begin("Inspect Car", &open);
        if (found) {
            const auto& bp = m_sim.profile(found->profile_id);
            ImGui::Text("ID: %u | Archetype: %s", found->id, bp.name.c_str());
            ImGui::Separator();
            ImGui::Text("Edge: %u Lane: %u", found->current_edge, found->current_lane);
            ImGui::Text("Offset: %.1f m | Speed: %.2f m/s (%.1f km/h)", found->offset, found->speed, found->speed*3.6);
            ImGui::Text("Acceleration: %.3f m/s^2", found->accel);
            ImGui::Separator();
            ImGui::Text("Trip dist: %.1f m", found->trip_distance);
            ImGui::Text("Lane Changes: %u | Violations: %u", found->lane_changes, found->violations);
            ImGui::Text("Intended lane: %u", found->intended_lane);
        } else {
            ImGui::TextDisabled("Car %u left the network.", m_selected);
        }
        ImGui::End();
        if (!open) m_selected = 0;
    }
    ImGui::SetNextWindowPos({0, 135}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({160, 130}, ImGuiCond_Always);
    ImGui::Begin("##legend", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoMove);
    ImGui::TextDisabled("Archetypes:");
    const char* names[5] = {"ideal", "cautious", "normal", "aggresive", "opportunist"};
    static const ImVec4 cols[5] = {
        {0.0f, 0.59f, 0.78f, 1},
        {0.31f, 0.78f, 0.31f, 1},
        {0.86f, 0.86f, 0.20f, 1},
        {0.86f, 0.31f, 0.31f, 1},
        {0.71f, 0.31f, 0.86f, 1},
    };
    for(int i=0; i<5; ++i) {
        ImGui::PushID(i);
        ImGui::ColorButton("##col", cols[i], ImGuiColorEditFlags_NoTooltip, {12, 12});
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::TextUnformatted(names[i]);
    }
    ImGui::End();
}

}
