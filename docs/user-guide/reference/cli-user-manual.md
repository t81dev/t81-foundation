# T81 CLI User Manual

This is the operator manual for the `t81` CLI.
Everything in this document is intended to match the current shipped binary behavior.

**Last Updated:** March 8, 2026

## 1. Quick Start

Build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Show top-level help:

```bash
./build/t81 --help # docs-smoke
```

Minimal compile/run flow:

```bash
./build/t81 code check examples/hello_world.t81
./build/t81 code build examples/hello_world.t81 -o hello.tisc
./build/t81 code run hello.tisc
```

## 2. Global Options

These options are accepted before or after the command token:

- `-h`, `--help`
- `-V`, `--version`
- `-q`, `--quiet`
- `-v`, `--verbose`

Examples:

```bash
./build/t81 -q version # docs-smoke
./build/t81 version -q # docs-smoke
./build/t81 help code # docs-smoke
./build/t81 help project # docs-smoke
./build/t81 help env # docs-smoke
./build/t81 help internal # docs-smoke
./build/t81 help completion # docs-smoke
./build/t81 help man # docs-smoke
./build/t81 help feedback # docs-smoke
./build/t81 compile --help # docs-smoke
./build/t81 help compile # docs-smoke
./build/t81 code test --json --list # docs-smoke
./build/t81 env doctor --json # docs-smoke
./build/t81 completion bash # docs-smoke
./build/t81 man # docs-smoke
./build/t81 help advanced # docs-smoke
./build/t81 help labs # docs-smoke
```

## 3. Command Tiers

`t81 --help` shows the core workflow commands.
Domain-first command families:

- `t81 code <action> [args]`
- `t81 project <action> [args]`
- `t81 env <action> [args]`
- `t81 internal <action> [args]`
- `t81 canonfs <action> [args]`
- `t81 determinism <action> [args]`
- `t81 vm <action> [args]`
- `t81 tisc <action> [args]`
- `t81 ir <action> [args]`
- `t81 weights <action> [args]`
- `t81 policy <action> [args]`
- `t81 axion <action> [args]`
- `t81 trace <action> [args]`

Additional curated help topics are available via:

- `t81 help advanced`
- `t81 help labs`

Current Labs commands include `pkg`, `benchmark`, `repro-hash`,
`canonize-tensor`, `canonize-file`, `memory-stats`, and `llama-run`.

## 4. Commands

### 4.1 Domain-First Groups

```text
t81 code <action> [args]
t81 project <action> [args]
t81 env <action> [args]
t81 internal <action> [args]
t81 canonfs <action> [args]
t81 determinism <action> [args]
t81 vm <action> [args]
t81 tisc <action> [args]
t81 ir <action> [args]
t81 weights <action> [args]
t81 policy <action> [args]
t81 axion <action> [args]
t81 trace <action> [args]
t81 completion <bash|zsh|fish>
t81 man [--install-dir <dir>]
t81 feedback <submit|report> [options]
```

Primary workflow actions:

