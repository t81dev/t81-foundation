# H04-R02 Verification and Reproduction Guide

**Document ID:** `experiments/h04_r02_adaptive_representation/VERIFICATION.md`
**Date:** October 2026
**Status:** Complete / Final Research Packet

---

## 1. Environment & Toolchain

- **OS / Platform:** Linux x86_64 / Debian Sandbox
- **Compiler:** GCC / Clang with C++17 support (`-std=c++17`)
- **Build System:** CMake 3.16+ / Ninja
- **Python:** Python 3.12+

---

## 2. Reproduction Commands

From a clean checkout at commit `aecb71c3b9af2979d31051c38d071178e143b3a3`:

```bash
# 1. Configure CMake build directory with test targets enabled
cmake -S . -B build -G Ninja -DT81_BUILD_TESTS=ON

# 2. Compile H04-R02 experiment binary
cmake --build build --target t81_h04_r02_adaptive

# 3. Execute H04-R02 CTest target suite
ctest --test-dir build -R h04_r02 --output-on-failure

# 4. Run standalone independent optimization oracle
python3 experiments/h04_r02_adaptive_representation/oracle.py

# 5. Run full research regressions suite
python3 scripts/ci/run_research_regressions.py --build-dir build
```

---

## 3. Expected Test Outcomes

- `t81_h04_r02_adaptive_test`: **PASSED**
- `t81_h04_r02_oracle_test`: **PASSED**
- `scripts/ci/run_research_regressions.py`: **8/8 PASSED (100%)**
