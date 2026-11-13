> **PROVISIONAL — preserved source comparison, not acceptance or closure authorization.**

# Source-backed supersession recommendations

Baseline: main `88300f14c1da1066ad6db25d31be0e8e912b3ef0`, tree
`fa633c105abfe229e77eabdccd2453c620636275`. Recommendations only: no PR was
closed, branch edited or merge performed. Historical PR commits and receipts
must remain accessible. No recommendation rests on age, draft status or CI failure.

## #747 — exact integration no-op

Head `b18075ad6de93091f9aea30707ef5237d479c9d1`.

`git merge-tree --write-tree MAIN HEAD` exits 0 and returns exactly main's tree
`fa633c105abfe229e77eabdccd2453c620636275`. Of its 26 changed paths, 25 head
blobs already equal main. The remaining root CMake file differs because main
also has later experiment/scope-lifetime registrations; the simulated merge
adds nothing. Its intended portability work is represented by merged #770
(`a4684582c9532061dc516836f10a8a0ed602d69f`) and #772
(`89223724038ff7dec349343469df4ea8389ede26`). Current Windows arithmetic and
subprocess repairs in #782 are separate work.

[Comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...b18075ad6de93091f9aea30707ef5237d479c9d1).

## #769 — exact integration no-op

Head `7c52be13dc0c3fb175271d63f1a8ba9aaeee170c`.

The same merge-tree command exits 0 and returns exactly main's tree. Three of
four changed blobs (semantic-analyzer header, API lock and regression test) are
byte-identical to main. Main's extra CMake changes do not imply a missing repair.
Merged #771 (`b96022c6b8e313a0ccd6bed6f1057d5e2368d092`) records the correction.
No unmerged scope-lifetime behavior remains in this head.

[Comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...7c52be13dc0c3fb175271d63f1a8ba9aaeee170c).

## #760 — initial harness represented; stale destructive residual

Head `dab59a63e4000f81799a6b2303bbfab26300d526` has the same complete tree as
its original implementation commit `525554a193af6a3e37f8ab43b895a2a9ca231419`
(`git diff --quiet ORIGINAL HEAD` exits 0). All seven initially added harness
paths exist on main. Two are byte-identical; other harness/report/schema/test
files have later fixes through merged #763
(`dbf107fda83a3755fa21d6b95caa3eb8ac450dd3`) and #765
(`36b3849aa1e6297e88de2ec1655777793ab0b59e`), then #775.
The residual delta deletes 50 newer diagnostic/dashboard paths and makes stale
visualization changes. Merging that residual would not preserve a unique new
harness. Preserve the original implementation report as historical evidence;
this recommendation is not scientific acceptance of any successor.

Exact 60-path status/blob list is under #760 in FILES_AND_EVIDENCE.md.
[Comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...dab59a63e4000f81799a6b2303bbfab26300d526).

## #766 — narrower correction superseded by merged #775

Head `a763ab6876bf07b8ae7c529656cdd2e5d9cf3766` modifies four paths, all present
on main. Current main's validator retains strict Python-int scalar validation,
rejects null/non-string publication SHA and uses full-string SHA matching. It
also requires the newer explicit nullable `evidence_generation_sha` distinction
and additional negative/parity tests, absent from #766. Current evidence/schema
and report preserve unknown original execution provenance rather than inferring
it from implementation/publication identity.

The source comparison against merged #775
(`d3a6cf55517a3098d7bae91b07c72694bc3472da`) supports supersession of this
bounded correction. Do not copy its older provenance interpretation back onto
main. Retain old record identity and candidate history.

**Do not extend this recommendation to #767**: its one-file follow-on still adds
calendar validation, finite JSON parsing, recursive schema checking and schema
keyword boundaries absent from main. #767 requires a current-main port/review
that preserves newer generation fields; it is not a duplicate.

[Main validator](https://github.com/t81dev/t81-foundation/blob/88300f14c1da1066ad6db25d31be0e8e912b3ef0/scripts/ci/validate_scg08_gate1_evidence.py).

## #645 — specification substance already present; only hard-break whitespace

Head `6d20d381322e4859e33524f4fc18d4c6569539ce` changes one specification file.
Direct comparison with current main shows only trailing Markdown spaces on five
introductory metadata lines. `git diff --quiet --ignore-space-at-eol MAIN HEAD --
experiments/h04_r03_matched_codec/SPECIFICATION.md` exits 0. The contract text
is already represented on main (history includes implementation `d8e1376a7`).
The optional hard-break rendering is not a missing scientific specification.
This is substantive supersession, not a claim of byte-identical rendered output.

[Comparison](https://github.com/t81dev/t81-foundation/compare/88300f14c1da1066ad6db25d31be0e8e912b3ef0...6d20d381322e4859e33524f4fc18d4c6569539ce).

## Cases deliberately NOT recommended for closure

- #778/#779: five shared scientific-contract files, but neither head contains
  the other and the harness/contract comparison changes eight paths substantially.
  #778 has unique rejection machinery and retained raw audit files. Choose a
  canonical correction and preserve unique evidence before closure.
- #718/#719: same proposed amendment path, no ancestor relation; 320 added and
  72 removed lines between heads. Later proposal is not proof all earlier intent
  is safely represented or accepted.
- #574–#577/#649: extensive source-path overlap, but no identical cumulative
  patch and different current-main integration context. Inspect surviving hunks
  and receipt identities; overlap alone is not duplicate proof.
- #554: 47 of 50 changed blobs equal main, but one absent and two differing paths
  remain. Partial representation is insufficient for whole-PR closure.
- Older unique audit/experiment records remain evidence even when their dated
  assertions are not current-main truth. Preserve or archive deliberately.
