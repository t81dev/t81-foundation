# T81-CPA-002 — Opcode conformance gate repair

## Scope and source identity

This corrects the deterministic runtime / CI stability lane. Only the opcode
matrix test and its validation records change. Opcode definitions, the decoder,
frozen specifications, and the freeze checker remain byte-identical to baseline.
No freestanding evaluator, merge, or research experiment is included.

- Baseline: `803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4`.
- Validated test-source commit: `74bc8bc824a9732c57e7225077bf15ddc6c05097`.
- Test Git blob: `74a51d042d4304a8b14bb03a7704ccb25fbaf3d0`.
- [Machine-readable commands, outputs, exits and identity checks](T81_CPA_002_OPCODE_CONFORMANCE.results.json).

The test source fetched at that commit was compared directly with the exact local
source compiled below. This report and its receipt are added afterward; the test
content is unchanged. Validation ran in an isolated clone, leaving the original
checkout's pre-existing edits untouched. This is agent-executed engineering
validation, not an independent-person review or scientific acceptance.

## Verified cause and corrective change

An independent source scan counted **244 named enum members**, with 244 matching
`opcode_name` cases and no duplicate enum values. `kAllOpcodes` instead constructs
**247 numerical values**, 0–246 inclusive. Values **221–223** have no enum member
or name. The old matrix skipped them when checking names, but included them in
its encoding/decoding roundtrip. The unchanged decoder correctly rejects them.
Values **247–255** are also undefined in the current implementation.

The repair scans the complete byte domain and uses the header's `opcode_name`
definitions to select positive roundtrips. An explicit 12-byte undefined list and
coverage-count check prevent a missing name from silently reducing coverage.
All 244 opcode values and their A/B/C operands roundtrip; 13-byte instruction
width is checked. All 12 undefined encodings are rejected both as a single
instruction and after a valid instruction prefix, with `InvalidOpcode` verified.

Every former `assert` is replaced with the existing `T81_TEST_CHECK` helper,
which evaluates its predicate and exits 1 on failure even under `NDEBUG`. The
success message is reached only after positive, mapping and negative checks.
No global build flags or shared test helper were changed. This validates opcode
encoding and rejection, not execution semantics for every named instruction.

## Before/after and effective Release checks

Executed on Darwin 25.5.0 ARM64 with Homebrew Clang 22.1.1, C++23 and Python
3.14.3. Debug-style flags were `-O0 -g -UNDEBUG`; Release-style flags were
`-O3 -DNDEBUG`. These are direct compiler-mode checks, not a full project
CMake/CTest run. The unchanged CMake target already compiles this source; using
an always-active helper also handles the project's global `NDEBUG` flag.

| Check | Debug / UNDEBUG | Release / NDEBUG |
|---|---|---|
| Baseline opcode matrix | SIGABRT, subprocess return -6, failed `decoded.has_value()` at line 64 | Exit 0; misleading success string |
| Repaired opcode matrix | Exit 0; 244 roundtrips and 12 undefined encodings checked | Exit 0; same checks and counts |
| Temporary wrong frozen mapping expectation | Exit 1 at `Int2BigInt == 192` | Exit 1 at the same predicate |
| Temporary decoder accepts undefined byte 221 | Exit 1 at `!rejected_single.has_value()` | Exit 1 at the same predicate |
| Existing encoding test | Exit 0 | Exit 0 |
| Existing ternary arithmetic test | Exit 0; `ternary_arith ok` | Exit 0; `ternary_arith ok` |

The mutation copies exist only in validation scratch space. The wrong expectation
changes 193 to 192 in a copy of the test. The decoder mutation changes its opcode
guard to `if (!is_valid_opcode(opcode) && opcode != 221)` in a copy of
`isa/encoding.cpp`. Both faulty variants fail in both modes and produce no
success message. This demonstrates meaningful Release-mode checks, beyond a
green run against only correct inputs. Neither mutation is part of the PR.

The unchanged freeze checker also exited 0, reporting 209 registry opcodes,
verified bitwise mappings and a clean reserved range. It checks the Markdown
registry, not agreement with the C++ header. Its pass does not resolve the
remaining discrepancies below. The unchanged encoding test still uses `assert`;
its UNDEBUG run executes its predicates, while its NDEBUG pass alone would not
prove those predicates ran. The repaired matrix checks remain active in both.

## Remaining registry/header discrepancies — not repaired here

| Source | Observed definition/count |
|---|---|
| `include/t81/isa/opcodes.hpp` | 244 named values; 0–220 and 224–246 |
| `kAllOpcodes` in that header | 247 entries, including holes 221–223; unchanged |
| `spec/tisc/opcode-registry.md` tables and consistency audit | 209 entries; audit still claims 209 header entries and no discrepancies |
| `spec/tisc/TISC_CAPABILITY_CONTRACT_v1.1.0.md` | 174 frozen values, 0–173; says 174–255 reserved |
| Freeze script | Docstring says 174 / reserved 174–255; executable checks expect 209 and a maximum of 215 |