```text
t81 lang check <file.t81>
t81 lang build <file.t81|file.t81w> [-o <file.tisc>] [--weights-model <model.t81w>]
t81 lang export <file.t81> [--json] [-o <file>]
t81 code check <file.t81>
t81 code fmt [options] <file...>
t81 code build <file.t81|file.t81w> [-o <file.tisc>] [--weights-model <model.t81w>]
t81 code run <file.t81|file.tisc> [--policy <policy.apl>] [--trace] [--trace-out <file>] [--weights-model <model.t81w>]
t81 code test [options] [-- <ctest args...>]
t81 code disasm <file.tisc>
t81 code debug <file.t81|file.tisc> [--policy <policy.apl>] [--weights-model <model.t81w>]
t81 code repl [--weights-model <model.t81w>] [--policy <policy.apl>]
t81 canonfs put-file <file> [--canonfs-root <path>]
t81 canonfs put-tensor <file> [--canonfs-root <path>]
t81 canonfs ls [--json] [--canonfs-root <path>]
t81 canonfs get <sha3-256:hash> [-o <file>] [--json] [--canonfs-root <path>]
t81 canonfs stat <sha3-256:hash> [--json] [--canonfs-root <path>]
t81 canonfs verify <sha3-256:hash> [--json] [--canonfs-root <path>]
t81 canonfs snapshot [--json] [--canonfs-root <path>]
t81 canonfs snapshot-diff <lhs> <rhs> [--json] [--canonfs-root <path>]
t81 canonfs rollback --to <hash> [--json] [--canonfs-root <path>]
t81 canonfs gc [--json] [--canonfs-root <path>]
t81 determinism verify [fixtures_dir]
t81 determinism verify-run <file.tisc> [--policy <policy.apl>] [--json]
t81 determinism certify <file.tisc> [--policy <policy.apl>] [--json]
t81 determinism explain <file.tisc> [--policy <policy.apl>] [--json]
t81 determinism hash <file> [--json]
t81 determinism trace-hash <trace.txt> [--json]
t81 determinism diff <lhs> <rhs> [--json]
t81 determinism diff-trace <lhs> <rhs> [--json]
t81 determinism baseline <dir> [--source-dir <src>] [--json]
t81 vm run <file.tisc> [--policy <policy.apl>] [-o <trace.txt>]
t81 vm debug <file.tisc> [--policy <policy.apl>]
t81 vm trace <file.tisc> [--policy <policy.apl>] [-o <trace.txt>]
t81 vm step <file.tisc> [--policy <policy.apl>] [--count <n>] [--json]
t81 vm regs <file.tisc> [--policy <policy.apl>] [--steps <n>] [--json]
t81 vm stack <file.tisc> [--policy <policy.apl>] [--steps <n>] [--limit <n>] [--json]
t81 vm mem <file.tisc> --addr <n> [--policy <policy.apl>] [--steps <n>] [--json]
t81 vm state <file.tisc> [--policy <policy.apl>] [--json]
t81 vm profile <file.tisc> [--policy <policy.apl>] [--json]
t81 vm explain-trap <file.tisc> [--policy <policy.apl>] [--json]
t81 tisc disasm <file.tisc>
t81 tisc validate <file.tisc> [--json]
t81 tisc encode <file.base81> [-o <out.tisc>] [--json]
t81 tisc decode <file.tisc> [-o <out.base81>] [--json]
t81 tisc diff <a.tisc> <b.tisc> [--json]
t81 ir show <file.t81>
t81 ir dump <file.t81>
t81 ir export <file.t81> [--json] [-o <file>]
t81 tensor canonize <file>
t81 tensor hash <file> [--json]
t81 tensor inspect <model.t81w> [--json]
t81 weights import <file> [-o <out>] [--format <fmt>]
t81 weights info <model.t81w> [--json]
t81 weights verify <model.t81w> [--json]
t81 weights quantize <input> --to-gguf <out>
t81 model import <file> [-o <out>] [--format <fmt>]
t81 model info <model.t81w> [--json]
t81 model verify <model.t81w> [--json]
t81 model quantize <input> --to-gguf <out>
t81 policy compile <file.apl> [-o <out>]
t81 policy run <file.apl|file.axionb> [--json]
t81 policy test <file.apl|file.axionb> --model-hash <hash> [--json]
t81 policy list [--json] [--dir <path>]
t81 axion status [--json]
t81 axion optimize [--tier N] [--json]
t81 axion simulate <file.tisc> [--json]
t81 axion explain <file.apl|file.axionb> [--json]
t81 axion snapshot [--json]
t81 axion snapshot-diff <lhs> <rhs> [--json]
t81 axion rollback --to <hash> [--json]
t81 axion log [--json] [--tail <n>]
t81 trace show <trace.txt> [--no-color]
t81 trace diff <trace1.txt> <trace2.txt> [--no-color]
t81 trace replay <file.tisc> <trace.txt> [--json]
t81 trace summary <trace.txt> [--json]
t81 trace stats <trace.txt> [--json]
t81 trace canonicalize <trace.txt> [-o <file>]
t81 trace export <trace.txt> [--format <json|csv>] [-o <file>]
t81 project init <project_name>
t81 project build [file.t81]
t81 project run [file.t81] [--policy <p>]
t81 project test [options]
t81 repl
t81 env check [--json]
t81 env doctor [--json]
t81 env paths [--json]
t81 env diag [--json]
t81 env toolchain [--json]
t81 env feedback <submit|report> [options]
t81 internal pkg <subcommand> [args]
t81 internal benchmark [benchmark_runner_flags...]
t81 internal repro-hash [fixtures_dir]
t81 internal canonize-tensor <file>
t81 internal canonize-file <file> [--canonfs-root <path>]
t81 internal memory-stats [file]
t81 internal llama-run <model.gguf|sha3-256:hash> <prompt> --policy <policy.apl> [options]
t81 bench [benchmark_runner_flags...]
t81 completion bash|zsh|fish
t81 man [--install-dir <dir>]
t81 feedback submit --rating <1-5> [--note <text>] [--path <file>]
t81 feedback report [--path <file>]
```

