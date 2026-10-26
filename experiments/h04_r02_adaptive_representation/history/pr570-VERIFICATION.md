# H04-R02 Verification and Reproduction Instructions

**Document ID:** `experiments/h04_r02_adaptive_representation/VERIFICATION.md`
**Date:** October 2026 (Audit Corrected)

---

## 1. Reproduction Instructions from Merged Source

To reproduce the corrected H04-R02 audit verification suite directly from the merged source:

```bash
# 1. Build the adaptive experiment C++ executable and tests
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target t81_h04_r02_adaptive

# 2. Run the CTest experiment suite (C++ evaluator, Python oracle, Audit regression tests)
ctest --test-dir build --output-on-failure -R h04_r02

# 3. Directly run the standalone audit regression test suite
python3 tests/ci/h04_r02_audit_regression_test.py
```

---

## 2. Verification Targets

1. `t81_h04_r02_adaptive_test`: C++ evaluator generating JSON trace ledgers.
2. `t81_h04_r02_oracle_test`: Python independent DP oracle verification script.
3. `t81_h04_r02_audit_regression_test`: Audit regression suite verifying DEF-01 through DEF-07 fixes.
