#include "gui/app.hpp"
#include <imgui.h>
#include <imgui-SFML.h>

namespace sim::gui {

App::App(Simulation sim) 
    : m_sim(std::move(sim))
    , m_win(sf::VideoMode({1280u, 720u}), "Traffic Simulation")
    , m_renderer(m_win, m_sim)
{
    ImGui::SFML::Init(m_win);
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
    ImGui::End();
}

}
