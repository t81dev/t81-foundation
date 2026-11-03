# R75 correctness investigation

Read [the semantic report](../../docs/research/base01/BASE01_R75_SEMANTIC_QUALIFICATION.md).
This isolated probe links the unchanged audited VM and includes the exact six
historical builders from its test source. There is no timer, batching engine,
performance collection, protected experiment or authorization activation.
Only `--validate-only` is accepted. The Python verifier unsets T81_CANONFS_ROOT
so the VM factory does not attach persistent storage.

Audited source must be exactly `cda9135d36144d4de2771af174e52ac7b30a83c9` in a
separate worktree. Reproduction commands (all correctness-only):

```sh
git worktree add --detach /tmp/r75-source cda9135d36144d4de2771af174e52ac7b30a83c9
cmake -S /tmp/r75-source -B /tmp/r75-source-build -G Ninja \
  -DCMAKE_CXX_COMPILER=/usr/bin/c++ -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DT81_BUILD_BENCHMARKS=OFF \
  -DT81_BUILD_TUI=OFF -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_FUZZ_TESTS=OFF \
  -DT81_ENABLE_TERNARYOS=OFF -DT81_ENABLE_DPE=OFF
cmake --build /tmp/r75-source-build --target t81_vm_workload_determinism_tiers_test -j 4
cmake -S experiments/base01_r75_semantics -B /tmp/r75-probe -G Ninja \
  -DCMAKE_CXX_COMPILER=/usr/bin/c++ -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DAUDITED_SOURCE=/tmp/r75-source -DAUDITED_BUILD=/tmp/r75-source-build
cmake --build /tmp/r75-probe
python3 -B experiments/base01_r75_semantics/verify.py --validate-only \
  --source /tmp/r75-source --source-build /tmp/r75-source-build \
  --build /tmp/r75-probe --output /tmp/r75-new-evidence
```

Use the same C++23-capable compiler/standard library for both builds; `/usr/bin/c++`
is the tested macOS AppleClang compiler. Linux reproduction must select its
appropriate compiler; Linux has not been executed here. Building the audited
target fetches its existing pinned Asio dependency; do not run a broad CTest or
benchmark target. The archived source byte identity, compiler/CMake commands and
archive hashes allow checking which code was actually linked. The probe CMake
is independent of the repository build, and never patches it.

Choose a new output directory: existing directories are rejected. A failed
verification retains raw data without a success summary. `raw.jsonl.gz` contains
one JSON record per program, step, final state and counterexample. Decompress
without executing anything, e.g. `gzip -dc raw.jsonl.gz`. Observer events are
`[syscall, pc, opcode, context_count, visible_r75]`; context_count is a projected
per-op count inside traces, and the committed private count at trace entry/exit.
The verifier checks every step against a source-derived path, not just a final
golden integer. Signature repetition is not used as proof of counting semantics.

`summary.json` identifies the committed probe revision plus per-file hashes;
final evidence is appended after the implementation commit. It records exactly
six ledger rows, three deliberately corrupted-ledger rejections and rejection of
`--measure`. It does not contain performance samples. All artifact files have a
SHA-256 index. Hashes identify content, not truth, authority or independent review.

`evidence/asan-check.json` and build logs describe a second, correctness-only
run with AddressSanitizer on the probe translation unit. The linked audited VM
archives were not rebuilt with ASan; no whole-VM sanitizer coverage is claimed.
Its raw stdout matches the verified normal run and stderr is empty.

`evidence/rejected-probe-raw.jsonl.gz` is explicitly **not authoritative**. An
initial probe retained a raw observer pointer after `load_program()` replaced
and destroyed that engine for embedded-policy programs. The verifier rejected
its missing policy callbacks. That probe had a lifetime bug (undefined behavior),
so none of its observations supports the report's conclusions. The corrected
probe injects observers only into no-policy programs; embedded-policy builders
run unmodified with their actual policy engine. The valid final evidence comes
from the corrected probe, checked against source and an unobserved fresh VM.

This directory is correctness apparatus, not a B1 measurement runner. PRs
#687–#689 and their readiness labels are unchanged. No merge is performed.
