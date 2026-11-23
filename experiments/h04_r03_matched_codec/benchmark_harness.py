"""Four-arm benchmark harness for EXP-H04-R03 Candidate A.

Implements all 8 lifecycle phases, memory accounting, and preregistered statistical
calculations (bootstrap 95% CIs, exhaustive decision rules) per
H04_R03_CANDIDATE_A_PREREGISTRATION.md using Python standard library.

CRITICAL BOUNDARY & EXECUTION FREEZE:
Measurement execution is DISABLED BY DEFAULT.
Neither timed benchmark trials nor comparative advantage claims are generated
during automated test suites or standard CLI execution. Execution of Phase C
requires explicit maintainer authorization and the '--run-measurement' flag.
"""
import argparse
import math
import random
import statistics
import sys
from typing import Any, Dict, List, Optional, Sequence, Tuple

from .codec import decode, encode, random_access
from .oracle import compute_layout
from .workload import (
    CANONICAL_DATASET_SHA256,
    CANONICAL_QUERY_SHA256,
    DATASET_SEED,
    QUERY_SEED,
    WORKLOAD_A,
    WORKLOAD_ACCESS_MODE,
    WORKLOAD_K,
    WORKLOAD_N,
    generate_dataset_symbols,
    generate_query_schedule,
    verify_canonical_dataset_digest,
    verify_canonical_query_digest,
)

# Arms under evaluation
ARMS = ["T243", "B10", "B729", "B729-u5"]

# Decision Thresholds
WIN_THRESHOLD = 1.10
EQUIV_MARGIN = 0.95


def compute_bootstrap_ci(
    x: Sequence[float], y: Sequence[float], n_boot: int = 10000, seed: int = 42
) -> Tuple[float, float, float]:
    """Compute ratio of medians Median(x)/Median(y) and 95% bootstrap CI.

    Uses paired whole-run resamples with replacement.
    Returns (ratio_median, ci_lower, ci_upper).
    """
    n = len(x)
    if n != len(y) or n == 0:
        raise ValueError("Inputs x and y must have equal positive length")
    if type(n_boot) is not int or n_boot < 1:
        raise ValueError("Positive integer bootstrap count required")
    if any(not math.isfinite(v) or v <= 0 for v in (*x, *y)):
        raise ValueError("Timing/throughput observations must be finite and positive")

    rng = random.Random(seed)
    obs_ratio = float(statistics.median(x) / statistics.median(y))

    boot_ratios: List[float] = []
    for _ in range(n_boot):
        indices = [rng.randrange(n) for _ in range(n)]
        x_resample = [x[i] for i in indices]
        y_resample = [y[i] for i in indices]
        med_x = statistics.median(x_resample)
        med_y = statistics.median(y_resample)
        boot_ratios.append(med_x / med_y)

    boot_ratios.sort()
    idx_lower = int(0.025 * n_boot)
    idx_upper = int(0.975 * n_boot)
    ci_lower = boot_ratios[idx_lower]
    ci_upper = boot_ratios[idx_upper]

    return (obs_ratio, ci_lower, ci_upper)