All registry names occur in the header, but 35 header names are missing from the
registry. Four common names have conflicting assignments:

| Name | Registry | Header |
|---|---:|---:|
| TVecSub | 212 | 205 |
| TNOT_SWAR | 213 | 206 |
| TAND_SWAR | 214 | 207 |
| TOR_SWAR | 215 | 220 |

The registry's encoding overview still says 200–255 reserved, while its later
reserved section says 216–255; its tables assign some of these values. The
current header names many values inside those historical reserved ranges. The
test targets the actual current header/decoder boundary; it does not declare
those historical specification conflicts resolved or authorize extensions.
Changing frozen authority, assignments or decoder behavior is outside this task.
The receipt lists all 35 missing names and the unchanged input hashes.

## Exact commands and reproduction

Every executed compile and run is retained as an argument vector, working
directory, exit code, stdout and stderr in the linked JSON receipt. Commands ran
from `/tmp/t81-cpa-002`; scratch binaries and mutation sources were under
`/tmp/t81-cpa-002-validation`. For example:

```sh
/opt/homebrew/opt/llvm/bin/clang++ -std=c++23 -O0 -g -UNDEBUG -Iinclude -Itests/cpp tests/cpp/tisc_opcode_matrix_test.cpp isa/encoding.cpp -o /tmp/t81-cpa-002-validation/matrix-debug
/tmp/t81-cpa-002-validation/matrix-debug
/opt/homebrew/opt/llvm/bin/clang++ -std=c++23 -O3 -DNDEBUG -Iinclude -Itests/cpp tests/cpp/tisc_opcode_matrix_test.cpp isa/encoding.cpp -o /tmp/t81-cpa-002-validation/matrix-release
/tmp/t81-cpa-002-validation/matrix-release
python3 -B scripts/ci/check_tisc_freeze_integrity.py
```

The repaired matrix output in each mode was:

```text
tisc opcode matrix tests passed: 244 defined opcodes, 12 undefined encodings rejected.
```

The following replays the retained commands on the same toolchain from a checkout
of this PR. It recreates temporary baseline/mutated sources outside the repo;
`resource` and the SIGABRT return value are specific to the recorded Unix host.
It requires no package installation or network access. The baseline Git object
must be available locally.

```python
import json
import resource
import subprocess
import tempfile
from pathlib import Path

resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
root = Path.cwd()
scratch = Path(tempfile.mkdtemp(prefix="t81-cpa-002-replay-"))
receipt = json.loads((root / "docs/records/audits/T81_CPA_002_OPCODE_CONFORMANCE.results.json").read_text())
base = receipt["baseline_revision"]
(scratch / "baseline_matrix.cpp").write_bytes(subprocess.check_output([
    "git", "show", base + ":tests/cpp/tisc_opcode_matrix_test.cpp"]))
matrix = (root / "tests/cpp/tisc_opcode_matrix_test.cpp").read_text()
(scratch / "bad-mapping.cpp").write_text(matrix.replace(
    "Opcode::Int2BigInt) == 193", "Opcode::Int2BigInt) == 192"))
decoder = (root / "isa/encoding.cpp").read_text()
(scratch / "accept-undefined.cpp").write_text(decoder.replace(
    "if (!is_valid_opcode(opcode))",
    "if (!is_valid_opcode(opcode) && opcode != 221)"))

# Stored after-results are sorted by label; compile every binary before runs.
rows = receipt["before"] + receipt["after"]
for row in sorted(rows, key=lambda r: (not r["label"].startswith("compile-"), r["label"])):
    argv = [arg.replace("/tmp/t81-cpa-002-validation", str(scratch)) for arg in row["command"]]
    result = subprocess.run(argv, cwd=root, text=True, capture_output=True)
    expected = row.get("expected_exit", row["exit_code"])
    if result.returncode != expected:
        raise RuntimeError((row["label"], result.returncode, result.stdout, result.stderr))
    if row["label"].startswith("run-"):
        print(row["label"], result.returncode, result.stdout, result.stderr)
```

Byte equality with the baseline was checked for the opcode header, decoder,
registry, capability document, freeze checker and shared assertion helper.
`git diff --check` passed. The PR has no production code or frozen ISA changes.
The added test uses standard C++ facilities and the existing portable helper;
no platform-specific runtime dependency is introduced. Linux, Windows, x86-64,
ARM64 cross-builds and remote CI were not executed here; only macOS ARM64 results
are claimed. Future registry repair and any freestanding evaluator remain
separate authorized tasks.
