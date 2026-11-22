#include "tools/cli/core/help_text.hpp"

#include <iostream>

#include "t81/canonfs/interchange_ops.hpp"
#include "t81/config.hpp"
#include "tools/cli/ai/ai_cli_shared.hpp"

void print_help_weights() {
  std::cerr << R"(
Usage: t81 weights <subcommand> [options]

Subcommands:
  import <file> [-o <out>] [--format <fmt>]  Import weights (safetensors/gguf) -> .t81w
  info <model.t81w> [--json]                 Print native model metadata
  verify <model.t81w> [--json]               Verify native weights integrity and metadata
  export <model.t81w> --to-safetensors <out> Export native weights to SafeTensors
  quantize <input> --to-gguf <out>           Quantize SafeTensors -> T3_K GGUF

Options:
  --format <fmt>       Input format (safetensors, bitnet, gguf). Default: infer from extension
  -o, --output <file>  Output file path

Examples:
  t81 weights import model.safetensors -o model.t81w
  t81 weights import bitnet_model.safetensors --format bitnet -o model.t81w
  t81 weights info model.t81w --json
  t81 weights verify model.t81w --json
  t81 weights export model.t81w --to-safetensors model.safetensors
  t81 weights quantize model.safetensors --to-gguf model.gguf
)";
}

void print_help_model() {
  std::cerr << R"(
Usage: t81 model <subcommand> [options]

Subcommands:
  import <file> [--json] [--report] [--format <fmt>] [-o <out>] [--manifest <file>]
                                 Inspect a model artifact through the existing
                                 loader path without writing `.t81w`
  diff <lhs> <rhs> [--json]
                                 Compare two model artifacts by imported format,
                                 tensor inventory, and tensor metadata

Options:
  --format <fmt>       Input format (safetensors, bitnet, gguf). Default: infer from extension
  --json               Machine-readable output
  --report             Human-readable report output (default)
  -o, --output <file>  Also write the emitted report/JSON to a file
  --manifest <file>    Also write a narrower persisted model manifest JSON

Examples:
  t81 model import model.safetensors --json
  t81 model import model.gguf --report
  t81 model import model.safetensors --json -o model-import.json
  t81 model import model.safetensors --json --manifest model-manifest.json
  t81 model diff model-a.safetensors model-b.safetensors --json
)";
}

void print_help_policy() {
  std::cerr << R"(
Usage: t81 policy <subcommand> [options]

Subcommands:
  compile <file.apl> [-o <out>]   Compile Axion Policy Language -> .axionb
  validate <file.apl|.axionb> [--json]
                                 Parse and validate an Axion policy without loading it
  run <file.apl|.axionb>          Validate and load an Axion policy
  test <file.apl|.axionb> --model-hash <hash>
                                 Validate a model hash against an Axion policy
  list [--json]                   List .apl policy files found in the project

APL syntax: Axion Policy Language uses s-expression syntax.
Example policy: (allow-all)

Options:
  -o, --output <out> Output file path

Examples:
  t81 policy compile policy.apl -o policy.axionb
  t81 policy validate policy.apl --json
  t81 policy run policy.apl --json
  t81 policy test policy.apl --model-hash sha3-256:...
)";
}

void print_help_axion() {
  std::cerr << R"(
Usage: t81 axion <subcommand> [options]

Axion Governor command surface (RFC-0000 §7).

Subcommands:
  status              Show current Axion governor state plus detected issues
  optimize [--tier N] Capture a snapshot, diff it against prior state, and record requested tier
  simulate <file>     Execute a TISC program and summarize Axion-visible events
  explain <policy>    Explain policy structure and validation status
  snapshot            Capture a canonical CanonFS snapshot of current state
  snapshot-diff <lhs> <rhs>
                     Compare two CanonFS snapshots through the Axion lens
  rollback [--to <hash>]  Roll back to a prior CanonFS snapshot
  log [--json] [--tail <n>]           Show Axion state plus recent snapshot history
  audit [--from <hash>] [--to <hash>] Export Axion/CanonFS snapshot diff metadata

Options:
  --tier N            Target cognition tier (1..9, default: 1)
  --to <hash>         CanonHash-81 snapshot target for rollback
  --json              Machine-readable output

Examples:
  t81 axion status
  t81 axion optimize --tier 2
  t81 axion simulate program.tisc
  t81 axion explain policy.apl
  t81 axion snapshot
  t81 axion snapshot-diff <lhs> <rhs>
  t81 axion rollback --to <hash>
)";
}

void print_help_trace() {
  std::cerr << R"(
Usage: t81 trace <subcommand> [args]

Subcommands:
  show <trace.txt>                Visualize an Axion trace with color
  diff <trace1.txt> <trace2.txt>  Diff two Axion traces
  replay <file.tisc> <trace.txt>  Replay and verify trace matches
  summary <trace.txt> [options]   Summarize opcode and trap distribution
  stats <trace.txt> [options]     Alias for summary
  filter <trace.txt> [options]    Filter trace entries by opcode/trap/PC range
  canonicalize <trace.txt> [options]
                                  Normalize trace content for replay/diff/hash workflows
  export <trace.txt> [options]    Export trace to JSON/CSV

Options:
  --format <json|csv>             Export format (default: json)
  -o, --output <file>             Output file path (default: stdout)
  --no-color                      Disable ANSI colors in show/diff output
  --json                          Machine-readable output (replay/summary)
  --opcode <name>                 Filter trace entries by opcode
  --trap <name>                   Filter trace entries by trap name
  --pc-start <n> --pc-end <n>     Filter trace entries by PC range

Examples:
  t81 trace show trace.log
  t81 trace diff run1.log run2.log --no-color
  t81 trace summary run.trace --json
  t81 trace filter run.trace --opcode ADD --json
  t81 trace canonicalize run.trace -o canonical.trace
  t81 trace export trace.log --format json -o trace.json
)";
}

void print_help_canonize_file() {
  std::cerr << R"(
Usage: t81 internal canonize-file <file> [--canonfs-root <path>]

Writes raw file bytes into CanonFS and prints `sha3-256:<hash>`.
)";
}

void print_help_memory_stats() {
  std::cerr << R"(
Usage: t81 internal memory-stats [file]

Displays memory pool statistics and profiling information.
If a T81 file is provided, runs the program and shows memory usage.
Without a file, shows current memory pool configuration.

Examples:
  t81 internal memory-stats program.t81
  t81 internal memory-stats
)";
}

void print_help_llama_run() {
  std::cerr << R"(
Usage: t81 internal llama-run <model.gguf|sha3-256:hash> <prompt> --policy <policy.apl> [options]

Options:
  --max-tokens <n>          Maximum generated tokens (default: 64)
  --seed <n>                Sampler seed (default: 0)
  --threads <n>             Threads for prompt/decode (default: 1)
  --temperature <x>         Sampling temperature (default: 0.0)
  --top-k <n>               Top-k (default: 1)
  --top-p <x>               Top-p (default: 1.0)
  --expected-model-hash <h> Enforce exact model hash before inference
  --canonfs-root <path>     CanonFS root for hash-based model loading (default: ./.t81_canonfs)

Notes:
  - This surface is experimental and non-DCP.
  - --policy is required and must include allowed-tensor-hashes for model authorization.
  - Bare model names are resolved from the nearest `./model/` or `./models/` directory.
)";
}

void print_help_compile() {
  std::cerr << R"(
Usage: t81 code build <file.t81|file.t81w> [-o <file.tisc>] [--weights-model <model.t81w|sha3-256:hash>]

Compiles T81Lang source (or T81 weight module source) into TISC bytecode.

Example:
  t81 code build program.t81 -o program.tisc
)";
}

void print_help_run() {
  std::cerr << R"(
Usage: t81 code run <file.t81|file.tisc> [--policy <policy.apl>] [--trace] [-o <file>] [--output <file>] [--weights-model <model.t81w|sha3-256:hash>]

Compiles (if needed) and executes a program via the VM.
`--trace-out` remains accepted for compatibility, but `-o/--output` is canonical.

Example:
  t81 code run program.tisc -o run.trace
)";
}

void print_help_profile() {
  std::cerr << R"(
Usage: t81 code profile <file.t81|file.tisc> [--policy <policy.apl>] [--json] [--weights-model <model.t81w|sha3-256:hash>]

Compiles (if needed) and reports deterministic VM execution and memory metrics.

Example:
  t81 code profile program.t81 --json
)";
}

