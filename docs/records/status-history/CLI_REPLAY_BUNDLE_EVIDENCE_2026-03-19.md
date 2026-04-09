# CLI Replay Bundle Evidence

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [CLI Replay Bundle Evidence](#cli-replay-bundle-evidence)
  - [Goal](#goal)
  - [Command](#command)
  - [Result](#result)
  - [Covered Workflow Slice](#covered-workflow-slice)
  - [Observed Stable Artifacts](#observed-stable-artifacts)
  - [Observed Stable JSON Output Hashes](#observed-stable-json-output-hashes)
  - [Artifact Paths](#artifact-paths)
  - [Notes](#notes)

<!-- T81-TOC:END -->


Date: 2026-03-19  
Status: Pass  
Schema: `t81.cli.replay-bundle.v1`

## Goal

Record the first Phase 2 replay-bundle evidence artifact for a core governed
`t81` CLI workflow slice.

## Command

```bash
python3 scripts/ci/collect_cli_replay_bundle.py \
  --t81-bin build/t81 \
  --out-dir build/cli-replay \
  --runs 2 \
  --program examples/hello_world.t81 \
  --model-fixture tests/fixtures/llama_cpp_repro/model.gguf
```

## Result

- status: `pass`
- deterministic_outputs: `true`
- expected_rc_match: `true`
- canonfs_roundtrip_matches_source: `true`
- bundle_sha256: `12144ffd40a1402fa78325797b65a65e710ff3e07b50a39f4ada14f07178f37f`

## Covered Workflow Slice

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

## Observed Stable Artifacts

- built TISC artifact SHA-256: `16973cbc5d4020e1a8dca86095b4124533e7ab4713b13faa2994ca17ad18fc88`
- trace artifact SHA-256: `86e2370f52bb49788c2d4be012df90938eddafd816767c1d14e7da79c16b161b`
- strict AI inference artifact SHA-256: `b208f90b367d444de973f56c1695bb5b3438fcbc59edd60ee5119e0488415ac7`
- CanonFS object identity: `sha3-256:X81PUA∞YfDZefz≤≤Jaw≈eWoa1≤02≈W6SbλKIX≈=≤`

## Observed Stable JSON Output Hashes

- `t81 determinism verify-run --json`: `de9f92b88247ca3e16d7aa75880915ccc2117c7792d6c6afa8373ae4a9fa8948`
- `t81 trace replay --json`: `590ef53edd75467b1c23dfa25bee30cd468998d1a67717ab04f17d1bd0adb77f`
- `t81 canonfs verify --json`: `60f03c52d9bac3161a308390f8bbf78c14aa4844b75b6a8ff454a4cdee3e3fb6`

## Artifact Paths

- bundle JSON: `build/cli-replay/cli_replay_bundle.json`
- bundle hash: `build/cli-replay/cli_replay_bundle.sha256`
- bundle summary: `build/cli-replay/cli_replay_bundle.md`

## Notes

- The collector reuses fixed artifact paths between repeated runs so command
  output remains directly comparable.
- Duration and host metadata are recorded for diagnostics but excluded from the
  canonical replay-equality projection.
- The AI lane is included only through `strict_deterministic` mode.
