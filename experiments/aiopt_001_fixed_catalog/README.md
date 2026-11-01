# T81-AIOPT-001 / M1

Experimental, correctness-only implementation of the [reconstructed v0.1.0 protocol](../../docs/research/aiopt/T81-AIOPT-001-M1-PROTOCOL.md).

Run from the repository root:

```sh
python3 experiments/aiopt_001_fixed_catalog/m1.py
python3 experiments/aiopt_001_fixed_catalog/m1.py
```

The script has no third-party dependencies. Compare the two complete JSON outputs (including `sha256_results`) for repeatability. It enumerates 81 inputs and 40 patterns, validates 3,240 semantic predicate decisions, performs 162 adapter/oracle filter comparisons, 162 encoding round trips, and fail-closed malformed-input assertions.

The semantic oracle is separately implemented from the byte and SWAR paths within the same file. This is **implementation independence at the algorithm level**, not yet an independent reviewer or independent codebase. Do not infer M1 acceptance solely from this script.

The results JSON is printed on stdout and contains the local Python version and host platform. For review, preserve the exact source commit, two output files, the runner environment, and independent reproduction evidence.

**Status:** implementation submitted for review; independent M1 acceptance outstanding. No performance benchmarks or AI selectors.
