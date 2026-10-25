#!/usr/bin/env python3
"""
H04-R02 Independent Optimization Oracle and Verification Script.

This module provides an independently derived mathematical optimization reference
using dynamic programming over finite trace domains for adaptive binary (AB)
and adaptive balanced ternary (AT).
"""

import sys
import json
import math
from typing import List, Dict, Any, Tuple

# Precision Tier Capacity Tables
# Tier 0: 8-bit / 6 trits
# Tier 1: 16-bit / 11 trits
# Tier 2: 32-bit / 21 trits
# Tier 3: 64-bit / 41 trits

BINARY_BITS = [8, 16, 32, 64]
TERNARY_TRITS = [6, 11, 21, 41]

BINARY_MIN = [-128, -32768, -2147483648, -9223372036854775808]
BINARY_MAX = [127, 32767, 2147483647, 9223372036854775807]

TERNARY_MIN = [-364, -88573, -5230176601, -9223372036854775808]
TERNARY_MAX = [364, 88573, 5230176601, 9223372036854775807]


def get_binary_tier(val: int) -> int:
    for t in range(4):
        if BINARY_MIN[t] <= val <= BINARY_MAX[t]:
            return t
    return 3


def get_ternary_tier(val: int) -> int:
    for t in range(4):
        if TERNARY_MIN[t] <= val <= TERNARY_MAX[t]:
            return t
    return 3


class IndependentOracle:
    """
    Independently derived dynamic programming optimizer for AB and AT cost bounds.
    Does not call the C++ implementation.
    """

    def __init__(
        self,
        symbol_cost_ratio: float = 1.0,
        metadata_bits: float = 2.0,
        block_size: int = 1,
        transition_cost_per_level: float = 1.0,
        conversion_cost: float = 0.5,
    ):
        self.c3_over_c2 = symbol_cost_ratio
        self.metadata_bits = metadata_bits
        self.block_size = block_size
        self.transition_cost_per_level = transition_cost_per_level
        self.conversion_cost = conversion_cost

    def evaluate_fixed_binary(self, trace: List[int]) -> Dict[str, Any]:
        n = len(trace)
        symbol_slots = n * 64
        weighted_cost = float(symbol_slots)
        return {
            "arm": "FB",
            "symbol_slots": symbol_slots,
            "weighted_symbol_cost": weighted_cost,
            "metadata_cost": 0.0,
            "transition_count": 0,
            "transition_cost": 0.0,
            "total_cost": weighted_cost,
        }

    def evaluate_fixed_ternary(self, trace: List[int]) -> Dict[str, Any]:
        n = len(trace)
        symbol_slots = n * 41
        weighted_cost = float(symbol_slots) * self.c3_over_c2
        return {
            "arm": "FT",
            "symbol_slots": symbol_slots,
            "weighted_symbol_cost": weighted_cost,
            "metadata_cost": 0.0,
            "transition_count": 0,
            "transition_cost": 0.0,
            "total_cost": weighted_cost,
        }

    def evaluate_adaptive_binary_dp(self, trace: List[int]) -> Dict[str, Any]:
        """
        DP min-cost calculation over trace for AB.
        """
        n = len(trace)
        if n == 0:
            return {"arm": "AB", "total_cost": 0.0}

        # Required minimum tier for each element
        req_tiers = [get_binary_tier(v) for v in trace]

        # DP state: dp[i][t] = min cost for prefix i ending at tier t
        # Cost components for element i:
        # symbol_cost = BINARY_BITS[t]
        # metadata = metadata_bits if i % block_size == 0 else 0
        # selector_cost = conversion_cost
        # transition_cost = abs(t - prev_t) * transition_cost_per_level

        dp = [{} for _ in range(n)]

        for t in range(req_tiers[0], 4):
            cost = BINARY_BITS[t] + self.metadata_bits + self.conversion_cost
            dp[0][t] = (cost, [t])

        for i in range(1, n):
            req_t = req_tiers[i]
            meta = self.metadata_bits if (i % self.block_size == 0) else 0.0

            for t in range(req_t, 4):
                best_cost = float("inf")
                best_seq = []
                sym_cost = BINARY_BITS[t] + meta + self.conversion_cost

                for prev_t, (prev_cost, prev_seq) in dp[i - 1].items():
                    trans_cost = abs(t - prev_t) * self.transition_cost_per_level
                    tot = prev_cost + sym_cost + trans_cost
                    if tot < best_cost:
                        best_cost = tot
                        best_seq = prev_seq + [t]

                dp[i][t] = (best_cost, best_seq)

        best_final_cost = float("inf")
        best_final_seq = []
        for t, (cost, seq) in dp[n - 1].items():
            if cost < best_final_cost:
                best_final_cost = cost
                best_final_seq = seq

        # Calculate exact ledger metrics for the best path
        symbol_slots = sum(BINARY_BITS[t] for t in best_final_seq)
        meta_cost = (
            sum(1 for i in range(n) if i % self.block_size == 0) * self.metadata_bits
        )
        trans_count = sum(
            1 for i in range(1, n) if best_final_seq[i] != best_final_seq[i - 1]
        )
        trans_cost = sum(
            abs(best_final_seq[i] - best_final_seq[i - 1])
            * self.transition_cost_per_level
            for i in range(1, n)
        )

        return {
            "arm": "AB",
            "tier_sequence": best_final_seq,
            "symbol_slots": symbol_slots,
            "weighted_symbol_cost": float(symbol_slots),
            "metadata_cost": meta_cost,
            "transition_count": trans_count,
            "transition_cost": trans_cost,
            "total_cost": best_final_cost,
        }

    def evaluate_adaptive_ternary_dp(self, trace: List[int]) -> Dict[str, Any]:
        """
        DP min-cost calculation over trace for AT.
        """
        n = len(trace)
        if n == 0:
            return {"arm": "AT", "total_cost": 0.0}

        req_tiers = [get_ternary_tier(v) for v in trace]

        dp = [{} for _ in range(n)]

        # Metadata in ternary (scaled by symbol ratio)
        meta_base = self.metadata_bits * self.c3_over_c2

        for t in range(req_tiers[0], 4):
            cost = (
                TERNARY_TRITS[t] * self.c3_over_c2
                + meta_base
                + self.conversion_cost
            )
            dp[0][t] = (cost, [t])

        for i in range(1, n):
            req_t = req_tiers[i]
            meta = meta_base if (i % self.block_size == 0) else 0.0

            for t in range(req_t, 4):
                best_cost = float("inf")
                best_seq = []
                sym_cost = (
                    TERNARY_TRITS[t] * self.c3_over_c2
                    + meta
                    + self.conversion_cost
                )

                for prev_t, (prev_cost, prev_seq) in dp[i - 1].items():
                    trans_cost = abs(t - prev_t) * self.transition_cost_per_level
                    tot = prev_cost + sym_cost + trans_cost
                    if tot < best_cost:
                        best_cost = tot
                        best_seq = prev_seq + [t]

                dp[i][t] = (best_cost, best_seq)

        best_final_cost = float("inf")
        best_final_seq = []
        for t, (cost, seq) in dp[n - 1].items():
            if cost < best_final_cost:
                best_final_cost = cost
                best_final_seq = seq

        symbol_slots = sum(TERNARY_TRITS[t] for t in best_final_seq)
        meta_cost = (
            sum(1 for i in range(n) if i % self.block_size == 0) * meta_base
        )
        trans_count = sum(
            1 for i in range(1, n) if best_final_seq[i] != best_final_seq[i - 1]
        )
        trans_cost = sum(
            abs(best_final_seq[i] - best_final_seq[i - 1])
            * self.transition_cost_per_level
            for i in range(1, n)
        )

        return {
            "arm": "AT",
            "tier_sequence": best_final_seq,
            "symbol_slots": symbol_slots,
            "weighted_symbol_cost": float(symbol_slots) * self.c3_over_c2,
            "metadata_cost": meta_cost,
            "transition_count": trans_count,
            "transition_cost": trans_cost,
            "total_cost": best_final_cost,
        }


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
    p_len = len_per_family // 4
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


