#pragma once
#include "core/simulation.hpp"
#include <SFML/Graphics.hpp>

namespace sim::gui {

struct Camera {
    sf::View view;
    bool dragging = false;
    sf::Vector2i drag_start;

    explicit Camera(sf::FloatRect world_bounds);
    void handle_event(const sf::Event& ev, sf::RenderWindow& win);
};

class Renderer {

public:

    Renderer(sf::RenderWindow& win, const Simulation& sim);

    void draw(sf::Vector2f mouse_world);

    CarId pick_car(sf::Vector2f mouse_world) const;

    Camera& camera() { return m_cam; }
    void set_selected(CarId id) { m_selected_car = id; }

private:
    sf::RenderWindow& m_win;
    const Simulation& m_sim;
    Camera m_cam;
    sf::Font m_font;
    bool m_font_loaded = false;
    CarId m_selected_car = 0;

    void draw_network();
    void draw_lights();
    void draw_cars(sf::Vector2f mouse_world);
    void draw_speed_limits();

    static sf::Color archetype_color(ProfileId id);

};


}