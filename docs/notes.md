______________________________________________________________________

# T81 Working Notes

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Working Notes](#t81-working-notes)
  - [1. Newly Added RFCs (Q3 Update)](#1-newly-added-rfcs-q3-update)
  - [2. Spec → Docs Cross-Reference Checklist](#2-spec-→-docs-cross-reference-checklist)
  - [3. Documentation To‑Dos](#3-documentation-to‑dos)
  - [4. Open Questions for Docs Team](#4-open-questions-for-docs-team)

<!-- T81-TOC:END -->




> Scratchpad for ongoing work across specs, RFCs, and the C++ implementation.\
> Non‑normative. Updated whenever the specs/rfcs/docs gain new material.

______________________________________________________________________

## 1. Newly Added RFCs (Q3 Update)

| RFC | Purpose | Status |
| --- | ------- | ------ |
| [RFC-0004](../spec/rfcs/RFC-0004-canonical-tensor-semantics.md) | Canonical tensor shapes, pools, Axion metadata | Draft |
| [RFC-0005](../spec/rfcs/RFC-0005-tisc-v0-4-extensions.md) | ISA v0.4 opcode plan (structural constructors, vector helpers) | Draft |
| [RFC-0006](../spec/rfcs/RFC-0006-deterministic-gc.md) | Deterministic GC algorithm & safepoints | Draft |
| [RFC-0007](../spec/rfcs/RFC-0007-t81lang-standard-library.md) | Standard module layout + versioning | Draft |
| [RFC-0008](../spec/rfcs/RFC-0008-formal-verification-harness.md) | Trace format + proof harness | Draft |
| [RFC-0009](../spec/rfcs/RFC-0009-axion-policy-language.md) | Declarative Axion policy language (APL) | Draft |

Action item: when any of these move to “Accepted”, mirror the key semantics into
`spec/*` and link them from `docs/developer-guide.md`.

______________________________________________________________________

## 2. Spec → Docs Cross-Reference Checklist

| Spec Section | Doc/Guide | Owner |
|--------------|-----------|-------|
| `spec/t81lang-spec.md §2.3` (Option/Result) | add to `docs/developer-guide.md` “Structural Types” | 🟡 |
| `spec/tisc-spec.md §5.2` (new opcodes) | mention in `docs/cpp-quickstart.md` “VM Notes” | ✅ |
| `spec/t81vm-spec.md` (GC) | expand `docs/developer-guide.md §6` once RFC-0006 accepted | 🟡 |
| `spec/axion-kernel.md` (policies) | create tutorial referencing APL syntax | 🔜 |

Legend: ✅ done, 🟡 pending polish, 🔜 requires future RFC/state.

______________________________________________________________________

## 3. Documentation To‑Dos

1. **Tensor How-To**

   - Convert RFC-0004 summary into a tutorial under `docs/` (maybe `tensor-guide.md`).
   - Include code snippets for shape guards and vector ops once ISA v0.4 lands.

1. **Policy Examples**

   - Add sample APL policies showing tier limits, opcode filters, proof hashes.
   - Cross-link from `docs/developer-guide.md` Axion section.

1. **Verification Walkthrough**

   - When RFC-0008 tooling is ready, document how to run `t81-verify` + interpret `.t81trace`.

______________________________________________________________________

## 4. Open Questions for Docs Team

- Should we host rendered RFCs under `/docs/rfcs` or rely on the spec tree?
- How do we version the documentation site relative to spec releases?
- Do we want per-RFC changelog entries in the site sidebar?

Drop notes here as decisions land; we can promote them into permanent pages later.

______________________________________________________________________
