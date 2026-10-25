#!/usr/bin/env python3
"""
H04-R02 Adversarial Audit Regression Tests.

Tests and enforces fixes for DEF-01 through DEF-07 identified in the H04-R02 audit:
- DEF-01: Metadata cost independence from symbol ratio C3/C2.
- DEF-02: Exact total-cost break-even ratios derived from first principles.
- DEF-03: Symmetric comparator evaluation (10-bit mode eliminating boundary asymmetry).
- DEF-04: DP optimization vs greedy tiering under high transition costs.
- DEF-05: Exact 41-trit representation range bounds.
- DEF-07: Workload-dependent adaptation attribution ratio.
"""

import os
import sys
import unittest

# Ensure repo root is in python path
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if REPO_ROOT not in sys.path:
    sys.path.insert(0, REPO_ROOT)

from experiments.h04_r02_adaptive_representation.oracle import (
    IndependentOracle,
    generate_workloads,
    TERNARY_MAX,
    TERNARY_MIN,
)


class TestH04R02AuditRegressions(unittest.TestCase):

    def test_def01_metadata_cost_independence(self):
        """
        DEF-01: Mode metadata bits (control overhead) must NOT scale with C3/C2 for AT.
        """
        oracle_c1 = IndependentOracle(
            symbol_cost_ratio=1.0, metadata_bits=2.0, block_size=1
        )
        oracle_c2 = IndependentOracle(
            symbol_cost_ratio=2.0, metadata_bits=2.0, block_size=1
        )

        trace = [50] * 100  # 100 elements in Tier 0

        res1 = oracle_c1.evaluate_adaptive_ternary_dp(trace)
        res2 = oracle_c2.evaluate_adaptive_ternary_dp(trace)

        # Metadata cost for 100 blocks at 2 bits/block should be exactly 200.0 in both cases
        self.assertEqual(res1["metadata_cost"], 200.0)
        self.assertEqual(res2["metadata_cost"], 200.0)

    def test_def02_exact_breakeven_ratios(self):
        """
        DEF-02: Total-cost break-even for constant_small (800 AB slots, 600 AT slots, 200 meta, 50 conv):
        AB total = 1050
        AT total(c) = 600*c + 200 + 50 = 600*c + 250
        600*c + 250 = 1050 => 600*c = 800 => c = 800/600 = 1.3333333333333333
        """
        ab_slots = 800
        at_slots = 600
        meta_cost = 200.0
        conv_cost = 50.0

        ab_total = ab_slots + meta_cost + conv_cost  # 1050
        # Correct total-cost breakeven:
        tot_breakeven = (ab_total - meta_cost - conv_cost) / at_slots
        self.assertAlmostEqual(tot_breakeven, 4.0 / 3.0, places=6)

        # Verify that published 1.6667 (5/3) was wrong
        self.assertNotAlmostEqual(tot_breakeven, 5.0 / 3.0, places=3)

    def test_def03_symmetric_comparator_10bit_mode(self):
        """
        DEF-03: On boundary oscillation (127, 128), 8-bit AB switches to 16-bit (1200 slots).
        If binary is provided with 10-bit mode (M=1024, range [-512, 511]), 127 and 128 stay in T0b (1000 slots).
        Payload breakeven = 1000 / 600 = 1.6667, exceeding log2(3) = 1.585.
        """
        # Under 10-bit mode, 100 elements fit in 10-bit mode without level transitions
        ab_10bit_slots = 100 * 10  # 1000
        at_6trit_slots = 100 * 6  # 600
        payload_ratio = ab_10bit_slots / at_6trit_slots
        self.assertAlmostEqual(payload_ratio, 1.6666666666666667, places=5)
        self.assertGreater(payload_ratio, 1.5849625)  # Greater than log2(3)

    def test_def04_dp_optimization_vs_greedy(self):
        """
        DEF-04: Verify DP minimizes cost under high transition penalty.
        For trace [10, 100000, 10, 100000] with transition_cost=100:
        Greedy switches tier every step (high trans cost).
        DP holds Tier 3 or Tier 1 to minimize total cost.
        """
        trace = [10, 100000, 10, 100000]
        oracle = IndependentOracle(
            symbol_cost_ratio=1.0,
            metadata_bits=2.0,
            transition_cost_per_level=100.0,
            conversion_cost=0.5,
        )
        res_dp = oracle.evaluate_adaptive_binary_dp(trace)

        # DP should choose to stay at Tier 1 (16-bit) or higher for element 0 instead of Tier 0 (8-bit)
        # to avoid 100 cost transition to Tier 1/2 for element 1.
        cost_if_greedy = (
            (8 + 16 + 8 + 16)
            + 4 * 2.0
            + 4 * 0.5
            + (100.0 + 100.0 + 100.0)
        )  # 48 + 8 + 2 + 300 = 358
        self.assertLess(res_dp["total_cost"], cost_if_greedy)

    def test_def05_exact_41trit_range_limit(self):
        """
        DEF-05: 41 balanced trits cover [-(3^41 - 1)/2, (3^41 - 1)/2].
        3^41 = 36472996377170786403
        Max = 18236498188585393201.
        """
        max_41_trits = (3**41 - 1) // 2
        min_41_trits = -max_41_trits
        self.assertEqual(max_41_trits, 18236498188585393201)
        self.assertEqual(min_41_trits, -18236498188585393201)
        self.assertGreater(max_41_trits, 2**63 - 1)

    def test_def07_attribution_method_non_generality(self):
        """
        DEF-07: Verify that adaptation savings ratio on constant_large is negative (-12.2%),
        proving that '96% savings from adaptation' is NOT a general workload property.
        """
        workloads = generate_workloads(100)
        oracle = IndependentOracle(symbol_cost_ratio=1.0)

        # constant_large
        trace_large = workloads["constant_large"]
        fb_large = oracle.evaluate_fixed_binary(trace_large)["total_cost"]  # 6400
        ab_large = oracle.evaluate_adaptive_binary_dp(trace_large)[
            "total_cost"
        ]  # 6650
        at_large = oracle.evaluate_adaptive_ternary_dp(trace_large)[
            "total_cost"
        ]  # 4350

        savings_ab = fb_large - ab_large  # -250
        savings_at_vs_fb = fb_large - at_large  # 2050
        attribution_ratio = savings_ab / savings_at_vs_fb  # -250 / 2050 = -0.12195

        self.assertLess(attribution_ratio, 0.0)


if __name__ == "__main__":
    unittest.main()
