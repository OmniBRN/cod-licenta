import subprocess, itertools, os
from pathlib import Path
from concurrent.futures import ProcessPoolExecutor

SEEDS = list(range(1, 11))
SCENARIOS = ["scenarios/networks/cross.json"]
DURATION = 12000
WARMUP = 6000
BIN = "./build/sim"
OUT = "data/output"

def run_one(args):
    scenario, seed = args
    tag = f"{Path(scenario).stem}_s{seed}"
    cmd = [BIN,
            "--scenario", scenario,
            "--seed", str(seed),
            "--ticks", str(DURATION),
            "--warmup", str(WARMUP),
            "--out", OUT]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print(f"FAILED {tag}{r.stderr[:300]}")
    else:
        print(f"OK {tag}")

if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    combos = list(itertools.product(SCENARIOS, SEEDS))
    workers = min(os.cpu_count() or 1, len(combos))
    print(f"Lansez {len(combos)} rulari pe {workers} fire...")
    with ProcessPoolExecutor(max_workers=workers) as ex:
        list(ex.map(run_one, combos))
    print("Gata. CSV-urile sunt in", OUT)