# Known limitations — draft release scope

All statements are scoped to main 9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2.
This is not a certification that the complete library/API is defect-free.

- **Storage compatibility:** old nonempty unmarked CanonFS stores reject. Portable
  filenames preserve hash identity but change filesystem layout. A safe migration
  must preserve objects, snapshots, capability decisions and provenance; no general
  migration tool was validated here. Persisted parity shards are rejected.
- **Instruction/profile compatibility:** current and v1.9.4 header probes disagree
  on affected byte assignments. RFC-00D6 also documents unresolved normative versus
  implemented LOAD/STORE/profile questions and untagged raw instruction records.
  Runtime 2.0.0 does not itself resolve or authorize a new frozen ISA/profile.
- **C++ source/ABI compatibility:** noexcept member-function consumers can fail to
  compile; public aggregates gain fields and SemanticAnalyzer changes its scope
  container. Rebuild consumers. SameMajorVersion CMake metadata is not an ABI proof.
- **Security behavior changes:** default verdicts deny; policy parsing is stricter.
  Low-level drivers still permit documented no-hook bootstrap operation. Hook
  replacement requires external serialization; no concurrent revocation/I/O
  transaction guarantee follows from pre-side-effect callback checks.
- **Cell arithmetic:** current-source probe reproduces representable 50*2=100
  rejected by intermediate shift overflow. cell.hpp is byte-identical to v1.9.4.
  Pending #780 characterizes 352 representable-product exceptions; that full count
  was not re-exhausted here and is not an endorsement of correct arithmetic.
  Division remains conversion-backed, not a demonstrated native ternary divider.
- **Portable int128:** supports the operator subset needed by T81, not every native
  __int128 operation. Selected compiler must compile AND link signed division and
  remainder. Do not substitute 64-bit intermediates or assume a preprocessor guard
  proves capability. The existing probe has a single-config build assumption.
- **Capture:** generic process capture has no global child timeout or output cap.
  Release regressions and their CTest timeout do not add such an API guarantee.
  Windows sanitizer coverage and unsupported reparse scenarios remain unverified.
- **Controlled AI:** verified user-consent handling is not established; the default
  consent requirement fails closed. Explicit policy-only operation does not prove
  consent. This surface is not broadly DCP-certified or production-ready.
- **Packaging/API exposure:** Direct API target is built but omitted from the
  installed export set while its headers are installed. The C API declaration
  header lives outside the installed include/t81 directory. Validate intended
  consumer support instead of advertising all repository declarations as SDK API.
- **Platform scope:** five archive platforms and CPython 3.9–3.13 are workflow
  targets, not artifacts built by this assessment. No Windows ARM64, musl, 32-bit,
  CPython 3.14 wheel, universal macOS binary or ARMv9 artifact is established here.
- **Version/license metadata:** CMake/config/banner say 1.9.1, Python says 1.9.2;
  CLI banner says MIT/GPL-3.0 while LICENSE, pyproject and Docker label say Apache
  2.0. These are source inconsistencies for maintainer reconciliation, not a new
  licensing determination or permission to change licenses automatically.
- **Evidence:** current main check snapshot is separate from prior PR checks and
  packaging. The reused core archive in the legacy-store probe is source-tree-equal
  to main, not freshly rebuilt. Initial invalid namespace probe failures remain.
  No new multi-platform release artifact, clean installed SDK, wheel or container
  was executed here. AI-authored/shared-effort summaries are not independent proof.
- **Research/governance:** H01–H08 and historical negative findings remain intact;
  W_ops is a stipulated selected-kernel model, not total cost or established
  hardware superiority. RFC-00E1/Gate 1/Gate 2 statuses are unchanged. DEC-015 is a
  historical, exact-head single-use integration exception, not a release waiver.