def calculate_crossover(trace: List[int]) -> Dict[str, float]:
    """
    Find break-even symbol cost ratio C3/C2 where AT total cost == AB total cost.
    """
    # Binary is fixed for a given trace at c3_over_c2 independent cost
    oracle_bin = IndependentOracle(
        symbol_cost_ratio=1.0, metadata_bits=2.0, transition_cost_per_level=0.0, conversion_cost=0.0
    )
    ab_res = oracle_bin.evaluate_adaptive_binary_dp(trace)
    ab_slots = ab_res["symbol_slots"]

    # AT slots
    oracle_ter = IndependentOracle(
        symbol_cost_ratio=1.0, metadata_bits=2.0, transition_cost_per_level=0.0, conversion_cost=0.0
    )
    at_res = oracle_ter.evaluate_adaptive_ternary_dp(trace)
    at_slots = at_res["symbol_slots"]

    payload_break_even = ab_slots / at_slots if at_slots > 0 else float("inf")

    return {
        "ab_slots": ab_slots,
        "at_slots": at_slots,
        "payload_break_even_c3_over_c2": payload_break_even,
    }


def main():
    oracle = IndependentOracle(symbol_cost_ratio=1.0)
    workloads = generate_workloads(len_per_family=100)

    summary = {}
    for name, trace in workloads.items():
        fb = oracle.evaluate_fixed_binary(trace)
        ab = oracle.evaluate_adaptive_binary_dp(trace)
        ft = oracle.evaluate_fixed_ternary(trace)
        at = oracle.evaluate_adaptive_ternary_dp(trace)
        crossover = calculate_crossover(trace)

        summary[name] = {
            "FB": fb,
            "AB": ab,
            "FT": ft,
            "AT": at,
            "crossover": crossover,
        }

    print(json.dumps(summary, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
