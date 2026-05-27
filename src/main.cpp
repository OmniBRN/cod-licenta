#include "core/simulation.hpp"
#include "gui/app.hpp"
#include "io/network_loader.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <filesystem>


static void usage() {
    std::cerr << "headless: sim --scenerio <json_path> [--ticks N] [--seed S] [--warmup N] --out <output_dir><\n"
              << "gui: sim --scenario <json_path> --seed N";
}

static std::string make_stamp(const std::string& sc, long seed) {
    auto base = std::filesystem::path(sc).stem().string();
    return base + "_s" + std::to_string(seed) + "_" + GIT_COMMIT_HASH;
}

int main(int argc, char** argv){
    std::string scenario;
    int ticks = 12000;
    long seed = 42;
    long warmup = 0;
    std::string out_dir;

    for (int i=1; i<argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> const char* {
            if (i + 1 >= argc) { usage(); std::exit(2);}
            return argv[++i];
        };
        if (a == "--scenario") scenario = next();
        else if (a == "--ticks") ticks = std::atoi(next());
        else if (a == "--seed") seed = std::atol(next());
        else if (a == "--warmup") warmup = std::atol(next());
        else if (a == "--out") out_dir = next();
        else { usage(); return 2; }

    }

    if (scenario.empty()) { usage(); return 2;}

    try {
        auto net = sim::io::load_network(scenario);
        sim::Simulation s(std::move(net));
        s.seed(static_cast<uint64_t>(seed));
        s.set_sources(sim::io::load_sources(scenario));
        s.set_lights(sim::io::load_lights(scenario));
        
        if(!out_dir.empty()) {
            s.set_output_dir(out_dir, make_stamp(scenario, seed));
            s.set_warmup_ticks(static_cast<sim::TickT>(warmup));
            for(int i=0; i < ticks; ++i) s.tick();
            std::cout << "spawned=" << s.spawned() << '\n'
                      << "exited=" << s.exited() << '\n'
                      << "in_net=" << s.in_network() << '\n';

        } else {
            // GUI
            sim::gui::App app(std::move(s));
            app.run();
        }

    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }

    return 0;


    
}

