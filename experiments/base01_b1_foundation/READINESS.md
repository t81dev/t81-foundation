# Engineering readiness — November 3, 2026

Scope: the isolated, non-measuring correctness foundation in this directory.
No workload is ready for measurement. Classification here is engineering scope,
not B1 host qualification, execution permission or acceptance of research claims.
Every receipt keeps qualification and measurement eligibility false.

| Workload | Final disposition | Evidence and remaining boundary |
| --- | --- | --- |
| WL-ARITH-CODEC-I64 | ENGINEERING COMPLETE — AWAITING QUALIFICATION | Exact 1,031-input corpus; separate allocating encode/pre-encoded decode; all outputs retained, canonical and checked by the unchanged B0 Python oracle. Logical capacity bounds enforced. A future larger measured batch and host/timer/retention qualification are outside this profile. |
| WL-CANONFS-MEM-THROUGHPUT | ENGINEERING COMPLETE — AWAITING QUALIFICATION | Exactly three payloads; two retained references and read outputs per size; byte equality and independent hashlib SHA3-512-prefix expectations pass. In-memory/bootstrap only. No disk, unique-ingest, policy-overhead or general cryptographic certification. |
| WL-VM-TISC-DETERMINISM-TIERS | ENGINEERING PARTIAL | All six unchanged builders pass isolated freshness/completion/R75/output/tag checks against PR #690. Status remains PROVISIONAL because independent review completion is not authoritatively established here. No semantic re-investigation or readiness promotion. |
| WL-RES-RUSAGE-METRICS | ENGINEERING PARTIAL | Linux/macOS unit, selector and lifecycle validators pass; actual child PID/parent/exit identities are retained. No real CPU/RSS collection API or child accounting collector is implemented; fixtures and selector constants do not qualify accounting accuracy. |
| WL-RES-ARTIFACT-SIZE | ENGINEERING COMPLETE — AWAITING QUALIFICATION | Exact SHA-256/file identity and bounded native-format section/segment parsing. Actual Mach-O metadata agrees with LLVM for 12 sections; ELF64 is fixture-tested only. Archive-member, fat/32-bit/big-endian/PE parsing unsupported. No ELF/Mach-O equivalence or bit-reproducible-build claim. |

No requested workload currently receives a BLOCKED engineering classification;
any failed validation exits without an eligible receipt. All measurement is
unavailable regardless of these labels. Qualification still needs separately
reviewed future instrumentation/authorization and an appropriate host. The VM
review and actual resource collector are explicit outstanding engineering items.

## Exact executed validation

The code/manifest source is pinned as in README. This implementation reused
already-built audited correctness archives after source-byte and configuration
checks; it did not change/rebuild the measured source revision or existing
Phase A/B0 artifacts. No changes were made to PRs #688–#690 or Jules' assignment.

Session paths:

- Checkout: `/private/tmp/t81-base01-b1-foundation`
- Audited source/build: `/private/tmp/t81-base01-audited`, `/private/tmp/t81-base01-source-build`
- Standalone native build: `/private/tmp/t81-b1-foundation-build`
- Final new correctness evidence: `/private/tmp/t81-b1-foundation-final-validation`

Exact build commands were README's standalone configure/build with those paths,
`/usr/bin/c++`, Release and CMake compile-command export. The target is
`foundation_check`; only its `--identity-only` and `--validate-only` modes run.
Native identity is compile-bound to CMakeLists/native/codec source files and
checked before workload launch, then checked again in output.

Correctness command:

```sh
python3 -B experiments/base01_b1_foundation/harness.py --validate-only \
  --source /private/tmp/t81-base01-audited \
  --source-build /private/tmp/t81-base01-source-build \
  --build /private/tmp/t81-b1-foundation-build \
  --output /private/tmp/t81-b1-foundation-final-validation
```

Test command (the native/raw variables enable all optional checks):

```sh
BASE01_FOUNDATION_NATIVE=/private/tmp/t81-b1-foundation-build/foundation_check \
  BASE01_FOUNDATION_RAW=/private/tmp/t81-b1-foundation-final-validation/native.json.gz \
  python3 -B -m unittest discover -s experiments/base01_b1_foundation -p 'test_*.py' -v
```

