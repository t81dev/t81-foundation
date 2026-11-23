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
        """Verify that the harness dry run completes across all 8 phases with Phase 4 verified."""
        harness = BenchmarkHarness(n=1000, k=64, a=1)
        res = harness.run_dry_run_validation()
        self.assertEqual(res["dry_run_status"], "PASSED")
        # Explicit smoke test labeling
        self.assertEqual(res["workload_mode"], "SMOKE_TEST")
        self.assertEqual(res["smoke_configuration"]["n"], 1000)
        self.assertEqual(res["smoke_configuration"]["num_queries"], 50)
        self.assertEqual(res["frozen_workload_reference"]["n"], 1048576)
        # Ensure all 8 phases are verified including Phase 4
        self.assertEqual(len(res["phases_verified"]), 8)
        self.assertIn("Phase 4: Retention Boundary", res["phases_verified"])
        # Check structured phase_status report
        self.assertIn("phase_status", res)
        self.assertEqual(res["phase_status"]["Phase 4: Retention Boundary"], "VERIFIED")
        self.assertEqual(res["phase_status"]["Phase 5: Frame Validation"], "VERIFIED")
        # Ensure retention evidence is present, ordering fence was called, and zero live input aliases remain
        self.assertIn("retention_evidence", res)
        self.assertEqual(res["retention_evidence"]["live_owned_buffers"], 0)
        self.assertTrue(res["retention_evidence"]["ordering_fence_called"])
        # Ensure raw input symbols are not leaked or retained as harness instance attributes
        self.assertFalse(hasattr(harness, "u_symbols"))
        self.assertFalse(hasattr(harness, "symbols"))

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

    def test_09_decision_rule_missing_or_partial_ci_rejected_as_invalid(self):
        """Check that missing or partial CIs fail closed as INVALID without fabricating CIs."""
        cv_good = {"T243": 0.01, "B10": 0.01, "B729": 0.01, "B729-u5": 0.01}
        full_query = {
            "B729-u5": (1.00, 0.96, 1.05),
            "B10": (1.02, 0.97, 1.08),
        }
        full_life = {
            "B729-u5": (1.18, 1.12, 1.25),
            "B10": (1.20, 1.14, 1.28),
            "B729": (1.50, 1.40, 1.65),
        }

        # Completely empty CIs when noise is low -> INVALID
        res = evaluate_decision_rules(cv_good, {}, {})
        self.assertEqual(res["classification"], "INVALID")
        self.assertIn("Incomplete or invalid confidence intervals", res["reason"])

        # Missing one required lifecycle arm (B729) even if other arms look favorable
        favorable_partial_life = {
            "B729-u5": (1.18, 1.12, 1.25),
            "B10": (1.20, 1.14, 1.28),
        }
        res_fav = evaluate_decision_rules(cv_good, favorable_partial_life, full_query)
        self.assertEqual(res_fav["classification"], "INVALID")
        self.assertIn("Incomplete or invalid confidence intervals", res_fav["reason"])

        # Missing one required query arm (B10)
        partial_query = {
            "B729-u5": (1.00, 0.96, 1.05),
        }
        res_q = evaluate_decision_rules(cv_good, full_life, partial_query)
        self.assertEqual(res_q["classification"], "INVALID")

        # Nonfinite values in CI
        nonfinite_life = dict(full_life)
        nonfinite_life["B729-u5"] = (1.18, float("nan"), 1.25)
        res_nan = evaluate_decision_rules(cv_good, nonfinite_life, full_query)
        self.assertEqual(res_nan["classification"], "INVALID")

        # Inverted CI bounds (lower > upper)
        inverted_life = dict(full_life)
        inverted_life["B10"] = (1.20, 1.30, 1.15)
        res_inv = evaluate_decision_rules(cv_good, inverted_life, full_query)
        self.assertEqual(res_inv["classification"], "INVALID")

        # Non-positive values in CI
        nonpos_life = dict(full_life)
        nonpos_life["B729"] = (1.50, 0.0, 1.65)
        res_nonpos = evaluate_decision_rules(cv_good, nonpos_life, full_query)
        self.assertEqual(res_nonpos["classification"], "INVALID")

        # Incomplete tuple length (e.g., 2 elements instead of 3)
        malformed_tuple_life = dict(full_life)
        malformed_tuple_life["B729-u5"] = (1.18, 1.12)  # type: ignore
        res_len = evaluate_decision_rules(cv_good, malformed_tuple_life, full_query)
        self.assertEqual(res_len["classification"], "INVALID")

        # Malformed non-numeric type (e.g., string or bool inside tuple)
        bad_type_life = dict(full_life)
        bad_type_life["B729-u5"] = (1.18, "invalid", 1.25)  # type: ignore
        res_type = evaluate_decision_rules(cv_good, bad_type_life, full_query)
        self.assertEqual(res_type["classification"], "INVALID")

        # Malformed non-tuple type entirely (e.g., int or None instead of tuple)
        bad_obj_life = dict(full_life)
        bad_obj_life["B10"] = 1.23  # type: ignore
        res_obj = evaluate_decision_rules(cv_good, bad_obj_life, full_query)
        self.assertEqual(res_obj["classification"], "INVALID")

    def test_10_high_noise_early_stop_without_calculating_ci(self):
        """Check Step 2: High noise stop legitimately returns INCONCLUSIVE without requiring CIs."""
        cv_high = {"T243": 0.06, "B10": 0.02, "B729": 0.01, "B729-u5": 0.02}
        # CIs can be completely omitted or empty on high-noise early stop
        res = evaluate_decision_rules(cv_high, {}, {})
        self.assertEqual(res["classification"], "INCONCLUSIVE")
        self.assertIn("Excessive run-to-run variation in T243: CV = 6.00% > 5.0%", res["reason"])

    def test_11_retention_boundary_negative_alias_leak_rejected(self):
        """Negative test: Retention barrier fails closed when an input alias remains live across Phase 4."""
        from experiments.h04_r03_matched_codec.retention import RetentionBarrier
        from experiments.h04_r03_matched_codec.measurement_engine import ProtocolError
        import hashlib
        from pathlib import Path
        import platform
        import subprocess
        import tempfile

        tmp = tempfile.TemporaryDirectory()
        bin_name = "fence.dylib" if platform.system() == "Darwin" else "fence.so"
        binary = Path(tmp.name) / bin_name
        source_path = Path(__file__).with_name("retention_fence.c")
        flag = "-dynamiclib" if platform.system() == "Darwin" else "-shared"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-fPIC", flag,
                        str(source_path), "-o", str(binary)], check=True, capture_output=True)
        sha = hashlib.sha256(binary.read_bytes()).hexdigest()

        retained_leak = []

        def leaky_barrier_factory(owners):
            # Adversarially capture and leak a strong reference to one of the owned input buffers
            retained_leak.append(owners[0])
            return RetentionBarrier(owners, str(binary), sha)

        harness = BenchmarkHarness(n=1000, k=64, a=1)
        with self.assertRaisesRegex(ProtocolError, "aliases remain live"):
            harness.run_dry_run_validation(retention_barrier_factory=leaky_barrier_factory)

        # Clear leaked reference and clean up
        retained_leak.clear()
        tmp.cleanup()

    def test_12_query_witness_negative_corrupted_query_detected(self):
        """Negative test: Corrupted random access queries fail closed against the query witness."""
        from unittest.mock import patch
        import experiments.h04_r03_matched_codec.codec as codec_mod

        harness = BenchmarkHarness(n=1000, k=64, a=1)

        original_admitted_ra = codec_mod.admitted_random_access

        def corrupt_admitted_ra(frame, index):
            val = original_admitted_ra(frame, index)
            if index == 10:
                return val + 1  # corrupt single query output
            return val

        with patch("experiments.h04_r03_matched_codec.codec.admitted_random_access", side_effect=corrupt_admitted_ra):
            with self.assertRaisesRegex(ValueError, "Query mismatch in .* against independent query witness"):
                harness.run_dry_run_validation()

    def test_13_retention_limitation_untracked_surviving_representation(self):
        """Document and verify retention boundary limitation regarding untracked surviving representations.

        Weak-reference retention enforcement tracks ONLY explicitly registered owners in RetentionBarrier.
        If an independent closure, external container, or undiscovered data structure captures a copy
        or slice of the raw input without being passed to RetentionBarrier, it is NOT detected by
        weak references alone. This test proves that:
        1. An untracked reference does not increment live_owned_buffers on registered owners.
        2. Therefore, retention guarantees are bounded to registered application buffers and
           cooperative lifecycle contracts, NOT absolute OS-level process memory guarantees.
        """
        from experiments.h04_r03_matched_codec.retention import OwnedSymbols, RetentionBarrier
        import hashlib
        from pathlib import Path
        import platform
        import subprocess
        import tempfile

        tmp = tempfile.TemporaryDirectory()
        bin_name = "fence.dylib" if platform.system() == "Darwin" else "fence.so"
        binary = Path(tmp.name) / bin_name
        source_path = Path(__file__).with_name("retention_fence.c")
        flag = "-dynamiclib" if platform.system() == "Darwin" else "-shared"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-fPIC", flag,
                        str(source_path), "-o", str(binary)], check=True, capture_output=True)
        sha = hashlib.sha256(binary.read_bytes()).hexdigest()

        # Registered input symbol array
        raw_symbols = OwnedSymbols([1, -2, 3, -4])

        # Adversarially create an untracked surviving representation (e.g. detached list copy)
        untracked_copy = list(raw_symbols)

        # Barrier registers only raw_symbols
        barrier = RetentionBarrier([raw_symbols], str(binary), sha)

        # Release registered symbols
        del raw_symbols

        # Barrier passes because registered owner was released, despite untracked_copy existing
        evidence = barrier.verify_release()
        self.assertEqual(evidence["live_owned_buffers"], 0)
        self.assertTrue(evidence["ordering_fence_called"])

        # untracked_copy survived undetected by the barrier
        self.assertEqual(len(untracked_copy), 4)

        del untracked_copy
        tmp.cleanup()


if __name__ == "__main__":
    unittest.main()
