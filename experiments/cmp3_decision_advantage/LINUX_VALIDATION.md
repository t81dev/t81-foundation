# Linux Compiler & Non-Performance Validation Summary (EXP-T81-COMP3-01/r1)

**Environment:**
- **OS:** Linux devbox `6.8.0 #1 SMP PREEMPT_DYNAMIC` (x86_64 x86_64 GNU/Linux)
- **GNU GCC:** `gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`
- **GNU G++:** `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`
- **Ubuntu Clang:** `Ubuntu clang version 18.1.3 (1ubuntu1)`

---

## Compiler Verification Results

### 1. GNU GCC / G++ 13.3.0
- **Status:** PASS
- **Command:**
  ```bash
  python3 experiments/cmp3_decision_advantage/validate.py --compiler g++ --out /tmp/cmp3_val_gxx --objdump objdump
  ```
- **Translation Units Compiled Independently:**
  - `benchmark.cpp`
  - `candidate.cpp`
  - `baseline1.cpp`
  - `baseline2.cpp`
  - `baseline3.cpp`
  - `baseline4.cpp`
  - `payload.cpp`
  - `analyze.cpp`
- **Validation Checks Run & Exit Codes:**
  - Optimized (`-O3 -fomit-frame-pointer -fno-lto -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`): **Exit Code 0 (PASS)**
  - ASan / UBSan (`-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all`): **Exit Code 0 (PASS)**
  - Self-test (`benchmark --selftest`): **Exit Code 0 (PASS)**
  - Fault injection correctness (`benchmark --selftest --test-fault-correctness`): **Exit Code 6 (Expected BLOCKED — CORRECTNESS_FAILURE)**
  - Fault injection payload (`benchmark --selftest --test-fault-payload`): **Exit Code 6 (Expected BLOCKED — PAYLOAD_PARITY)**
  - Independent PCG64 oracle verification (`verify_pcg64.py`): **Exit Code 0 (PASS)**
  - Independent corpus validation (`test_corpus.py`): **Exit Code 0 (PASS - 30,000 pairs matched)**
  - Assembly lowering inspection (`inspect_assembly.py`): **Exit Code 0 (PASS - Candidate vs Baseline2 parity verified)**

### 2. Ubuntu Clang 18.1.3
- **Status:** FAIL / BLOCKED (Compiler Environment Issue)
- **Observation:** `clang++` on this Ubuntu 24.04 sandbox crashes with `Segmentation fault (exit code 139)` in `libclang-cpp.so.18.1` (`clang::Lexer::SkipWhitespace`) when parsing libstdc++13 C++ standard library header files (e.g. `<iostream>`, `<vector>`, `<fstream>`).
- **Resolution / Accounting:** Explicitly recorded as **BLOCKED / FRONTEND CRASH ON LIBSTDC++13**. GNU GCC 13 serves as the primary verified C++20 Linux compiler.

---

## Assembly Lowering & Dispatch Parity
Inspection of generated x86-64 assembly via `inspect_assembly.py` confirmed:
- Candidate CMP3 dispatch (`cmp3_s64`) lowers to `cmpq` + `setge`/`setg` / `addl` indexing into `targets(%rip)`.
- Candidate and Baseline2 share identical dispatch table structure for pattern A/C evaluation.
- All five dispatch variants maintain identical payload calculation parity.

---

## Reproducibility Instructions
To rerun the non-performance validation suite on Linux:
```bash
python3 experiments/cmp3_decision_advantage/validate.py --compiler g++ --out /tmp/cmp3_val_gxx --objdump objdump
python3 experiments/cmp3_decision_advantage/test_protocol_identity.py
python3 experiments/cmp3_decision_advantage/test_r1.py
```
