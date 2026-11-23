# T81 Foundation — Operational Handoff (2026-11-13)

**Classification:** Non-normative operational update. No scientific gate advancement, RFC ratification, release publication, or merge authorization.
**Observed main:** `88300f14c1da1066ad6db25d31be0e8e912b3ef0`.
**Evidence:** [dated GitHub receipt](receipts/2026-11-13-pr-integration-observations.json). GitHub observations were refreshed during this update; no local build or experiment was executed by this collector. Review outcomes below are supplied Codex review reports in the maintainer session, not fresh independent execution by this document's author.

## Active changes

| PR | Observed head | Review and integration status |
| --- | --- | --- |
| [#779](https://github.com/t81dev/t81-foundation/pull/779) | `4d5f3586c04a69c520e613166291c0974877628d` | Supplied targeted Codex review ACCEPT: malformed trytes, repack accounting, exact product bounds; ASan and NDEBUG paths reported tested. Open draft, unmerged. Compare competing #778 before integration; acceptance does not settle their unique-content overlap. |
| [#780](https://github.com/t81dev/t81-foundation/pull/780) | `a31b862e6bbdc7c136a74c9117aa085a13b4da2c` | Supplied Codex review ACCEPT carried forward through token-equivalent formatting. Open draft, unmerged. Bounded host-local baseline only. |
| [#782](https://github.com/t81dev/t81-foundation/pull/782) | `125a1897b7e7e068ef6d576d97c487295c7203a7` | Open draft, unmerged. Both Windows builds, all seven native platform build lanes, sanitizers, cross-platform determinism comparison, format, and quality gate / required report SUCCESS at snapshot. Analyze (cpp), TUI size gate and informational ARMv9 cross-compile remain running. Final review closure remains pending in supplied handoff. |
| [#778](https://github.com/t81dev/t81-foundation/pull/778) | `56ad0d57b90487d153b35ccfdeb134fcc858adf7` | Open draft competing contract correction. Five files overlap #779 according to Codex audit; reconcile unique corrections and evidence before closure or integration. |

### BASE-PERF-00 boundaries

Reviewed medians are CPU-time throughput: add 163.39, subtract 139.04, multiply 22.34, divide 61.60, conversion round trip 24.84 Mops/s. These are Apple M2 workstation results with five repetitions and 10,000 operations per timed batch, not single-operation latency, VM IPS, or AI tokens/s. Arithmetic workload is [-10,+10] (division excludes zero); conversions span [-121,+121].

The corrected explanation distinguishes three-trit operands from products requiring up to five trits. Full-domain multiplication still throws for 352 representable-product pairs; the exact-count assertion is an explicitly accepted test-scope exception, not endorsement of defective arithmetic. Division uses to_int + host integer division + from_int. Original raw measurements are preserved; original build/environment assertions remain author-reported and source-to-binary binding unverified. No reference-hardware baseline or binary comparative advantage is established.

### Windows repair and verification boundaries

#782 includes 128-bit backend capability correction, Unicode argv handling, FFI test-library exports/dependencies, secure process capture, and compiler fallback source-path normalization. Earlier capture overwrite and test-coverage findings required correction. Correction reports cite exclusive private creation, retained opened objects, and collision/reparse/Unicode/quoting/large-output tests; final reviewer disposition at the current head must still be retained.

The repro-hash drift was attributed through an exact hash-matching path-separator reconstruction; fallback source names use generic separators and the diagnostic-path test follows that contract. The golden hash is unchanged according to the supplied correction report. Current Windows CI success is observed metadata, not a fresh local reproduction or an independent review of every changed path.

Codex authored the earlier 128-bit correction; its review of later Gemini changes is not independent review of its own arithmetic contribution. Gemini's separate review of that contribution remains to be confirmed.

The AI Experiments workflow is FAILED at this head. An earlier inspected run at d9701d82 stopped on three stale documents (AI_RFC_BACKLOG.md, CI_WORKFLOW_CONFIRMATION.md, CI_GATE_STATUS.md; age 8 vs limit 7). The latest failing run has not been log-classified by this update; do not assume baseline equivalence. Refresh document substance and evidence rather than merely changing dates.

## Backlog audit and completed closures

Codex preserved the 80-PR inventory on `audit/open-pr-inventory-2026-11-13` at `1cc1b3cd270cd7365fa77dfb77db8db3fd005914`:
[durable audit package](https://github.com/t81dev/t81-foundation/tree/1cc1b3cd270cd7365fa77dfb77db8db3fd005914/docs/records/audits/2026-11-13-open-pr-inventory).
All classifications were provisional; the audit and revalidation were authored by Codex, not independent review.

After explicit maintainer authorization, #645, #747, #760, #766 and #769 were closed WITHOUT MERGE on 2026-11-13, with evidence-linked explanations. No branch-deletion operation was performed. Current receipt confirms closed/unmerged states.

| Closed PR | Supersession evidence |
| --- | --- |
| #645 | Specification represented by d8e1376a7a77902cc26269c6e8d4991bcde117a7; residual trailing Markdown whitespace. |
| #747 | Replaced by #770 a4684582c9532061dc516836f10a8a0ed602d69f and #772 89223724038ff7dec349343469df4ea8389ede26; audit merge tree equals main. |
| #760 | Harness represented by #763 dbf107fda83a3755fa21d6b95caa3eb8ac450dd3, #765 36b3849aa1e6297e88de2ec1655777793ab0b59e and #775 d3a6cf55517a3098d7bae91b07c72694bc3472da. Residual deletions of 50 newer unrelated paths must not be merged. |
| #766 | Corrections retained by #775 with newer provenance rules. Historical evidence remains; #767 is separate. |
| #769 | Replaced by #771 b96022c6b8e313a0ccd6bed6f1057d5e2368d092; audit merge tree equals main. |

Open PR count after those closures: **75**, before opening this documentation update. This is a dated inventory, not a permanent count.

Dependent work survives: #748/#750/#752 target the retained #747 branch; #751 stacks on #750. Codex reports clean main simulations for #748/#752 and CMakeLists.txt conflicts for #750/#751. #767 at bd77c00122ff9cc63097d0ee2cd88895266878cc contains unique validator checks (175 additions/11 removals in its own delta); port carefully without replacing #775 provenance rules.

## Next actions and work ownership

1. Finish #782 review at the exact head, classify remaining CI, and retain component-level authorship disclosures. No automatic merge.
2. Codex: reconcile #778 versus #779 while preserving accepted scope and unique evidence. #780 remains a separate baseline.
3. Prepare integration decisions, then review #748/#750/#751 and port surviving #767 checks. Existing audit work should be handed to Gemini explicitly rather than assumed known.
4. Release preparation is a proposal: latest published version observed previously was v1.9.4 (2026-04-02). No new version, release scope, tag or publication authorized. Determine version and artifact tests against the exact eventual integrated commit.
5. Kernel/MMU interpretation belongs to a separate AI workstream at the user's direction. Do not redirect this CI/backlog lane into kernel implementation.

## Preserved governance and history

RFC-00E1 remains DRAFT; formal SCG-08 Gate 1 acceptance WITHHELD and Gate 2 STRICT NO-GO. #779 corrective review acceptance is not Gate 1 acceptance. Preserve H01–H08, SCG-07 QUALIFIED WITH LIMITATIONS, historical Hanoi/CWEB negatives (including the bounded 10.6x scalar addition penalty), CPA/ISA/BASE qualifications, and earlier dated snapshots.

This overlay supersedes older operational next actions only. CURRENT_STATE.json's selected historical view and scientific observations remain unchanged; its navigation points here. Neither this document nor its receipt grants authority.
