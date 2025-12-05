# T81 CLI User Manual

This manual walks through every major `t81` command, flags, and workflow so developers, auditors, and Axion stewards can use the CLI as the canonical interface to the compiler, runtime, and policy tooling.

## 1. Getting started

1. **Build the toolchain**: After syncing the repo, ensure you have a release build:

   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ```

2. **Run the CLI driver** (`t81`):

   ```bash
   ./build/t81 --help
   ```

   The `t81` binary dispatches subcommands like `compile`, `repl`, `run`, and others listed below. Always run from the repository root so relative paths (scripts, artifacts, policy files) resolve as explained in the docs.

## 2. Compilation workflow

### 2.1 `t81 compile`

Primary compile command; emits `.tisc` bytecode plus Axion metadata.

```
t81 compile [options] <source>.t81 -o <output>.tisc
```

Key options:

- `-P policy/<name>.axion`: embed an Axion policy (see `policy/guards.axion`).
- `--verbose`: prints loop/match metadata, guard events, and Axion trace strings (useful for auditors).
- `--trace-match`: records match metadata even for structural matches that normally skip logging.
- `--include <dir>`: add include paths for `import` statements.

The CLI generates logs that mirror `docs/guides/axion-trace.md` samples:

```
verdict.reason="enum=Option variant=Some match=pass guard=pass payload=i32"
verdict.reason="stack frame allocated stack addr=243 size=16"
```

These strings feed Axion policies (RFC-0009) and must match the canonical text used by regressions like `axion_policy_match_guard_test`.

### 2.2 `t81 run`

Execute an existing `.tisc` file:

```
t81 run <program>.tisc
```

- Picks up the Axion policy embedded in the program.
- Dumps per-opcode `AxionEvent` entries when `AXTRACE` instrumentation is enabled.
- Use `AXTRACE=full` environment variable for verbose guard/segment logging (matching `scripts/capture-axion-trace.sh` output).

## 3. REPL & tracing

Launch the interactive prompt:

```
t81 repl [--policy policy/axion-pkg.axion]
```

At the REPL, after executing guard-heavy code, type `:trace` to dump the current `State::axion_log`. This command prints the same `verdict.reason` strings seen during compilation and regression runs, proving the runtime and Axion trace infrastructure agree.

Use `:rules` to inspect the active policy, `:load` to load `.tisc`, and `:trace chart` to render segment timing in ASCII.

## 4. Policy tooling integration

The CLI exposes two helper targets:

- `axion_policy_trace` (built via `cmake --build build --target axion_policy_trace`): runs a program that touches stack, heap, tensor, and meta segments while emitting Axion events. Look for lines such as `opcode=22 reason="AxRead guard segment=stack addr=5"`.
- `axion_policy_runner` (see `examples/axion_policy_runner.cpp`): enforces `(require-match-guard ...)` and `(require-segment-event ...)` clauses while printing `build/artifacts/axion_policy_runner.log`. Use `scripts/capture-axion-trace.sh` to rerun it and capture the deterministic artifact mentioned in section 6 of `docs/guides/axion-trace.md`.

When compiling with `-P policy/guards.axion`, the CLI output matches RFC-0009/0013/0019 expectations. Pair this with `--verbose` to confirm the guard strings before Axion policy enforcement occurs.

## 5. Debugging & automation

- `t81 compile --trace-guards` enables the guard instrumentation used by `t81_cli_trace_test`.  
- `t81 compile` pipelines used in CI should redirect their output into `build/logs/cli-<timestamp>.log` so release artifacts can include `t81 compile --verbose` transcripts.
- If you see “policy trap” errors, run `t81 repl` with `:trace` to inspect which `verdict.reason` string is missing (the CLI shows the exact match string enforced by `policy_engine`).

## 6. Advanced switches

- `--axion-policy`/`--axion-policy-text` (deprecated alias) allow you to embed policy s-expressions directly without a file. Prefer the `.axion` file for reproducibility.
- `--profile` writes instruction counts and guard coverage for every Axion event; feed the resulting JSON into `tools/axion-profiler` to visualize guard hot spots.
- `--emit-axion-log`: emits `State::axion_log` into `build/artifacts/axion-<run>.log` automatically, matching the CLI samples in this manual.

## 7. Reference snippets

Include the following in release notes to preserve canonical log snippets:

1. `./scripts/capture-axion-trace.sh` (produces `build/artifacts/axion_policy_runner.log` with policy strings).
2. `ctest --test-dir build -R axion_segment_trace_test --output-on-failure` snippet (records `bounds fault …` and segment `verdict.reason` strings).
3. `t81 compile --verbose match_guard.t81 -P policy/guards.axion` transcript (records guard trace strings such as `enum=Option …`).

These samples ensure auditors can audit Axion traces without reading the VM code or rerunning the entire suite.

## 8. Related resources

- `docs/guides/axion-trace.md` (Axion trace reference with CLI/REPL samples).  
- `spec/axion-kernel.md` section 1.5‑1.10 (Axion responsibilities, metadata contract).  
- RFC-0009, RFC-0013, RFC-0019 (policy language, segment trace semantics, match metadata).  
- `docs/guides/cli-toolkit.md` for tooling that wraps the CLI in reproducible scripts.
