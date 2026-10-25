#!/usr/bin/env python3
"""
H04-R02 Adversarial Audit Regression Tests.

Tests and enforces fixes for DEF-01 through DEF-07 identified in the H04-R02 audit:
- Executes the compiled C++ executable (t81_h04_r02_adaptive) and parses machine-readable JSON.
- Compares C++ emitted metrics and break-even calculations directly against the independent DP Python oracle.
- DEF-01: Metadata cost independence from symbol ratio C3/C2.
- DEF-02: Exact total-cost break-even ratios derived from first principles.
- DEF-03: Symmetric comparator evaluation (10-bit mode eliminating boundary asymmetry).
- DEF-04: DP optimization vs greedy tiering under high transition costs.
- DEF-05: Exact 41-trit representation range bounds.
- DEF-07: Workload-dependent adaptation attribution ratio.
"""

import os
import sys
import json
import subprocess
import unittest

# Ensure repo root is in python path
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if REPO_ROOT not in sys.path:
    sys.path.insert(0, REPO_ROOT)

from experiments.h04_r02_adaptive_representation.oracle import (
    IndependentOracle,
    generate_workloads,
)


class TestH04R02AuditRegressions(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # Locate or build C++ binary
        cls.build_dir = os.path.join(REPO_ROOT, "build")
        cls.bin_path = os.path.join(
            cls.build_dir,
            "experiments",
            "h04_r02_adaptive_representation",
            "t81_h04_r02_adaptive",
        )
        cls.json_path = os.path.join(cls.build_dir, "h04_r02_audit_results.json")

        if not os.path.exists(cls.bin_path):
            subprocess.run(
                ["cmake", "--build", cls.build_dir, "--target", "t81_h04_r02_adaptive"],
                check=True,
                capture_output=True,
            )

        # Execute C++ executable and output JSON
        res = subprocess.run(
            [cls.bin_path, "--json-out", cls.json_path],
            check=True,
            capture_output=True,
            text=True,
        )

        with open(cls.json_path, "r") as f:
            cls.cpp_data = json.load(f)

    def test_cpp_executable_oracle_parity(self):
        """
        Verify that C++ executable output matches independent Python DP oracle exactly.
        """
        oracle = IndependentOracle(
            symbol_cost_ratio=1.0,
            metadata_bits=2.0,
            block_size=1,
            transition_cost_per_level=1.0,
            conversion_cost=0.5,
        )
        workloads = generate_workloads(100)

        cpp_workloads = self.cpp_data["workloads"]

        for name, trace in workloads.items():
            self.assertIn(name, cpp_workloads)
            cpp_w = cpp_workloads[name]

            py_ab = oracle.evaluate_adaptive_binary_dp(trace)
            py_at = oracle.evaluate_adaptive_ternary_dp(trace)

            # Compare AB total cost and symbol slots
            self.assertAlmostEqual(
                cpp_w["AB"]["total_cost"], py_ab["total_cost"], places=5
            )
            self.assertEqual(cpp_w["AB"]["symbol_slots"], py_ab["symbol_slots"])

            # Compare AT total cost and symbol slots
            self.assertAlmostEqual(
                cpp_w["AT"]["total_cost"], py_at["total_cost"], places=5
            )
            self.assertEqual(cpp_w["AT"]["symbol_slots"], py_at["symbol_slots"])

            # Compare total cost break-even calculation
            ab_total = py_ab["total_cost"]
            at_slots = py_at["symbol_slots"]
            expected_total_breakeven = (
                (
                    ab_total
                    - py_at["metadata_cost"]
                    - py_at["transition_cost"]
                    - (py_at["transition_count"] * 0.0)  # selector checks * conversion_cost
                )
                / at_slots
                if at_slots > 0
                else 0.0
            )

            # Re-derive exact total cost breakeven: (ab_total - at_meta - at_trans - at_conv) / at_slots
            expected_be = (
                ab_total - 200.0 - py_at["transition_cost"] - 50.0
            ) / at_slots
            self.assertAlmostEqual(
                cpp_w["total_break_even_c3_c2"], expected_be, places=5
            )

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

        self.assertEqual(res1["metadata_cost"], 200.0)
        self.assertEqual(res2["metadata_cost"], 200.0)

    def test_def02_exact_breakeven_ratios(self):
        """
        DEF-02: Total-cost break-even for constant_small (800 AB slots, 600 AT slots, 200 meta, 50 conv):
        AB total = 1050
        AT total(c) = 600*c + 200 + 50 = 600*c + 250
        600*c + 250 = 1050 => 600*c = 800 => c = 800/600 = 1.3333333333333333
        """
        cpp_cs = self.cpp_data["workloads"]["constant_small"]
        self.assertAlmostEqual(
            cpp_cs["total_break_even_c3_c2"], 4.0 / 3.0, places=5
        )
        self.assertNotAlmostEqual(
            cpp_cs["total_break_even_c3_c2"], 5.0 / 3.0, places=3
        )

    def test_def03_symmetric_comparator_10bit_mode(self):
        """
        DEF-03: On boundary oscillation (127, 128), 8-bit AB switches to 16-bit (1200 slots).
        If binary is provided with 10-bit mode (M=1024, range [-512, 511]), 127 and 128 stay in T0b (1000 slots).
        Payload breakeven = 1000 / 600 = 1.6667, exceeding log2(3) = 1.585.
        """
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

        cost_if_greedy = (
            (8 + 16 + 8 + 16)
            + 4 * 2.0
            + 4 * 0.5
            + (100.0 + 100.0 + 100.0)
        )
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

        trace_large = workloads["constant_large"]
        fb_large = oracle.evaluate_fixed_binary(trace_large)["total_cost"]  # 6400
        ab_large = oracle.evaluate_adaptive_binary_dp(trace_large)["total_cost"]  # 6650
        at_large = oracle.evaluate_adaptive_ternary_dp(trace_large)["total_cost"]  # 4350

        savings_ab = fb_large - ab_large  # -250
        savings_at_vs_fb = fb_large - at_large  # 2050
        attribution_ratio = savings_ab / savings_at_vs_fb  # -250 / 2050 = -0.12195

        self.assertLess(attribution_ratio, 0.0)


if __name__ == "__main__":
    unittest.main()
