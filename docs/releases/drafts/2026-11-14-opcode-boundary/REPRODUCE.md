# Replay the bounded reproduction

Run from a checkout of main 9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2 containing
this documentation packet. These commands compile probes, not a release. They
write only owned temporary directories. Compiler/platform are recorded in
`evidence/source-manifest.json`; exact original argv and results are in the JSON
files beside the probe. Generated files are reproductions, not historical outputs.

```sh
baseline_dir=$(mktemp -d)
probe_dir=$(mktemp -d)
probe_source=docs/releases/drafts/2026-11-14-opcode-boundary/evidence/artifact_probe.cpp

git archive 295fc67245179f1b26e5284c70520019e72bb6c3 include isa/encoding.cpp isa/binary_io.cpp codec/base81.cpp | tar -x -C "$baseline_dir"

clang++ -std=c++23 -DNDEBUG -I "$baseline_dir/include" "$probe_source" "$baseline_dir/isa/encoding.cpp" "$baseline_dir/isa/binary_io.cpp" "$baseline_dir/codec/base81.cpp" -o "$probe_dir/old"
clang++ -std=c++23 -DNDEBUG -I include "$probe_source" isa/encoding.cpp isa/binary_io.cpp codec/base81.cpp -o "$probe_dir/current"

"$probe_dir/old" write "$probe_dir/old-produced"
"$probe_dir/old" read "$probe_dir/old-produced"
"$probe_dir/current" read "$probe_dir/old-produced"

"$probe_dir/current" write "$probe_dir/current-produced"
"$probe_dir/current" read "$probe_dir/current-produced"
"$probe_dir/old" read "$probe_dir/current-produced"
# Last command deliberately exits 5: persisted loader accepts Unknown, raw decoder rejects.
# Preserve that nonzero result; do not turn it into a passing arithmetic/execution claim.

clang++ -std=c++23 -DNDEBUG -I include tools/tisc_base81.cpp isa/encoding.cpp isa/base81_view.cpp codec/base81.cpp codec/trit_packing.cpp -o "$probe_dir/raw-tool"
"$probe_dir/raw-tool" decode "$probe_dir/old-produced.raw13"
```

The packet retains initial missing-dependency linking failures too. Corrected
commands above explicitly include base81.cpp and (for the standalone tool)
trit_packing.cpp. No prebuilt archive is linked by these successful commands.
Do not infer a full CLI, VM execution, cross-platform run or published-binary
source binding from these selected-source probes.
