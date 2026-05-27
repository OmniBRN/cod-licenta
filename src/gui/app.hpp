#pragma once
#include "core/simulation.hpp"
#include "gui/renderer.hpp"
#include <SFML/Graphics.hpp>
#include <deque>

namespace sim::gui {

struct PlaybackState {
    bool paused = false;
    int speed_mult = 1;
};

class App {

public:
    explicit App(Simulation sim);
    void run();

private:
    Simulation m_sim;
    sf::RenderWindow m_win;
    Renderer m_renderer;
    PlaybackState m_play;
    CarId m_selected = 0;

    std::deque<size_t> m_tput_window;

    void process_events();
    void update();
    void draw_imgui();
};


}