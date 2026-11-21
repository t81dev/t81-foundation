# Python SDK Quickstart: Deterministic Decision Table & Persistent CanonFS

This quickstart walks through the first end-to-end Python SDK workflow for T81:

1. Install a `t81` Python wheel in an isolated virtual environment.
2. Compile and run a small deterministic T81Lang ternary decision-table program (`-1` deny, `0` review, `+1` approve).
3. Persist the canonical result bytes in CanonFS and save the 64-character hex digest.
4. Retrieve the exact bytes in a **fresh Python process** using only the store path and the saved digest.
5. Modify the decision-table threshold, rerun, and compare both stored results by digest.

> **Scope boundary:** This example demonstrates **deterministic execution** and **content-addressed storage**. **Axion policy enforcement is outside this example.** The native `t81` CLI and MCP server remain separate native builds.

## 1. Install the wheel

```bash
python3.12 -m venv .venv-t81
source .venv-t81/bin/activate
pip install path/to/t81-*.whl
```

## 2. Run the decision table and persist the result (Process 1)

Run [`examples/python/canonfs_decision_workflow.py`](../../../examples/python/canonfs_decision_workflow.py) to execute the T81Lang decision table (`score=75`, `approve_threshold=70` -> `+1` approve) and write the canonical JSON bytes to a persistent CanonFS directory:

```bash
python examples/python/canonfs_decision_workflow.py run \
  --store ./canonfs-demo-store \
  --score 75 \
  --risk-flag 0 \
  --approve-threshold 70
```

Example output:

```json
{"decision": 1, "digest": "<64-char-hex-digest-a>", "payload": "{\"approve_threshold\":70,\"decision\":1,\"label\":\"approve\",\"review_threshold\":40,\"risk_flag\":0,\"score\":75}"}
```

Save the returned `"digest"` value (`DIGEST_A`). Running the exact same command again produces the **exact same digest** because both execution and content addressing are deterministic.

## 3. Retrieve the exact bytes in a fresh Python process (Process 2)

In a separate Python process with no shared in-memory state, reconstruct `t81.CanonRef` from the saved digest and read the exact bytes from persistent CanonFS:

```bash
python examples/python/canonfs_decision_workflow.py read \
  --store ./canonfs-demo-store \
  --digest "$DIGEST_A"
```

Or directly in Python:

```python
import t81

driver = t81.make_persistent_driver("./canonfs-demo-store")
ref = t81.CanonRef.from_hex(digest_a)  # or t81.CanonRef.from_bytes(raw_32_bytes)
exact_bytes = driver.read_object_bytes(ref)
print(exact_bytes.decode("utf-8"))
```

## 4. Modify the program, rerun, and compare results

Tighten the approval rule in the T81Lang decision table by raising `--approve-threshold` from `70` to `80` (with the same input `score=75`, the decision changes from `+1` approve to `0` review):

```bash
python examples/python/canonfs_decision_workflow.py run \
  --store ./canonfs-demo-store \
  --score 75 \
  --risk-flag 0 \
  --approve-threshold 80
```

Save the new `"digest"` as `DIGEST_B`, then compare both persisted artifacts in a fresh process:

```bash
python examples/python/canonfs_decision_workflow.py compare \
  --store ./canonfs-demo-store \
  --digest-a "$DIGEST_A" \
  --digest-b "$DIGEST_B"
```

Because CanonFS is content-addressed, both `DIGEST_A` (`decision: 1`) and `DIGEST_B` (`decision: 0`) remain independently retrievable by their digests without overwriting each other.

## 5. Error semantics

- **Malformed digests:** `t81.CanonRef.from_hex(...)` requires exactly 64 ASCII hex characters (no whitespace, `0x`, or `h1-` prefix) and `t81.CanonRef.from_bytes(...)` requires exactly 32 bytes. Invalid inputs raise `ValueError` (or `TypeError` for wrong types).
- **Absent digests:** Looking up a syntactically valid 32-byte digest that is not present in the store via `driver.read_object_bytes(ref)` raises `RuntimeError` reporting `NotFound`.