void print_help_disasm() {
  std::cerr << R"(
Usage: t81 disasm <file.tisc>

Prints human-readable TISC disassembly.

Example:
  t81 disasm program.tisc
)";
}

void print_help_debug() {
  std::cerr << R"(
Usage: t81 debug <file.t81|file.tisc> [--policy <policy.apl>] [--weights-model <model.t81w|sha3-256:hash>]

Compiles (if needed) and starts the interactive debugger.

Example:
  t81 debug program.tisc
)";
}

void print_help_check() {
  std::cerr << R"(
Usage: t81 check <file.t81>
       t81 lint  <file.t81>

Performs syntax and semantic checks without emitting bytecode.

Example:
  t81 check program.t81
)";
}

void print_help_repl() {
  std::cerr << R"(
Usage: t81 repl [--weights-model <model.t81w|sha3-256:hash>] [--policy <policy.apl>]

Starts the interactive REPL.

Example:
  t81 repl --policy policy.apl
)";
}

void print_help_test() {
  std::cerr << R"(
Usage: t81 test [options] [-- <ctest args...>]

Runs project tests via CTest.

Options:
  --build-dir <path>   Build directory containing CTest metadata (default: build)
  --filter <regex>     CTest regex filter (passes through as -R)
  --json               Emit machine-readable execution summary
  --list               List discovered tests instead of executing

Examples:
  t81 test
  t81 test --filter cli_
  t81 test --json --build-dir out/build
)";
}

void print_help_doctor() {
  std::cerr << R"(
Usage: t81 env doctor [toolchain|canonfs|vm] [--json]

Runs local environment readiness checks and reports actionable diagnostics.

Scopes:
  toolchain     Compiler/build/test tool availability
  canonfs       CanonFS root/store readiness
  vm            Build/runtime readiness for VM workflows

Examples:
  t81 env doctor
  t81 env doctor canonfs
  t81 env doctor --json
)";
}

void print_help_env_diag() {
  std::cerr << R"(
Usage: t81 env diag [--json]

Aggregates repo/build discovery, tool availability, CanonFS status, and Axion state.

Examples:
  t81 env diag
  t81 env diag --json
)";
}

void print_help_fmt() {
  std::cerr << R"(
Usage: t81 fmt [options] <file...>

Formats files with deterministic whitespace normalization.

Options:
  --check              Dry-run; report files that would change
  --json               Emit machine-readable summary
  --version            Print formatter version identifier

Examples:
  t81 fmt examples/core-language/hello_world.t81
  t81 fmt --check src/a.t81 src/b.t81
)";
}

void print_help_completion() {
  std::cerr << R"(
Usage: t81 completion <bash|zsh|fish>

Prints shell completion script to stdout.

Examples:
  t81 completion bash > ~/.local/share/bash-completion/completions/t81
  t81 completion zsh > ~/.zfunc/_t81
)";
}

void print_help_man() {
  std::cerr << R"(
Usage: t81 man [--install-dir <dir>]

Shows an embedded manpage-style summary, or writes `t81.1` into install dir.

Examples:
  t81 man
  t81 man --install-dir ~/.local/share/man/man1
)";
}

void print_help_tui() {
  std::cerr << R"(
T81 Dual TUI Frontends

COMMANDS
  t81 studio                            Human Operator Interface
  t81 agent [--resume <f>]              AI-Native / Agentic Interface
             [--session <f>]
  t81 ui                                Interactive launcher (choose studio/agent)

STUDIO (t81 studio)
  Navigation sidebar  7 views: Workspace, Compiler, Determinism,
                      CanonFS, Axion, Trace, REPL
  Ctrl+P              Command palette — fuzzy search all t81 commands
  Enter               Activate selected item / submit REPL input
  q / Escape          Quit

AGENT (t81 agent)
  /help               List all slash commands
  /compile <file>     Compile a .t81 source file
  /run <file>         Compile and execute a .t81 file
  /trace <file>       Show execution trace
  /axion              Show Axion policy status
  /policy <action>    Axion policy commands
  /hash <file>        Compute determinism hash
  /tier <n>           Set displayed cognitive tier
  /trits              Toggle trit-probability display
  /save               Save session to --session path
  /clear              Clear conversation history
  /quit               Exit
  Escape              Exit

FLAGS (t81 agent)
  --resume <path>     Resume a saved JSONL session file
  --session <path>    Auto-save session to this file on exit

BUILD
  Requires -DT81_BUILD_TUI=ON (default: ON).
  Disable with -DT81_BUILD_TUI=OFF; commands remain but print an error.
)";
}

void print_help_feedback() {
  std::cerr << R"(
Usage: t81 feedback <submit|report> [options]

Subcommands:
  submit --rating <1-5> [--note <text>] [--path <file>]   Append feedback record
  report [--path <file>]                                    Summarize feedback records

Default path:
  .t81_cli_feedback.jsonl
)";
}

void print_help_code() {
  std::cerr << R"(
Usage: t81 code <action> [args]

Actions:
  check <file.t81>                    Validate syntax and semantics
  lint <file.t81>                     Alias for check
  fmt [options] <file...>             Format source files
  build <file.t81|file.t81w> [...]    Compile to TISC bytecode
  run <file.t81|file.tisc> [...]      Execute program
  profile <file.t81|file.tisc> [...]  Compile if needed and report VM runtime/memory metrics
  test [options] [-- ...]             Run tests via CTest
  disasm <file.tisc>                  Disassemble TISC
  debug <file.t81|file.tisc> [...]    Start debugger
  repl                                Start interactive REPL

Examples:
  t81 code check examples/core-language/hello_world.t81
  t81 code build examples/core-language/hello_world.t81 -o hello.tisc
  t81 code run hello.tisc
)";
}

void print_help_project() {
  std::cerr << R"(
Usage: t81 project <action> [args]

Actions:
  init <project_name>                 Scaffold a new T81 project
  build [file.t81]                    Compile project source (defaults to main.t81)
  run [file.t81] [--policy <p>]       Build and execute project source
  test [options]                      Run project tests via CTest

Examples:
  t81 project init my_project
  t81 project build
  t81 project run
)";
}

void print_help_env() {
  std::cerr << R"(
Usage: t81 env <action> [args]

Actions:
  check [--json]                      Quick pass/fail readiness check (exit 0=ready, 1=not)
  doctor [toolchain|canonfs|vm] [--json]
                                     Run environment readiness checks with detail
  paths [--json]                      Print important repo/runtime paths
  diag [--json]                       Aggregate environment, CanonFS, and Axion diagnostics
  toolchain [--json]                  Inspect compiler/build/test tool availability
  clean [--build-dir <path>] [--force] [--json]
                                     Remove local build artifacts
  feedback <subcommand> [args]        Record/report local CLI UX feedback

Example:
  t81 env doctor --json
)";
}

void print_help_internal() {
  std::cerr << R"(
Usage: t81 internal <action> [args]

Actions:
  pkg <subcommand> [args]             Package manifest helpers (experimental)
  benchmark [args]                    Benchmark runner entrypoint
  repro-hash [fixtures_dir]           Reproducibility gate helper
  canonize-tensor <file>              CanonFS tensor canonicalization
  canonize-file <file>                CanonFS raw-file canonicalization
  llama-run ...                       Experimental governed llama.cpp path
  memory-stats [file]                 Memory pool statistics and profiling

These commands are non-default and intended for internal/expert workflows.
)";
}

void print_help_canonfs() {
  std::cerr << R"(
Usage: t81 canonfs <action> [args]

Actions:
  put-file <file> [--canonfs-root <path>]       Store raw bytes and print canonical hash
  put-tensor <file> [--canonfs-root <path>]     Canonicalize tensor/model payloads
  import <path> [--json] [--policy <file.apl>] [--policy-profile <name>] [--canonfs-root <path>]
                                                 Import a host file or directory tree into CanonFS
  ls [--json] [--canonfs-root <path>]           List stored objects
  get <sha3-256:hash> [-o <file>] [--json] [--canonfs-root <path>]
  export <sha3-256:hash> -o <path> [--json] [--policy <file.apl>] [--policy-profile <name>] [--canonfs-root <path>]
                                                 Export a CanonFS object or interchange manifest

Policy profiles:
)";
  for (t81::canonfs::InterchangePolicyProfile profile :
       {t81::canonfs::InterchangePolicyProfile::Permissive,
        t81::canonfs::InterchangePolicyProfile::ImportOnly,
        t81::canonfs::InterchangePolicyProfile::ExportOnly,
        t81::canonfs::InterchangePolicyProfile::DenyAll}) {
    const auto info = t81::canonfs::interchange_policy_profile_info(profile);
    std::cerr << "  " << info.name << "  " << info.summary << "\n";
  }
  std::cerr << R"(
  stat <sha3-256:hash> [--json] [--canonfs-root <path>]
  verify <sha3-256:hash> [--json] [--canonfs-root <path>]
  snapshot [--json] [--canonfs-root <path>]
  snapshot-diff <lhs> <rhs> [--json] [--canonfs-root <path>]
  rollback --to <hash> [--dry-run] [--json] [--canonfs-root <path>]
  gc [--dry-run] [--json] [--canonfs-root <path>]  Remove unreferenced objects from the store
  fsck [--verify-hashes] [--json] [--canonfs-root <path>]
                                                   Verify object readability, optional hashes, and snapshot manifests
  repair [--dry-run] [--json] [--canonfs-root <path>]
                                                   Remove legacy snapshot-local object copies

