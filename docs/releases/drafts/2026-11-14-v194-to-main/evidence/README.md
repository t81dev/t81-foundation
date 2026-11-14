# Evidence guide

Source scope is pinned in ../scope-manifest.json. Baseline headers and encoding.cpp
were extracted from commit 295fc67245179f1b26e5284c70520019e72bb6c3 into
/private/tmp/t81-release-baseline-v194. Current probes use main 9256b6ec.
Commands are recorded as argv arrays in probe-commands.json, wire-results.json,
fraction-noexcept-results.json and legacy-store-provenance.json. Run from this
worktree root; temporary executable/library paths must be recreated for replay.

- opcode and wire probes: both revisions compile and execute; VLoad's encoded
  first byte changes from 194 to 240 in a 13-byte record.
- noexcept_consumer: wrong namespace, both failures inconclusive, retained.
- fraction_noexcept_compat: corrected consumer; old compile 0, new compile 1 is
  the expected source-compatibility differential, not a passing current build.
- cell_limit: exit 0 means the known representable-product exception reproduced;
  it does not mean multiplication arithmetic is correct.
- legacy_store: rejects old unmarked fixture and checks retained original bytes;
  links a previously built archive at the tree-equal d71b526c revision. See the
  archive hash and explicit source-to-binary limitation in its provenance record.
- main-and-pr780-check-snapshot: GitHub API observation, not local execution;
  some checks remained in progress and required quality aggregation was absent.
- changed-files/history/diff: complete source inventory and selected contract diff.
- published-v194-release and anchore-pinned-action: upstream metadata/specification
  snapshots, not authentication of the original release binaries or new builds.
- packet-validation: repeated unchanged-source checks and JSON validation only.

No release archive, wheel, sdist or image was built here. No tag, workflow
publication, merge, push, performance campaign or scientific-gate action occurred.
Raw negative logs are retained unchanged, including compiler trailing whitespace.
SHA256SUMS establishes packet content identity, not independent validation.
