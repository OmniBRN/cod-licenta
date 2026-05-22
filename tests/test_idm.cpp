#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/behaviour.hpp"
#include "test_helpers.hpp"
#include <limits>
#include <algorithm>
#include <vector>

using namespace sim;
using Catch::Approx;

static std::vector<double> sorted_offsets(const Simulation& s) {
    std::vector<double> v;
    for (const auto& c: s.cars()) v.push_back(c.offset);
    std::sort(v.begin(), v.end());
    return v;
}


TEST_CASE("single car reaches desired speed", "[idm]") {
    BehaviourProfile bp;
    bp.desired_speed = 15.0;

    MetersPerSec v = 0.0;
    const Seconds dt = TICK_DT;
    const double inf = std::numeric_limits<double>::infinity();

    for (int i=0; i < 5000; ++i) {
        double a = idm_accel(bp, v, inf, 0.0);
        v = std::max(0.0, v + a * dt);
    }
    REQUIRE(v == Approx(bp.desired_speed).margin(0.05));
}

TEST_CASE("standstill behind stopped leader", "[idm]") {
    BehaviourProfile bp;
    double a = idm_accel(bp, 0.0, 2.0, 0.0);
    REQUIRE(a <= 0.0);
}

TEST_CASE("platoon converges to stable headway", "[idm]") {
    Simulation s = simtest::make_long_lane_sim(2000.0);

    BehaviourProfile slow; slow.desired_speed = 5.0;
    ProfileId slow_id = s.register_profile(slow);
    Car lead; lead.current_edge = 0; lead.current_lane = 0;
    lead.offset = 200.0; lead.speed = 5.0;
    lead.route = {0}; lead.route_index = 0; lead.profile_id = slow_id;
    s.spawn_car(lead);

    for (int i=0; i < 5; ++i){
        Car proto;
        proto.current_edge = 0;
        proto.current_lane = 0;
        proto.offset = 50.0 + i * 12.0;
        proto.speed = 10.0;
        proto.route = {0};
        proto.route_index = 0;
        proto.profile_id = 0;
        s.spawn_car(proto);
    }

    for (int i = 0; i<3000; ++i) s.tick();

    auto off = sorted_offsets(s);
    std::vector<double> gaps;
    for (size_t i = 1; i + 1 < off.size(); i++)
        gaps.push_back(off[i] - off[i-1]);
    double mn = *std::min_element(gaps.begin(), gaps.end());
    double mx = *std::max_element(gaps.begin(), gaps.end());
    REQUIRE(mx - mn < 1.0);
}

TEST_CASE("no negative gap ever", "[idm]") {
    Simulation s = simtest::make_long_lane_sim(500.0);
    for (int i=0; i<5; ++i) {
        Car proto;
        proto.current_edge = 0;
        proto.current_lane = 0;
        proto.offset = 30.0 + i * 6.0;
        proto.speed = 12.0;
        proto.route = {0};
        proto.route_index = 0;
        proto.profile_id = 0;
        s.spawn_car(proto);
    }

    for (int i=0; i<3000; ++i) {
        s.tick();
        auto off = sorted_offsets(s);
        for (size_t k = 1; k < off.size(); ++k) {
            REQUIRE(off[k] - off[k-1] >= CAR_LENGTH - 1e-6);
        }
    }
}

TEST_CASE("car cross edge boundary", "[sim]") {
    Simulation s = simtest::make_two_edge_sim(50.0);
    Car proto;
    proto.current_edge = 0; proto.offset = 49.9; proto.speed = 5.0;
    proto.route = {0, 1};
    proto.route_index = 0;
    proto.profile_id = 0;
    CarId id = s.spawn_car(proto);

    s.tick();

    const Car* cc = nullptr;
    for (const auto& x: s.cars()) if (x.id == id) cc = &x;
    REQUIRE(cc != nullptr);
    REQUIRE(cc->current_edge == 1);
    REQUIRE(cc->offset < 5.0);

}