Examples:
  t81 canonfs put-file README.md
  t81 canonfs import README.md --json
  t81 canonfs export sha3-256:... -o restored.bin --json
  t81 canonfs stat sha3-256:... --json
  t81 canonfs snapshot
)";
}

void print_help_artifact() {
  std::cerr << R"(
Usage: t81 artifact <action> [args]

Actions:
  write-record --schema <schema-id> --out <file> --field key=value [--field key=value ...]
                                     Write one fixed downstream record with canonical field ordering
  write-store-record --schema <schema-id> --field key=value [--field key=value ...] [--canonfs-root <path>]
                                     Write, validate, and store one fixed downstream record into CanonFS
  store-bundle --schema <schema-id> --field key=value [--field key=value ...] [--canonfs-root <path>]
                                     Validate and store one fixed downstream bundle into CanonFS
  validate-record <file|sha3-256:hash> --schema <schema-id> [--canonfs-root <path>]
                                     Validate a fixed downstream record against a supported schema
  store-record --schema <schema-id> --file <path> [--canonfs-root <path>]
                                     Validate and store one fixed downstream record into CanonFS
  read-field <file|sha3-256:hash> --schema <schema-id> --field <name> [--canonfs-root <path>]
                                     Read one scalar field from a validated fixed downstream artifact

Supported schemas:
  t81.ai.task.assess-fixed.host-action-record.v1
  t81.ai.task.assess-fixed.bundle.v1
  t81.ai.task.route-fixed.path-selection-record.v1
  t81.ai.task.route-fixed.bundle.v1
  t81.ai.task.classify-fixed.rule-selection-record.v1
  t81.ai.task.classify-fixed.bundle.v1

Canonical runnable example:
  See the `Assess-Fixed OS-Object Chain` section in
  examples/ai-and-inference/model-load-canonfs/README.md
  and the companion script
  examples/ai-and-inference/model-load-canonfs/run_assess_fixed_host_action.sh
  for the current canonical end-to-end use of these typed artifact helpers.
  Additional bounded compositions using the same object model are available in
  examples/ai-and-inference/model-load-canonfs/run_route_fixed_path_selection.sh
  and
  examples/ai-and-inference/model-load-canonfs/run_classify_fixed_rule_selection.sh

Examples:
  t81 artifact write-record --schema t81.ai.task.assess-fixed.host-action-record.v1 --out record.json --field decision=ALLOW --field reason_code=GREETING_PAIR --field selected_action=write_allow_marker --field selected_path=actions/allow.marker --field action_ref=sha3-256:... --field source_result_ref=sha3-256:... --field source_provenance_ref=sha3-256:... --field termination_reason=single_step_max_score
  t81 artifact write-store-record --schema t81.ai.task.assess-fixed.host-action-record.v1 --field decision=ALLOW --field reason_code=GREETING_PAIR --field selected_action=write_allow_marker --field selected_path=actions/allow.marker --field action_ref=sha3-256:... --field source_result_ref=sha3-256:... --field source_provenance_ref=sha3-256:... --field termination_reason=single_step_max_score --canonfs-root .t81_canonfs
  t81 artifact store-bundle --schema t81.ai.task.assess-fixed.bundle.v1 --field source_result_ref=sha3-256:... --field source_provenance_ref=sha3-256:... --field action_ref=sha3-256:... --field record_ref=sha3-256:... --canonfs-root .t81_canonfs
  t81 artifact write-store-record --schema t81.ai.task.route-fixed.path-selection-record.v1 --field route=A --field selected_action=write_route_a_marker --field selected_path=routes/a.target --field action_ref=sha3-256:... --field source_result_ref=sha3-256:... --field source_provenance_ref=sha3-256:... --field termination_reason=single_step_max_score --canonfs-root .t81_canonfs
  t81 artifact store-bundle --schema t81.ai.task.route-fixed.bundle.v1 --field source_result_ref=sha3-256:... --field source_provenance_ref=sha3-256:... --field action_ref=sha3-256:... --field record_ref=sha3-256:... --canonfs-root .t81_canonfs
  t81 artifact write-store-record --schema t81.ai.task.classify-fixed.rule-selection-record.v1 --field label=POSITIVE --field selected_rule_set=positive-default --field rule_set_ref=sha3-256:... --field source_result_ref=sha3-256:... --field source_provenance_ref=sha3-256:... --field termination_reason=single_step_max_score --canonfs-root .t81_canonfs
  t81 artifact store-bundle --schema t81.ai.task.classify-fixed.bundle.v1 --field source_result_ref=sha3-256:... --field source_provenance_ref=sha3-256:... --field action_ref=sha3-256:... --field record_ref=sha3-256:... --canonfs-root .t81_canonfs
  t81 artifact validate-record record.json --schema t81.ai.task.assess-fixed.host-action-record.v1
  t81 artifact store-record --schema t81.ai.task.assess-fixed.host-action-record.v1 --file record.json --canonfs-root .t81_canonfs
  t81 artifact validate-record sha3-256:... --schema t81.ai.task.assess-fixed.host-action-record.v1 --canonfs-root .t81_canonfs
  t81 artifact read-field record.json --schema t81.ai.task.assess-fixed.host-action-record.v1 --field selected_action
)";
}

void print_help_vm() {
  std::cerr << R"(
Usage: t81 vm <action> [args]

Actions:
  run <file.tisc> [--policy <file>] [-o <trace.txt>]
                                                 Execute a TISC program in the VM
  debug <file.tisc> [--policy <file>]            Start the interactive VM debugger
  trace <file.tisc> [--policy <file>] [-o <trace.txt>]
                                                 Execute and write replay-safe trace output
  until <file.tisc> --pc <n> [--policy <file>] [--steps N] [--json]
                                                 Execute until a target PC is reached or step budget is exhausted
  step <file.tisc> [--policy <file>] [--count N] [--json]
                                                 Execute a bounded number of VM steps
  regs <file.tisc> [--policy <file>] [--steps N] [--json]
                                                 Inspect registers after a bounded number of steps
  stack <file.tisc> [--policy <file>] [--steps N] [--limit N] [--json]
                                                 Inspect stack contents after a bounded number of steps
  mem <file.tisc> --addr <n> [--policy <file>] [--steps N] [--json]
                                                 Inspect one memory cell after a bounded number of steps
  state <file.tisc> [--policy <file>] [--json]   Execute and print final VM state summary
  profile <file.tisc> [--policy <file>] [--json] Execute and report runtime/memory metrics
  explain-trap <file.tisc> [--policy <file>] [--json]
                                                 Explain the resulting VM trap, if any

Examples:
  t81 vm run build/hello.tisc
  t81 vm step build/hello.tisc --count 5 --json
  t81 vm regs build/hello.tisc --steps 3
  t81 vm trace build/hello.tisc -o hello.trace
  t81 vm until build/hello.tisc --pc 4 --steps 16 --json
  t81 vm state build/hello.tisc --json
)";
}

void print_help_tisc() {
  std::cerr << R"(
Usage: t81 tisc <action> [args]

Actions:
  disasm <file.tisc>                 Print human-readable TISC disassembly
  validate <file.tisc> [--json]      Validate that a TISC artifact loads cleanly
  stats <file.tisc> [--json]         Report opcode frequency distribution
  encode <file.base81> [-o <out.tisc>] [--json]
                                     Encode Base81 symbolic TISC text into .tisc
  decode <file.tisc> [-o <out.base81>] [--json]
                                     Render a .tisc artifact as canonical Base81 symbolic text
  diff <a.tisc> <b.tisc> [--json]    Compare two TISC artifacts at the instruction level

Examples:
  t81 tisc disasm build/hello.tisc
  t81 tisc validate build/hello.tisc --json
  t81 tisc stats build/hello.tisc --json
  t81 tisc encode build/hello.base81 -o build/hello.tisc
  t81 tisc decode build/hello.tisc -o build/hello.base81
  t81 tisc diff build/v1.tisc build/v2.tisc
)";
}

