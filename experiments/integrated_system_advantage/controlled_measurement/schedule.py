"""Generate the accepted fixed design. This module launches no executions."""
import argparse
import hashlib
import json
from pathlib import Path
import random

SEED = 62708
BLOCKS = 60
SCENARIOS = (("greet_hello", "greet hello"), ("hello_world", "hello world"), ("greet", "greet"))
CONDITIONS = ("empty_store", "populated_store")
CELLS = tuple(f"{name}/{condition}" for name, _ in SCENARIOS for condition in CONDITIONS)


def canonical(data):
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()


def generate():
    rng = random.Random(SEED)
    orders = {}
    for cell in CELLS:
        value = [["A", "B"] for _ in range(30)] + [["B", "A"] for _ in range(30)]
        rng.shuffle(value)
        orders[cell] = value
    pairs = []
    prompts = dict(SCENARIOS)
    for block in range(BLOCKS):
        cells = list(CELLS)
        rng.shuffle(cells)
        for position, cell in enumerate(cells):
            scenario, condition = cell.split("/")
            pairs.append(dict(block=block, position=position, cell=cell,
                              prompt=prompts[scenario], condition=condition,
                              order=orders[cell][block]))
    return dict(schema="t81.i08.schedule.v1", seed=SEED, blocks=BLOCKS,
                pairs_per_cell=60, idle_between_blocks_seconds=10, pairs=pairs,
                measurement_authorized=False)


def write(path):
    data = generate()
    raw = canonical(data) + b"\n"
    with Path(path).open("xb") as stream:
        stream.write(raw)
    return hashlib.sha256(raw).hexdigest()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", required=True)
    print(write(parser.parse_args().out))
