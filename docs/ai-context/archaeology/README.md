# T81 Repository Archaeology Workspace

**Mission ID:** T81-RA-001
**Baseline SHA:** `f775680aba7db200691c0f4e2a3174b0e7e51581`
**Tree SHA:** `6d9cabb6bcd84689b05b955b091d3d25772cc8a3`
**Branch:** `jules-17280007468303369368-c487c695`
**Date:** 2026-10-30

## Overview
This workspace is an additive, non-invasive archaeology environment designed to construct a durable, verifiable understanding of all 4,080 tracked files in the T81 Foundation repository without exceeding context boundaries or mutating normative contracts.

## Architecture
- `STATE.json`: Machine-readable execution state and batch progress.
- `HANDOFF.md`: Compact continuation brief for fresh AI sessions.
- `BUDGET.jsonl`: Append-only telemetry of token, batch, and tool operations.
- `OPEN_QUESTIONS.jsonl`: Unresolved classifications, conflicting claims, and missing proofs.
- Detailed inventories reside in `docs/records/inventories/repository-archaeology/`.

## Loading Instructions
Future agents should read `HANDOFF.md` first, check `STATE.json` for current progress, and consult specific inventory lines on demand.