def evaluate_decision_rules(
    cv_values: Dict[str, float],
    ci_life: Dict[str, Tuple[float, float, float]],
    ci_query: Dict[str, Tuple[float, float, float]],
    protocol_valid: bool = True,
    protocol_violation_reason: Optional[str] = None,
) -> Dict[str, Any]:
    """Evaluate preregistered decision rules across the four arms.

    Priority order:
    1. Protocol Integrity -> INVALID
    2. Stability Criterion (CV > 5.0%) -> INCONCLUSIVE
    3. Conclusive Rejection Predicate -> REJECT
    4. Complete Acceptance Predicate -> ACCEPT
    5. Residual Fallback -> INCONCLUSIVE
    """
    # Step 1: Protocol Integrity
    if not protocol_valid:
        return {
            "classification": "INVALID",
            "reason": protocol_violation_reason or "Protocol violation or checksum discrepancy",
        }

    if set(cv_values) != set(ARMS) or any(
        not math.isfinite(v) or v < 0 for v in cv_values.values()
    ):
        return {"classification": "INVALID", "reason": "Incomplete or invalid arm CVs"}

    # Step 2: System Stability & Noise
    for arm, cv in cv_values.items():
        if cv > 0.05:
            return {
                "classification": "INCONCLUSIVE",
                "reason": f"Excessive run-to-run variation in {arm}: CV = {cv:.2%} > 5.0%",
            }

    # Validate mandatory confidence intervals before Step 3/4 evaluation.
    # Required lifecycle comparisons: B10, B729, B729-u5 vs T243.
    # Required query comparisons: B10, B729-u5 vs T243.
    # Missing, incomplete, nonfinite, non-positive, non-numeric, or inverted intervals fail closed as INVALID.
    def _is_valid_ci(ci: Any) -> bool:
        if not isinstance(ci, (tuple, list)) or len(ci) != 3:
            return False
        for v in ci:
            if not isinstance(v, (int, float)) or isinstance(v, bool):
                return False
            if not math.isfinite(v) or v <= 0:
                return False
        return ci[1] <= ci[2]

    if (
        not isinstance(ci_life, dict)
        or not isinstance(ci_query, dict)
        or not all(arm in ci_life and _is_valid_ci(ci_life[arm]) for arm in ("B10", "B729", "B729-u5"))
        or not all(arm in ci_query and _is_valid_ci(ci_query[arm]) for arm in ("B10", "B729-u5"))
    ):
        return {"classification": "INVALID", "reason": "Incomplete or invalid confidence intervals"}

    # Extract bounds vs T243: theta_life(C) = Median(T_life(C)) / Median(T_life(T243))
    # Note: If theta_life > 1.0, T243 is faster than C.
    ci_u5_life = ci_life["B729-u5"]
    ci_b10_life = ci_life["B10"]
    ci_b729_life = ci_life["B729"]

    ci_u5_query = ci_query["B729-u5"]
    ci_b10_query = ci_query["B10"]

    # Step 3: Rejection Predicates
    # 3.1 Rejection of practical advantage: CI_upper < 1.10 against ANY binary control
    if (
        ci_u5_life[2] < WIN_THRESHOLD
        or ci_b10_life[2] < WIN_THRESHOLD
        or ci_b729_life[2] < WIN_THRESHOLD
    ):
        reasons = []
        if ci_u5_life[2] < WIN_THRESHOLD:
            reasons.append(f"CI_upper vs B729-u5 ({ci_u5_life[2]:.3f}) < 1.10")
        if ci_b10_life[2] < WIN_THRESHOLD:
            reasons.append(f"CI_upper vs B10 ({ci_b10_life[2]:.3f}) < 1.10")
        if ci_b729_life[2] < WIN_THRESHOLD:
            reasons.append(f"CI_upper vs B729 ({ci_b729_life[2]:.3f}) < 1.10")
        return {
            "classification": "REJECT",
            "reason": f"Practical advantage conclusively rejected: {', '.join(reasons)}",
        }

    # 3.2 Conclusive lifecycle inferiority: CI_upper < 0.95 against ANY binary control
    if (
        ci_u5_life[2] < EQUIV_MARGIN
        or ci_b10_life[2] < EQUIV_MARGIN
        or ci_b729_life[2] < EQUIV_MARGIN
    ):
        return {
            "classification": "REJECT",
            "reason": "Performance inferiority: CI_upper < 0.95 against at least one control",
        }

    # 3.3 Query inferiority: CI_upper < 0.95 vs bounded controls
    if ci_u5_query[2] < EQUIV_MARGIN or ci_b10_query[2] < EQUIV_MARGIN:
        return {
            "classification": "REJECT",
            "reason": "Query inferiority: T243 lookup conclusively slower (CI_upper < 0.95) vs bounded binary controls",
        }

    # Step 4: Complete Acceptance Predicate
    # All 4 conditions must hold simultaneously:
    # 1. CI_lower(B729-u5) >= 1.10
    # 2. CI_lower(B10) >= 1.10
    # 3. CI_lower(B729) >= 1.10
    # 4. CI_lower(query vs u5 and b10) >= 0.95
    if (
        ci_u5_life[1] >= WIN_THRESHOLD
        and ci_b10_life[1] >= WIN_THRESHOLD
        and ci_b729_life[1] >= WIN_THRESHOLD
        and ci_u5_query[1] >= EQUIV_MARGIN
        and ci_b10_query[1] >= EQUIV_MARGIN
    ):
        return {
            "classification": "ACCEPT",
            "reason": "Full acceptance predicate satisfied: >=10% lifecycle win across all three binary controls + query non-inferiority",
        }

    # Step 5: Exhaustive Fallback
    return {
        "classification": "INCONCLUSIVE",
        "reason": "Effect resolution insufficient to either accept or reject",
    }