void print_help_ir() {
  std::cerr << R"(
Usage: t81 ir <action> [args]

Actions:
  show <file.t81>                    Lower source to IR and print instructions
  dump <file.t81>                    Alias for show
  validate <file.t81> [--json]       Validate parsing, semantics, and IR lowering
  export <file.t81> [--json] [-o <file>]
                                    Lower source to machine-readable IR JSON

Examples:
  t81 ir show examples/core-language/hello_world.t81
  t81 ir validate examples/core-language/hello_world.t81 --json
  t81 ir export examples/core-language/hello_world.t81 --json
)";
}

void print_help_determinism() {
  std::cerr << R"(
Usage: t81 determinism <action> [args]

Actions:
  verify [dir]                        Verify a baseline.json directory, or run the fixture gate
  verify-run <file.tisc> [--policy <file>] [--json]
                                     Execute a program twice and compare outputs/traces
  certify <file.tisc> [--policy <file>] [--json]
                                     Emit a reproducibility certificate for one program artifact
  explain <file.tisc> [--policy <file>] [--json]
                                     Explain where repeated executions diverge, if they do
  compare-run <file.tisc> [--policy <file>] [--json]
                                     Emit run-to-run comparison details for one artifact
  hash <file> [--json]                Compute SHA3-512 of an artifact
  trace-hash <trace.txt> [--json]     Compute SHA3-512 of canonicalized trace lines
  diff <lhs> <rhs> [--json]           Compare two artifacts by exact bytes
  diff-trace <lhs> <rhs> [--json]     Compare canonicalized trace content
  multi-run <file.tisc> --count <n> [--policy <file>] [--json]
                                     Execute a program N times and report divergence
  baseline <dir> [--source-dir <dir>] [--json]
                                     Create determinism fixture baseline from .tisc files
  bisect <dir> [--json]
                                     Report the first baseline entry that no longer matches current execution

Examples:
  t81 determinism verify
  t81 determinism compare-run build/hello.tisc --json
  t81 determinism hash build/hello.tisc
  t81 determinism multi-run build/hello.tisc --count 5 --json
  t81 determinism trace-hash run.trace --json
  t81 determinism bisect tests/fixtures/baseline --json
  t81 determinism baseline tests/fixtures/baseline
)";
}

void print_help_repro_hash() {
  std::cerr << R"(
Usage: t81 internal repro-hash [fixtures_dir]

Runs the T81Lang reproducibility fixture hash gate.

Example:
  t81 internal repro-hash tests/fixtures/t81lang_determinism
)";
}

void print_help_canonize_tensor() {
  std::cerr << R"(
Usage: t81 internal canonize-tensor <file>

Canonicalizes tensor input into CanonFS object storage.

Note: Prefer `t81 canonfs put-tensor` or `t81 tensor canonize` for this operation.
      `internal canonize-tensor` is retained for backwards compatibility.

Example:
  t81 internal canonize-tensor model.t81w
)";
}

void print_help_init() {
  std::cerr << R"(
Usage: t81 init <project_name>

Scaffolds a new T81 project.

Example:
  t81 init my_project
)";
}

void print_help_pkg() {
  std::cerr << R"(
Usage: t81 internal pkg <subcommand> [args]

Subcommands:
  init [package_name]
  check [package.t81] [--json]

Examples:
  t81 internal pkg init my_pkg
  t81 internal pkg check
  t81 internal pkg check --json
)";
}

void print_help_benchmark() {
  std::cerr << R"(
Usage: t81 internal benchmark [vm|canonfs|weights|determinism] [benchmark_runner_flags...]

Runs the core benchmark suite. By default this uses the benchmark runner's smoke
profile. Set `T81_BENCHMARK_PROFILE=full` for the bounded human-usable full
profile, or `T81_BENCHMARK_PROFILE=deep` for the exhaustive research pass.
Benchmark report files are not written unless `T81_BENCHMARK_WRITE_REPORTS=1`.
Subsystem selectors expand to benchmark filters:
  vm            BM_VMSimulation.*
  canonfs       BM_CanonFS.*
  weights       BM_TensorPromotion.*
  determinism   BM_T81LangCompile.*|BM_OverflowDetection.*

Example:
  t81 internal benchmark --benchmark_filter=BM_VMSimulation_Dispatch
  t81 internal benchmark canonfs --benchmark_min_time=0.05
)";
}

void print_help_mlir() {
  std::cerr << R"(
Usage: t81 mlir <subcommand> <input> [-o <output>] [options]

Translate TISC programs to MLIR using standard dialects (arith, math, func,
cf, memref), then optionally lower to LLVM IR.

Subcommands:
  compile  <input.tisc|input.t81>  [-o out.mlir]   Emit MLIR text
  lower    <input.mlir>            [-o out.ll]      Lower MLIR → LLVM IR
  pipeline <input.tisc|input.t81>  [-o out.ll]      TISC → MLIR → LLVM IR

Options:
  -o <path>         Output file (default: derived from input extension)
  --dialect=t81     Emit custom t81.reg_* register access ops
  --dialect=std     Emit standard memref register accesses (default)
  --mode=dcp        DCP float mode: replace math.* ops with func.call
                    @t81_dmath_* — bit-exact when linked against t81_dmath_runtime
  --mode=compat     Standard math.* dialect (IEEE-754 f64, default)
  --no-comments     Omit PC annotations from basic block names
  -h, --help        Show this help

Float modes:
  compat (default)  Uses math.sin, math.cos, etc. — non-DCP surface.
                    Maximum toolchain compatibility; any mlir-opt can process.
  dcp               Replaces math.* with func.call @t81_dmath_* externals.
                    Preserves T81 determinism when t81_dmath_runtime is linked.
                    The MLIR text is still valid standard MLIR.

Dialect modes:
  std (default)     Emits only standard dialect ops.
  t81               Emits custom t81.reg_* ops for register-file access, then
                    lowers them back to memref before LLVM conversion.

Notes:
  Requires build with -DT81_ENABLE_MLIR=ON -DT81_ENABLE_LLVM=ON.
  MLIR output is compatible with: mlir-opt, mlir-cpu-runner, LLVM ORC JIT.
  Use 't81 llvm compile' for direct TISC → LLVM IR (no MLIR intermediate).
)";
}

void print_help_llvm() {
  std::cerr << R"(
Usage: t81 llvm compile <input.tisc|input.t81> -o <output> [options]

Translate a TISC program to LLVM IR or bitcode.

Arguments:
  <input>                Input file: .tisc binary or .t81 source (compiled first)
  -o <output>            Output file (.ll for IR text, .bc for bitcode)

Options:
  --bitcode              Emit LLVM bitcode (.bc) instead of IR text (.ll)
  --no-comments          Suppress metadata comments in generated IR
  -h, --help             Show this help

Notes:
  Requires build with -DT81_ENABLE_LLVM=ON.
  The generated @t81_main() function uses a flat [243 x i64] integer register
  file and [243 x double] float register file mirroring the T81VM layout.
  Invoke the IR with: lli output.ll  or compile with: llc -o output.s output.ll
)";
}

void print_help_c() {
  std::cerr << R"(
Usage: t81 c compile <input.c> [-o <output>] [options]

Compile a tightly restricted C subset to MLIR through the T81 compiler stack.

Arguments:
  <input>                Input C source file (.c)
  -o <output>            Output MLIR file (.mlir)

Options:
  --emit mlir            Emit MLIR text (the only supported mode in v0)
  --dialect=t81          Emit custom t81.* ops where applicable
  --dialect=std          Emit standard MLIR ops (default)
  --mode=dcp             Select DCP float lowering mode when supported
  --mode=compat          Select standard math lowering mode (default)
  --no-comments          Suppress metadata comments in generated MLIR
  -h, --help             Show this help

Current subset v0:
  - exactly one entry function: int main()
  - additional int helper functions with int parameters
  - local int variables with initializers
  - same-translation-unit calls without recursion
  - integer literals, assignment, arithmetic, and comparisons
  - compound blocks, if, while, for, and reachable return

Notes:
  Requires build with -DT81_ENABLE_C_FRONTEND=ON -DT81_ENABLE_MLIR=ON -DT81_ENABLE_LLVM=ON.
  Unsupported constructs fail closed with explicit diagnostics.
)";
}

