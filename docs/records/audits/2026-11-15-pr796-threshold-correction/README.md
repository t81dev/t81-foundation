# PR #796 threshold correction and frozen readiness packet

The archived completed readiness results belong **only** to old head
`b48e1a4da29784860a528e5397f896c73bc48450`, main CI run `43717280854`,
attempt 2. They do not qualify this correction or a later head.

The original threshold step could mask checker failures through tee, and the
original checker could pass incomplete data. Consequently, its passing hosted
step did not establish reliable SIMD threshold enforcement. The historical
packet is retained unchanged, including the old report, first-attempt network
failure and ARMv9 informational failure. Its gate-assurance claim is superseded
by this explicit correction; other executed test results retain their old scope.

The checked-in `pr796-b48e1a4da-readiness-packet.tar.gz` contains all 58 original
files byte-for-byte, including the original SHA256SUMS.json. The separate
`old-packet-SHA256SUMS.json` is an identical copy of that internal manifest.
`publication.json` supplies the archive SHA-256 and exact old-head attribution.
Use a GitHub URL pinned to the publishing commit for immutable access; this is
repository evidence, not a temporary Actions artifact or local-only path.

## Correction contract

- The threshold step explicitly runs Bash with `set -euo pipefail` before the
  actual checker/tee pipeline, so checker failure is the step's failure.
- An independent `--benchmark_list_tests` invocation of the same binary records
  compiled TAnd implementations. SWAR is always registered; AVX2 and NEON follow
  the actual source's compile guards. CPU-name guessing and surviving timing
  results do not determine the expected implementation set. No nonexistent
  TOr/TNot benchmarks are required.
- Each enabled implementation must have all seven registered sizes (16, 64,
  256, 1024, 4096, 16384, 65536), with a SWAR baseline and at least one enabled
  SIMD comparator. Unsupported baseline-only builds cannot pass this gate.
- Missing/unreadable files, malformed/empty JSON, duplicate keys/results,
  incomplete size/pair coverage, unexpected records, benchmark errors, missing
  timings, invalid/non-finite/nonpositive timings and unsupported units fail
  explicitly. Raw iteration results and positive iteration counts are required.
- Timing units are normalized before comparing each enabled SIMD/SWAR pair at
  sizes 16384 and 65536 against the unchanged 1.15 limit. A threshold violation
  returns 1, including through tee.

`tests/python/test_simd_regression.py` extracts the threshold step verbatim from
`.github/workflows/ci.yml` and executes it with Bash -e but **without** an ambient
pipefail setting. It tests both direct CLI and actual workflow routes, positive
AVX2/NEON controls, 46 result rejections, seven inventory rejections and a boundary/
unit control. Removing only pipefail recreates the false-success exit; the real
workflow must reject the same threshold violation. Tests run normally and under
Python -O in the existing CI job. No standalone workflow is introduced.

All 48 accepted component files and raw Cell measurement blobs remain unchanged.
No Cell timing campaign is run. Fresh correction results are recorded separately
from the immutable old-head packet; no merge or closure is performed.

## Local correction verification

The five focused unittest methods pass normally and under Python -O. Each mode
executes 46 bad-result and seven bad-inventory scenarios through both direct CLI
and the verbatim workflow pipeline, plus positive AVX2/NEON, threshold-boundary/
unit controls and the deliberately weakened-pipefail control. Exact commands,
stdout/stderr and source hashes are retained in local-validation/results.json.
Workflow action/permissions audits, tabnanny and diff checks pass.

The native existing binary was enumerated without running timed loops; its
registry contains all seven SWAR and NEON sizes. Archived old Linux SIMD data
was replayed successfully against an explicitly supplied AVX2 profile fixture.
That replay is not a fresh measurement or an independently regenerated old
binary registry. All 48 accepted files and protected historical artifacts match
their recorded identities. Fresh hosted correction checks are pending.
