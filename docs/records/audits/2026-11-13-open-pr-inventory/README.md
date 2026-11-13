# Provisional open-PR inventory and prepared closure recommendations

**All 80 classifications are PROVISIONAL.** They are triage recommendations, not
acceptance, merge/closure permission, authenticated research findings or policy
activation. This separate audit branch changes no implementation or active PR.

## Frozen identities and scope

- Inventory main: `88300f14c1da1066ad6db25d31be0e8e912b3ef0`. Publication main was refreshed and is unchanged.
- Original inventory checkpoint: `2026-11-13 11:51:53 UTC`.
- All 80 exact head SHAs, actual bases, source-file/blob comparisons, dependency
  and overlap maps, review records and CI snapshots remain retained.
- PR #782 remains a frozen inventory row at its historical snapshot head
  `125a1897b7e7e068ef6d576d97c487295c7203a7`. It was not modified or refreshed
  as part of this preservation task; its active review is separate.
- Current metadata for the five closure candidates was rechecked: all remained
  open/unmerged at the inventoried heads. See closure-pr-metadata-check.json.

## Complete inventory

- [Provisional decision table](DECISIONS.md) and [CSV](decisions.csv).
- [Machine-readable inventory](inventory.json): every record explicitly has
  `classification_status: PROVISIONAL`; source SHA and classification choices
  are unchanged from the original inventory.
- [Exact files, dependencies, overlaps and review/CI evidence](FILES_AND_EVIDENCE.md).
- [Original source-grounded closure comparison](CLOSURE_EVIDENCE.md).
- [Complete unmodified original evidence package](original-snapshot/complete-original-inventory.zip).
  SHA-256: `8c5eb646e4da4821349c2f6bb16e76863a05d4dc02ab7872a272c807b83088f2`. The ZIP retains every original report, JSON, raw GitHub
  response, comparison script/log and original checksum file.

The original snapshot files inside the ZIP are historical inputs. Their wording
is subject to this explicit provisional overlay, and their CI states are not
asserted to be current. Source identity is not proof of original-run authenticity.
The original temporary paths in historical reproduction scripts are retained
verbatim; they describe collection history and are not live service dependencies.

## Five individual recommendations — prepared, not posted

| PR | Proposed disposition | Prepared recommendation |
|---|---|---|
| #645 | Provisional integration supersession; specification content represented | [PR_645.md](closure-recommendations/PR_645.md) |
| #747 | Provisional integration supersession; exact main-tree no-op | [PR_747.md](closure-recommendations/PR_747.md) |
| #760 | Provisional integration supersession; preserve old harness, reject stale deletions | [PR_760.md](closure-recommendations/PR_760.md) |
| #766 | Provisional integration supersession by merged #775; retain #767 separately | [PR_766.md](closure-recommendations/PR_766.md) |
| #769 | Provisional integration supersession; exact main-tree no-op | [PR_769.md](closure-recommendations/PR_769.md) |

Each recommendation pins the candidate, main and replacement commits, identifies
retained evidence, states remaining limits, and supplies unposted closure text.
[Structured recommendations](closure-recommendations.json) record
`posted: false` and `executed: false` for all five.

## Additional retained source and integrity

[closure-source-snapshots.zip](closure-source-snapshots.zip) retains exact source
blobs at each selected head, merge-base and main, plus relevant raw commit
objects. This is a changed-path evidence packet, not a full independent Git
repository or complete history backup. Do not delete original branches/history.
The [source manifest](closure-source-manifest.json) supplies Git blob identity,
SHA-256, source commit, path and byte count for every archived file.
[Comparison receipts](comparison-receipts.json) retain rerun read-only commands,
exit codes, output hashes and archived output paths in
[comparison-output.zip](comparison-output.zip). Raw diff whitespace is preserved
in that ZIP rather than normalized. [SHA256SUMS.json](SHA256SUMS.json)
covers every package file except itself. Hashes establish content identity, not
truth, original execution provenance or authority.

## Verification and limits

Run `python3 verify_package.py` from this directory for read-only stdlib integrity checks.

Preservation checks validate original checksums, exact ZIP identity, 80 unchanged
source/head/classification pairs with explicit provisional status, all five
recommendations, all archived source hashes, replacement ancestry in main, and
read-only no-op/content comparisons. They are artifact checks, not new software
qualification, benchmarks, model downloads, boot campaigns or scientific runs.
Earlier #782 arithmetic/capture corrections and earlier audit work were authored
by this same Codex session; this package is not independent validation of them.
All original correctness, provenance, hardware, authority and comparative-
performance limits remain. In particular, #780's known 352 representable-product
exceptions remain defects, not correct full-domain arithmetic; original-run
source-to-binary binding is unverified. SCG-08 acceptance/execution is not advanced.

No original PR or branch was changed, no closure recommendation was posted, and
no PR was closed or merged. Only this isolated audit branch/evidence package is
prepared for durable publication under the user's preservation instruction.
