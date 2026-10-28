#!/usr/bin/env python3
"""Fresh-process conventional worker. Timings are implementation observations only."""
import time
START = time.perf_counter_ns()
import argparse
import json
from pathlib import Path
from baseline_stack import ConventionalTaskRunner
IMPORTED = time.perf_counter_ns()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('operation', choices=('run', 'replay'))
    parser.add_argument('--store', required=True)
    parser.add_argument('--model')
    parser.add_argument('--policy')
    parser.add_argument('--tokenizer')
    parser.add_argument('--input')
    parser.add_argument('--bundle')
    args = parser.parse_args()
    load_start = time.perf_counter_ns()
    runner = ConventionalTaskRunner(args.store)
    if args.operation == 'run':
        model = Path(args.model).read_bytes()
        policy = Path(args.policy).read_bytes().decode("utf-8")
        tokenizer = Path(args.tokenizer).read_bytes().decode("utf-8")
        loaded = time.perf_counter_ns()
        result = runner.run_assess_fixed(model, args.input, policy, tokenizer)
        result['timings_ns']['fixture_loading_ns'] = loaded-load_start
    else:
        result = runner.verify_and_replay(args.bundle)
        if not result['valid']:
            raise ValueError('independent replay verification failed: ' + str(result))
    result['worker_module_initialization_ns'] = IMPORTED-START
    result['worker_total_ns'] = time.perf_counter_ns()-START
    print(json.dumps(result, ensure_ascii=False))


if __name__ == '__main__':
    main()