### 4.2 Legacy Top-Level Aliases

Legacy aliases remain supported for compatibility, but they are intentionally hidden from
the default `t81 --help` and shell completion surfaces.

### 4.1 `compile`

```text
t81 compile <file.t81|file.t81w> [-o <file.tisc>] [--weights-model <model.t81w>]
```

Compiles source into TISC bytecode.

### 4.2 `run`

```text
t81 run <file.t81|file.tisc> [--policy <policy.apl>] [--trace] [--trace-out <file>] [--weights-model <model.t81w>]
```

Compiles if needed and executes via VM.
`--trace-out` writes replay-compatible trace lines to a file without mixing them into program stdout.

### 4.3 `disasm`

```text
t81 disasm <file.tisc>
```

Prints human-readable disassembly.

### 4.4 `debug`

```text
t81 debug <file.t81|file.tisc> [--policy <policy.apl>] [--weights-model <model.t81w>]
```

Compiles if needed and starts debugger.

### 4.5 `check` / `lint`

```text
t81 check <file.t81>
t81 lint <file.t81>
```

Syntax + semantic validation without bytecode emission.

### 4.6 `repl`

```text
t81 repl [--weights-model <model.t81w>] [--policy <policy.apl>]
```

Starts the interactive REPL.

### 4.7 `repro-hash`

```text
t81 repro-hash [fixtures_dir]
```

Runs the T81Lang reproducibility fixture hash gate.

### 4.8 `canonize-tensor`

```text
t81 canonize-tensor <file>
```

Canonicalizes tensor input into CanonFS object storage.

### 4.9 `canonize-file`

```text
t81 canonize-file <file> [--canonfs-root <path>]
```

Writes raw file bytes to CanonFS and prints `sha3-256:<hash>`.

### 4.10 `init` / `project`

```text
t81 init <project_name>
t81 project init <project_name>
t81 project build [file.t81]
t81 project run [file.t81] [--policy <policy.apl>]
t81 project test [options]
```

`init` (and `project init`) creates a project directory with `main.t81` and `README.md`.
`project build` compiles the project's T81 source to TISC bytecode via `code build`.
`project run` compiles (if needed) and executes via `code run`; accepts `--policy` to attach
an Axion policy file.
`project test` delegates to `code test` and runs the project test suite through CTest.

### 4.10a `memory-stats`

```text
t81 memory-stats [file]
```

Shows memory pool configuration. If a file is supplied, the current implementation reports
runtime memory footprint, allocation counters, and peak pool usage after execution.

### 4.11 `test`

```text
t81 test [options] [-- <ctest args...>]
t81 test --build-dir <path>
t81 test --filter <regex>
t81 test --json
t81 test --list
```

Runs project tests through CTest.
`--json` uses schema `t81.test.v1` and includes a summary block.

### 4.12 `doctor`

```text
t81 doctor [--json]
```

Runs environment/toolchain readiness checks and prints actionable fixes.
`--json` uses schema `t81.doctor.v1`.

### 4.12a `env check` / `env paths` / `env diag` / `env toolchain`

```text
t81 env check [--json]
t81 env paths [--json]
t81 env diag [--json]
t81 env toolchain [--json]
```

`env check` is a quick pass/fail readiness probe: exits 0 if the build environment is ready,
1 if not. Minimal output by default; `--json` uses schema `t81.env-check.v1`.
`env paths` reports important working directories. `env diag` aggregates repo/build discovery,
toolchain readiness, CanonFS readiness, and Axion state with schema `t81.env-diag.v1`.
`env toolchain` probes common developer tools with schema `t81.env-toolchain.v1`.

### 4.13 `fmt`

```text
t81 fmt [options] <file...>
t81 fmt --check <file...>
t81 fmt --json <file...>
t81 fmt --version
```

Normalizes whitespace and applies parser-aware `.t81` indentation formatting.
For `.t81`, formatting runs syntax validation before and after rewrite and enforces idempotence.
`--json` uses schema `t81.fmt.v1`.

### 4.14 `pkg` (labs)

