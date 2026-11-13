# ARCH-002 documentation validation

> Current R1 closure results are in [R1 closure validation](#r1-closure-validation).
> The original draft-validation record below is retained as historical context
> for pinned commit 182634e0d5e4ac3a80413791d07a27dd7e82bc27; it is not a new
> correctness result or implementation/execution authorization.

Baseline: `d134407ae1059c4d7f6a9418177e695e0e5d3b51`. Assessment date: 2026-11-05. These are documentation
checks, not correctness experiments or independent scientific validation.

## Results

- Proposal syntax and Draft 2020-12 schema with format checking: PASS.
- Commons contribution shape/reference check: exit 0, `valid_proposal_shape`,
  `execution: unavailable`; no authorization authenticity or scientific acceptance.
- All six discovery operation types passed (including both H03 and H05).
- 6 commit-pinned Markdown source links/line anchors resolve locally;
  relative protocol/proposal/validation links resolve.
- Static consistency checks cover the shared 0.99375 t quantile, 0.975 per-endpoint
  planning power, latency threshold, strict admission, caps and phase separation.
  They do not establish statistical assumptions or validate a future harness.
- No kernel/oracle implementation, scientific corpus generation, timing pilot,
  benchmark, simulation, production change, CMake build or CTest was executed.
- Workflow files were inspected; the documentation commit uses `[skip ci]` to
  avoid automatically requesting performance execution. No CI success is claimed.

## Tool/environment and repeatable checks

- Python `3.14.3`, `Darwin arm64`.
- jsonschema `4.23.0` from temporary
  `PYTHONPATH=/tmp/t81-h02-r01-schema`; no dependency files changed.
- Exact source reads: `git show <baseline>:<path>`; SHA-256 below.
- Schema validation: `Draft202012Validator(schema, format_checker=FormatChecker()).validate(proposal)`.
- Contribution validation:

```sh
PYTHONPATH=/tmp/t81-h02-r01-schema python3 -B tools/research_commons/commons.py --validate-contribution docs/research/arch002/proposal.json
```

The temporary dependency path is local tooling, not a required production path;
a reviewer can use any environment with the recorded validator.

## Non-duplication audit

Tracked-main searches were scoped to source/tests/benchmarks/experiments and
research indexes. Matching masks, catalog filtering, codec and arithmetic work
was then inspected as listed in PROTOCOL section 1. Remote GitHub PR searches
`repo:t81dev/t81-foundation "equality"` and `repo:t81dev/t81-foundation "ARCH-002"`
returned related work and no ARCH-002 PR respectively. Search results are not a
proof about all unpublished or unindexed work. No equivalent completed packet
was found; discovery of one before implementation triggers STOP/reuse.

Command: `git grep -n -i -E count_equal|equality_count|equal_positions|hamming d134407ae1059c4d7f6a9418177e695e0e5d3b51 -- include benchmarks experiments tests :!*.json :!*.txt :!*.log :!*.gz`

Exit 1; no matches.

Command: `git grep -n -i -E ARCH-002|ARCH-001 d134407ae1059c4d7f6a9418177e695e0e5d3b51 -- docs/research experiments benchmarks`

Exit 1; no matches.

## Discovery receipts

Operation | Exit | SHA-256 of exact response
---|---:|---
`list_research_tracks` | 0 | `e71052118759ee614d09f5212a79f9a54e359eb8d06752bf70ea251e7c69d13c`
`list_hypotheses` | 0 | `c93c6abcdf2e2730724133864e5bdd64d0ed19235f519b734da8923cb50b7f8e`
`get_hypothesis` | 0 | `d0206aab2c9819bfdd39efb96c738351666059d2c4b800a0512ab19ddde4e0d5`
`get_hypothesis` | 0 | `1c85aa6a81d283183a43798cf929e6c608acc1ed9fc0294cab03aabdfc7403c0`
`get_evidence` | 0 | `ac354e8682acc430f6515727976d675f58deec367c0ae778313ed2afca62d37e`
`get_reproduction_instructions` | 0 | `51edbba586c90820c0b689641a05ae9b8bb58292b12d94759ae64db22156e095`
`list_open_research_questions` | 0 | `4ba2a1d2e9d6a570cdbd30b7dc66452b5682219bddd763e0c618ad51c9ac5a26`

Responses use the Commons historical manifest, not live repository status.

## Inspected source identities

Path | SHA-256 at baseline
---|---
| `docs/research/continuity/HYPOTHESIS_REGISTER_v1.json` | `af8ca65dcef136f294eb33010b5a6621875a52db4f3e60b94a4e28db5318a1c1` |
| `include/t81/experimental/packed_trit_vector.hpp` | `6da8852191272edeaa85d310bbb26be43b9660ba656e180db4727d9a796cb67a` |
| `include/t81/packed_trit_vector.hpp` | `f2695aa4d6ab21874732abefa1f23f0eb65dd956e61c0dd7570741c620bc7ac6` |
| `codec/trit_packing.cpp` | `0675981cf277cb66e4ae1f7c2eb5939c0c89c3d546408e76310d7e7524037a4a` |
| `include/t81/native.hpp` | `f982788052247bcdfacfe368e0d719f29bdc396a20bc1695a8f4264ba32f7952` |
| `include/t81/support/bitopts.hpp` | `403ce1f5ff55d0a533ed86a23021c83d91b28a5124d5a35b25e148029ce388fc` |
| `tests/cpp/test_tritwise_backend_equivalence.cpp` | `e3c40bf11cde8181160b8825e13ef43836cf956e876238c348b2bcf6377b5ebc` |
| `tests/cpp/test_packed_trit_vector.cpp` | `33aa93749ba805fc3e6e57011dbb25ae1b9605ff704ba9fed571eabfa4f5f4a2` |
| `benchmarks/BM_TritwiseWorkloads.cpp` | `09426f5882b0b3893f9eddf409dcd454fb0b36894d56be1c4d1190d4bfe31929` |
| `benchmarks/BM_PackedTritVector.cpp` | `f2363c506dc28d65675a605be733f1fe783a47f6900c7af7aa0277c22c72e30f` |
| `experiments/h04_r03_matched_codec/SPECIFICATION.md` | `27d6748476f2ae662d96fccecf831380aae9847203bf6cd93a76beb742469b22` |
| `docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-DESIGN-ASSESSMENT.md` | `a620a0494d6272fe6f0ce3287b1c9997aad77765a226caa2b8563bdcb48523eb` |
| `docs/research/commons/manifest.v1.json` | `c6e2642365399b85a98e53727d1ec5a8240c3cb51ea86416565543a0658de911` |

## Final document-only checks

Final strict JSON parsing/schema and local Markdown-link checks: PASS. The
staged diff passes `git diff --cached --check` and contains exactly the three
new files in this directory. Static checks confirm the final shared t quantile,
latency threshold, pilot cap definition and explicit drift/order rules.
No experimental execution or remote CI result is claimed.

## R1 closure validation

Assessment date: 2026-11-05. Recommendation: **ACCEPTANCE-READY for independent
protocol review**, not accepted and not authorized for correctness implementation
or execution. R1 editor: Codex; original draft and the user's six ARCH-003
ambiguity descriptions were the prior context. No independent-person review,
kernel behavior, empirical feasibility or performance result is claimed.

Amendment baseline: `182634e0d5e4ac3a80413791d07a27dd7e82bc27`, verified as PR
#714's initial head. Reusable source links retain their original baseline
`d134407ae1059c4d7f6a9418177e695e0e5d3b51`. This distinction avoids rewriting
the historical inventory. The final commit identity is supplied by the PR head
and delivery message; it is not embedded in its own commit's file contents.

### Scope and decisions

Only these three EXISTING documentation files are modified:

- `docs/research/arch002/PROTOCOL.md`: revised request/admission, frozen matrix,
  evidence format, sanitizer profiles, aggregate accounting and six-entry R1
  decision ledger in section 9.
- `docs/research/arch002/proposal.json`: amendment pin and accurate R1 method,
  attribution, output/resource/coverage and documentation-only authorization
  language. Status remains proposed, H03 remains canonical, model stays null.
- `docs/research/arch002/VALIDATION.md`: this current check record, retaining the
  preceding historical record and its receipts.

Manual protocol review resolved: R1-01 L/V/W process roles and non-resettable
aggregate resources; R1-02 two operand-local format tags and total error
precedence; R1-03 mandatory correctness-only NATIVE; R1-04 twelve finite ordered
families rather than an implicit Cartesian product; R1-05 exact status IDs,
88/32-byte records, digest and hash-root rules; R1-06 full D/R/S replay,
instrumentation boundaries, negative probes and explicit unavailable-toolchain
outcomes. No branch of the protocol can drop a required arm/profile to obtain
a complete correctness pass. All compiler/runtime/diagnostic availability checks
are FUTURE tasks after authorization, not checks executed for this amendment.

The three-process runtime tree is deliberately one owner, one independent
verifier and one computation worker, not three computational workers. The owner
can observe the verifier's expected nonzero negative-control exits without
resetting the campaign clock. The aggregate RSS closure is conservative absolute
512 MiB, with no baseline subtraction; all builds/oracles/probes/guards and
profiles share 1,800 s and 100 MiB retained-output ceilings. This strengthens
accounting rather than multiplying the old process budget. No statistical
performance criterion is changed.

### Executed documentation checks

| Check | Result / limit |
|---|---|
| Strict JSON with duplicate-key/nonfinite rejection; Draft 2020-12 schema and format checker | PASS |
| Commons contribution reference/shape check | Exit 0; valid_proposal_shape; execution unavailable; no authenticity/acceptance proof |
| All six Commons discovery operation types | Exit 0, including separate H03/H05 reads; historical pinned index only |
| Entire PROTOCOL section 7 against the pinned amendment baseline | Byte-identical; SHA-256 `3ef0a7897cd931df9ee5f030751fbd49143dcbcd47480f913f04e1c312d94b63` |
| Proposal measurement_plan, research_question, matched_control, falsification_criterion and historical evidence refs | Unchanged values |
| Mathematical operation paragraph and all five canonical payload/admission table cells | Unchanged (NATIVE's optional/mandatory label alone changes) |
| Original SplitMix/phase/seed definition and four conventional-control table rows | Unchanged |
| Schedule cardinalities, eligible-role multiplicities and paired-offset coverage | PASS by metadata arithmetic, not by generating any symbol arrays |
| Explicit little-endian sizes and output-budget arithmetic | PASS; no experimental record bytes or expected scientific counts produced |
| Six original commit-pinned source anchors and local document links | Resolve locally at their pinned revisions |
| File allowlist / whitespace | Only the three existing ARCH-002 files; git diff --check PASS |

Cardinality: 408,034 request records, 483,553 core-entry-point calls/profile,
1,450,659 across D/R/S; six oracle streams account for 2,448,204 record
evaluations. Exact normal binary sizes sum to 82,328,144 bytes, with 392 guard
bytes additionally specified. Their total is below 84 MiB, and correctness
sub-budgets plus headroom sum to 100 MiB. These are finite-table/field-width
calculations only. Actual wall time, RSS, sanitizer overhead and byte-for-byte
future oracle agreement are unknown; a future cap breach must be retained as
incomplete evidence rather than remedied by deleting cases.

Environment: Python 3.14.3, Darwin arm64. JSON validator 4.23.0 was absent from
the default interpreter and installed only in a temporary directory. Exact
resolved helper versions: attrs 26.1.0, jsonschema-specifications 2025.9.1,
referencing 0.37.0, rpds-py 2026.9.1. No repository dependency file changed.

```sh
python3 -m pip install --no-cache-dir --target /private/tmp/arch002-doccheck jsonschema==4.23.0
PYTHONPATH=/private/tmp/arch002-doccheck python3 -B tools/research_commons/commons.py --validate-contribution docs/research/arch002/proposal.json
PYTHONPATH=/private/tmp/arch002-doccheck python3 -B /private/tmp/validate_arch002_r1.py
git diff --name-status 182634e0d5e4ac3a80413791d07a27dd7e82bc27
git diff --check
```

The contribution result was exactly:

```json
{"execution":"unavailable","limitation":"Shape/reference checks establish neither authorization authenticity nor scientific acceptance.","status":"valid_proposal_shape"}
```

The static validator used is reproduced below so the temporary filename is not
a dependency. Save the block outside the repository and invoke it from a clean
checkout of this revision containing the pinned baseline object. It checks
documentation structure/identity and arithmetic only; it implements no
experimental encoder, RNG, oracle, kernel, corpus, sanitizer probe or statistic.

```python
"""Documentation checks only: no symbol arrays, kernels, or experimental cases."""
import hashlib
import json
import re
import struct
import subprocess
from pathlib import Path
from jsonschema import Draft202012Validator, FormatChecker

BASE = '182634e0d5e4ac3a80413791d07a27dd7e82bc27'
DIR = Path('docs/research/arch002')
allowed = {str(DIR / f) for f in ('PROTOCOL.md', 'proposal.json', 'VALIDATION.md')}

def check(ok, message):
    if not ok:
        raise SystemExit('FAIL: ' + message)
    print('PASS: ' + message)

def git(*args):
    return subprocess.check_output(['git', *args])

def pairs(items):
    value = {}
    for k, v in items:
        if k in value:
            raise ValueError('duplicate JSON key: ' + k)
        value[k] = v
    return value

def strict(raw):
    return json.loads(raw, object_pairs_hook=pairs,
                      parse_constant=lambda x: (_ for _ in ()).throw(ValueError(x)))

proposal = strict((DIR / 'proposal.json').read_text())
schema = strict(Path('docs/research/commons/experiment-proposal.schema.json').read_text())
Draft202012Validator(schema, format_checker=FormatChecker()).validate(proposal)
check(True, 'strict JSON and Draft 2020-12 proposal schema with format checking')
old = git('show', BASE + ':' + str(DIR / 'PROTOCOL.md')).decode()
new = (DIR / 'PROTOCOL.md').read_text()
before = strict(git('show', BASE + ':' + str(DIR / 'proposal.json')).decode())

def section(text, number, next_number):
    return text[text.index('## ' + str(number) + '.'):text.index('## ' + str(next_number) + '.')]

old_stat = section(old, 7, 8)
check(section(new, 7, 8) == old_stat, 'entire performance-statistics section byte-preserved')
print('section_7_sha256=' + hashlib.sha256(old_stat.encode()).hexdigest())
check(all(proposal[k] == before[k] for k in ('measurement_plan','research_question','matched_control','falsification_criterion','evidence')), 'statistical proposal fields, strongest matched controls and historical evidence refs unchanged')
check(proposal['status'] == 'proposed' and proposal['hypothesis_id'] == 'H03' and proposal['attribution']['model'] is None and proposal['human_authorization']['status'] == 'not_requested', 'proposal remains proposed/unexecuted, H03 and unknown model retained, no execution grant')
check(proposal['repository_revision'] == BASE, 'amendment pin is the user-specified baseline')
math = re.search(r'Inputs are two immutable.*?(?=\n\n)', old, re.S).group()
check(math in new, 'mathematical operation paragraph unchanged')
for fmt in ('BYTE','C2','PT5','NATIVE','PLANES'):
    def cell(s):
        line = next(l for l in section(s,3,4).splitlines() if l.startswith('| '+fmt+' ') or l.startswith('| '+fmt+' |'))
        return line.split('|',2)[2]
    check(cell(old) == cell(new), fmt + ' canonical encoding/admission cell unchanged')
for arm in ('B-BYTE','B-PLANES','B-C2','B-PT5'):
    row = next(l for l in section(old,5,6).splitlines() if l.startswith('| '+arm+' |'))
    check(row in section(new,5,6), arm + ' strongest comparator requirement unchanged')
generator = old[old.index('Fixed generator for later authorized implementation:'):old.index('## 5.')]
check(generator in new, 'complete SplitMix/phase/seed definition unchanged')

# Metadata cardinalities only; this does not enumerate any symbol vectors.
L = json.loads(re.search(r'`(\[0,1,2,3,4,5,7,8,9,.*?4097\])`', new, re.S).group(1))
K = json.loads(re.search(r'`K=(\[.*?\])`', new).group(1))
check(L == [0,1,2,3,4,5,7,8,9,15,16,17,31,32,33,63,64,65,127,128,129,255,256,257,1023,1024,1025,4097], 'all original boundary lengths retained')
positions = lambda n: {i for i in [0,n-1,*[q for k in K for q in (k-1,k)]] if 0 <= i < n}
logical = [10, sum(9**n for n in range(1,6)), sum(14+2*len(positions(n)) for n in L), len(L)*2*32, 4, len(L)*32*2]
counts = [(5*n,6*n) for n in logical]
counts += [(256*2*5,256*2*6), (128*2,128*2),
           (6*3*2 + 10*2*2 + (127*128//2)*3*2 + 28*3*2,
            6*3*2*2 + 10*2*2 + (127*128//2)*3*2 + 28*3*2),
           (20+10+550,24+12+660),
           (20+251*3+5,(20+251*3)*6+24), (11+18*4,11+18*5)]
rows = re.findall(r'^\| (\d+) [A-Z_]+ \| .*? \| (\d+) \| (\d+) \|$', new, re.M)
check(len(rows) == 12 and all(int(i)==n and (int(e),int(o))==counts[n] for n,(i,e,o) in enumerate(rows)), 'all twelve schedule rows match independent metadata formulas')
E,O = map(sum,zip(*counts))
check((E,O,O*3) == (408034,483553,1450659), 'request/call totals and three-profile multiplier')
check(E*6 == 2448204, 'six full oracle streams explicitly budgeted')
check({(7*o+3)%32 for o in range(32)} == set(range(32)), '32 paired offsets cover each operand individually')
check(struct.calcsize('<4sBBBBQ') == 16 and struct.calcsize('<QBBBBQQQQQBBBB32s') == 88 and struct.calcsize('<QBBBBQQI') == 32 and struct.calcsize('<BQ') == 9, 'little-endian header/expected/observed/sink layout sizes')
normal = 16 + 88*E + 3*(16+32*O)
guards = 16+88 + 6*(16+32)
check(normal == 82328144 and guards == 392 and normal+guards < 84*2**20, 'normal and guard records fit numeric evidence allocation')
check(84+4+8+2+2 == 100, 'output allocations plus headroom preserve 100 MiB ceiling')
print('normal_binary_bytes=' + str(normal))
check(all(('R1-0'+str(i)) in new for i in range(1,7)), 'six decision-ledger resolutions present')
check('NATIVE (mandatory correctness adapter)' in new and 'NATIVE (optional correctness adapter)' not in new and 'NATIVE is mandatory for the entire correctness matrix' in new, 'mandatory NATIVE scope is consistent')
check('<=512 MiB aggregate absolute resident memory' in new and '<=1,800 seconds' in new and 'three runtime processes' in proposal['resource_requirements'][0], 'proposal/protocol agree on process and aggregate campaign budget')

for path in DIR.glob('*.md'):
    prose = re.sub(r'```.*?```', '', path.read_text(), flags=re.S)
    for label, target in re.findall(r'\[([^\]]*)\]\(([^)]+)\)', prose):
        if not re.match(r'[a-zA-Z]+:', target) and not target.startswith('#'):
            check((path.parent / target.split('#')[0]).exists(), 'local link ' + target)
pinned = re.findall(r'https://github.com/t81dev/t81-foundation/blob/([0-9a-f]{40})/([^\s)#]+)#L(\d+)', new)
for revision, path, line in pinned:
    data = git('show',revision+':'+path)
    check(1 <= int(line) <= len(data.splitlines()), 'pinned source anchor ' + path + ':' + line)
check(len(pinned) == 6, 'six original pinned source anchors retained')
changed = set(git('diff','--name-only',BASE).decode().splitlines())
check(changed <= allowed, 'changed-file allowlist contains only existing ARCH-002 documentation')
subprocess.run(['git','diff','--check'],check=True)
print('PASS: whitespace check; no C++ build, corpus generation, oracle, kernel, sanitizer or statistics executed')
```

### R1 discovery receipts

Commands use the same temporary PYTHONPATH and
`python3 -B tools/research_commons/commons.py --request '<request JSON>'`.
Each request has protocol t81.research-commons.v1, the operation below, and the
literal arguments shown. These are offline read-only discovery operations;
returned reproduction instructions were not executed.

| Operation | Arguments | Exit | SHA-256 of exact stdout |
|---|---|---:|---|
| list_research_tracks | {} | 0 | e71052118759ee614d09f5212a79f9a54e359eb8d06752bf70ea251e7c69d13c |
| list_hypotheses | {} | 0 | c93c6abcdf2e2730724133864e5bdd64d0ed19235f519b734da8923cb50b7f8e |
| get_hypothesis | {"hypothesis_id":"H03"} | 0 | d0206aab2c9819bfdd39efb96c738351666059d2c4b800a0512ab19ddde4e0d5 |
| get_hypothesis | {"hypothesis_id":"H05"} | 0 | 1c85aa6a81d283183a43798cf929e6c608acc1ed9fc0294cab03aabdfc7403c0 |
| get_evidence | {"evidence_id":"register"} | 0 | c5bc4d0fc0744d0f6a6bedf40c2c2ae3421142bf066c90649047fc7705c6116f |
| get_reproduction_instructions | {"hypothesis_id":"H03"} | 0 | 51edbba586c90820c0b689641a05ae9b8bb58292b12d94759ae64db22156e095 |
| list_open_research_questions | {} | 0 | 4ba2a1d2e9d6a570cdbd30b7dc66452b5682219bddd763e0c618ad51c9ac5a26 |

R1 get_evidence explicitly requests register; the old receipt above did not
specify that evidence argument. Their differing response hashes are not a
claim that the underlying canonical register changed.

### Current content identities and limits

| Current file | SHA-256 of exact bytes |
|---|---|
| PROTOCOL.md | `46f4dc125efa572d0402dad93fbe66f3a08f76fb57e21885232a58408b610752` |
| proposal.json | `8ece77930dbcf4e19495a3505aab2b1746a5ab682ed65c917127c2c741044014` |

ASan/UBSan flag and coverage descriptions were checked against the primary
[Clang ASan documentation](https://clang.llvm.org/docs/AddressSanitizer.html)
and [Clang UBSan documentation](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html).
This establishes documented configuration intent, not availability or successful
instrumentation on this machine. The revised protocol explicitly excludes
LSan/TSan/MSan, unqualified UAR/scope submodes and a universal safety claim.

No kernels, oracle implementations, symbol-vector corpus, correctness campaign,
sanitizer/compiler probe, CMake build, CTest, simulation, statistical calculation,
pilot or benchmark was executed. No production code, ISA, Axion, governance,
CPA artifact or historical H03/H05 finding changed. Publication uses [skip ci]
to avoid requesting automatic performance execution; no remote CI pass is
claimed. No merge or independent acceptance is claimed.

Remaining gates are independent acceptance of the protocol and later explicit
commit-scoped correctness-only implementation/execution authorization. Host,
toolchain, memory/time feasibility and future harness validity remain empirical
qualification questions. No unresolved experimental-rule placeholder is left by
the six R1 decisions; those future facts cannot be converted into documentary
passes or execution permission by this report.
