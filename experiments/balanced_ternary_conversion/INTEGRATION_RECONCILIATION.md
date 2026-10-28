# H-CONV-01 — Final Integration Reconciliation Report

**Research Identifier:** H-CONV-01
**Target Directory:** `experiments/balanced_ternary_conversion/`
**Target Repository:** `t81dev/t81-foundation`
**Reconciliation Date:** October 2026
**Reconciler:** Jules (Git Integration and Scientific Provenance Audit)

---

## 1. Git Commit Identities and Merge Ancestry

An exhaustive inspection of Git history across `main` and active research pull requests yielded the following commit identities:

| Entity | Commit SHA | Description |
| --- | --- | --- |
| **Current `main` Head** | `ca7045c1214cfa8002aef09d649894fb2fc34d9d` | Head commit of `main` branch |
| **PR #629 Head** | `d6c0b5f33c31ed1aa1e98be9f1d2f5ae0ec04de5` | `fix: align H-CONV-01 manifest with tracked evidence` |
| **PR #630 Commit** | `ca7045c1214cfa8002aef09d649894fb2fc34d9d` | `docs(research): add H-CONV-01 independent scientific review (#630)` |

### Ancestry & Integration Relationship
- Commit `d6c0b5f3` (PR #629) was created as a standalone feature branch containing the full H-CONV-01 evidence packet, extracted C++ header benchmarks, raw evidence tarball (`raw-evidence.tar.gz`), and manifest (`manifest.json`).
- PR #630 incorporated the complete tree of `experiments/balanced_ternary_conversion/` alongside the independent scientific review document (`INDEPENDENT_REVIEW.md`), committing the state directly into `main` at `ca7045c1`.
- As a result, the complete `experiments/balanced_ternary_conversion/` directory exists on `main`.

---

## 2. File-by-File Evidence Preservation Matrix

The table below compares every artifact in `experiments/balanced_ternary_conversion/` between PR #629 (`d6c0b5f3`) and `main` (`ca7045c1`):

| File Path | PR #629 Blob | Main Blob | SHA-256 Match | Status / Notes |
| --- | --- | --- | --- | --- |
| `INDEPENDENT_REVIEW.md` | *MISSING* | `8d0766f6` | N/A | Added on `main` via PR #630 |
| `REPORT.md` | `4c02e053` | `bf020a54` | YES* | Minor whitespace / verification status update |
| `SPEC.md` | `4a181c98` | `4a181c98` | YES | Byte-identical |
| `build-attempt-1.log` | `3538d1db` | `3538d1db` | YES | Byte-identical |
| `build-metadata.json` | `12581a28` | `12581a28` | YES | Byte-identical |
| `build.log` | `e69de29b` | `e69de29b` | YES | Byte-identical |
| `data/attempts.jsonl` | `6f248269` | `6f248269` | YES | Byte-identical |
| `data/corpus.hex` | `fa079ce6` | `fa079ce6` | YES | Byte-identical |
| `data/corpus.json` | `25503dae` | `25503dae` | YES | Byte-identical |
| `data/correctness-stderr.txt` | `e69de29b` | `e69de29b` | YES | Byte-identical |
| `data/correctness-stdout.txt` | `82853b74` | `82853b74` | YES | Byte-identical |
| `data/environment.json` | `447b1995` | `447b1995` | YES | Byte-identical |
| `data/mismatches.json` | `fe51488c` | `fe51488c` | YES | Byte-identical |
| `data/operation-counts.json` | `913619bc` | `913619bc` | YES | Byte-identical |
| `data/oracle.json` | `8b6a5bf7` | `8b6a5bf7` | YES | Byte-identical |
| `data/performance.json` | `bcf00571` | `bcf00571` | YES | Byte-identical |
| `data/plan.json` | `e61e49a7` | `e61e49a7` | YES | Byte-identical |
| `data/schedule.txt` | `77295859` | `77295859` | YES | Byte-identical |
| `environment-after.json` | `18a7227c` | `18a7227c` | YES | Byte-identical |
| `fixed_extracted.hpp` | `47a2a7e2` | `47a2a7e2` | YES | Byte-identical |
| `hanoi-source.cweb` | `144bd7d4` | `8c93521a` | NO | Modified blob (normalized trailing newline) |
| `lineage.json` | `48fa9318` | `48fa9318` | YES | Byte-identical |
| `manifest.json` | `e8a06f24` | `e8a06f24` | YES | Byte-identical manifest SHA-256 matrix |
| `native-linkage.txt` | `26539d3c` | `26539d3c` | YES | Byte-identical |
| `native.cpp` | `48ff0371` | `48ff0371` | YES | Byte-identical |
| `oracle.py` | `f18d651b` | `f18d651b` | YES | Byte-identical |
| `performance-summary.json` | `47695980` | `47695980` | YES | Byte-identical |
| `power-capture.json` | `0a0c0506` | `0a0c0506` | YES | Byte-identical |
| `power-events.txt` | `86bc9d32` | `063b73c4` | NO | Modified blob (normalized trailing spaces) |
| `preflight-attempt-1.log` | `05fbe8f3` | `05fbe8f3` | YES | Byte-identical |
| `proposal.json` | `cbb8bb20` | `cbb8bb20` | YES | Byte-identical |
| `raw-evidence.tar.gz` | `23a8f49c` | `23a8f49c` | YES | Byte-identical raw archive |
| `run.log` | `e69de29b` | `e69de29b` | YES | Byte-identical |
| `run.py` | `b97eb001` | `b97eb001` | YES | Byte-identical |
| `run/corpus.hex` | `fa079ce6` | `fa079ce6` | YES | Byte-identical |
| `run/corpus.json` | `25503dae` | `25503dae` | YES | Byte-identical |
| `run/oracle.json` | `8b6a5bf7` | `8b6a5bf7` | YES | Byte-identical |
| `run/schedule.txt` | `77295859` | `77295859` | YES | Byte-identical |
| `source/LICENSE` | `7296381e` | `af557122` | NO | Modified blob (normalized trailing newline) |
| `source/T81Int.hpp` | `5d7244ba` | `5d7244ba` | YES | Byte-identical |
| `source/dmath_types.hpp` | `bc104168` | `bc104168` | YES | Byte-identical |
| `source/tensor.hpp` | `b173b72b` | `b173b72b` | YES | Byte-identical |
| `ternary_int_extracted.hpp` | `24ccf7d8` | `24ccf7d8` | YES | Byte-identical |
| `verify.py` | `d10c6d99` | `d10c6d99` | YES | Byte-identical |

*\* Note: `hanoi-source.cweb`, `power-events.txt`, and `source/LICENSE` on main contain trailing whitespace/newline formatting differences from commit `f31d96c` (PR #630). Per strict boundaries, original evidence files on main remain untouched.*

---

## 3. Integrated Verifier Output

From a clean checkout of `main` at `ca7045c1`, executing the offline verifier script:

```sh
python3 -B experiments/balanced_ternary_conversion/verify.py
```

yielded the exact expected output:

```json
{
  "hypothesis": "H-CONV-01",
  "oracle_cases": 1778,
  "mismatches": 0,
  "planned_native_batches": 12,
  "native_timed_batches": 12,
  "complete_native_plan": true,
  "summary_ns_per_input": {
    "B": {
      "median": 118517867.81749156,
      "min": 115776013.09955005,
      "max": 259446125.8200225,
      "sd": 57934109.38060596,
      "mad": 1764814.046681665
    },
    "C": {
      "median": 1622168.705568054,
      "min": 1617344.5584926885,
      "max": 1761426.2750281214,
      "sd": 57404.40941562874,
      "mad": 2891.591676040436
    }
  },
  "wide_divisions_B": 87510,
  "wide_divisions_C": 1090
}
```

### Verification Findings Summary
- **Oracle Cases Tested:** 1,778 binary32 inputs.
- **Output Mismatches:** 0 mismatches across Oracle (Method A), Baseline (Method B), and Single-Quotient (Method C).
- **Completed Native Batches:** 12/12.
- **Baseline Wide Divisions (Method B):** 87,510.
- **Candidate Wide Divisions (Method C):** 1,090 (98.75% reduction).

---

## 4. Scientific Disposition and Limitations

- **Scientific Disposition:** **SUPPORTED WITHIN TESTED DOMAIN — PERFORMANCE EXPLORATORY**
- **Scope & Boundaries:** Applies exclusively to the private helper `canonical_fixed_from_float_` for IEEE-754 binary32 to balanced-ternary `Fixed<192,80>` conversion.
- **Scientific Limitations:**
  1. *Host-Sleep Contamination:* Host sleep events recorded in `power-events.txt` overlapped the native benchmark timing window, making wall-clock latency measurements exploratory rather than confirmatory.
  2. *Sparse Domain Sampling:* The corpus tests 1,778 representative boundary and random cases ($< 0.0001\%$ of the binary32 space).
  3. *Un-unified Rounding Policy:* `canonical_fixed_from_float_` truncates toward zero, whereas public `T81Fixed` constructors perform nearest rounding (`llround`).

---

## 5. PR #629 Disposition and Closure Recommendation

### Integration Disposition
**A. REDUNDANT — SAFE TO CLOSE**

### Maintainer Recommendation
**Close PR #629 without merging.**

**Justification:** All 41 original evidence artifacts, source extractions, raw tarball archives (`raw-evidence.tar.gz`), and manifest SHA-256 definitions from PR #629 were already fully incorporated into `main` via PR #630 (`ca7045c1`). Maintaining PR #629 is redundant and unnecessary.

---

## 6. Remaining Material Risks and Final Summary

- **Material Risks:** None. Offline verification passes 100% of hash, extraction, oracle, and manifest checks without modifying original experimental evidence.
- **Blocking Findings Count:** 0.

---

```text
INTEGRATION DISPOSITION: REDUNDANT
EVIDENCE PRESERVATION: COMPLETE
MAIN VERIFICATION: PASS
PR #629 RECOMMENDATION: CLOSE
NEW PR: PENDING_SUBMISSION
BLOCKING FINDINGS: 0
```