void print_help_rust() {
  std::cerr << R"(
Usage: t81 rust compile <input.rs> [-o <output>] [options]

Compile an experimental Rust-subset ingress path to MLIR.

Arguments:
  <input>                Input Rust source file (.rs)
  -o <output>            Output MLIR file (.mlir)

Options:
  --emit mlir            Emit MLIR text (the only planned mode in v0)
  --dialect=t81          Emit custom t81.* ops where applicable
  --dialect=std          Emit standard MLIR ops (default)
  --mode=dcp             Select DCP float lowering mode when supported
  --mode=compat          Select standard math lowering mode (default)
  --no-comments          Suppress metadata comments in generated MLIR
  -h, --help             Show this help

Status:
  - constrained Rust subset is implemented experimentally
  - build requires -DT81_ENABLE_RUST_FRONTEND=ON -DT81_ENABLE_C_FRONTEND=ON -DT81_ENABLE_MLIR=ON -DT81_ENABLE_LLVM=ON
  - a Rust toolchain with rustc on PATH is required
  - accepted subset v0: fn main() -> i32, i32 helpers, let bindings,
    fixed local [i32; N] arrays with compile-time constant indexing,
    assignment, arithmetic/bitwise/comparison/logical expressions,
    if/else, while, and return
  - unsupported constructs fail closed with explicit diagnostics
)";
}

void print_help_python() {
  std::cerr << R"(
Usage: t81 python compile <input.py> [-o <output>] [options]

Compile an experimental Python-subset ingress path to MLIR.

Arguments:
  <input>                Input Python source file (.py)
  -o <output>            Output MLIR file (.mlir)

Options:
  --emit mlir            Emit MLIR text (the only planned mode in v0)
  --dialect=t81          Emit custom t81.* ops where applicable
  --dialect=std          Emit standard MLIR ops (default)
  --mode=dcp             Select DCP float lowering mode when supported
  --mode=compat          Select standard math lowering mode (default)
  --no-comments          Suppress metadata comments in generated MLIR
  -h, --help             Show this help

Status:
  - minimal scalar Python subset is implemented experimentally
  - build requires -DT81_ENABLE_PYTHON_FRONTEND=ON -DT81_ENABLE_C_FRONTEND=ON -DT81_ENABLE_MLIR=ON -DT81_ENABLE_LLVM=ON
  - accepted subset v0: def main() -> int, int helpers, annotated local bindings,
    fixed local list literals with compile-time constant indexing,
    assignment, arithmetic/bitwise/comparison/logical expressions,
    if/else, while, same-file helper calls, and return
  - unsupported constructs fail closed with explicit diagnostics
)";
}

void print_help_advanced() {
  std::cerr << R"(
Advanced commands (supported expert workflows):
  weights <subcommand> [args]         Model weights import/info/verify/quantize tools
  model <subcommand> [args]           Model artifact intake and review-oriented inspection
  policy <subcommand> [args]          Axion policy compile/run/test tools
  axion <subcommand> [args]           Axion governor: status/optimize/simulate/explain/snapshot/snapshot-diff/rollback
  trace <subcommand> [args]           Trace inspection, diff, replay, canonicalize, export
  determinism <action> [args]         Determinism verification and hashing
  canonfs <action> [args]             CanonFS inspection and snapshot tooling
  artifact <action> [args]            Fixed-schema artifact validation helpers
  vm <action> [args]                  VM-focused run/debug/trace/state/profile entry points
  tisc <action> [args]                TISC artifact inspection, validation, encode/decode
  ir <action> [args]                  IR inspection and export for frontend lowering
  c <subcommand> [args]               Experimental C-subset frontend to MLIR
  rust <subcommand> [args]            Experimental Rust-subset frontend to MLIR
  python <subcommand> [args]          Experimental Python-subset frontend to MLIR
  llvm <subcommand> [args]            LLVM IR backend: translate TISC to LLVM IR/bitcode
  mlir <subcommand> [args]            MLIR frontend: translate TISC to MLIR / LLVM IR

Use:
  t81 help weights
  t81 help model
  t81 help policy
  t81 help axion
  t81 help trace
  t81 help determinism
  t81 help canonfs
  t81 help artifact
  t81 help vm
  t81 help tisc
  t81 help ir
  t81 help c
  t81 help llvm
  t81 help mlir
)";
}

void print_help_labs() {
  std::cerr << R"(
Labs/internal commands (experimental or operations-focused):
  pkg <subcommand> [args]              Experimental package manifest helpers
  benchmark                            Internal benchmark runner entrypoint
  repro-hash [fixtures_dir]            Reproducibility gate helper
  canonize-tensor <file>               CanonFS tensor canonicalization utility
  canonize-file <file>                 CanonFS raw-file canonicalization utility
  memory-stats [file]                  Runtime-backed memory pool profiling
  llama-run ...                        Experimental governed llama.cpp path

These commands are not part of the core beginner workflow.
)";
}

void print_help_weights_import() {
  std::cerr << R"(
Usage: t81 weights import <file> [-o <out>] [--format <safetensors|bitnet|gguf>]

Imports model weights into native `.t81w`.

Bare model names are resolved from the nearest `./model/` or `./models/`
directory when possible.
)";
}

void print_help_model_import() {
  std::cerr << R"(
Usage: t81 model import <file> [--json] [--report] [--format <safetensors|bitnet|gguf>] [-o <out>] [--manifest <file>]

Loads a model artifact through the existing import path and emits a
review-oriented report instead of writing `.t81w`.

Bare model names are resolved from the nearest `./model/` or `./models/`
directory when possible.
)";
}

void print_help_model_diff() {
  std::cerr << R"(
Usage: t81 model diff <lhs> <rhs> [--json] [--mode <raw|normalized>]

Loads model artifacts or `t81.model-manifest.v1` files and compares
artifact format, aggregate counts, tensor membership, and tensor metadata.
)";
}

void print_help_weights_info() {
  std::cerr << R"(
Usage: t81 weights info <model.t81w> [--json]

Prints native model metadata (human-readable or JSON).
)";
}

void print_help_weights_verify() {
  std::cerr << R"(
Usage: t81 weights verify <model.t81w> [--json]

Verifies that a native weights file loads and matches its derived metadata.
)";
}

void print_help_weights_quantize() {
  std::cerr << R"(
Usage: t81 weights quantize <input> --to-gguf <out>

Quantizes SafeTensors input into GGUF output.
)";
}

void print_help_policy_compile() {
  std::cerr << R"(
Usage: t81 policy compile <file.apl> [-o <out>]

Compiles Axion policy source into bytecode (.axionb).
APL uses s-expression syntax. Example: (allow-all)
)";
}

void print_help_policy_run() {
  std::cerr << R"(
Usage: t81 policy run <file.apl|file.axionb> [--json]

Parses/loads policy and reports validation result.
)";
}

void print_help_policy_validate() {
  std::cerr << R"(
Usage: t81 policy validate <file.apl|file.axionb> [--json]

Parses/loads policy and reports structural validation without executing a test case.
)";
}

void print_help_policy_test() {
  std::cerr << R"(
Usage: t81 policy test <file.apl|file.axionb> --model-hash <hash> [--json]

Checks whether a candidate model hash is allowed by the supplied policy.
)";
}

void print_help_policy_list() {
  std::cerr << R"(
Usage: t81 policy list [--json] [--dir <path>]

Lists .apl policy files found in the project or the specified directory.
)";
}

void print_help_env_check() {
  std::cerr << R"(
Usage: t81 env check [--json]

Quick pass/fail readiness check. Exits 0 if the build environment is ready,
1 if any critical check fails. No verbose output.
)";
}

void print_help_tisc_diff() {
  std::cerr << R"(
Usage: t81 tisc diff <a.tisc> <b.tisc> [--json]

Compares two TISC artifacts at the instruction level and reports differences.
)";
}

void print_help_axion_log() {
  std::cerr << R"(
Usage: t81 axion log [--json] [--tail <n>]

Shows the current Axion state and persisted CanonFS snapshot history.
Options:
  --json       Machine-readable output (schema: t81.axion-log.v1)
  --tail <n>   Limit output to the last N snapshot entries (default: 10)
)";
}

void print_help_determinism_baseline() {
  std::cerr << R"(
Usage: t81 determinism baseline <dir> [--source-dir <src>] [--json]

Scans <dir> (or --source-dir) for .tisc files, executes each once, and writes
artifact/output/trace SHA3-512 hashes to <dir>/baseline.json.
Use `t81 determinism verify <dir>` to compare current executions against the baseline.
)";
}

