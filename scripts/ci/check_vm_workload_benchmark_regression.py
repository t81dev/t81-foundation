#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path


def _parse_arg(name: str) -> int | None:
    m = re.search(r"/(\d+)$", name)
    if not m:
        return None
    return int(m.group(1))


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: check_vm_workload_benchmark_regression.py <bench.json>")
        return 2

    path = Path(sys.argv[1])
    data = json.loads(path.read_text(encoding="utf-8"))

    dispatch: dict[int, float] = {}
    native: dict[int, float] = {}
    for bench in data.get("benchmarks", []):
        name = str(bench.get("name", ""))
        arg = _parse_arg(name)
        if arg is None:
            continue
        real_time = float(bench.get("real_time", 0.0))
        if name.startswith("BM_VMSimulation_Dispatch/"):
            dispatch[arg] = real_time
        elif name.startswith("BM_NativeCall_Loop/"):
            native[arg] = real_time

    required_args = [32, 256]
    missing: list[str] = []
    for arg in required_args:
        if arg not in dispatch:
            missing.append(f"missing BM_VMSimulation_Dispatch/{arg}")
        if arg not in native:
            missing.append(f"missing BM_NativeCall_Loop/{arg}")
    if missing:
        print("vm workload benchmark regression check FAILED")
        for issue in missing:
            print(f"- {issue}")
        return 1

    # Guardrail: dispatch overhead should remain within bounded multiplier vs direct native loop.
    # This is a workload-level regression sentinel, not an absolute performance guarantee.
    ratio_limit = {32: 3.0, 256: 3.0}
    failures: list[str] = []
    for arg in required_args:
        ratio = dispatch[arg] / native[arg] if native[arg] > 0 else float("inf")
        print(
            f"arg={arg}: dispatch={dispatch[arg]:.2f} ns native={native[arg]:.2f} ns "
            f"ratio={ratio:.3f}x"
        )
        if ratio > ratio_limit[arg]:
            failures.append(
                f"dispatch/native ratio exceeded for arg={arg}: {ratio:.3f}x > {ratio_limit[arg]:.3f}x"
            )

    if failures:
        print("vm workload benchmark regression check FAILED")
        for issue in failures:
            print(f"- {issue}")
        return 1

    print("vm workload benchmark regression check PASSED")
    print("- benchmark families checked: BM_VMSimulation_Dispatch, BM_NativeCall_Loop")
    print("- args checked: 32, 256")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
