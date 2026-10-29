# Linux Compiler & Non-Performance Validation Summary (EXP-T81-COMP3-01/r1)

**Date of Execution:** 2026-10-29
**Validated Commit:** `872ac1b4c8a303fc5417d0b21551a430d0a50200`
**Environment:**
- **OS:** Linux devbox `6.8.0 #1 SMP PREEMPT_DYNAMIC` (x86_64 GNU/Linux)
- **GNU GCC / G++:** `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`
- **Ubuntu Clang:** `Ubuntu clang version 18.1.3 (1ubuntu1)`
- **Python:** `Python 3.12.13`

---

## Validation Executed & Audit Receipts

### 1. GNU GCC / G++ 13.3.0 — PASS (100% Correctness)
- **Validation Command:**
  ```bash
  python3 experiments/cmp3_decision_advantage/validate.py --compiler g++ --out /tmp/cmp3_val_gxx --objdump objdump
  ```
- **Source Files Compiled Independently:**
  - `benchmark.cpp`
  - `candidate.cpp`
  - `baseline1.cpp`
  - `baseline2.cpp`
  - `baseline3.cpp`
  - `baseline4.cpp`
  - `payload.cpp`
  - `analyze.cpp`
- **Validation Steps & Terminal Outcomes:**
  - **Optimized Compilation (`-std=c++20 -O3 -fomit-frame-pointer -fno-lto -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`):** Exit Code 0 (PASS)
  - **Sanitizer Build (`-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all`):** Exit Code 0 (PASS)
  - **Self-Test (`benchmark --selftest`):** Exit Code 0 (PASS — `PASS PCG=4 BOUNDARY=300 POLICY=120 GENERATED=150000 DETERMINISM=30000`)
  - **Fault Injection Correctness Test (`benchmark --selftest --test-fault-correctness`):** Exit Code 6 (Expected BLOCKED — `CORRECTNESS_FAILURE`)
  - **Fault Injection Payload Test (`benchmark --selftest --test-fault-payload`):** Exit Code 6 (Expected BLOCKED — `PAYLOAD_PARITY`)
  - **Independent PCG64 Oracle Verification (`verify_pcg64.py`):** Exit Code 0 (PASS — `PCG64_INDEPENDENT_VERIFICATION_SUCCESS`)
  - **Independent Corpus Verification (`test_corpus.py`):** Exit Code 0 (PASS — `INDEPENDENT_CORPUS_PASS pairs=30000 golden_pairs=24`)
  - **Assembly Lowering Inspection (`inspect_assembly.py`):** Exit Code 0 (PASS — `{"PASS": 48, "FAIL": 0, "SKIP": 1}`)
  - **Protocol Identity Preservation (`test_protocol_identity.py`):** Exit Code 0 (PASS — 5/5 tests passed)
  - **R1 Evidence and Statistical Suite (`test_r1.py`):** Exit Code 0 (PASS — 41/41 tests passed)

### 2. Ubuntu Clang 18.1.3 — BLOCKED (Compiler Frontend Segfault)
- **Validation Command:**
  ```bash
  python3 experiments/cmp3_decision_advantage/validate.py --compiler clang++ --out /tmp/cmp3_val_clang --objdump objdump
  ```
- **Terminal Exit Code:** 139 (Segmentation fault)
- **Reproducer Diagnostic:**
  `clang++` on Ubuntu 24.04 crashes inside `libclang-cpp.so.18.1` (`clang::Lexer::SkipWhitespace`) when parsing libstdc++13 standard library header files (`<iostream>`, `<vector>`, `<fstream>`, specifically `/usr/include/c++/13/ext/alloc_traits.h`).
- **Disposition:** Reproducer captured and preserved. GNU GCC 13.3.0 is the sole active verified Linux compiler lane.

---

## Source Hashes & SHA-256 Inventory

| Artifact File Path | SHA-256 Digest |
|:---|:---|
| `experiments/cmp3_decision_advantage/evidence-r1-publication/provisional/PROVISIONAL.patch` | `1123bd8f4d52ce212b69ce796cc6c806d87cfbf11b3bfd4342c8323a2d2889f3` |
| `experiments/cmp3_decision_advantage/T1_T8_READINESS_MATRIX.md` | `361d6f7cb0ef0d1802ce4d477c63e85d7a874723bdd33f991524fcdf45947e9b` |
| `experiments/cmp3_decision_advantage/benchmark.cpp` | `efdf2e96b3bb997b0ecda352333f158e5af9aea37f724d28b8aa2b8f9d2568f0` |
| `experiments/cmp3_decision_advantage/runner_r1.py` | `c6fe6f6bf6415d11df2f681a1b2152a6a1ad953454d1b25ac94ae65d7b8b0ba2` |
| `experiments/cmp3_decision_advantage/test_r1.py` | `9b8398199f2505c0340a757d96c175eef7ec0112f38b16da558267d99afbf280` |
| `experiments/cmp3_decision_advantage/test_protocol_identity.py` | `14ba7874861e36425f5ec4a9ab08d9c32a68cfd8a764a7eab53ee515be36ac3c` |

---

## Reproducibility Commands

To rerun non-performance validation on Linux:
```bash
python3 experiments/cmp3_decision_advantage/validate.py --compiler g++ --out /tmp/cmp3_val_gxx --objdump objdump
python3 experiments/cmp3_decision_advantage/test_protocol_identity.py
python3 experiments/cmp3_decision_advantage/test_r1.py
```
