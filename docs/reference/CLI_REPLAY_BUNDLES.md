# CLI Replay Bundles

This guide defines the current replay-bundle ritual for core governed `t81`
CLI workflows.

## Goal

The replay bundle is an evidence artifact, not a blanket claim that every CLI
surface is bit-identical across all hosts.

The current supported bundle covers one stable vertical slice:

- `t81 code build`
- `t81 code run --trace`
- `t81 determinism verify-run`
- `t81 trace replay`
- `t81 canonfs put-file`
- `t81 canonfs verify`
- `t81 canonfs get`
- `t81 ai model inspect`
- `t81 ai verify determinism`
- `t81 ai inference run --mode strict_deterministic`

The bundle collector repeats that workflow with fixed artifact paths and
compares:

- command return codes
- stdout/stderr content hashes
- output artifact hashes
- CanonFS round-trip source equality

## Collector

Primary tool:

- `scripts/ci/collect_cli_replay_bundle.py`

Schema:

- `t81.cli.replay-bundle.v1`

Primary outputs:

- `cli_replay_bundle.json`
- `cli_replay_bundle.sha256`
- `cli_replay_bundle.md`

## Canonical Local Invocation

```bash
python3 scripts/ci/collect_cli_replay_bundle.py \
  --t81-bin build/t81 \
  --out-dir build/cli-replay \
  --runs 2 \
  --program examples/hello_world.t81 \
  --model-fixture tests/fixtures/llama_cpp_repro/model.gguf
```

## Bundle Rules

- Repeated runs reuse the same artifact paths so command output remains
  comparable.
- Duration fields are recorded for diagnostics but are not part of the replay
  equality projection.
- Host metadata and generation timestamp are recorded but are not part of the
  canonical bundle hash.
- The AI lane is only included through `strict_deterministic` mode.
- The bundle currently proves one governed workflow slice, not the whole CLI.

## Interpreting Results

Pass criteria:

- all commands return expected exit codes
- replayable stdout/stderr and output artifacts match across runs
- CanonFS retrieved bytes match the source fixture

Fail criteria:

- any command returns an unexpected exit code
- any replayable output hash changes across repeated runs
- CanonFS round-trip bytes differ from the source fixture

## Relationship to Other Evidence

- [REPRODUCIBILITY.md](/docs/reference/REPRODUCIBILITY.md)
  remains the authoritative T81Lang fixture reproducibility guide.
- AI-specific multi-lane evidence remains under the AI evidence collectors in
  `scripts/ci/`.
- The replay bundle is the current Phase 2 evidence format for core governed
  CLI workflows.