void print_help_canonfs_gc() {
  std::cerr << R"(
Usage: t81 canonfs gc [--json] [--canonfs-root <path>]

Scans the CanonFS object store and removes objects not referenced by any
snapshot. Prints a summary of bytes reclaimed.
)";
}

void print_help_canonfs_fsck() {
  std::cerr << R"(
Usage: t81 canonfs fsck [--verify-hashes] [--json] [--canonfs-root <path>]

Verifies that CanonFS object blocks are readable and that each snapshot contains
its manifest. Returns non-zero when corruption or missing metadata is detected.
)";
}

void print_help_canonfs_repair() {
  std::cerr << R"(
Usage: t81 canonfs repair [--dry-run] [--json] [--canonfs-root <path>]

Removes legacy snapshot-local `objects/` directories left behind by older snapshot implementations.
The immutable object store at `<canonfs-root>/objects` is preserved.
Symlinked paths under snapshot storage are rejected.
)";
}

void print_help_trace_show() {
  std::cerr << R"(
Usage: t81 trace show <trace.txt> [--no-color]
)";
}

void print_help_trace_diff() {
  std::cerr << R"(
Usage: t81 trace diff <trace1.txt> <trace2.txt> [--no-color]
)";
}

void print_help_trace_replay() {
  std::cerr << R"(
Usage: t81 trace replay <file.tisc> <trace.txt> [--json]
)";
}

void print_help_trace_canonicalize() {
  std::cerr << R"(
Usage: t81 trace canonicalize <trace.txt> [-o <file>]

Normalizes trace content into a replay-safe canonical form.
)";
}

void print_help_trace_export() {
  std::cerr << R"(
Usage: t81 trace export <trace.txt> [--format <json|csv>] [-o <file>]
)";
}

void print_help_trace_summary(std::string_view subcommand) {
  std::cerr << "Usage: t81 trace " << subcommand << " <trace.txt> [--json]\n\n"
            << "Summarizes opcode, trap, and line-count information for a trace.\n";
}

void print_help_trace_filter() {
  std::cerr << R"(
Usage: t81 trace filter <trace.txt> [--opcode <name>] [--trap <name>] [--pc-start <n>] [--pc-end <n>] [--json]

Filters replay-safe trace lines by opcode, trap, or PC range.
)";
}

void print_help_vm_trace() {
  std::cerr << R"(
Usage: t81 vm trace <file.tisc> [--policy <file>] [-o <trace.txt>] [--output <trace.txt>]

Executes a TISC program and writes replay-safe trace output.
)";
}

void print_help_tier() {
  std::cerr << R"(
Usage: t81 tier <info|check|gate> [args]

Actions:
  info [--json]                      Show supported tier labels
  check <file.tisc> [--json]         Report the required tier inferred for a program
  gate <file.tisc> --max-tier <n> [--json]
                                     Fail if the program exceeds the allowed tier

Examples:
  t81 tier info
  t81 tier check build/hello.tisc --json
  t81 tier gate build/hello.tisc --max-tier 2
)";
}

void print_help_lang() {
  std::cerr << R"(
Usage: t81 lang <action> [args]

T81 language frontend workflow surface.

Actions:
  check|lint|fmt|build|run|test|disasm|debug|repl|profile
  show|dump|export|validate                IR lowering and export helpers

Examples:
  t81 lang build examples/core-language/hello_world.t81 -o build/hello.tisc
  t81 lang validate examples/core-language/hello_world.t81 --json
)";
}

void print_help_tensor() {
  std::cerr << R"(
Usage: t81 tensor <subcommand> [args]

Subcommands:
  canonize <file>         Canonicalize tensor/model payload into CanonFS
  hash <file> [--json]    Compute SHA3-512 for a tensor artifact
  inspect <model.t81w> [--json]
                          Inspect native tensor model metadata

Examples:
  t81 tensor canonize model.t81w
  t81 tensor hash model.t81w --json
  t81 tensor inspect model.t81w
)";
}

void print_help_ai() { t81::cli::ai::print_usage("t81 ai"); }

void print_help_mcp() {
  std::cerr << R"(Usage: t81 mcp <action> [options]

Native C++ JSON-RPC 2.0 MCP server (zero Python runtime dependency).

Actions:
  serve [--root PATH]...                Start stdio JSON-RPC 2.0 MCP server (15 native tools)

File tools accept paths only inside a --root directory after symlink and ".." resolution.
With no --root, the working directory is the root. Repeat --root to allow more than one tree.

Native MCP tools exposed by `t81 mcp serve`:
  Governance & Receipts:
    t81.session_bootstrap               1-call workspace ground-truth & host SIMD/GPU capabilities
    t81.policy_validate                 Validate an Axion .apl policy file
    t81.policy_test                     Test whether a model/artifact hash is admitted by .apl
    t81.get_effective_policy            Resolve effective policy floor for an action type
    t81.gate_check                      Fail-closed anti-downgrade artifact gate with SHA3-512
    t81.canonfs_fsck                    Verify CanonFS object hashes and snapshots in-process
    t81.determinism_hash                Compute canonical SHA3-512 digest of a file

  Atomic Transactions:
    t81.tx_begin / t81.tx_commit / t81.tx_abort
                                        SHA3-512 verified backup, policy-gated commit, exact rollback

  Subgraphs & Dual-Rail Ternary AI:
    t81.verify_subgraph                 Static verifier for narrowed TISC tensor+policy subgraphs
    t81.run_subgraph                    Verify + execute TISC subgraph in T81VM with SHA3-512 receipt
    t81.diff_check                      Exact Hamming flip-budget audit on Dual-Rail (P, N) planes
    t81.memory_search                   Zero-float deterministic top-K Dual-Rail RAG with receipt
    t81.adapter_apply                   Zero-dequantization saturated adapter compose (dense or SBDR1)
)";
}

void print_usage(const char* prog) {
  std::cerr << R"(T81 Foundation - Ternary-Native Computing Stack
Version )" << T81_VERSION_STR
            << R"(


Usage: )" << prog
            << R"( <command> [options] [args]


Core Workflow:
  code    <action> [args]               Primary code workflow commands
  lang    <action> [args]               T81 language frontend workflow commands
  project <action> [args]               Project lifecycle commands
  vm <action> [args]                    VM execution and state inspection
  tisc <action> [args]                  TISC artifact inspection and validation
  ir <action> [args]                    IR inspection for frontend lowering
  env     <action> [args]               Environment/toolchain diagnostics

Governance, Verification & Storage:
  policy <action> [args]                Axion policy compile, validate, and test
  axion <action> [args]                 Axion governor and policy-facing operations
  canonfs <action> [args]               CanonFS inspection and snapshot commands
  determinism <action> [args]           Determinism verification and hashing commands
  trace <action> [args]                 Trace inspection, replay, export, canonicalization
  tier   <action> [args]                Cognitive tier inspection and gating

AI, Tensors & Ternary Weights:
  ai     <action> [args]                Bounded AI tasks, model inspection, and partial inference tools
  weights <action> [args]               Model weights import, inspect, and verify
  model  <action> [args]                Model artifact intake and inspection
  tensor <action> [args]                Tensor artifact canonicalization and inspection

Agent, MCP & Interactive:
  mcp     <action> [args]               Native C++ MCP server (`t81 mcp serve`, 15 tools)
  repl                                  Start interactive REPL
  studio                                Human Operator TUI
  agent   [--resume <f>] [--session <f>] AI-Native / Agentic TUI
  ui                                    Interactive TUI launcher (choose studio/agent)

Experimental Frontends, Backends & Utilities:
  c     <subcommand> [args]             Experimental C-subset frontend to MLIR
  rust  <subcommand> [args]             Experimental Rust-subset frontend to MLIR
  python <subcommand> [args]            Experimental Python-subset frontend to MLIR
  llvm  <subcommand> [args]             LLVM IR backend: translate TISC to LLVM IR/bitcode
  mlir  <subcommand> [args]             MLIR frontend: translate TISC to MLIR / LLVM IR
  internal <action> [args]              Internal/experimental command group
  completion <shell>                    Print shell completion script
  man [--install-dir <dir>]             Show or install CLI manpage
  feedback <subcommand> [args]          Local CLI UX feedback loop
  version                              Show version
  help [command]                       Show help for command/topic


