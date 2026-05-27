#include "gui/renderer.hpp"

namespace sim::gui {

static const sf::Color ARCHETYPE_COLORS[5] = {
    {0, 150, 200},
    {80, 200, 80},
    {220, 220, 50},
    {220, 80, 50},
    {180, 80, 220}
};

sf::Color Renderer::archetype_color(ProfileId id) {
    return (id < 5) ? ARCHETYPE_COLORS[id] : sf::Color::White;
}

Camera::Camera(sf::FloatRect bounds) : view(bounds) {}

void Camera::handle_event(const sf::Event& ev, sf::RenderWindow& win) {
    if (const auto* e = ev.getIf<sf::Event::MouseButtonPressed>()) {
        if (e->button == sf::Mouse::Button::Middle) {
            dragging = true;
            drag_start = sf::Mouse::getPosition(win);
        }
    }
    if (const auto* e = ev.getIf<sf::Event::MouseButtonReleased>()) {
        if (e->button == sf::Mouse::Button::Middle) {
            dragging = false;
        }
    }
    if (ev.is<sf::Event::MouseMoved>() && dragging) {
        sf::Vector2i cur = sf::Mouse::getPosition(win);
        win.setView(view);
        sf::Vector2f delta = win.mapPixelToCoords(drag_start) - win.mapPixelToCoords(cur);
        view.move(delta);
        drag_start = cur;
    }
    if (const auto* e = ev.getIf<sf::Event::MouseWheelScrolled>()) {
        float f = (e->delta > 0.f) ? 0.9f : 1.1f;
        view.zoom(f);
    }

}

Renderer::Renderer(sf::RenderWindow& win, const Simulation& sim) 
    : m_win(win), m_sim(sim), m_cam(sf::FloatRect({-350.f, -350.f}, {700.f, 700.f})) {}

void Renderer::draw_network() {
    const Network net = m_sim.network();
    for (const auto& edge: net.edges) {
        for (LaneIdx l=0 ; l<edge.lanes_forward; ++l) {
            Vec2 p0 = net.point_at(edge.id, l, 0.0);
            Vec2 p1 = net.point_at(edge.id, l, edge.length);
            sf::Vertex line[2] = {
                sf::Vertex{{(float)p0.x, (float)p0.y}, sf::Color(80,80,80)},
                sf::Vertex{{(float)p1.x, (float)p1.y}, sf::Color(80,80,80)},
            };
            m_win.draw(line, 2, sf::PrimitiveType::Lines);
        }
    }
}

void Renderer::draw_lights() {
    for (const auto& [eid, tl] : m_sim.lights()) {
        const Edge& e = m_sim.network().edges[eid];
        Vec2 p = m_sim.network().point_at(eid, 0, e.length - 3.0);
        sf::CircleShape c(5.f);
        c.setOrigin({5.f, 5.f});
        c.setPosition({(float)p.x, (float)p.y});
        if (tl.is_green()) c.setFillColor(sf::Color::Green);
        else if (tl.is_yellow()) c.setFillColor(sf::Color::Yellow);
        else  c.setFillColor(sf::Color::Red);
        c.setOutlineColor(sf::Color::Black);
        c.setOutlineThickness(1.f);
        m_win.draw(c);
    }
}

void Renderer::draw_cars(sf::Vector2f /*mouse_world*/){
    const Network& net = m_sim.network();
    for (const auto& car : m_sim.cars()) {
        Vec2 pos = net.point_at(car.current_edge, car.current_lane, car.offset);
        sf::RectangleShape rect(
            {(float)CAR_LENGTH, (float)(LANE_WIDTH * 0.6f)}
        );
        rect.setOrigin({(float)CAR_LENGTH / 2.f, (float)(LANE_WIDTH * 0.3f)});
        rect.setPosition({(float)pos.x, (float)pos.y});
        rect.setFillColor(archetype_color(car.profile_id));
        m_win.draw(rect);
    }
}

void Renderer::draw(sf::Vector2f mouse_world) {
    m_win.setView(m_cam.view);
    draw_network();
    draw_lights();
    draw_cars(mouse_world);
    m_win.setView(m_win.getDefaultView());
}

CarId Renderer::pick_car(sf::Vector2f mw) const {
    const Network& net = m_sim.network();
    constexpr float R = 6.0f;
    for (const auto& car : m_sim.cars()) {
        Vec2 p = net.point_at(car.current_edge, car.current_lane, car.offset);
        float dx = (float)p.x - mw.x;
        float dy = (float)p.y - mw.y;
        if (dx*dx + dy*dy < R*R) return car.id;
    }
    return 0;
}

}