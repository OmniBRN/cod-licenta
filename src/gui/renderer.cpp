#include "gui/renderer.hpp"
#include <cmath>

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
    : m_win(win), m_sim(sim), m_cam([&win]() {
        auto sz = win.getSize();
        float half_h = 350.f;
        float half_w = half_h * (float)sz.x / (float)sz.y;
        return sf::FloatRect({-half_w, -half_h}, {2.f * half_w, 2.f * half_h});
    }())
{
    m_font_loaded = m_font.openFromFile("include/imgui/misc/fonts/Roboto-Medium.ttf");
}

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
        Vec2 p = m_sim.network().point_at(eid, 0, e.length - CAR_LENGTH);
        constexpr float R = 1.2f;
        sf::CircleShape c(R);
        c.setOrigin({R, R});
        c.setPosition({(float)p.x, (float)p.y});
        if (tl.is_green()) c.setFillColor(sf::Color::Green);
        else if (tl.is_yellow()) c.setFillColor(sf::Color::Yellow);
        else  c.setFillColor(sf::Color::Red);
        c.setOutlineColor(sf::Color::Black);
        c.setOutlineThickness(0.3f);
        m_win.draw(c);
    }
}

void Renderer::draw_cars(sf::Vector2f /*mouse_world*/){
    const Network& net = m_sim.network();
    for (const auto& car : m_sim.cars()) {
        Vec2 pos = net.point_at(car.current_edge, car.current_lane, car.offset);
        Vec2 dir = net.direction_at(car.current_edge, car.current_lane, car.offset);

        sf::RectangleShape rect(
            {(float)CAR_LENGTH, (float)(LANE_WIDTH * 0.6f)}
        );
        rect.setOrigin({(float)CAR_LENGTH / 2.f, (float)(LANE_WIDTH * 0.3f)});
        rect.setPosition({(float)pos.x, (float)pos.y});

        float angle_deg = std::atan2f((float)dir.y, (float)dir.x) * (180.0f / 3.14159265f);
        rect.setRotation(sf::degrees(angle_deg));

        rect.setFillColor(archetype_color(car.profile_id));
        if (car.id == m_selected_car) {
            rect.setOutlineColor(sf::Color::White);
            rect.setOutlineThickness(0.6f);
        }
        m_win.draw(rect);
    }
}

void Renderer::draw_speed_limits() {
    if (!m_font_loaded) return;
    const Network& net = m_sim.network();
    for (const auto& edge : net.edges) {
        if (edge.polyline.size() < 2) continue;

        Meters pos_along = edge.length * 0.3;
        Vec2 dir = net.direction_at(edge.id, 0, pos_along);
        Vec2 right = perpendicular(dir);

        Vec2 lane0 = net.point_at(edge.id, 0, pos_along);
        double lateral = (edge.lanes_forward + 0.5) * LANE_WIDTH + 6.0;
        Vec2 sp = lane0 + right * lateral;

        constexpr float W = 12.0f, H = 9.0f;
        sf::RectangleShape box({W, H});
        box.setOrigin({W / 2.f, H / 2.f});
        box.setPosition({(float)sp.x, (float)sp.y});
        box.setFillColor(sf::Color::White);
        box.setOutlineColor(sf::Color::Red);
        box.setOutlineThickness(0.8f);
        m_win.draw(box);

        int kmh = static_cast<int>(std::round(edge.speed_limit * 3.6));
        sf::Text txt(m_font, std::to_string(kmh), 100);
        txt.setFillColor(sf::Color::Black);
        constexpr float TEXT_H = 5.5f;
        float scale = TEXT_H / 100.f;
        txt.setScale({scale, scale});
        sf::FloatRect lb = txt.getLocalBounds();
        txt.setOrigin({lb.position.x + lb.size.x / 2.f, lb.position.y + lb.size.y / 2.f});
        txt.setPosition({(float)sp.x, (float)sp.y});
        m_win.draw(txt);
    }
}

void Renderer::draw(sf::Vector2f mouse_world) {
    m_win.setView(m_cam.view);
    draw_network();
    draw_lights();
    // Looks ugly, rework it later
    draw_speed_limits();
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