# Restructure Phase 1: Preflight Inventory

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Restructure Phase 1: Preflight Inventory](#restructure-phase-1-preflight-inventory)
  - [1. Phase 1 Move Map](#1-phase-1-move-map)
  - [2. Include Rewrite Map](#2-include-rewrite-map)
    - [Pattern A: `t81/core/...` → `t81/types/...`](#pattern-a-`t81core`-→-`t81types`)
    - [Pattern B: `data_types/...` → `t81/types/...` (Legacy)](#pattern-b-`data_types`-→-`t81types`-legacy)
  - [3. Build Reference Map](#3-build-reference-map)
  - [4. Risk Notes](#4-risk-notes)

<!-- T81-TOC:END -->


**Date:** 2026-02-24
**Status:** Pre-execution Analysis

This document inventories the state of the repository regarding the Phase 1 restructure (Core Types Migration). It identifies files to move, include paths to rewrite, and risks.

## 1. Phase 1 Move Map

The following source directories targeted for Phase 1 are currently **missing or empty** in the file system, suggesting a partial migration or different prior state:

| Source Path | Target Path | Current Status | Notes |
| :--- | :--- | :--- | :--- |
| `src/data_types/` | `core/types/` | **MISSING** | Directory does not exist. |
| `src/core/` | `core/types/` | **MISSING** | Directory does not exist. (See `core/types` below). |
| `include/t81/core/` | `include/t81/types/` | **MISSING** | Directory does not exist. |

**Existing Destination Content:**
*   `core/types/` already exists and contains:
    *   `bigint.cpp`
    *   `fraction.cpp`
    *   `README.md`
    *   `test.md`
*   `include/t81/types/` already exists and contains full set of headers (e.g., `T81BigInt.hpp`, `T81Fraction.hpp`).

**Remnant Directories:**
*   `src/bigint/` exists but contains only `README.md`. This README documents files (`divmod.cpp`, `gcd.cpp`) that are **missing from the repository**.

## 2. Include Rewrite Map

The following include patterns need to be rewritten.

### Pattern A: `t81/core/...` → `t81/types/...`

Found **11 occurrences** in `benchmarks/`. These likely cause build failures if benchmarks are enabled.

| File | Occurrence | Proposed Fix |
| :--- | :--- | :--- |
| `benchmarks/BM_LimbMul.cpp` | `#include "t81/core/T81Limb.hpp"` | `#include "t81/types/T81Limb.hpp"` |
| `benchmarks/runner/limb_arith_throughput.cpp` | `#include "t81/core/T81Limb.hpp"` | `#include "t81/types/T81Limb.hpp"` |
| `benchmarks/runner/roundtrip_accuracy.cpp` | `#include "t81/core/cell.hpp"` | `#include "t81/types/cell.hpp"` |
| `benchmarks/runner/addition.cpp` | `#include "t81/core/T81Int.hpp"` | `#include "t81/types/T81Int.hpp"` |
| `benchmarks/runner/addition.cpp` | `#include "t81/core/T81Limb.hpp"` | `#include "t81/types/T81Limb.hpp"` |
| `benchmarks/runner/arith_throughput.cpp` | `#include "t81/core/cell.hpp"` | `#include "t81/types/cell.hpp"` |
| `benchmarks/runner/overflow_detection.cpp` | `#include "t81/core/cell.hpp"` | `#include "t81/types/cell.hpp"` |
| `benchmarks/runner/packing_density.cpp` | `#include "t81/core/cell.hpp"` | `#include "t81/types/cell.hpp"` |
| `benchmarks/runner/packing_density.cpp` | `#include "t81/core/packing.hpp"` | `#include "t81/types/packing.hpp"` |
| `benchmarks/runner/negation_speed.cpp` | `#include "t81/core/cell.hpp"` | `#include "t81/types/cell.hpp"` |
| `benchmarks/runner/negation_speed.cpp` | `#include "t81/core/cell_packed.hpp"` | `#include "t81/types/cell_packed.hpp"` |

### Pattern B: `data_types/...` → `t81/types/...` (Legacy)

Found **3 occurrences** in `src/t81_core.h`. These includes point to non-existent files.

| File | Occurrence | Issue |
| :--- | :--- | :--- |
| `src/t81_core.h` | `#include "data_types/t81_bigint.h"` | File does not exist. Likely should be `<t81/types/T81BigInt.hpp>` (C++) or similar. |
| `src/t81_core.h` | `#include "data_types/t81_fraction.h"` | File does not exist. |
| `src/t81_core.h` | `#include "data_types/t81_tensor.h"` | File does not exist. |

## 3. Build Reference Map

**CMakeLists.txt**

The root `CMakeLists.txt` has mostly been updated to reflect the new structure.

*   **Matches New Structure:**
    *   `add_library(t81_core ... core/types/bigint.cpp ...)`
    *   `add_library(t81_core ... core/types/fraction.cpp ...)`
*   **Legacy/Missing References:**
    *   None found for `src/core` or `src/data_types`.

## 4. Risk Notes

1.  **Missing Source Files (`src/bigint`)**:
    *   `src/bigint/README.md` claims ownership of `divmod.cpp` and `gcd.cpp`.
    *   These files are **not present** in `src/bigint`, `core/types`, or anywhere else in the tree (verified via `find`).
    *   **Risk:** Functionality loss or dead documentation.

2.  **Broken Public Header (`src/t81_core.h`)**:
    *   `src/t81_core.h` includes `data_types/t81_bigint.h` etc., which do not exist.
    *   **Risk:** Any consumer of this C-style header will fail to build.

3.  **Broken Benchmarks**:
    *   Benchmarks reference `t81/core/` headers which are missing.
    *   **Risk:** `T81_BUILD_BENCHMARKS=ON` will likely fail compilation.

4.  **Incomplete Cleanup**:
    *   `src/bigint` directory remains with only a README.
    *   It should likely be removed or populated with the missing files if they can be recovered.
