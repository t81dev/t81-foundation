#!/usr/bin/env python3
"""Independent suffix optimizer; exhaustive rational checks live in the audit tests.

This is a symbolic cost model over int64 traces, not an arithmetic benchmark.
"""
from copy import copy
from fractions import Fraction
import json
import math
from typing import List, Dict

BINARY_BITS = [8, 16, 32, 64]
TERNARY_TRITS = [6, 11, 21, 41]
BINARY_MIN = [-(1 << (b-1)) for b in BINARY_BITS]
BINARY_MAX = [(1 << (b-1))-1 for b in BINARY_BITS]
# Mathematical bounds, independently derived; inputs remain int64 for every arm.
TERNARY_MAX = [(3**n-1)//2 for n in TERNARY_TRITS]
TERNARY_MIN = [-v for v in TERNARY_MAX]


def validate_trace(trace):
    if any(type(v) is not int or not -(1 << 63) <= v < (1 << 63) for v in trace):
        raise ValueError("common int64 input domain required")


def get_binary_tier(v):
    validate_trace([v])
    return next(t for t, (a,b) in enumerate(zip(BINARY_MIN, BINARY_MAX)) if a <= v <= b)


def get_ternary_tier(v):
    validate_trace([v])
    return next(t for t, (a,b) in enumerate(zip(TERNARY_MIN, TERNARY_MAX)) if a <= v <= b)


class IndependentOracle:
    def __init__(self, symbol_cost_ratio=1.0, metadata_bits=2.0, block_size=1,
                 transition_cost_per_level=1.0, conversion_cost=0.5, binary_tier0_bits=8):
        if (type(block_size) is not int or block_size <= 0 or binary_tier0_bits not in (8,10)
            or any(not math.isfinite(c) or c < 0 for c in
                   (symbol_cost_ratio, metadata_bits, transition_cost_per_level, conversion_cost))):
            raise ValueError("invalid cost parameters")
        self.c3_over_c2 = symbol_cost_ratio
        self.metadata_bits = metadata_bits
        self.block_size = block_size
        self.transition_cost_per_level = transition_cost_per_level
        self.conversion_cost = conversion_cost
        self.binary_tier0_bits = binary_tier0_bits

    def evaluate(self, arm, trace, *, exact=False):
        validate_trace(trace)
        binary, adaptive = arm in ("FB", "AB"), arm in ("AB", "AT")
        widths = [self.binary_tier0_bits,16,32,64] if binary else TERNARY_TRITS
        ratio = 1 if binary else self.c3_over_c2
        # Retain shared overhead here, unlike C++ cancellation, but compare
        # exact rationals so it cannot swallow small schedule differences.
        exact_ratio = Fraction(ratio)
        exact_transition = Fraction(self.transition_cost_per_level)
        exact_metadata = Fraction(self.metadata_bits)
        exact_conversion = Fraction(self.conversion_cost)
        n = len(trace)
        blocks = [trace[i:i+self.block_size] for i in range(0,n,self.block_size)]
        path = [3] * n
        if adaptive and n:
            # Backwards suffix DP, contrasting with C++ prefix/backpointer DP.
            allowed = []
            for values in blocks:
                if binary:
                    tiers = [t for t,b in enumerate(widths)
                             if all(-(1 << (b-1)) <= v < (1 << (b-1)) for v in values)]
                else:
                    tiers = [t for t,k in enumerate(widths)
                             if all(abs(v) <= (3**k-1)//2 for v in values)]
                allowed.append(tiers)
            suffix = {None: (0, [])}
            for j in range(len(blocks)-1,-1,-1):
                current = {}
                for t in allowed[j]:
                    cost, tail = min(
                        (c + (0 if u is None else abs(u-t)*exact_transition), seq)
                        for u,(c,seq) in suffix.items())
                    local = len(blocks[j])*(widths[t]*exact_ratio+exact_conversion)+exact_metadata
                    current[t] = (local+cost, [t]+tail)
                suffix = current
            _, modes = min(suffix.values())
            path = [t for t,values in zip(modes,blocks) for _ in values]
        slots = sum(widths[t] for t in path)
        delta = sum(abs(a-b) for a,b in zip(path,path[1:])) if adaptive else 0
        meta = len(blocks)*self.metadata_bits if adaptive else 0
        checks = n if adaptive else 0
        total = (slots*exact_ratio + (len(blocks)*exact_metadata if adaptive else 0)
                 + delta*exact_transition + checks*exact_conversion) if exact else (
                     slots*ratio+meta+delta*self.transition_cost_per_level+checks*self.conversion_cost)
        return dict(arm=arm, total_elements=n, tier_sequence=path, symbol_slots=slots,
                    weighted_symbol_cost=slots*ratio, metadata_cost=meta,
                    transition_count=sum(a!=b for a,b in zip(path,path[1:])) if adaptive else 0,
                    transition_level_delta=delta, transition_cost=delta*self.transition_cost_per_level,
                    selector_checks=checks,
                    total_cost=total)

    def crossover(self, trace):
        """Independent exact-total bisection, including (not subtracting) overhead.

        Exact rational comparisons deliberately differ from C++'s analytical
        cancellation. The bracket is refined with 80 exact rational bisections.
        """
        if not trace:
            return Fraction(0)
        target = self.evaluate("AB", trace, exact=True)["total_cost"]
        candidate = copy(self)
        lo, hi = Fraction(0), Fraction(1)
        candidate.c3_over_c2 = hi
        while candidate.evaluate("AT", trace, exact=True)["total_cost"] < target:
            hi *= 2
            candidate.c3_over_c2 = hi
        for _ in range(80):
            mid = (lo+hi)/2
            candidate.c3_over_c2 = mid
            if candidate.evaluate("AT", trace, exact=True)["total_cost"] < target:
                lo = mid
            else:
                hi = mid
        return (lo+hi)/2

    def evaluate_fixed_binary(self, trace): return self.evaluate("FB", trace)
    def evaluate_fixed_ternary(self, trace): return self.evaluate("FT", trace)
    def evaluate_adaptive_binary_dp(self, trace): return self.evaluate("AB", trace)
    def evaluate_adaptive_ternary_dp(self, trace): return self.evaluate("AT", trace)


def generate_workloads(len_per_family: int = 100) -> Dict[str, List[int]]:
    workloads = {}

    # 1. Constant-small
    workloads["constant_small"] = [(i % 200) - 100 for i in range(len_per_family)]

    # 2. Constant-large
    workloads["constant_large"] = [
        1000000000000000000 + i for i in range(len_per_family)
    ]

    # 3. Phased
    phased = []
    p_len = max(1, len_per_family // 4)
    for i in range(len_per_family):
        p = i // p_len
        if p == 0:
            phased.append(50)
        elif p == 1:
            phased.append(10000)
        elif p == 2:
            phased.append(500000000)
        else:
            phased.append(2000000000000000000)
    workloads["phased"] = phased

    # 4. Alternating
    workloads["alternating"] = [
        42 if (i % 2 == 0) else 3000000000000000000 for i in range(len_per_family)
    ]

    # 5. Boundary Oscillation
    workloads["boundary_oscillation"] = [
        127 if (i % 2 == 0) else 128 for i in range(len_per_family)
    ]

    # 6. Signed Extremes
    targets = [
        -9223372036854775808,
        -2147483648,
        -128,
        0,
        127,
        9223372036854775807,
    ]
    workloads["signed_extremes"] = [
        targets[i % len(targets)] for i in range(len_per_family)
    ]

    # 7. Sparse Large
    workloads["sparse_large"] = [
        5000000000000000000 if (i % 50 == 0) else (i % 50)
        for i in range(len_per_family)
    ]

    # 8. Adversarial Transition
    t_vals = [10, 10000, 1000000000, 5000000000000000000]
    workloads["adversarial_transition"] = [
        t_vals[i % 4] for i in range(len_per_family)
    ]

    return workloads


def main():
    oracle = IndependentOracle()
    print(json.dumps({name: {arm: oracle.evaluate(arm, trace) for arm in ("FB","AB","FT","AT")}
                      for name,trace in generate_workloads().items()}, indent=2))


if __name__ == "__main__":
    main()