```text
t81 pkg <subcommand> [args]
t81 pkg init [package_name]
t81 pkg check [package.t81] [--json]
```

Creates/validates `package.t81`.
This surface is currently experimental and not part of the default core help view.
`pkg check --json` uses schema `t81.pkg-check.v1`.

### 4.15 `benchmark`

```text
t81 benchmark [benchmark_runner_flags...]
```

Runs benchmark runner with forwarded benchmark flags.

### 4.16 `weights`

```text
t81 weights <subcommand> [options]
t81 weights import <file> [-o <out>] [--format <fmt>]
t81 weights info <model.t81w> [--json]
t81 weights verify <model.t81w> [--json]
t81 weights quantize <input> --to-gguf <out>
```

Weight import/info/quantization helpers.
`weights info --json` uses schema `t81.weights-info.v1` and returns `ok: false` with
an `error` field on load failures.
`weights verify --json` uses schema `t81.weights-verify.v1`.

### 4.17 `policy`

```text
t81 policy <subcommand> [options]
t81 policy compile <file.apl> [-o <out>]
t81 policy run <file.apl|file.axionb> [--json]
t81 policy test <file.apl|file.axionb> --model-hash <hash> [--json]
t81 policy list [--json] [--dir <path>]
```

Policy compile/validation helpers. Policies are written in APL (Axion Policy Language), an
s-expression dialect — e.g. `(allow-all)`, `(deny opcode TLOADHASH)`.

`policy compile` compiles an `.apl` source file to a binary `.axionb` artifact.
`policy list` recursively scans `<dir>` (default: current directory) for `.apl` policy files
and prints their paths. `--json` uses schema `t81.policy-list.v1`.
`policy run --json` uses schema `t81.policy-run.v1`.
`policy test --json` uses schema `t81.policy-test.v1`.

### 4.17a `axion`

```text
t81 axion status [--json]
t81 axion optimize [--tier N] [--json]
t81 axion simulate <file.tisc> [--json]
t81 axion explain <file.apl|file.axionb> [--json]
t81 axion snapshot [--json]
t81 axion snapshot-diff <lhs> <rhs> [--json]
t81 axion rollback --to <hash> [--json]
t81 axion log [--json] [--tail <n>]
```

Axion governor and policy-diagnostics helpers.
`axion log` reads the persisted Axion event journal from `<canonfs-root>/axion/state.json`
and prints a summary of recorded policy events. `--tail <n>` limits output to the last `n`
entries. `--json` uses schema `t81.axion-log.v1`.
`axion explain --json` uses schema `t81.axion-explain.v1`.
`axion status --json` uses schema `t81.axion-status.v1`.
`axion optimize --json` uses schema `t81.axion-optimize.v1`.
`axion simulate --json` uses schema `t81.axion-simulate.v1`.
`axion snapshot-diff --json` reuses schema `t81.canonfs-snapshot-diff.v1`.

### 4.18 `trace`

```text
t81 trace <subcommand> [args]
t81 trace show <trace.txt> [--no-color]
t81 trace diff <trace1.txt> <trace2.txt> [--no-color]
t81 trace replay <file.tisc> <trace.txt> [--json]
t81 trace summary <trace.txt> [--json]
t81 trace stats <trace.txt> [--json]
t81 trace canonicalize <trace.txt> [-o <file>]
t81 trace export <trace.txt> [--format <json|csv>] [-o <file>]
```

Trace inspection and export utilities.
`trace export --format json` emits entries with schema `t81.trace-export-entry.v1`.
`trace replay` expects a clean trace file; `t81 run --trace-out <file>` produces the intended input.
If the trace file cannot be opened, `trace replay --json` returns `kind: "open_error"` without extra human-readable stderr noise.
`trace summary --json` uses schema `t81.trace-summary.v1`.
`trace canonicalize` normalizes ad hoc traces into replay-safe canonical form.

### 4.18a `canonfs`

```text
t81 canonfs put-file <file> [--canonfs-root <path>]
t81 canonfs put-tensor <file> [--canonfs-root <path>]
t81 canonfs ls [--json] [--canonfs-root <path>]
t81 canonfs get <sha3-256:hash> [-o <file>] [--json] [--canonfs-root <path>]
t81 canonfs stat <sha3-256:hash> [--json] [--canonfs-root <path>]
t81 canonfs verify <sha3-256:hash> [--json] [--canonfs-root <path>]
t81 canonfs snapshot [--json] [--canonfs-root <path>]
t81 canonfs snapshot-diff <lhs> <rhs> [--json] [--canonfs-root <path>]
t81 canonfs rollback --to <hash> [--json] [--canonfs-root <path>]
```

