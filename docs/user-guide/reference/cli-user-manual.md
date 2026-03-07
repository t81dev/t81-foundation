# T81 CLI User Manual

This is the operator manual for the `t81` CLI.
Everything in this document is intended to match the current shipped binary behavior.

**Last Updated:** February 26, 2026

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

Additional command groups are discoverable via:

- `t81 help advanced`
- `t81 help labs`

Current Labs commands include `pkg`, `benchmark`, `repro-hash`,
`canonize-tensor`, `canonize-file`, and `llama-run`.

## 4. Commands

### 4.1 Domain-First Groups

```text
t81 code <action> [args]
t81 project <action> [args]
t81 env <action> [args]
t81 internal <action> [args]
t81 completion <bash|zsh|fish>
t81 man [--install-dir <dir>]
t81 feedback <submit|report> [options]
```

Primary workflow actions:

```text
t81 code check <file.t81>
t81 code fmt [options] <file...>
t81 code build <file.t81|file.t81w> [-o <file.tisc>] [--weights-model <model.t81w>]
t81 code run <file.t81|file.tisc> [--policy <policy.apl>] [--trace] [--weights-model <model.t81w>]
t81 code test [options] [-- <ctest args...>]
t81 code disasm <file.tisc>
t81 code debug <file.t81|file.tisc> [--policy <policy.apl>] [--weights-model <model.t81w>]
t81 code repl [--weights-model <model.t81w>] [--policy <policy.apl>]
t81 project init <project_name>
t81 env doctor [--json]
t81 env feedback <submit|report> [options]
t81 internal pkg <subcommand> [args]
t81 internal benchmark [benchmark_runner_flags...]
t81 internal repro-hash [fixtures_dir]
t81 internal canonize-tensor <file>
t81 internal canonize-file <file> [--canonfs-root <path>]
t81 internal llama-run <model.gguf|sha3-256:hash> <prompt> --policy <policy.apl> [options]
t81 completion bash|zsh|fish
t81 man [--install-dir <dir>]
t81 feedback submit --rating <1-5> [--note <text>] [--path <file>]
t81 feedback report [--path <file>]
```

### 4.2 Legacy Top-Level Aliases

### 4.1 `compile`

```text
t81 compile <file.t81|file.t81w> [-o <file.tisc>] [--weights-model <model.t81w>]
```

Compiles source into TISC bytecode.

### 4.2 `run`

```text
t81 run <file.t81|file.tisc> [--policy <policy.apl>] [--trace] [--weights-model <model.t81w>]
```

Compiles if needed and executes via VM.

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

### 4.10 `init`

```text
t81 init <project_name>
```

Creates a project directory with `main.t81` and `README.md`.

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
t81 weights quantize <input> --to-gguf <out>
```

Weight import/info/quantization helpers.
`weights info --json` uses schema `t81.weights-info.v1` and returns `ok: false` with
an `error` field on load failures.

### 4.17 `policy`

```text
t81 policy <subcommand> [options]
t81 policy compile <file.apl> [-o <out>]
t81 policy run <file.apl|file.axionb> [--json]
```

Policy compile/validation helpers.
`policy run --json` uses schema `t81.policy-run.v1`.

### 4.18 `trace`

```text
t81 trace <subcommand> [args]
t81 trace show <trace.txt> [--no-color]
t81 trace diff <trace1.txt> <trace2.txt> [--no-color]
t81 trace replay <file.tisc> <trace.txt>
t81 trace export <trace.txt> [--format <json|csv>] [-o <file>]
```

Trace inspection and export utilities.
`trace export --format json` emits entries with schema `t81.trace-export-entry.v1`.

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
