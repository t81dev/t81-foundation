---
layout: page
title: Reproducibility Ledger Dashboard
---

# Reproducibility Ledger Dashboard

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Reproducibility Ledger Dashboard](#reproducibility-ledger-dashboard)
  - [1. What It Captures](#1-what-it-captures)
  - [2. Run Locally](#2-run-locally)
  - [3. CI Workflow](#3-ci-workflow)
  - [4. Determinism Notes](#4-determinism-notes)

<!-- T81-TOC:END -->


This guide documents the automated dashboard that turns CI outputs into a deterministic ledger artifact.

The workflow is defined in `.github/workflows/repro-ledger.yml` and emits:

- `build/repro/dashboard.md`
- `build/repro/dashboard.json`

Both files are uploaded as the `reproducibility-ledger` artifact.

______________________________________________________________________

## 1. What It Captures

The dashboard records:

1. Build/test status (`ctest` JUnit summary)
2. T3_K reproducibility gate hash (`scripts/ci/t3k_repro_gate.py`)
3. Benchmark snapshot (`build/bench.json`)
4. Axion trace log digests (`build/artifacts/*.log`)
5. SHA3-512 digest table for all included artifacts

This keeps model/tensor/trace evidence in one reproducible bundle.

______________________________________________________________________

## 2. Run Locally

From repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure --output-junit build/ctest-results.xml

python3 scripts/ci/t3k_repro_gate.py \
  --t81-bin build/t81 \
  --workdir build/t3k-repro \
  --hash-out build/t3k-repro/hash.txt

./scripts/capture-axion-trace.sh
cmake --build build --parallel --target benchmark_runner
./build/benchmarks/benchmark_runner --benchmark_format=json --benchmark_out=build/bench.json

python3 scripts/ci/generate_repro_dashboard.py \
  --out-md build/repro/dashboard.md \
  --out-json build/repro/dashboard.json \
  --ctest-junit build/ctest-results.xml \
  --t3k-hash build/t3k-repro/hash.txt \
  --bench-json build/bench.json \
  --axion-log build/artifacts/axion_policy_runner.log \
  --axion-log build/artifacts/axion_heap_compaction_trace_test.log \
  --axion-log build/artifacts/vm_bounds_trace_test.log \
  --axion-log build/artifacts/canonfs_axion_trace_test.log
```

______________________________________________________________________

## 3. CI Workflow

`repro-ledger.yml` runs weekly and on manual dispatch.

It performs:

1. Release configure/build/test
2. T3_K reproducibility gate
3. Axion trace capture
4. Benchmark generation
5. Dashboard generation + artifact upload

The Markdown report is also copied into the GitHub Actions job summary for quick inspection.

______________________________________________________________________

## 4. Determinism Notes

- The dashboard itself is derived from deterministic artifacts already produced by the pipeline.
- SHA3-512 entries let auditors verify artifact identity without re-running the full job.
- If `ctest` fails or the T3_K gate hash is malformed, dashboard status is marked `FAIL`.
