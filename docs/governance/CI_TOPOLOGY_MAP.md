# CI Topology Map

## 1. Workflow Classification

| Domain | Workflow | Trigger | Criticality |
| :--- | :--- | :--- | :--- |
| **Build & Test** | `ci.yml` | Push, PR | **Hard-Fail** |
| | `sanitizers.yml` (merged) | Push, PR | **Hard-Fail** |
| | `fuzzing.yml` (merged) | Push, PR | **Hard-Fail** |
| | `static-analysis.yml` (merged) | Push, PR | **Hard-Fail** |
| **Determinism** | `ci.yml` (determinism-slice) | Push, PR | **Hard-Fail** |
| | `repro-ledger.yml` | Schedule, Dispatch | **Hard-Fail** |
| | `tritwise_determinism_gate.yml` | Push, PR | **Hard-Fail** |
| **ISA Freeze** | `ci.yml` (spec-and-docs) | Push, PR | **Hard-Fail** |
| **Benchmark** | `bench.yml` | Dispatch | Soft-Fail |
| | `benchmark_packed_trit_vector.yml` | Push, PR | **Hard-Fail** |
| **Governance** | `runtime-contract.yml` | Schedule, Push, PR | **Hard-Fail** |
| | `ci.yml` (spec-and-docs) | Push, PR | **Hard-Fail** |
| **Documentation** | `pdf.yaml` | Push (paths) | Warning |
| | `sidebar.yml` | Push (paths) | Warning |
| | `toc.yml` | Push (paths) | Warning |
| | `static.yml` (Search Index) | Push (paths) | Warning |
| **Release** | `release.yml` | Tag | **Hard-Fail** |
| **Security** | `codeql.yml` | Schedule, PR | Warning |

## 2. Redundancy & Fragmentation Analysis

### Benchmarking Fragmentation
- **Issue**: Three separate workflows run benchmarks (`bench.yml`, `benchmark_packed_trit_vector.yml`, `ci.yml`).
- **Risk**: Inconsistent environments and duplicated compute usage. `bench.yml` is manual, while `benchmark_packed_trit_vector.yml` is a regression gate.
- **Resolution**: Consolidate into `benchmarks.yml` with distinct jobs for regression (gate) and oracle (reporting).

### Determinism Scatters
- **Issue**: Tritwise determinism checks live in `tritwise_determinism_gate.yml`, while general determinism slices are in `ci.yml` and deep ledgers in `repro-ledger.yml`.
- **Risk**: A developer might pass `ci.yml` but fail `tritwise` checks if not careful.
- **Resolution**: Merge `tritwise_determinism_gate.yml` into `ci.yml` to ensure all commit-gating determinism checks run together. Keep `repro-ledger.yml` for long-running scheduled verification.

### Documentation Sprawl
- **Issue**: Four separate workflows for documentation chores (`pdf`, `sidebar`, `toc`, `search`).
- **Risk**: Clutter in the Actions tab; maintenance burden.
- **Resolution**: Merge into a single `documentation.yml` with conditional jobs based on changed paths.

### Governance Overlap
- **Issue**: Governance checks are split between `ci.yml` (audit scripts) and `runtime-contract.yml`.
- **Resolution**: Keep `runtime-contract.yml` separate due to its specific external dependency check (sync with `t81-vm`), but acknowledge it as a distinct governance pillar.

## 3. Consolidation Plan

### Target State (≤ 8 Workflows)

1. **`ci.yml`** (The Monolith Gate)
   - **Merges**: `tritwise_determinism_gate.yml`, `format.yml`
   - **Scope**: Build, Test, Sanitizers, Fuzzing, Determinism Slices, Tritwise Equivalence, ISA Freeze, Linting.
   - **Trigger**: Push/PR to `main`.

2. **`benchmarks.yml`**
   - **Merges**: `bench.yml`, `benchmark_packed_trit_vector.yml`
   - **Scope**: Performance regression gating (hard fail) and Speed Oracle badge updates (on dispatch/schedule).
   - **Trigger**: Push/PR (regression), Dispatch/Schedule (oracle).

3. **`documentation.yml`**
   - **Merges**: `pdf.yaml`, `sidebar.yml`, `toc.yml`, `static.yml`
   - **Scope**: PDF generation, Sidebar update, TOC injection, Search Index build.
   - **Trigger**: Push/PR to `docs/**`, `spec/**`.

4. **`reproducibility.yml`** (Renamed from `repro-ledger.yml`)
   - **Merges**: `t81lang-repro-hash-refresh.yml` (as a dispatch job)
   - **Scope**: Deep reproducibility verification (T3K, T81Lang, Axion Traces) and hash refresh utilities.
   - **Trigger**: Schedule, Dispatch.

5. **`security.yml`**
   - **Existing**: `codeql.yml`
   - **Scope**: CodeQL analysis.
   - **Trigger**: Schedule, PR.

6. **`release.yml`**
   - **Existing**: `release.yml`
   - **Scope**: Release artifacts, SBOM, Signing.
   - **Trigger**: Tag `v*`.

7. **`governance.yml`**
   - **Existing**: `runtime-contract.yml`
   - **Scope**: Cross-repo contract synchronization.
   - **Trigger**: Schedule, Push/PR.

### Migration Steps

1. **Consolidate Documentation**: Create `documentation.yml`, move jobs from `pdf.yaml`, `sidebar.yml`, `toc.yml`, `static.yml`. Delete old files.
2. **Consolidate Benchmarks**: Create `benchmarks.yml`, move logic from `bench.yml` and `benchmark_packed_trit_vector.yml`. Delete old files.
3. **Enhance CI**: Move `tritwise_determinism_gate.yml` jobs into `ci.yml`. Move `format.yml` job into `ci.yml`. Delete old files.
4. **Rename & Refine Repro**: Rename `repro-ledger.yml` to `reproducibility.yml`. Add hash refresh job. Delete `t81lang-repro-hash-refresh.yml`.
5. **Rename Contract**: Rename `runtime-contract.yml` to `governance.yml`.

## 4. Trigger Mapping & Severity

| Workflow | Trigger | Severity |
| :--- | :--- | :--- |
| `ci.yml` | Push, PR | **BLOCKING** |
| `benchmarks.yml` | Push, PR | **BLOCKING** (Regression) |
| | Dispatch | Informational |
| `reproducibility.yml` | Schedule | **BLOCKING** (Alerts) |
| `documentation.yml` | Push, PR | Warning |
| `governance.yml` | Schedule | **BLOCKING** (Alerts) |
| `security.yml` | Schedule | Warning |
| `release.yml` | Tag | **BLOCKING** |
