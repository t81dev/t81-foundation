# `spec/rfcs`

Request-for-Comments (RFC) pipeline for evolving T81 specifications.

## Purpose
- Capture proposals before they become normative spec text.
- Track rationale, tradeoffs, migration plans, and compatibility impacts.

## Required Metadata
Each RFC should include, near the top of file:
- `RFC-ID` (filename `RFC-NNNN-...`)
- `Title`
- `Status`: `draft` | `proposed` | `accepted` | `integrated` | `superseded` | `rejected`
- `Type`: `standards-track` | `informational` | `process`
- `Applies-To`
- `Created` / `Updated`
- Optional: `Supersedes`, `Superseded-By`, `Discussion`

Use `spec/rfcs/template.md` for new RFCs.

## Lifecycle
1. `draft`: authoring and initial review.
2. `proposed`: ready for wider review.
3. `accepted`: design approved, implementation pending/in progress.
4. `integrated`: normative spec text updated and linked.
5. `superseded`/`rejected`: closed with explicit rationale.

## Editing Rules
- Keep RFC numbers monotonic; do not renumber merged RFCs.
- If an RFC replaces another, update both files plus `spec/rfcs/index.md`.
- When integrating, add links to the normative sections in `spec/*.md`.

## When To Write An RFC
- The change freezes a new ABI, syscall, `KernelCall`, wire format, or opcode.
- The change introduces or changes governance, audit, policy, or capability semantics.
- The change defines a lifecycle or state-machine contract that future work will depend on.
- The change creates a stable public contract that should remain intelligible after implementation details change.

## When Not To Write An RFC
- Shell built-ins over already-existing state.
- Smoke checks, CI additions, diagnostics, or runbook updates.
- Small allowlist expansions or proof-harness extensions.
- Narrow implementation steps inside an already-accepted series where the contract is already frozen.

## Series-First Planning
- Prefer planning by series when the architecture is already established.
- Current examples:
  - `RFC-00BC..00CE`: Axion freestanding EL0 bring-up, scheduler, CanonFS executable lane, fault lifecycle, supervisor recovery
  - `RFC-DPE-0001..0009`: epoch execution and audit history
  - `RFC-0034` + `RFC-00BA` + `RFC-00BB`: model ingestion and compatibility
- For those areas, use implementation tasks, roadmap notes, and progress docs for incremental work. Write a new RFC only when the series crosses a new contract boundary.

## Catalog Discipline
- `spec/rfcs/index.md` must remain a complete catalog of primary RFC records.
- Auxiliary templates or per-target profile drafts that reuse a parent RFC id may live beside the catalog, but they should be called out as auxiliary rather than silently replacing the primary record.