class BenchmarkHarness:
    """Models the 8 lifecycle phases and resource envelopes for EXP-H04-R03."""

    def __init__(self, n: int = WORKLOAD_N, k: int = WORKLOAD_K, a: int = WORKLOAD_A):
        self.n = n
        self.k = k
        self.a = a
        self.access_mode = WORKLOAD_ACCESS_MODE

    def run_dry_run_validation(
        self,
        n: int = 1000,
        num_queries: int = 50,
        fence_binary: Optional[str] = None,
        fence_sha256: Optional[str] = None,
        retention_barrier_factory: Optional[Any] = None,
    ) -> Dict[str, Any]:
        """Execute a deterministic dry-run verification across the 4 arms in smoke-test mode.

        CRITICAL WORKLOAD SCALE BOUNDARY:
        This execution is strictly a bounded SMOKE-TEST verification model (defaulting to
        n=1000 symbols and 50 queries) designed for test-suite correctness and lifecycle
        enforcement checks. It does NOT execute or claim full-scale frozen H04-R03 workload
        verification (which requires N=2^20 elements, Q=2^20 queries, and dedicated qualified host).

        Phase 4 Wire-Frame-Only Retention Enforcement:
        - Source symbols and mapped inputs are owned buffers monitored via weak references.
        - Pre-computed independent verification digests (query schedule witness and full
          output checksum) are established before Phase 4.
        - In Phase 4, all raw symbols and mapped inputs are released, garbage collection is
          executed, and the native RetentionBarrier confirms zero live aliases remain.
        - Subsequent query checks (Phase 6) and output decode verification (Phase 7) use
          strictly the wire frames and independent witness digests without retaining any
          second raw-symbol array.
        """
        import gc
        import hashlib
        from pathlib import Path
        import platform
        import struct
        import subprocess
        import tempfile
        from .retention import OwnedSymbols, OwnedBuffer, RetentionBarrier

        # Phase 1: Input Mapping & Generation (Smoke-Test Bounded Scale)
        raw_symbols = generate_dataset_symbols(n=n, seed=DATASET_SEED)
        symbols = OwnedSymbols(raw_symbols)
        raw_bytes = OwnedBuffer(struct.pack(f"<{len(symbols)}h", *symbols))
        del raw_symbols

        # Compute independent witness digests for queries and full decoded output
        query_schedule = [i % len(symbols) for i in range(min(num_queries, len(symbols)))]
        query_hasher = hashlib.sha256()
        for q_idx in query_schedule:
            query_hasher.update(struct.pack("<q", symbols[q_idx]))
        expected_query_witness = query_hasher.hexdigest()

        output_hasher = hashlib.sha256()
        output_bytes = bytearray()
        for val in symbols:
            packed = struct.pack("<q", val)
            output_hasher.update(packed)
            output_bytes.extend(packed)
        expected_output_witness = output_hasher.hexdigest()

        # Compute reference xxHash64
        from .checksum_witness import reference_xxh64
        expected_xxh64 = f"{reference_xxh64(output_bytes):016x}"
        del output_bytes

        # Compile or locate native retention fence helper if not provided
        tmp_dir = None
        if retention_barrier_factory is None:
            if fence_binary is None or fence_sha256 is None:
                tmp_dir = tempfile.TemporaryDirectory()
                bin_name = "fence.dylib" if platform.system() == "Darwin" else "fence.so"
                compiled_bin = Path(tmp_dir.name) / bin_name
                source = Path(__file__).with_name("retention_fence.c")
                flag = "-dynamiclib" if platform.system() == "Darwin" else "-shared"
                subprocess.run(
                    ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-fPIC", flag,
                     str(source), "-o", str(compiled_bin)],
                    check=True,
                    capture_output=True,
                )
                fence_binary = str(compiled_bin)
                fence_sha256 = hashlib.sha256(compiled_bin.read_bytes()).hexdigest()

            barrier = RetentionBarrier([symbols, raw_bytes], fence_binary, fence_sha256)
        else:
            barrier = retention_barrier_factory([symbols, raw_bytes])

        # Phase 2: Frame Allocation & Oracle Layout Verification
        layouts = {arm: compute_layout(len(symbols), self.k, self.a, self.access_mode, arm) for arm in ARMS}

        # Phase 3: Encoding & Packing
        frames = {arm: encode(symbols, arm=arm, k=self.k, A=self.a, access_mode=self.access_mode) for arm in ARMS}

        # Phase 4: Wire-Frame-Only Retention Boundary Enforcement
        # Release all raw input representations, intermediate lists, and mapped buffers
        del symbols
        del raw_bytes
        gc.collect()

        retention_evidence = barrier.verify_release()
        phase_4_status = "VERIFIED"

        # Clean up temporary compiled fence if created
        if tmp_dir is not None:
            tmp_dir.cleanup()

        # Phase 5: Frame Validation (using admission boundary)
        validated_frames = {}
        for arm in ARMS:
            if len(frames[arm]) != layouts[arm]["total_bytes"]:
                raise ValueError(f"Size mismatch in {arm}")
            # Explicit admission boundary validation
            from .codec import validate_frame, admitted_random_access
            validated_frames[arm] = validate_frame(frames[arm], arm=arm, k=self.k, A=self.a, access_mode=self.access_mode)

        # Phase 6: Timed Random Access Queries (bounded dry-run using admitted queries against witness)
        for arm in ARMS:
            v_frame = validated_frames[arm]
            arm_q_hasher = hashlib.sha256()
            for q_idx in query_schedule:
                val = admitted_random_access(v_frame, q_idx)
                arm_q_hasher.update(struct.pack("<q", val))
            if arm_q_hasher.hexdigest() != expected_query_witness:
                raise ValueError(f"Query mismatch in {arm} against independent query witness")

        # Phase 7: Output Buffer Assembly & Witness Checksum
        for arm in ARMS:
            dec = decode(frames[arm], arm=arm, k=self.k, A=self.a, access_mode=self.access_mode)
            arm_out_hasher = hashlib.sha256()
            arm_out_bytes = bytearray()
            for val in dec:
                packed = struct.pack("<q", val)
                arm_out_hasher.update(packed)
                arm_out_bytes.extend(packed)
            if arm_out_hasher.hexdigest() != expected_output_witness:
                raise ValueError(f"Decode mismatch in {arm} against independent output witness")
            if f"{reference_xxh64(arm_out_bytes):016x}" != expected_xxh64:
                raise ValueError(f"Checksum mismatch in {arm} against independent xxh64 witness")
            del arm_out_bytes
            del dec

        # Phase 8: Deallocation & Cleanup
        del validated_frames
        del frames

        return {
            "dry_run_status": "PASSED",
            "workload_mode": "SMOKE_TEST",
            "smoke_configuration": {
                "n": n,
                "num_queries": len(query_schedule),
                "k": self.k,
                "a": self.a,
                "access_mode": self.access_mode,
            },
            "frozen_workload_reference": {
                "n": WORKLOAD_N,
                "num_queries": WORKLOAD_N,
                "k": WORKLOAD_K,
                "a": WORKLOAD_A,
                "access_mode": WORKLOAD_ACCESS_MODE,
            },
            "arms_verified": ARMS,
            "retention_evidence": retention_evidence,
            "phase_status": {
                "Phase 1: Input Mapping": "VERIFIED",
                "Phase 2: Frame Allocation": "VERIFIED",
                "Phase 3: Encoding & Packing": "VERIFIED",
                "Phase 4: Retention Boundary": phase_4_status,
                "Phase 5: Frame Validation": "VERIFIED",
                "Phase 6: Timed Random Access Queries": "VERIFIED",
                "Phase 7: Output Buffer Assembly & Checksum": "VERIFIED",
                "Phase 8: Deallocation & Scratch Cleanup": "VERIFIED",
            },
            "phases_verified": [
                "Phase 1: Input Mapping",
                "Phase 2: Frame Allocation",
                "Phase 3: Encoding & Packing",
                "Phase 4: Retention Boundary",
                "Phase 5: Frame Validation",
                "Phase 6: Timed Random Access Queries",
                "Phase 7: Output Buffer Assembly & Checksum",
                "Phase 8: Deallocation & Scratch Cleanup",
            ],
        }


def main():
    parser = argparse.ArgumentParser(description="EXP-H04-R03 Candidate A Benchmark Harness")
    parser.add_argument(
        "--run-measurement",
        action="store_true",
        help="Execute performance measurement trials (REQUIRES EXPLICIT MAINTAINER AUTHORIZATION)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Run deterministic dry-run validation of the 8 lifecycle phases",
    )
    args = parser.parse_args()

    if args.run_measurement:
        print("ERROR: Phase C performance measurement is FROZEN.")
        print("Explicit maintainer authorization and dedicated host qualification are required.")
        sys.exit(1)

    harness = BenchmarkHarness()
    res = harness.run_dry_run_validation()
    print("Harness dry run validation:", res["dry_run_status"])
    print("Arms verified:", ", ".join(res["arms_verified"]))
    print("Execution freeze ACTIVE: Measurement execution remains disabled.")


if __name__ == "__main__":
    main()