CanonFS inspection and snapshot tooling.
`canonfs ls --json` uses schema `t81.canonfs-list.v1`.
`canonfs get --json` uses schema `t81.canonfs-get.v1`.
`canonfs stat --json` uses schema `t81.canonfs-stat.v1`.
`canonfs verify --json` uses schema `t81.canonfs-verify.v1`.
`canonfs snapshot-diff --json` uses schema `t81.canonfs-snapshot-diff.v1`.
`canonfs gc` removes unreferenced objects from the store; `--json` uses schema `t81.canonfs-gc.v1`.

### 4.18b `determinism`

```text
t81 determinism verify [fixtures_dir]
t81 determinism verify-run <file.tisc> [--policy <policy.apl>] [--json]
t81 determinism certify <file.tisc> [--policy <policy.apl>] [--json]
t81 determinism explain <file.tisc> [--policy <policy.apl>] [--json]
t81 determinism hash <file> [--json]
t81 determinism trace-hash <trace.txt> [--json]
t81 determinism diff <lhs> <rhs> [--json]
t81 determinism diff-trace <lhs> <rhs> [--json]
```

Determinism verification and artifact hashing tools.
`determinism verify-run --json` uses schema `t81.determinism-verify-run.v1`.
`determinism certify --json` uses schema `t81.determinism-certificate.v1`.
`determinism explain --json` uses schema `t81.determinism-explain.v1`.
`determinism hash --json` uses schema `t81.determinism-hash.v1`.
`determinism trace-hash --json` uses schema `t81.determinism-trace-hash.v1`.
`determinism diff --json` uses schema `t81.determinism-diff.v1`.
`determinism diff-trace --json` uses schema `t81.determinism-trace-diff.v1`.
`determinism baseline` scans a directory for `.tisc` files and writes `baseline.json` with
SHA3-512 hashes for use with `determinism verify`; `--json` uses schema `t81.determinism-baseline.v1`.

### 4.18c `vm`

```text
t81 vm run <file.tisc> [--policy <policy.apl>] [-o <trace.txt>]
t81 vm debug <file.tisc> [--policy <policy.apl>]
t81 vm trace <file.tisc> [--policy <policy.apl>] [-o <trace.txt>]
t81 vm step <file.tisc> [--policy <policy.apl>] [--count <n>] [--json]
t81 vm regs <file.tisc> [--policy <policy.apl>] [--steps <n>] [--json]
t81 vm stack <file.tisc> [--policy <policy.apl>] [--steps <n>] [--limit <n>] [--json]
t81 vm mem <file.tisc> --addr <n> [--policy <policy.apl>] [--steps <n>] [--json]
t81 vm state <file.tisc> [--policy <policy.apl>] [--json]
t81 vm profile <file.tisc> [--policy <policy.apl>] [--json]
t81 vm explain-trap <file.tisc> [--policy <policy.apl>] [--json]
```

VM-oriented entry points for execution, replay-safe trace capture, debugging, and final-state reporting.
`vm step --json` uses schema `t81.vm-step.v1`.
`vm regs --json` uses schema `t81.vm-regs.v1`.
`vm stack --json` uses schema `t81.vm-stack.v1`.
`vm mem --json` uses schema `t81.vm-mem.v1`.
`vm state --json` uses schema `t81.vm-state.v1`.
`vm profile --json` uses schema `t81.vm-profile.v1`.
`vm explain-trap --json` uses schema `t81.vm-trap-explain.v1`.

### 4.18d `tisc`

```text
t81 tisc disasm <file.tisc>
t81 tisc validate <file.tisc> [--json]
t81 tisc encode <file.base81> [-o <out.tisc>] [--json]
t81 tisc decode <file.tisc> [-o <out.base81>] [--json]
t81 tisc diff <a.tisc> <b.tisc> [--json]
```

TISC-oriented artifact inspection commands.
`tisc validate --json` uses schema `t81.tisc-validate.v1`.
`tisc encode --json` uses schema `t81.tisc-encode.v1`.
`tisc decode --json` uses schema `t81.tisc-decode.v1`.
`tisc diff` compares two TISC artifacts at the instruction level; exits 0 if identical, 1 if they differ.
`tisc diff --json` uses schema `t81.tisc-diff.v1`.

