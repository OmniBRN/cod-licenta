# Microscopic Traffic Simulator

A microscopic road-traffic simulator written in C++17, built as my bachelor's degree
project in Computer Science at the University of Bucharest (thesis and project graded
**10/10**).

Each vehicle is simulated individually: it is assigned a driver behaviour profile, a
route computed over the road network, and it reacts every tick to the car in front, to
traffic lights, and to the surrounding lanes. The simulator can run either with a
real-time SFML/ImGui visualisation or headless, in which case it writes per-trip and
per-tick metrics to CSV for statistical analysis.

## Contents

- [Model](#model)
- [Building](#building)
- [Running](#running)
- [Scenario format](#scenario-format)
- [Output metrics](#output-metrics)
- [Tests](#tests)
- [Project layout](#project-layout)

## Model

**Time and space.** The simulation advances in fixed ticks of `0.1 s`. The road network
is a directed graph: nodes are sources, sinks or junctions, and edges are polylines with
a number of forward lanes and a speed limit. Positions are stored as
`(edge, lane, offset)` and converted to world coordinates only for rendering.

**Car following.** Longitudinal acceleration comes from the Intelligent Driver Model
(IDM), using the leader's gap and relative speed. Leaders are found through a per-tick
lane index, and a red light on the current edge acts as a virtual stopped leader at the
stop line.

**Routing.** Routes are computed with A\* over the edge graph from the spawn node to the
assigned destination. A car also keeps an *intended lane*, derived from the turn map of
the network: if the next edge on its route can only be reached from certain lanes, the
car will try to reach the closest of them.

**Lane changing.** Changes are either mandatory (to reach the intended lane) or
discretionary (the current leader is too slow). A change is only started if the gap in
the target lane is accepted; the safe front and rear gaps shrink with the driver's
aggressiveness. Cars move one lane at a time, the transition is interpolated over 15
ticks so it is visible in the GUI, and changes inside an intersection are forbidden.

**Junctions.** Traffic lights cycle green → yellow → red with per-edge durations and a
configurable starting colour, so intersections can be kept in phase. Cars that are about
to take a turn brake in advance to a junction crossing speed.

**Driver archetypes.** Five behaviour profiles differ in desired speed, maximum
acceleration, comfortable braking, reaction time, minimum headway, aggressiveness and
traffic-law compliance:

| Id | Profile | Desired speed | Aggressiveness | Notes |
|----|--------------|---------------|----------------|-------|
| 0 | `ideal` | 50 km/h | 0.00 | reference driver, never breaks a rule |
| 1 | `cautious` | 40 km/h | 0.10 | large headway, slow reactions |
| 2 | `normal` | 50 km/h | 0.50 | baseline |
| 3 | `aggresive` | 70 km/h | 0.90 | speeds, short headway, hard braking |
| 4 | `opportunist` | 50 km/h | 0.85 | legal speed, but constantly hunting for gaps |

Each source spawns cars according to an `archetype_mix`, so a scenario can be run with
different populations of drivers and the resulting throughput compared.

**Reproducibility.** All randomness (arrivals, archetype and destination sampling,
compliance, lane-change decisions) is drawn from separate `mt19937_64` streams derived
from a single master seed via a SplitMix64 finalizer, so adjacent seeds stay
uncorrelated and a run is fully determined by `--seed`. Arrivals at each source follow a
Poisson process with the configured rate. The simulator also asserts vehicle
conservation (`spawned == exited + in_network`) on every tick.

## Building

Requirements:

- CMake ≥ 3.22 and a C++17 compiler (tested with GCC on Linux)
- the system packages SFML needs to build (X11, OpenGL, udev headers)
- an internet connection on the first configure, for Catch2

SFML, Dear ImGui and ImGui-SFML are git submodules; [nlohmann/json](https://github.com/nlohmann/json)
3.12.0 is vendored in `include/json`.

```bash
git clone --recursive <repo-url> cod-licenta
cd cod-licenta
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

If the repository was cloned without `--recursive`:

```bash
git submodule update --init --recursive
```

An address/UB sanitizer build is available with `-DSIM_SANITIZE=ON`.

## Running

### GUI

Omitting `--out` starts the interactive mode:

```bash
./build/sim --scenario scenarios/networks/cross.json --seed 42
```

Controls: middle mouse button drags the camera, the scroll wheel zooms, left click
selects a car and opens an inspector showing its archetype, edge, lane, speed,
acceleration, trip distance, lane changes and intended lane. The toolbar has
pause/step/speed (1x, 2x, 10x) and a panel with the current tick, the number of cars in
the network and throughput averaged over the last 100 ticks. Cars are coloured by
archetype.

### Headless

Passing `--out` runs the simulation as fast as possible and writes metrics instead of
opening a window:

```bash
./build/sim --scenario scenarios/networks/grid2x2_aggressive.json \
            --seed 1 --ticks 12000 --warmup 6000 --out data/output
```

| Flag | Default | Meaning |
|------|---------|---------|
| `--scenario <path>` | required | scenario JSON |
| `--seed <n>` | `42` | master seed |
| `--ticks <n>` | `12000` | number of ticks to simulate (12000 ticks = 20 min) |
| `--warmup <n>` | `0` | ticks discarded before metrics start being recorded |
| `--out <dir>` | — | output directory; if absent, the GUI is launched |

The warmup exists so that the measurements are taken on a network that has already
filled up, instead of on the transient from an empty road.

### Batch experiments

`scripts/run_experiments.py` sweeps scenarios × seeds in parallel and drops the CSVs in
`data/output`. Edit `SCENARIOS`, `SEEDS`, `DURATION` and `WARMUP` at the top of the file,
then:

```bash
python3 scripts/run_experiments.py
```

### Scenarios

`scenarios/networks` contains the networks used for the experiments: a single merge, a
lane drop, a turn lane, a weaving section, a four-way signalised intersection, a
signalised corridor and a 2×2 grid of intersections. The `_normal`, `_aggressive` and
`_opportunist` variants are the same geometry with different `archetype_mix` values,
which is what the comparisons in the thesis are based on.

## Scenario format

A scenario is a single JSON file describing the network, the traffic lights and the
demand:

```json
{
  "nodes": [
    { "id": 0, "pos": [-1500, 0], "kind": "source" },
    { "id": 1, "pos": [0, 0],     "kind": "junction" },
    { "id": 2, "pos": [1500, 0],  "kind": "sink" }
  ],
  "edges": [
    { "id": 0, "from": 0, "to": 1, "lanes_forward": 2,
      "polyline": [[-1500, 0], [0, 0]], "speed_limit_kmh": 50 }
  ],
  "turn_map": [
    { "from_edge": 0, "from_lane": 0, "to_edge": 1, "to_lane": 0 }
  ],
  "lights": [
    { "edge": 0, "green_dur": 300, "yellow_dur": 30, "red_dur": 300,
      "start_color": "green" }
  ],
  "sources": [
    { "node": 0, "rate_per_sec": 0.9,
      "archetype_mix": [0, 0.3, 0.45, 0.25, 0],
      "destinations": [2], "destination_mix": [1.0] }
  ]
}
```

Notes:

- `kind` is `source`, `sink` or `junction` (the default).
- `speed_limit_kmh` is optional and defaults to 50.
- `turn_map` declares which lane of an incoming edge may feed which lane of an outgoing
  edge; it is what drives lane-exclusive turns.
- light durations are in ticks, `start_color` is `green`, `yellow` or `red`.
- `archetype_mix` has one entry per profile, in the order of the table above, and
  `destination_mix` one per destination; both must sum to 1. The loader validates this
  and rejects malformed scenarios.

## Output metrics

Each headless run writes two CSV files into the output directory, stamped with the
scenario name, the seed and the current git commit hash
(e.g. `trips_cross_s1_bc84aec.csv`), so results can always be traced back to the code
that produced them.

`trips_*.csv`, one row per vehicle that reached its destination:

| Column | Description |
|--------|-------------|
| `car_id` | vehicle id |
| `spawn_tick`, `exit_tick` | entry and exit tick (travel time = difference × 0.1 s) |
| `trip_distance_m` | distance actually driven |
| `avg_speed_mps` | trip distance divided by travel time |
| `lane_changes` | number of completed lane changes |
| `violations` | reserved for red-light violations, currently not counted |
| `archetype` | name of the driver profile |

`ticks_*.csv`, one row per tick: `tick`, `in_network` (vehicles currently on the
network) and `throughput` (vehicles that exited during that tick). Together they give
the flow-density behaviour of a scenario.

## Tests

The test suite uses Catch2 (fetched automatically by CMake) and covers the geometry
helpers, the network loader, the RNG streams, the IDM formula, A\* routing, intended-lane
selection, traffic lights, lane changing, the metrics writer and a few end-to-end runs:

```bash
cmake --build build -j
ctest --test-dir build --output-on-failure
# or directly:
./build/sim_tests
```

## Project layout

```
src/core      network, cars, IDM, routing, traffic lights, simulation loop
src/sim       spawner (Poisson arrivals, archetype and destination sampling)
src/io        scenario loader, CSV metrics writer
src/rng       seeded RNG streams
src/gui       SFML renderer and ImGui panels
scenarios/    scenario definitions used in the experiments
scripts/      batch experiment runner
tests/        Catch2 test suite
include/      SFML, Dear ImGui, ImGui-SFML (submodules) and nlohmann/json
```

## Author

Tudor Boureanu — Faculty of Mathematics and Computer Science, University of Bucharest.