Global options:
  -V, --version                        Show version
  -v, --verbose                        Verbose diagnostic output
  -q, --quiet                          Suppress non-error output
  -h, --help                           Show help

Diagnostics:
  `t81 code build` now prints any semantic or parsing errors with the originating
  source file, line, and column so you can jump directly to the issue without
  rerunning separate diagnostics.

More command groups:
  t81 help code
  t81 help project
  t81 help env
  t81 help mcp
  t81 help internal
  t81 help advanced
  t81 help canonfs
  t81 help determinism
  t81 help vm
  t81 help tisc
  t81 help ir
  t81 help tier
  t81 help ai
  t81 help weights
  t81 help model
  t81 help policy
  t81 help axion
  t81 help trace
  t81 help c
  t81 help rust
  t81 help python
  t81 help llvm
  t81 help mlir
  t81 help labs

)";
}

bool print_help_topic(std::string_view topic, const char* prog) {
  // Removed topics: return false so caller can emit recommendation/error.
  if (topic == "compile" || topic == "run" || topic == "profile" || topic == "disasm" ||
      topic == "debug" || topic == "check" || topic == "lint") {
    return false;
  }
  if (topic == "code") {
    print_help_code();
    return true;
  }
  if (topic == "lang") {
    print_help_lang();
    return true;
  }
  if (topic == "project") {
    print_help_project();
    return true;
  }
  if (topic == "env") {
    print_help_env();
    return true;
  }
  if (topic == "internal") {
    print_help_internal();
    return true;
  }
  if (topic == "weights") {
    print_help_weights();
    return true;
  }
  if (topic == "model") {
    print_help_model();
    return true;
  }
  if (topic == "tensor") {
    print_help_tensor();
    return true;
  }
  if (topic == "ai") {
    print_help_ai();
    return true;
  }
  if (topic == "mcp") {
    print_help_mcp();
    return true;
  }
  if (topic == "policy") {
    print_help_policy();
    return true;
  }
  if (topic == "axion") {
    print_help_axion();
    return true;
  }
  if (topic == "trace") {
    print_help_trace();
    return true;
  }
  if (topic == "repl") {
    print_help_repl();
    return true;
  }
  if (topic == "completion") {
    print_help_completion();
    return true;
  }
  if (topic == "man") {
    print_help_man();
    return true;
  }
  if (topic == "feedback") {
    print_help_feedback();
    return true;
  }
  if (topic == "tui" || topic == "studio" || topic == "agent" || topic == "ui") {
    print_help_tui();
    return true;
  }
  if (topic == "benchmark") {
    print_help_benchmark();
    return true;
  }
  if (topic == "canonfs") {
    print_help_canonfs();
    return true;
  }
  if (topic == "artifact") {
    print_help_artifact();
    return true;
  }
  if (topic == "vm") {
    print_help_vm();
    return true;
  }
  if (topic == "tisc") {
    print_help_tisc();
    return true;
  }
  if (topic == "ir") {
    print_help_ir();
    return true;
  }
  if (topic == "c") {
    print_help_c();
    return true;
  }
  if (topic == "rust") {
    print_help_rust();
    return true;
  }
  if (topic == "python") {
    print_help_python();
    return true;
  }
  if (topic == "tier") {
    print_help_tier();
    return true;
  }
  if (topic == "determinism") {
    print_help_determinism();
    return true;
  }
  if (topic == "version" || topic == "--version" || topic == "-V") {
    print_version();
    return true;
  }
  if (topic == "advanced") {
    print_help_advanced();
    return true;
  }
  if (topic == "labs") {
    print_help_labs();
    return true;
  }
  if (topic == "llvm") {
    print_help_llvm();
    return true;
  }
  if (topic == "mlir") {
    print_help_mlir();
    return true;
  }
  if (topic == "help" || topic.empty()) {
    print_usage(prog);
    return true;
  }
  return false;
}

