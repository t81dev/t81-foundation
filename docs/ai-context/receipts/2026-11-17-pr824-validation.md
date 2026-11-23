# PR #824 AI-context Reconciliation — Validation Record

**NON-NORMATIVE / DOCUMENTATION ONLY / NO AUTHORIZATION**

Base: `ab15820a2dd8ebcadde0efecf1fbb0d43b57d3f9`. Collector: Codex,
reconciliation author with prior F01 authoring context, not independent reviewer.
The record below describes the pre-commit working tree by hashes. Resolve its
publication commit from Git history; it does not invent a self-referential SHA.
No runtime/harness, proof checker, probe, benchmark or A–F was executed and no
dependencies were installed. Only documentation/data validators were run.

## Outcomes and limits

- Existing stdlib validator passes in normal and optimized Python modes.
- All 14 existing continuity contract tests pass, including schema parity and
  historical validation-fixture replay. That replay concerns JSON validation,
  not F01 proof checks or experimental execution.
- Strict JSON parsing (duplicate keys/non-JSON constants rejected),
  Draft202012 schema validation with format checking, all 10 pinned Git-blob
  hashes, all 21 baseline AI-context file identities, two earlier operational
  views, one historical snapshot and authorization preservation pass.
- The new view's four scientific observations are exact historical
  carry-forwards. Their original timestamps and source pins remain unchanged.
- Reviewed and merged #824 plan bytes match. Its squash delta is exactly one
  planning document. #823 is not an ancestor of main (expected Git exit 1).
  Its open/draft status is separately evidenced by the retained GitHub metadata.
- Active-doc link hygiene and local reconciliation links pass. Whitespace
  validation passes. Existing schema and validator bytes are unchanged.
- **Baseline failure:** `check_docs_structure.py` exits 1 on both this candidate
  and a clean detached worktree at the exact base, with identical output:
  missing `book/book-en`, `book/book-cn`, `book/book-es`, `book/book-pt`,
  `book/book-ru`. No book, gate or validator repair is included.
- No repository-wide PR count, new CI conclusion, scientific result or host
  qualification is claimed. The pre-existing selector/test coupling remains:
  `selected_view` stays historical; navigation and `next_gate` direct readers to
  the new dated view. User-reported Gemini/human decision provenance is not
  independently authenticated by this validation.

## Reproducing the preservation and source checks

Run from the candidate repository root. The script reads Git blobs and JSON
only; it does not invoke harness or proof-checking logic.

```sh
python3 -B - <<'PY_CHECK'
import hashlib, json, subprocess
from pathlib import Path
base = 'ab15820a2dd8ebcadde0efecf1fbb0d43b57d3f9'
def blob(rev, path):
    return subprocess.check_output(['git', 'show', f'{rev}:{path}'])
state = json.loads(Path('docs/ai-context/CURRENT_STATE.json').read_text())
old = json.loads(blob(base, 'docs/ai-context/CURRENT_STATE.json'))
receipt = json.loads(Path('docs/ai-context/receipts/2026-11-17-pr824-reconciliation.json').read_text())
for key, value in old['operational_views'].items():
    assert state['operational_views'][key] == value
for key in ['historical_snapshots', 'authorization', 'selected_view']:
    assert state[key] == old[key]
assert receipt['historical_index_controls'] == {k: old[k] for k in ['generated_at', 'selected_view', 'next_gate']}
assert state['operational_views']['2026-11-17-pr824-reconciled']['scientific_observations'] == old['operational_views'][old['selected_view']]['scientific_observations']
for pin in receipt['pinned_sources']:
    assert hashlib.sha256(blob(pin['source_revision']['commit_sha'], pin['path'])).hexdigest() == pin['sha256']
for pin in receipt['baseline_ai_context_inventory']:
    path = pin['path']; before = blob(base, path); after = Path(path).read_bytes()
    assert hashlib.sha256(before).hexdigest() == pin['sha256']
    if path.endswith('CURRENT_STATE.json'):
        continue
    if path.endswith('START_HERE.md'):
        assert after.endswith(before)
    elif path.endswith('ACTIVE_RESEARCH_HANDOFF.md'):
        assert after.startswith(before)
    else:
        assert after == before
print('PASS: historical preservation, attribution controls and pinned hashes')
PY_CHECK
```

## Retained commands, outputs, environment and source identities

Working directories, exit statuses and outputs are retained below. The receipt
file hashes the baseline schema and validator; this record hashes the five
other changed files. Its own identity is supplied by the publication commit,
not a recursive self-hash. Successful data validation establishes structure and
content identity only. All operational STOP conditions remain unchanged.

