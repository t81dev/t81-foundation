"""Unit tests for the four-arm benchmark harness and statistical decision protocol."""
import unittest

from experiments.h04_r03_matched_codec.benchmark_harness import (
    BenchmarkHarness,
    compute_bootstrap_ci,
    evaluate_decision_rules,
)


class TestBenchmarkHarnessAndStatistics(unittest.TestCase):
    """Test bootstrap estimation, decision rule evaluation, and harness dry-run."""

    def test_01_dry_run_validation(self):
        """Verify that the harness dry run completes successfully across all 8 phases."""
        harness = BenchmarkHarness(n=1000, k=64, a=1)
        res = harness.run_dry_run_validation()
        self.assertEqual(res["dry_run_status"], "PASSED")
        self.assertEqual(len(res["phases_verified"]), 8)

    def test_02_bootstrap_ci_calculation(self):
        """Verify bootstrap confidence interval calculation for paired ratios."""
        x = [10.0, 10.2, 9.8, 10.1, 10.0, 9.9, 10.3, 10.0, 10.1, 9.7] * 3
        y = [8.0, 8.1, 7.9, 8.0, 8.2, 8.0, 7.8, 8.1, 8.0, 7.9] * 3
        med_ratio, ci_l, ci_u = compute_bootstrap_ci(x, y, n_boot=2000, seed=123)
        self.assertAlmostEqual(med_ratio, 10.0 / 8.0, places=1)
        self.assertTrue(ci_l <= med_ratio <= ci_u)

    def test_03_decision_rule_invalid(self):
        """Check Step 1: Protocol invalidation maps to INVALID."""
        res = evaluate_decision_rules(
            cv_values={"T243": 0.01},
            ci_life={},
            ci_query={},
            protocol_valid=False,
            protocol_violation_reason="Checksum mismatch in trial 4",
        )
        self.assertEqual(res["classification"], "INVALID")

    def test_04_decision_rule_noise_inconclusive(self):
        """Check Step 2: High run-to-run variation (CV > 5%) maps to INCONCLUSIVE."""
        res = evaluate_decision_rules(
            cv_values={"T243": 0.06, "B10": 0.02, "B729": 0.01, "B729-u5": 0.02},
            ci_life={},
            ci_query={},
        )
        self.assertEqual(res["classification"], "INCONCLUSIVE")
        self.assertIn("CV = 6.00% > 5.0%", res["reason"])

    def test_05_decision_rule_practical_advantage_rejection(self):
        """Check Step 3: Practical advantage rejected if CI_upper < 1.10 vs any control."""
        res = evaluate_decision_rules(
            cv_values={"T243": 0.01, "B10": 0.01, "B729": 0.01, "B729-u5": 0.01},
            ci_life={
                "B729-u5": (1.04, 1.01, 1.07),  # CI_upper = 1.07 < 1.10 -> REJECT
                "B10": (1.20, 1.14, 1.26),
                "B729": (1.50, 1.42, 1.60),
            },
            ci_query={
                "B729-u5": (1.00, 0.97, 1.03),
                "B10": (1.02, 0.98, 1.06),
            },
        )
        self.assertEqual(res["classification"], "REJECT")
        self.assertIn("Practical advantage conclusively rejected", res["reason"])

    def test_06_decision_rule_query_inferiority_rejection(self):
        """Check Step 3: Query inferiority rejected if CI_upper < 0.95 vs bounded controls."""
        res = evaluate_decision_rules(
            cv_values={"T243": 0.01, "B10": 0.01, "B729": 0.01, "B729-u5": 0.01},
            ci_life={
                "B729-u5": (1.20, 1.12, 1.28),
                "B10": (1.20, 1.14, 1.26),
                "B729": (1.50, 1.42, 1.60),
            },
            ci_query={
                "B729-u5": (0.85, 0.80, 0.90),  # CI_upper = 0.90 < 0.95 -> REJECT
                "B10": (1.02, 0.98, 1.06),
            },
        )
        self.assertEqual(res["classification"], "REJECT")
        self.assertIn("Query inferiority", res["reason"])

    def test_07_decision_rule_full_acceptance(self):
        """Check Step 4: Full acceptance predicate satisfied when all conditions hold."""
        res = evaluate_decision_rules(
            cv_values={"T243": 0.01, "B10": 0.01, "B729": 0.01, "B729-u5": 0.01},
            ci_life={
                "B729-u5": (1.18, 1.12, 1.25),  # CI_lower >= 1.10
                "B10": (1.20, 1.14, 1.28),      # CI_lower >= 1.10
                "B729": (1.50, 1.40, 1.65),     # CI_lower >= 1.10
            },
            ci_query={
                "B729-u5": (1.00, 0.96, 1.05),  # CI_lower >= 0.95
                "B10": (1.02, 0.97, 1.08),      # CI_lower >= 0.95
            },
        )
        self.assertEqual(res["classification"], "ACCEPT")

    def test_08_decision_rule_inconclusive_straddle(self):
        """Check Step 5: Straddling interval yields INCONCLUSIVE."""
        res = evaluate_decision_rules(
            cv_values={"T243": 0.01, "B10": 0.01, "B729": 0.01, "B729-u5": 0.01},
            ci_life={
                "B729-u5": (1.10, 1.05, 1.15),  # Straddles 1.10: lower < 1.10, upper >= 1.10
                "B10": (1.20, 1.14, 1.28),
                "B729": (1.50, 1.40, 1.65),
            },
            ci_query={
                "B729-u5": (1.00, 0.96, 1.05),
                "B10": (1.02, 0.97, 1.08),
            },
        )
        self.assertEqual(res["classification"], "INCONCLUSIVE")


if __name__ == "__main__":
    unittest.main()
