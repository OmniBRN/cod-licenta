#include "core/simulation.hpp"
#include "io/network_loader.hpp"
#include <iostream>
#include <string>
#include <cstdlib>


static void usage() {
    std::cerr << "usage: sim --network <path> [--ticks N] [--seed S]\n";
}

int main(int argc, char** argv){

    std::string network_path;
    int ticks = 100;
    long seed = 42;

    for (int i=1; i<argc; ++i) {
        std::string a = argv[i];
        auto need_next = [&](const char* name) -> const char* {
            if (i + 1 >= argc) { usage(); std::exit(2);}
            (void)name;
            return argv[++i];
        };
        if (a == "--network") network_path = need_next("network");
        else if (a == "--ticks") ticks = std::atoi(need_next("ticks"));
        else if (a == "--seed") seed = std::atol(need_next("seed"));
        else { usage(); return 2; }

    }

    if (network_path.empty()) { usage(); return 2;}

    try {
        auto net = sim::io::load_network(network_path);
        sim::Simulation s(std::move(net));
        s.seed(static_cast<uint64_t>(seed));

        s.add_car_at(0, 0, 10.0, 0);
        s.add_car_at(2, 0, 20.0, 0);
        s.add_car_at(4, 0, 20.0, 0);
        s.add_car_at(6, 0, 20.0, 0);
        s.add_car_at(8, 0, 20.0, 0);

        for(int i=0; i < ticks; ++i) s.tick();
        s.dump_state(std::cout);

    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }

    return 0;


    
}