```json
{
  "environment": {
    "python": "3.14.3 (main, Feb  3 2026, 15:32:20) [Clang 17.0.0 (clang-1700.6.3.2)]",
    "platform": "macOS-26.5.2-arm64-arm-64bit-Mach-O",
    "jsonschema": "4.26.0"
  },
  "checks": {
    "strict_json_parse_and_Draft202012_schema_with_formats": "PASS",
    "prior_operational_views_unchanged": 2,
    "historical_snapshots_unchanged": 1,
    "all_baseline_ai_context_files_accounted": 21,
    "source_hashes_verified": 10,
    "reviewed_and_merged_plan_bytes_identical": true,
    "pr823_not_integrated": true,
    "authorization_and_schema_unchanged": true,
    "local_reconciliation_links": "PASS",
    "summary": "All scoped data/hash/history checks passed; one baseline documentation-structure failure reproduced. Ancestor exit 1 is the expected proof that #823 is not integrated."
  },
  "commands": [
    {
      "command": [
        "/private/tmp/t81-pr795-parity-venv/bin/python",
        "-B",
        "scripts/validate_current_state.py"
      ],
      "working_directory": "/private/tmp/t81-ai-context-pr824",
      "exit": 0,
      "stdout": "CURRENT_STATE: structure valid; provenance and freshness require live verification\n",
      "stderr": ""
    },
    {
      "command": [
        "/private/tmp/t81-pr795-parity-venv/bin/python",
        "-B",
        "-O",
        "scripts/validate_current_state.py"
      ],
      "working_directory": "/private/tmp/t81-ai-context-pr824",
      "exit": 0,
      "stdout": "CURRENT_STATE: structure valid; provenance and freshness require live verification\n",
      "stderr": ""
    },
    {
      "command": [
        "/private/tmp/t81-pr795-parity-venv/bin/python",
        "-B",
        "tests/ci/test_current_state_contract.py"
      ],
      "working_directory": "/private/tmp/t81-ai-context-pr824",
      "exit": 0,
      "stdout": "",
      "stderr": "..............\n----------------------------------------------------------------------\nRan 14 tests in 17.165s\n\nOK\n"
    },
    {
      "command": [
        "/private/tmp/t81-pr795-parity-venv/bin/python",
        "-B",
        "scripts/governance/check_active_docs_link_hygiene.py"
      ],
      "working_directory": "/private/tmp/t81-ai-context-pr824",
      "exit": 0,
      "stdout": "active docs link hygiene check PASSED\n- no legacy docs/guides path references found on the active docs surface\n",
      "stderr": ""
    },
    {
      "command": [
        "/private/tmp/t81-pr795-parity-venv/bin/python",
        "-B",
        "scripts/governance/check_docs_structure.py"
      ],
      "working_directory": "/private/tmp/t81-ai-context-pr824",
      "exit": 1,
      "stdout": "docs structure check FAILED\n- missing directory: book/book-en\n- missing directory: book/book-cn\n- missing directory: book/book-es\n- missing directory: book/book-pt\n- missing directory: book/book-ru\n",
      "stderr": ""
    },
    {
      "command": [
        "git",
        "diff",
        "--check"
      ],
      "working_directory": "/private/tmp/t81-ai-context-pr824",
      "exit": 0,
      "stdout": "",
      "stderr": ""
    },
    {
      "command": [
        "git",
        "merge-base",
        "--is-ancestor",
        "52604ee1d4b7d90f69b19651a942f68dd5653183",
        "ab15820a2dd8ebcadde0efecf1fbb0d43b57d3f9"
      ],
      "working_directory": "/private/tmp/t81-ai-context-pr824",
      "exit": 1,
      "stdout": "",
      "stderr": ""
    }
  ],
  "baseline_comparison": {
    "commit": "ab15820a2dd8ebcadde0efecf1fbb0d43b57d3f9",
    "working_directory": "/private/tmp/t81-ai-context-pr824-baseline",
    "command": [
      "/private/tmp/t81-pr795-parity-venv/bin/python",
      "-B",
      "scripts/governance/check_docs_structure.py"
    ],
    "exit": 1,
    "stdout": "docs structure check FAILED\n- missing directory: book/book-en\n- missing directory: book/book-cn\n- missing directory: book/book-es\n- missing directory: book/book-pt\n- missing directory: book/book-ru\n",
    "stderr": "",
    "classification": "Identical pre-existing failure; no book or validator changes."
  },
  "working_tree_source_hashes": {
    "docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md": "0025e17fb782245f80c02cbf646d9598a844aaddcb86d8a753da179cd7e172fa",
    "docs/ai-context/CURRENT_STATE.json": "cb552d0e984c37d9bff7ac3dc1dc29d0e2427d7d72a982a7c475bdf92fc9c371",
    "docs/ai-context/START_HERE.md": "3c4a0e489befce70d0ea867bb4cceb149f2b48c2d4bd625d333c22a8cc9df235",
    "docs/ai-context/T81_RESEARCH_STATUS_2026-11-17_PR824.md": "b9118f0970cde5fc21c445ae7eb6b2331c36dca5c53ab20bc91c06b5787c68aa",
    "docs/ai-context/receipts/2026-11-17-pr824-reconciliation.json": "5baaf0a5e4df6973de44b6efaff658090cdef745a4c1fd782580f939113ca2d2"
  }
}
```
