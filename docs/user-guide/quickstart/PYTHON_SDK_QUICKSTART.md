# Python SDK quickstart for a deterministic decision table

Run a small T81Lang decision table, store its canonical result bytes in
CanonFS, and recover them in a fresh Python process. This example covers
execution and storage. Axion policy enforcement is outside its scope. The
native `t81` CLI and MCP server are separate builds.

## 1. Install the PR candidate

This Apple Silicon candidate requires macOS 11 or later and CPython 3.12.
The wheel shares its filename and version with the published PyPI wheel.
Use the download below in a fresh environment to get the PR candidate.

- [PR #840 candidate wheel](https://github.com/t81dev/t81-foundation/releases/download/sdk-candidate-pr840-apple-silicon-c78126c80/t81-2.1.0-cp312-cp312-macosx_11_0_arm64.whl)
- [Matching Python example](https://github.com/t81dev/t81-foundation/releases/download/sdk-candidate-pr840-apple-silicon-c78126c80/canonfs_decision_workflow.py)
- Wheel source revision `c78126c80a70e6dfd5b0a624c1ebd79e9dd820d9`
- Wheel SHA-256 `b7514f019a3463a54f39ad2373f216b95191a6de1e2956cf49d58f1698dd215d`

The [candidate release record](https://github.com/t81dev/t81-foundation/releases/tag/sdk-candidate-pr840-apple-silicon-c78126c80)
records the exact PR head for the downloadable example and this quickstart.
Paste this block in macOS Terminal. It creates a clean working directory and
verifies the wheel before installing from the local file.

```bash
demo_dir=$(mktemp -d "${TMPDIR:-/tmp}/t81-pr840.XXXXXX") &&
cd "$demo_dir" &&
base=https://github.com/t81dev/t81-foundation/releases/download/sdk-candidate-pr840-apple-silicon-c78126c80 &&
wheel=t81-2.1.0-cp312-cp312-macosx_11_0_arm64.whl &&
curl -fLO "$base/$wheel" &&
curl -fLO "$base/canonfs_decision_workflow.py" &&
printf '%s  %s\n' b7514f019a3463a54f39ad2373f216b95191a6de1e2956cf49d58f1698dd215d "$wheel" | shasum -a 256 -c - &&
python3.12 -m venv .venv-t81 &&
source .venv-t81/bin/activate &&
python -m pip install --no-index --no-deps "./$wheel"
```

## 2. Run and recover a decision

```bash
python canonfs_decision_workflow.py run \
  --store ./canonfs-demo-store \
  --score 75 --risk-flag 0 --approve-threshold 70 \
  --next-approve-threshold 80
```

The first run returns `1` (`approve`). Its JSON output contains the payload and
64-character Python hex digest. The `Read:` line contains a complete command with that
digest. Copy the command after `Read:` and run it. It starts a fresh Python
process and reopens the store from its path. The CLI adds a newline for display.
`CanonDriver.read_object_bytes()` returns the original bytes.

### Verify and retrieve it with the native CLI

The same run emits `cli_handle` and prints `Native verify:` and `Native get:`
commands. `CanonRef.to_cli_handle()` returns the native `sha3-256:<Base-81 hash>`
form. Use `.hex()` and `CanonRef.from_hex()` for Python digest round trips.

With a separately built native `t81` on `PATH`, copy the commands after
`Native verify:` and `Native get:`. If your binary is elsewhere, replace the
leading `t81` with its path. Both commands already contain the quoted handle and
the same absolute `--canonfs-root` used by Python. `verify` checks the stored
object. `get` sends its original bytes directly to stdout. Use the printed
`get` command as-is for raw bytes. This path requires neither a staging file nor
`put-file`.

## 3. Change the approval threshold and compare

Copy the command after `Rerun:`. It preserves the inputs and raises the approval
threshold to `80`, so score `75` now returns `0` (`review`). It carries the original
digest in `--compare-to`. Copy the new `Read:` and `Compare:` commands to recover
the revised result and verify both records by digest in fresh processes. The
comparison should report `same_digest: false` and `same_bytes: false`.

## 4. Add a rule and choose its precedence

Open the downloaded `canonfs_decision_workflow.py` and find `fn decide` inside
`build_decision_program`. The rules are evaluated from top to bottom. The first
matching rule returns immediately. A nonzero risk flag comes first and forces
`-1` (`deny`). Approval precedes review, so a score at or above both thresholds
is approved when the risk flag is zero.

To force a manual review for score `75`, add this block after the risk check and
before the approval check. Keep the doubled braces because the T81Lang source
is inside a Python f-string.

```text
    if (score == 75) {{
        return 0;
    }}
```

Run the command in step 2 again. Score `75` now qualifies for both the new rule
and approval at threshold `70`. The new rule wins because it comes first. If
placed after the approval check, approval wins. Use the new `Read:` command to
verify the stored result. The digest identifies the result bytes, so a rule
change that leaves the payload identical can produce the same digest.

## Error semantics

`CanonRef.from_hex()` requires 64 ASCII hex characters without whitespace or
prefixes. `CanonRef.from_bytes()` requires 32 bytes. Invalid lengths or content
raise `ValueError`, and incorrect types raise `TypeError`. A valid digest missing
from the store raises `RuntimeError` with `NotFound`.