### 4.18e `ir`

```text
t81 ir show <file.t81>
t81 ir dump <file.t81>
t81 ir export <file.t81> [--json] [-o <file>]
```

Prints or exports frontend IR after parse + semantic analysis and before final bytecode emission.
`ir export --json` uses schema `t81.ir-export.v1`.

### 4.19 `llama-run` (experimental)

```text
t81 llama-run <model.gguf|sha3-256:hash> <prompt> --policy <policy.apl> [options]
```

Options:

- `--max-tokens <n>`
- `--seed <n>`
- `--threads <n>`
- `--temperature <x>`
- `--top-k <n>`
- `--top-p <x>`
- `--expected-model-hash <h>`
- `--canonfs-root <path>`

### 4.20 `completion`

```text
t81 completion <bash|zsh|fish>
```

Prints shell completion script to `stdout`.

### 4.21 `man`

```text
t81 man [--install-dir <dir>]
```

Shows embedded manpage text or installs `t81.1`.

### 4.22 `feedback`

```text
t81 feedback submit --rating <1-5> [--note <text>] [--path <file>]
t81 feedback report [--path <file>]
```

Local CLI UX feedback loop.
Submit writes JSONL entries with schema `t81.feedback.v1`.
Report emits schema `t81.feedback-report.v1`.

## 5. Help Contract

Supported help forms:

```bash
./build/t81 --help # docs-smoke
./build/t81 help # docs-smoke
./build/t81 help code # docs-smoke
./build/t81 help code build # docs-smoke
./build/t81 code build --help # docs-smoke
./build/t81 help compile # docs-smoke
./build/t81 help test # docs-smoke
./build/t81 help doctor # docs-smoke
./build/t81 help fmt # docs-smoke
./build/t81 help completion # docs-smoke
./build/t81 help man # docs-smoke
./build/t81 help feedback # docs-smoke
./build/t81 compile --help # docs-smoke
./build/t81 help advanced # docs-smoke
./build/t81 help labs # docs-smoke
```

Unknown help topics return non-zero.

Legacy top-level aliases print a deprecation warning on `stderr` with the
domain-first equivalent (for example, `compile` -> `code build`).

## 6. Exit and Output Behavior

- `0` on success.
- `1` on usage/user input failures.
- `2` on environment/runtime tool execution failures for selected commands.
- command result output is written to `stdout`.
- diagnostics/errors are written to `stderr` and prefixed with `error:`.

Command-specific non-zero exits:

| Command | Exit Code | Meaning |
| :--- | :--- | :--- |
| `test` | `2` | `ctest` not available, build metadata missing, or tests failed |
| `doctor` | `2` | one or more readiness checks failed |
| `fmt --check` | `2` | formatting drift detected |
| `fmt` | `1` | invalid input or formatter write/read failures |
| `pkg check` | `2` | manifest invalid |
| `trace export` | `1` | invalid args/format/path |
| `weights info` | `1` | usage or file-loading failure |
| `policy run` | `1` | usage or policy parse/load failure |
| `env check` | `1` | one or more environment checks failed |
| `tisc diff` | `1` | programs differ (instruction-level mismatch) |
| `canonfs gc` | `1` | store not initialized or I/O failure |
| `completion` | `1` | unsupported shell or usage error |
| `man` | `2` | install directory/file write failure |
| `feedback` | `2` | feedback file read/write failure |

Runtime trap exit codes used by `t81 run` / `t81 debug`:

| Code | Meaning |
| :--- | :--- |
| `10` | Division fault |
| `12` | Bounds fault |
| `13` | Security fault |
| `14` | Decode fault |
| `15` | Explicit trap instruction |
| `16` | Type fault |
| `17` | Stack fault |
| `18` | Shape fault |
| `19` | Tier fault |

## 7. Known Command Validation Rules

- `check` / `lint` require `.t81`.
- `disasm` requires `.tisc`.
- `run` and `debug` require `.t81` or `.tisc`.
- `compile` accepts `.t81` and `.t81w`.
- `pkg init` enforces package name characters: alphanumeric, `_`, `-`.

## 8. Notes on Documentation Scope

This manual intentionally excludes deprecated or non-shipping flags.
If behavior changes, update this file in the same change set as the CLI implementation.