Results: **25 passed, 0 failures, 0 skips**; full native/controller correctness
validation PASS with VM PROVISIONAL. The suite's intentional malformed-data and
forbidden-request rejections are passing negative tests, not failures or measured
samples. It verifies Python measurement rejection with source/worker/subprocess
functions spied as uncalled, native request rejection with no output/files,
rejection of persistent-root configuration, stale implementation rejection before
worker launch, source/manifest identity, exact oracle-copy identity, retained
outputs and corruption, provisional VM scope, parser bounds/unsupported formats,
RSS fixtures, selector constants/no sampling, and worker lifecycle misuse.

Platform/compiler: macOS ARM64, Apple clang 21.0.0 (clang-2100.1.1.101), target
arm64-apple-darwin25.5.0, `/usr/bin/c++`, C++23 Release with LTO disabled. Actual
compiler version, OS/kernel/architecture and both build configurations/commands
are retained in the final receipt/caches. Linux execution unverified; Windows
unsupported. Simulated Windows rejection and synthetic ELF/RSS fixtures do not
establish execution on either platform. No B1 host qualification was attempted.

Static artifact cross-check: LLVM 22 `llvm-readobj --sections` read the actual
worker binary without executing it. Its 12 sections match parser name, segment,
address, memory size, file offset and section type. Raw output and tool/version/
binary identity are in `evidence/artifact-reference*`. One preliminary **text
adapter** attempt failed because LLVM printed an unnamed section type directly
as `0x16`; the adapter was corrected and the final comparison passed. The
production byte parser was unchanged and no workload test failed. This diagnostic
failure is retained in `artifact-reference-check.json`, not rewritten as a pass.
Static code formatting and `git diff --check` passed. Existing duplicate archive
link warnings remain in raw build output; no unrelated CI repair was attempted.

## Retained structured evidence

`evidence/final/receipt.json` uses the documented schema tag and records the
committed implementation revision plus per-file hashes. The evidence-only commit
then adds the final packet; that does not relabel earlier historical receipts.
`native.json.gz` preserves every returned codec digit/integer, CanonFS raw hash/
read byte and provisional VM field. `lifecycle.json` records actual spawn/reap
identity without clocks/resources. CPU/RSS sample collections and performance
samples are empty. No getrusage, wait4, timer, warm-up or benchmark path is invoked.

Source tracked bytes are verified against the exact Git tree before/after checks,
not only cached Git status. The complete bundled manifest is hash-pinned.
Compiler/CMake/compile commands and static artifact hashes are retained. The final
SHA-256 index covers every final raw record; content identity does not establish
authentic provenance or independent scientific validation. Existing final B0/R75
evidence remains untouched. Optional archive section results are UNSUPPORTED,
not silently skipped or mislabeled test failures.

The complete files in this increment are confined to
`experiments/base01_b1_foundation/`:

- `CMakeLists.txt`, `native.cpp`, `codec.cpp`, `codec.hpp`
- `harness.py`, `oracle.py`, `observers.py`, `test_foundation.py`
- `phase-a-manifest.json`
- `README.md`, `RETENTION_AND_BOUNDS.md`, `AUTHORIZATION_INTERFACE.md`, `READINESS.md`
- `evidence/.gitattributes`, `evidence/configure.log`, `evidence/build.log`, `evidence/tests.log`
- `evidence/artifact-reference.txt`, `evidence/artifact-reference-check.json`
- `evidence/final/receipt.json`, `evidence/final/native.json.gz`, `evidence/final/native-stderr.txt`
- `evidence/final/lifecycle.json`, `evidence/final/sha256.json`
- `evidence/final/source-cache.txt.gz`, `evidence/final/native-cache.txt.gz`
- `evidence/final/source-compile-commands.json.gz`, `evidence/final/native-compile-commands.json.gz`

## Unavailable operations and next boundary

All timing/warm-up/pilot/trial/campaign/permit selectors reject. Fake authorization
environment variables do not alter rejection. The future interface proposal is a
Markdown document only; there is no signed-permit dispatcher or activation code.
Native `--identity-only` is metadata-only, never an execution eligibility grant.
No general-purpose command/selector allows protected workloads to be loaded.

The new branch/PR commits use `[skip ci]`; no benchmark job is dispatched or rerun.
No general CI pass is claimed. No performance measurements, PMU, SPICE, CMP3 or
H04-R03 Phase C execution, frozen-artifact changes, root build/workflow edits,
measurement activation or merge occurred. Stop at the draft and engineering report.
