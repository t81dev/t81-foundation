# Build-association contract — no approved runtime association yet

Descriptor identity identifies a source interpretation. It does not identify a
built evaluator or prove semantic equivalence. The host accepts a container only
when a separately reviewed local association binds that exact descriptor to the
actual executing build, permitted codec variants and resource policy.

A valid association record must contain non-null values for every field in
build-association-template.json except its explanatory template status, which is
replaced by an actual review/approval state. No template/null record is acceptable.
Required evidence:

- Exact SHA-256 of the executing binary and any loaded components affecting
  evaluation; exact build source revision/tree, toolchain/target/SDK, commands,
  dependencies, all feature definitions and numeric backends. Hash/record the
  native-int128 compile-and-link probe and selected backend; no 64-bit fallback.
- Descriptor digest and its pinned semantic source revision. This package pins
  hosted behavior at 9256b6ec; a later build with admission-only changes must retain
  an explicit source-diff/semantic-closure review and applicable conformance evidence.
  No automatic ancestor-based acceptance. Any changed VM/numeric/opcode behavior
  is outside this package and invalidates that assumed association.
- Payload ABI facts, explicit supported variant labels, and raw budgeted-decoder,
  Release-active rejection and signed-boundary/conformance evidence for those labels.
  Absence of a qualified variant rejects before construction. Architecture support
  is limited to the actual qualified target; different-target evidence is not reused
  as execution proof.
- Exact resource-policy digest from identities.json; runtime configuration may be
  stricter, never silently looser. Larger caps require a new reviewed policy record.
- Actual approval record URLs/IDs, reviewed SHA, reviewer identities, timestamps and
  scope. Build reproducibility/test logs and content hashes establish evidence
  identity, not their truth or independent authorship. Missing records reject.

The local registry is installed/configured through a maintainer-controlled trust
channel. An association downloaded from an artifact, a self-consistent manifest or
an arbitrary caller-writable path is untrusted and cannot authorize itself. The
implementation must tie the record to the running image and loaded semantic
components, not simply hash an attacker-replaceable pathname. Qualification must
show how platform executable/module identity is established; inability to establish
it fails admission. No new attestation/signature service is implemented or claimed.

All explicit host, caller, envelope and existing outer-container declarations must
agree on shared identity fields. No precedence override, nearest-version selection,
warning fallback or auto-registration. Producer_revision is informational. Runtime
policy/weights/entropy/budgets remain separate explicit inputs.

The current descriptor's 244 named values record a map; existing unsupported/stubbed
operations retain existing behavior. Association review cannot certify them all by
name enumeration or broaden DCP claims. The header-only inspection is not VM proof.
This package contains no valid build association and grants no runtime execution.

## Exact accepted-record shape (for future implementation review)

Canonical UTF-8 JSON uses recursive ASCII-key lexicographic order, compact separators and no
terminal LF; object keys are unique and unknown keys reject. Required keys are
exactly those in build-association-template.json, with these concrete constraints:

- association_version is integer 1; status is exactly `approved`; descriptor_sha256,
  binary_sha256 and caps_policy_sha256 are 64 lowercase hex. Revisions/trees are
  exactly 40 lowercase hex. Null/template values always reject.
- semantic_source_revision equals the descriptor's implementation revision.
  semantic_equivalence_evidence is an object with base_revision, target_revision,
  report_sha256 and locator; revisions match the semantic/build sources.
- semantic_components is an array of objects with role, locator and sha256. Empty
  means an explicit reviewed assertion that the entire semantic evaluator is in
  the named binary; it is not a default when component evidence is absent.
- toolchain_and_target has compiler_id, compiler_version, target_os, target_arch,
  target_abi and sdk, all nonempty strings. feature_definitions is the complete
  resolved string-to-string feature map, not a list of a few favorable flags.
- numeric_backends has signed_int128 (`native` or `portable`), math_configuration
  (nonempty exact build setting) and capability_evidence_sha256 (64 hex).
- payload_abi is exactly char_bits=8, byte_order=`little`, size_t_bits=64, int_bits=32,
  i32_bits=32, i64_bits=64, float_format=`ieee-binary32`, double_format=`ieee-binary64`.
- supported_codec_variants is a unique sorted array of the spec's exact labels,
  including insn18 and alias-enum-metadata for the mandatory base representation.
  No unknown label grants permission. Each label's bounded-decoder evidence must
  be covered by commands_and_raw_evidence and actual review scope.
- commands_and_raw_evidence is a nonempty array of objects with locator and sha256.
  Locators point to retained evidence in the trusted installation/review packet;
  admission must not fetch arbitrary remote URLs to bootstrap trust.
- review_records contains at least two actual distinct eligible reviewers' records,
  each with reviewer, reviewed_commit, review_url and archived_response_sha256.
  They approve the exact build/source association and evidence; reviewed_commit
  equals build_source_revision. Required reviews must not be dismissed, stale or
  self-approvals. Owner design direction is not a substitute.

String values are valid Unicode scalars, without normalization. Encode non-ASCII
scalars directly as UTF-8; escape quote/backslash as \"/\\ and controls with the
standard short escapes (backspace, form feed, newline, carriage return, tab),
otherwise lowercase \u00xx. Reject surrogate scalars, alternate escapes, escaped
slash and other noncanonical encodings. Object keys are the fixed ASCII names or
ASCII feature-macro names. This permits Windows paths/quoted macro values without
changing the artifact manifest's stricter ASCII-only contract. Locator syntax and
allowed local installation roots are host-controlled;
unsupported locator forms fail, never initiate network execution. A valid shape
is necessary but insufficient: only records admitted by the maintainer-controlled
local registry count. A user-generated `status:approved` record is not authority.
The registry trust/distribution mechanism and running-image checks need target
qualification before any association can be installed; no existing mechanism is
asserted or implemented by this draft.
