# Minimum current execution descriptor — descriptive candidate only

[CURRENT_EXECUTION_DESCRIPTOR.json](CURRENT_EXECUTION_DESCRIPTOR.json) captures
current hosted non-JIT interpretation at source
9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2. It does **not** register a profile, mint a
normative ISA version, accept the RFC-00D6 proposals or approve execution.

Minimum binding material is:

1. Exact source revision/tree and the existing hosted interpreter target. The
   source tree pins transitive source identity; selected file hashes aid review,
   but are not a proof of semantic completeness or source-to-binary binding.
2. Exact byte-to-existing-name map and LiteralKind assignments. The freshly
   compiled header inspection emits 244 named values; map SHA-256 is
   `41d1fd9102a4b297b58bee39ba8660bad6314cb616c619f6821b09563f015c3e`.
   Map membership does not promise each operation is implemented/verified: current
   stubs, policy denials and existing VM faults keep their current behavior.
3. Existing operand widths and source references for execution. Current direct
   Load/Store, A/B branch conventions, wrapped-difference TXor and existing
   fault/PC/tick behavior remain unchanged. This does not reconcile their known
   differences from normative specification claims.
4. An approved build association external to the artifact: actual binary digest,
   compiler/target/features, numeric backend and deterministic configuration,
   with applicable conformance evidence. Matching a JSON descriptor to a source
   commit does not establish that a binary implements it. No such association is
   approved by this packet; qualification is still pending.

Payload codec is a separate field in the artifact. Raw13's i32 B and saved18's i64
B/LiteralKind must never be conflated. Little-endian native payload ABI restrictions
are codec constraints, not alternative VM semantics. Runtime policy, model bytes,
entropy and budgets remain explicit execution inputs, not facts authenticated by
a descriptor hash.

No accepted profile name/digest is invented here. The JSON is a review candidate
with explanatory fields, not a new production schema or canonical descriptor
serialization. Its ordinary file hash identifies this document. Before runtime
use, maintainers must approve the precise operational fields/canonical bytes and
local registry/build association. Documentation edits must not silently change an
already accepted descriptor; a new incompatible binding would need a new identity.
All other/old descriptors reject in the initial proposed admission path; a caller
cannot authorize an unsupported evaluator just by declaring one.