bool print_family_subcommand_help(std::string_view family, std::string_view sub) {
  if (family == "code" || family == "project") {
    if (sub == "build") {
      print_help_compile();
      return true;
    }
    if (sub == "check" || sub == "lint") {
      print_help_check();
      return true;
    }
    if (sub == "fmt") {
      print_help_fmt();
      return true;
    }
    if (sub == "run") {
      print_help_run();
      return true;
    }
    if (sub == "test") {
      print_help_test();
      return true;
    }
    if (sub == "disasm") {
      print_help_disasm();
      return true;
    }
    if (sub == "debug") {
      print_help_debug();
      return true;
    }
    if (sub == "repl") {
      print_help_repl();
      return true;
    }
    if (sub == "profile") {
      print_help_profile();
      return true;
    }
    if (family == "project" && sub == "init") {
      print_help_init();
      return true;
    }
  }
  if (family == "lang") {
    if (sub == "build") {
      print_help_compile();
      return true;
    }
    if (sub == "check" || sub == "lint") {
      print_help_check();
      return true;
    }
    if (sub == "fmt") {
      print_help_fmt();
      return true;
    }
    if (sub == "run") {
      print_help_run();
      return true;
    }
    if (sub == "test") {
      print_help_test();
      return true;
    }
    if (sub == "disasm") {
      print_help_disasm();
      return true;
    }
    if (sub == "debug") {
      print_help_debug();
      return true;
    }
    if (sub == "repl") {
      print_help_repl();
      return true;
    }
    if (sub == "show" || sub == "dump" || sub == "export") {
      print_help_ir();
      return true;
    }
    if (sub == "validate") {
      std::cerr << "Usage: t81 lang validate <file.t81> [--json]\n";
      return true;
    }
  }
  if (family == "weights") {
    if (sub == "import") {
      print_help_weights_import();
      return true;
    }
    if (sub == "info") {
      print_help_weights_info();
      return true;
    }
    if (sub == "verify") {
      print_help_weights_verify();
      return true;
    }
    if (sub == "quantize") {
      print_help_weights_quantize();
      return true;
    }
    if (sub == "export") {
      std::cerr << "Usage: t81 weights export <model.t81w> --to-safetensors <out> [--json]\n";
      return true;
    }
  }
  if (family == "model") {
    if (sub == "import") {
      print_help_model_import();
      return true;
    }
    if (sub == "diff") {
      print_help_model_diff();
      return true;
    }
  }
  if (family == "policy") {
    if (sub == "compile") {
      print_help_policy_compile();
      return true;
    }
    if (sub == "validate") {
      print_help_policy_validate();
      return true;
    }
    if (sub == "run") {
      print_help_policy_run();
      return true;
    }
    if (sub == "test") {
      print_help_policy_test();
      return true;
    }
    if (sub == "list") {
      print_help_policy_list();
      return true;
    }
  }
  if (family == "trace") {
    if (sub == "show") {
      print_help_trace_show();
      return true;
    }
    if (sub == "diff") {
      print_help_trace_diff();
      return true;
    }
    if (sub == "replay") {
      print_help_trace_replay();
      return true;
    }
    if (sub == "summary" || sub == "stats") {
      print_help_trace_summary(sub);
      return true;
    }
    if (sub == "filter") {
      print_help_trace_filter();
      return true;
    }
    if (sub == "canonicalize") {
      print_help_trace_canonicalize();
      return true;
    }
    if (sub == "export") {
      print_help_trace_export();
      return true;
    }
  }
  if (family == "vm") {
    if (sub == "run") {
      std::cerr << "Usage: t81 vm run <file.tisc> [--policy <file>] [-o <trace.txt>]\n";
      return true;
    }
    if (sub == "debug") {
      std::cerr << "Usage: t81 vm debug <file.tisc> [--policy <file>]\n";
      return true;
    }
    if (sub == "trace") {
      print_help_vm_trace();
      return true;
    }
    if (sub == "until") {
      std::cerr
          << "Usage: t81 vm until <file.tisc> --pc <n> [--policy <file>] [--steps N] [--json]\n";
      return true;
    }
    if (sub == "step") {
      std::cerr << "Usage: t81 vm step <file.tisc> [--policy <file>] [--count N] [--json]\n";
      return true;
    }
    if (sub == "regs") {
      std::cerr << "Usage: t81 vm regs <file.tisc> [--policy <file>] [--steps N] [--json]\n";
      return true;
    }
    if (sub == "stack") {
      std::cerr
          << "Usage: t81 vm stack <file.tisc> [--policy <file>] [--steps N] [--limit N] [--json]\n";
      return true;
    }
    if (sub == "mem") {
      std::cerr
          << "Usage: t81 vm mem <file.tisc> --addr <n> [--policy <file>] [--steps N] [--json]\n";
      return true;
    }
    if (sub == "state") {
      std::cerr << "Usage: t81 vm state <file.tisc> [--policy <file>] [--json]\n";
      return true;
    }
    if (sub == "profile") {
      std::cerr << "Usage: t81 vm profile <file.tisc> [--policy <file>] [--json]\n";
      return true;
    }
    if (sub == "explain-trap") {
      std::cerr << "Usage: t81 vm explain-trap <file.tisc> [--policy <file>] [--json]\n";
      return true;
    }
  }
  if (family == "tisc") {
    if (sub == "disasm") {
      std::cerr << "Usage: t81 tisc disasm <file.tisc>\n";
      return true;
    }
    if (sub == "validate") {
      std::cerr << "Usage: t81 tisc validate <file.tisc> [--json]\n";
      return true;
    }
    if (sub == "stats") {
      std::cerr << "Usage: t81 tisc stats <file.tisc> [--json]\n";
      return true;
    }
    if (sub == "encode") {
      std::cerr << "Usage: t81 tisc encode <file.base81> [-o <out.tisc>] [--json]\n";
      return true;
    }
    if (sub == "decode") {
      std::cerr << "Usage: t81 tisc decode <file.tisc> [-o <out.base81>] [--json]\n";
      return true;
    }
    if (sub == "diff") {
      print_help_tisc_diff();
      return true;
    }
  }
  if (family == "determinism") {
    if (sub == "verify") {
      std::cerr << "Usage: t81 determinism verify [fixtures_dir]\n";
      return true;
    }
    if (sub == "verify-run") {
      std::cerr << "Usage: t81 determinism verify-run <file.tisc> [--policy <file>] [--json]\n";
      return true;
    }
    if (sub == "compare-run") {
      std::cerr << "Usage: t81 determinism compare-run <file.tisc> [--policy <file>] [--json]\n";
      return true;
    }
    if (sub == "certify") {
      std::cerr << "Usage: t81 determinism certify <file.tisc> [--policy <file>] [--json]\n";
      return true;
    }
    if (sub == "explain") {
      std::cerr << "Usage: t81 determinism explain <file.tisc> [--policy <file>] [--json]\n";
      return true;
    }
    if (sub == "hash") {
      std::cerr << "Usage: t81 determinism hash <file> [--json]\n";
      return true;
    }
    if (sub == "trace-hash") {
      std::cerr << "Usage: t81 determinism trace-hash <trace.txt> [--json]\n";
      return true;
    }
    if (sub == "diff") {
      std::cerr << "Usage: t81 determinism diff <lhs> <rhs> [--json]\n";
      return true;
    }
    if (sub == "diff-trace") {
      std::cerr << "Usage: t81 determinism diff-trace <lhs> <rhs> [--json]\n";
      return true;
    }
    if (sub == "baseline") {
      print_help_determinism_baseline();
      return true;
    }
    if (sub == "bisect") {
      std::cerr << "Usage: t81 determinism bisect <dir> [--json]\n";
      return true;
    }
    if (sub == "multi-run") {
      std::cerr << "Usage: t81 determinism multi-run <file.tisc> --count <n> [--policy <file>] "
                   "[--json]\n";
      return true;
    }
  }
  if (family == "axion") {
    if (sub == "status") {
      std::cerr << "Usage: t81 axion status [--json]\n";
      return true;
    }
    if (sub == "optimize") {
      std::cerr << "Usage: t81 axion optimize [--tier N] [--json]\n";
      return true;
    }
    if (sub == "simulate") {
      std::cerr << "Usage: t81 axion simulate <file.tisc> [--json]\n";
      return true;
    }
    if (sub == "explain") {
      std::cerr << "Usage: t81 axion explain <file.apl|file.axionb> [--json]\n";
      return true;
    }
    if (sub == "snapshot") {
      std::cerr << "Usage: t81 axion snapshot [--json]\n";
      return true;
    }
    if (sub == "snapshot-diff") {
      std::cerr << "Usage: t81 axion snapshot-diff <lhs> <rhs> [--json]\n";
      return true;
    }
    if (sub == "rollback") {
      std::cerr << "Usage: t81 axion rollback --to <hash> [--json]\n";
      return true;
    }
    if (sub == "log") {
      print_help_axion_log();
      return true;
    }
    if (sub == "audit") {
      std::cerr << "Usage: t81 axion audit [--from <hash>] [--to <hash>] [--json]\n";
      return true;
    }
  }
  if (family == "tensor") {
    if (sub == "canonize") {
      std::cerr << "Usage: t81 tensor canonize <file>\n";
      return true;
    }
    if (sub == "hash") {
      std::cerr << "Usage: t81 tensor hash <file> [--json]\n";
      return true;
    }
    if (sub == "inspect") {
      std::cerr << "Usage: t81 tensor inspect <model.t81w> [--json]\n";
      return true;
    }
  }
  if (family == "env") {
    if (sub == "check") {
      print_help_env_check();
      return true;
    }
    if (sub == "doctor") {
      print_help_doctor();
      return true;
    }
    if (sub == "diag") {
      print_help_env_diag();
      return true;
    }
    if (sub == "paths" || sub == "toolchain") {
      print_help_env();
      return true;
    }
    if (sub == "feedback") {
      print_help_feedback();
      return true;
    }
    if (sub == "clean") {
      std::cerr << "Usage: t81 env clean [--build-dir <path>] [--force] [--json]\n";
      return true;
    }
  }
  if (family == "canonfs") {
    if (sub == "put-file") {
      std::cerr << "Usage: t81 canonfs put-file <file> [--canonfs-root <path>]\n";
      return true;
    }
    if (sub == "put-tensor") {
      std::cerr << "Usage: t81 canonfs put-tensor <file> [--canonfs-root <path>]\n";
      return true;
    }
    if (sub == "ls") {
      std::cerr << "Usage: t81 canonfs ls [--json] [--canonfs-root <path>]\n";
      return true;
    }
    if (sub == "get") {
      std::cerr << "Usage: t81 canonfs get <sha3-256:hash> [-o <file>] [--json] [--canonfs-root "
                   "<path>]\n";
      return true;
    }
    if (sub == "stat") {
      std::cerr << "Usage: t81 canonfs stat <sha3-256:hash> [--json] [--canonfs-root <path>]\n";
      return true;
    }
    if (sub == "verify") {
      std::cerr << "Usage: t81 canonfs verify <sha3-256:hash> [--json] [--canonfs-root <path>]\n";
      return true;
    }
    if (sub == "snapshot") {
      std::cerr << "Usage: t81 canonfs snapshot [--json] [--canonfs-root <path>]\n";
      return true;
    }
    if (sub == "snapshot-diff") {
      std::cerr
          << "Usage: t81 canonfs snapshot-diff <lhs> <rhs> [--json] [--canonfs-root <path>]\n";
      return true;
    }
    if (sub == "rollback") {
      std::cerr << "Usage: t81 canonfs rollback --to <hash> [--dry-run] [--json] [--canonfs-root "
                   "<path>]\n";
      return true;
    }
    if (sub == "gc") {
      print_help_canonfs_gc();
      return true;
    }
    if (sub == "fsck") {
      print_help_canonfs_fsck();
      return true;
    }
    if (sub == "repair") {
      print_help_canonfs_repair();
      return true;
    }
  }
  if (family == "ir" && sub == "validate") {
    std::cerr << "Usage: t81 ir validate <file.t81> [--json]\n";
    return true;
  }
  if (family == "tier") {
    if (sub == "info") {
      print_help_tier();
      return true;
    }
    if (sub == "check") {
      std::cerr << "Usage: t81 tier check <file.tisc> [--json]\n";
      return true;
    }
    if (sub == "gate") {
      std::cerr << "Usage: t81 tier gate <file.tisc> --max-tier <n> [--json]\n";
      return true;
    }
  }
  return false;
}

bool handle_help_request(const std::string& command, const std::vector<std::string>& command_args,
                         const char* prog) {
  if (command == "help") {
    if (command_args.empty()) {
      print_usage(prog);
      return true;
    }
    if (command_args.size() >= 2 &&
        print_family_subcommand_help(command_args[0], command_args[1])) {
      return true;
    }
    if (print_help_topic(command_args[0], prog)) {
      return true;
    }
    return false;
  }

  if (command.empty()) {
    print_usage(prog);
    return true;
  }
  if (!command_args.empty() && print_family_subcommand_help(command, command_args[0])) {
    return true;
  }
  if (print_help_topic(command, prog)) {
    return true;
  }
  return false;
}
