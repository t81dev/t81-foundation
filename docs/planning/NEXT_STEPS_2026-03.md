# T81 Foundation: Next Steps (Sprint 2026-03)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Foundation: Next Steps (Sprint 2026-03)](#t81-foundation-next-steps-sprint-2026-03)
  - [Executive Summary](#executive-summary)
  - [Prioritized Milestones](#prioritized-milestones)
  - [Risks & Mitigations](#risks-&-mitigations)
  - [Task Breakdown](#task-breakdown)
    - [Technical Actions (60–70% capacity)](#technical-actions-60–70%-capacity)
    - [Project Management & Docs Actions (30–40% capacity)](#project-management-&-docs-actions-30–40%-capacity)
  - [Appendix](#appendix)
    - [New RFC Stubs Needed](#new-rfc-stubs-needed)

<!-- T81-TOC:END -->


## Executive Summary

As of v1.9.5, the T81 core is frozen and stable (TISC, bit-exact determinism, Axion policy engine). Our top strategic priority for this sprint is to move the bare-metal Axion/ternaryos kernel from alpha to beta, unlocking our "first deterministic ternary-native OS" narrative without host dependencies. Concurrently, we will deliberately tackle the "first-killer-app" barrier by delivering a concrete, 60-second governed ternary inference demo (T3K quantization + policy enforcement) to drastically improve external discoverability and SEO, turning our internal CI success into external momentum.

## Prioritized Milestones

| Milestone | Owner | Est. Effort | Success Criteria | Dependencies / RFC |
|-----------|-------|-------------|------------------|--------------------|
| **M1: bare-metal kernel → beta**<br>(Axion/ternaryos EL0 + scheduler stabilization) | Core Tech (@t81dev & team) | 3–4 engineer-days | - QEMU bare-metal boot succeeds with zero host deps<br>- All EL0 fault-containment tests pass 100%<br>- Scheduler determinism traces match CanonHash81 exactly<br>- Tagged `v1.10.0-beta` | RFC-017 (scheduler graduation) |
| **M2: Governed Ternary Inference Demo (first-killer-app)**<br>(T3K quantization + policy enforcement notebook) | PM + Inference Lead | 2–3 engineer-days | - One-command `cargo run --example governed_llm` works on bare-metal<br>- Zero FP drift vs. reference TQUANT trace<br>- Policy interception demo shows "blocked side-effect" in <60s<br>- Notebook published to GitHub Pages + website | RFC-019 (governed-inference cookbook) |
| **M3: Discoverability / SEO baseline lift** | PM | 1–2 engineer-days | - GitHub topics + badges updated<br>- Short 90-second demo video rendered & linked in README<br>- /docs mirrored to ternaryos.dev with canonical URLs<br>- Organic traffic + stars target: +8 stars, +20% repo visits | SEO plan stub (new) |
| **M4: Documentation & governance hygiene** | PM | 1 engineer-day | - Architecture diagrams updated in /book/<br>- NEXT_STEPS_2026-03.md merged<br>- ACTIVE_RISKS.md refreshed & all sprint risks closed or accepted<br>- Release notes drafted for v1.10.0-beta | Existing governance scripts |

**Total sprint effort**: ~7–10 engineer-days (fits a focused 2-week window)
**Overall success metric**: bare-metal reaches beta + killer-app demo live + discoverability signals visibly improving

## Risks & Mitigations

We carry forward existing architectural risks while adding explicit tracking for our market-facing challenges.

| ID | Risk | Severity | Owner | Mitigation | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **R-05** | **Governed non-DCP surface growth outpacing boundary and evidence updates** | Medium | @t81dev | Refresh status/registry/boundary docs each monthly governance cadence. Ensure new proof lanes (e.g., bare-metal determinism) are reflected in operator docs. | Monitoring |
| **R-19** | **Bare-metal determinism regressions during EL0 stabilization** | High | Core Tech | Strict adherence to CanonHash81 trace verification on every kernel PR. No merging if QEMU bare-metal traces diverge from the guest VM reference traces. | **New** |
| **R-20** | **Discoverability stall despite technical readiness** | High | PM | Explicitly dedicate 30-40% of sprint capacity to M2 and M3 (Demo + SEO). Do not allow M1 depth to fully cannibalize external-facing momentum. | **New** |
| **R-06** | **Documentation maintenance burden after reorganization** | Low | PM | Rely on content-based structure; automated link checking in CI remains active. | Monitoring |
| **R-07** | **Benchmark variability — false signal in `vm workload gate` guardrail** | Low | @t81dev | Alert threshold >5% regression; review guardrail if consecutive divergence detected. | Monitoring |

*(Note: Risks R-01 through R-04 and R-08 through R-18 have been successfully closed in previous sprints. See `/docs/status/ACTIVE_RISKS.md` for historical details.)*

## Task Breakdown

### Technical Actions (60–70% capacity)
1. **[M1] Kernel EL0 Stabilization**: Implement and verify EL0 fault containment mechanisms in the Axion kernel.
2. **[M1] Scheduler Graduation**: Complete RFC-017 implementation, ensuring the new scheduler is 100% deterministic against CanonHash81.
3. **[M1] QEMU Bare-Metal Polish**: Refine the boot sequence to ensure zero host dependencies and clean initialization to the `t81>` prompt.
4. **[M2] T3K / TQUANT Polishing**: Finalize the ternary-native weights optimization layer for stable, efficient inference execution.
5. **[M2] Demo Implementation**: Wire the governed LLM inference example (`cargo run --example governed_llm`) demonstrating pre-side-effect policy interception.

### Project Management & Docs Actions (30–40% capacity)
1. **[M3] SEO / Repo Optimization**: Update GitHub topics, repository tags, and README badges to highlight "ternary AI", "deterministic OS", and "verifiable inference".
2. **[M3] Demo Media**: Record the 90-second CLI/QEMU governed inference demo and embed/link it in the README and website.
3. **[M3] Website Mirroring**: Setup CI or manual pipeline to mirror `/docs` content to `ternaryos.dev` with proper canonical URLs.
4. **[M4] Architecture Docs**: Refresh diagrams in `/book/` to accurately reflect the bare-metal architecture and T3K inference pipeline.
5. **[M4] Release Prep**: Draft release notes for the upcoming `v1.10.0-beta` bare-metal graduation.
6. **[M4] Governance Routine**: Update `ACTIVE_RISKS.md`, the README maturity table, and any relevant RFC statuses.

## Appendix

### New RFC Stubs Needed
- **RFC-019 (Governed-Inference Cookbook)**: Needs a draft detailing the canonical way to apply Axion policies to ternary inference workloads.
- **SEO & Discoverability Plan (Stub)**: An internal tracking document for targeted keywords, backlink strategies, and community outreach.

---
*Prepared by the Acting Project Manager.*
