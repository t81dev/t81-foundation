/**
 * @file main.cpp
 * @brief T81 Foundation Command-Line Interface
 *
 * Sovereign-grade, zero-dependency, ternary-native toolchain driver.
 * MIT + GPL-3.0 dual-licensed.
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <map>
#include <optional>
#include <random>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <vector>
#if !defined(_WIN32)
#include <sys/wait.h>
#include <unistd.h>
#else
#include <io.h>
#endif

#include "internal/tooling/logging.hpp"
#include "t81/axion/policy_validator.hpp"
#include "t81/config.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/cli/driver.hpp"
#include "t81/crypto/sha3.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/base81_view.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/isa/binary_io.hpp"
#include "t81/isa/encoding.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/tracing/canonhash.hpp"
#include "t81/vm/vm.hpp"
#include "core/vm/internal/memory_segments.hpp"
#include "t81/weights.hpp"
#if defined(T81_HAS_LLAMA_CPP)
#include "t81/experimental/llama_cpp_adapter.hpp"
#endif
#if defined(T81_HAS_TUI)
#include "tooling/tui/studio.hpp"
#include "tooling/tui/agent.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#endif

namespace fs = std::filesystem;

// ──────────────────────────────────────────────────────────────
// Version & Build Info
// ──────────────────────────────────────────────────────────────
#define STR(x) #x
#define XSTR(x) STR(x)

constexpr const char* T81_VERSION = T81_VERSION_STR;
constexpr const char* T81_FULL_VERSION =
    "T81 Foundation " T81_VERSION_STR " (" __DATE__ " " __TIME__ ")";
constexpr const char* T81_FMT_VERSION = "t81-fmt " T81_VERSION_STR;
struct TempTiscFile {
  fs::path path;

  explicit TempTiscFile(const std::string& hint = "t81") {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    while (true) {
      path =
          fs::temp_directory_path() / ("t81-" + hint + "-" + std::to_string(dist(gen)) + ".tisc");

      std::string path_str = path.string();
#if defined(_WIN32)
      int fd =
          _open(path_str.c_str(), _O_CREAT | _O_EXCL | _O_RDWR | _O_BINARY, _S_IREAD | _S_IWRITE);
#else
      int fd = open(path_str.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
#endif
      if (fd != -1) {
#if defined(_WIN32)
        _close(fd);
#else
        close(fd);
#endif
        break;
      } else {
        if (errno == EEXIST) {
          continue;
        }
        throw std::runtime_error("Failed to create temporary file: " + path_str +
                                 " (errno: " + std::to_string(errno) + ")");
      }
    }
  }

  ~TempTiscFile() {
    std::error_code ec;
    fs::remove(path, ec);  // best-effort cleanup
  }
};

struct TempCaptureFile {
  fs::path path;

  explicit TempCaptureFile(const std::string& hint, const std::string& ext) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    while (true) {
      path = fs::temp_directory_path() / ("t81-" + hint + "-" + std::to_string(dist(gen)) + ext);
      std::string path_str = path.string();
#if defined(_WIN32)
      int fd =
          _open(path_str.c_str(), _O_CREAT | _O_EXCL | _O_RDWR | _O_BINARY, _S_IREAD | _S_IWRITE);
#else
      int fd = open(path_str.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
#endif
      if (fd != -1) {
#if defined(_WIN32)
        _close(fd);
#else
        close(fd);
#endif
        break;
      }
      if (errno != EEXIST) {
        throw std::runtime_error("Failed to create temporary file: " + path_str +
                                 " (errno: " + std::to_string(errno) + ")");
      }
    }
  }

  ~TempCaptureFile() {
    std::error_code ec;
    fs::remove(path, ec);
  }
};

struct TempBinaryFile {
  fs::path path;

  explicit TempBinaryFile(const std::string& hint, const std::string& ext = ".bin") {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    while (true) {
      path = fs::temp_directory_path() / ("t81-" + hint + "-" + std::to_string(dist(gen)) + ext);

      std::string path_str = path.string();
#if defined(_WIN32)
      int fd =
          _open(path_str.c_str(), _O_CREAT | _O_EXCL | _O_RDWR | _O_BINARY, _S_IREAD | _S_IWRITE);
#else
      int fd = open(path_str.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
#endif
      if (fd != -1) {
#if defined(_WIN32)
        _close(fd);
#else
        close(fd);
#endif
        break;
      }
      if (errno != EEXIST) {
        throw std::runtime_error("Failed to create temporary file: " + path_str +
                                 " (errno: " + std::to_string(errno) + ")");
      }
    }
  }

  ~TempBinaryFile() {
    std::error_code ec;
    fs::remove(path, ec);
  }
};

// ──────────────────────────────────────────────────────────────
// Version & Help
// ──────────────────────────────────────────────────────────────
void print_version() {
  std::cout << T81_FULL_VERSION << R"(
Ternary-Native Computing Stack
Copyright © 2025 T81 Foundation
Licensed under MIT and GPL-3.0
)";
}

struct ScopedStreamRedirect {
  std::ostream& from;
  std::streambuf* old_buf;

  ScopedStreamRedirect(std::ostream& from_stream, std::ostream& to_stream)
      : from(from_stream), old_buf(from_stream.rdbuf(to_stream.rdbuf())) {}

  ~ScopedStreamRedirect() { from.rdbuf(old_buf); }
};

template <typename Fn>
int emit_help(Fn&& fn, int rc = 0) {
  ScopedStreamRedirect redirect(std::cerr, std::cout);
  fn();
  return rc;
}

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
  --format <fmt>       Input format (safetensors, gguf). Default: safetensors
  -o, --output <file>  Output file path

Examples:
  t81 weights import model.safetensors -o model.t81w
  t81 weights info model.t81w --json
  t81 weights verify model.t81w --json
  t81 weights export model.t81w --to-safetensors model.safetensors
  t81 weights quantize model.safetensors --to-gguf model.gguf
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
  log [--json] [--tail <n>]           Show Axion state plus recent snapshot receipts
  audit [--from <hash>] [--to <hash>] Export Axion/CanonFS receipt metadata

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
)";
}

void print_help_compile() {
  std::cerr << R"(
Usage: t81 code build <file.t81|file.t81w> [-o <file.tisc>] [--weights-model <model.t81w>]

Compiles T81Lang source (or T81 weight module source) into TISC bytecode.

Example:
  t81 code build program.t81 -o program.tisc
)";
}

void print_help_run() {
  std::cerr << R"(
Usage: t81 code run <file.t81|file.tisc> [--policy <policy.apl>] [--trace] [-o <file>] [--output <file>] [--weights-model <model.t81w>]

Compiles (if needed) and executes a program via the VM.
`--trace-out` remains accepted for compatibility, but `-o/--output` is canonical.

Example:
  t81 code run program.tisc -o run.trace
)";
}

void print_help_profile() {
  std::cerr << R"(
Usage: t81 code profile <file.t81|file.tisc> [--policy <policy.apl>] [--json] [--weights-model <model.t81w>]

Compiles (if needed) and reports VM runtime and memory metrics.

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
Usage: t81 debug <file.t81|file.tisc> [--policy <policy.apl>] [--weights-model <model.t81w>]

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
Usage: t81 repl [--weights-model <model.t81w>] [--policy <policy.apl>]

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
  t81 fmt examples/hello_world.t81
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
RFC-0033: Dual TUI Frontends for T81

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
  t81 code check examples/hello_world.t81
  t81 code build examples/hello_world.t81 -o hello.tisc
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
  ls [--json] [--canonfs-root <path>]           List stored objects
  get <sha3-256:hash> [-o <file>] [--json] [--canonfs-root <path>]
  stat <sha3-256:hash> [--json] [--canonfs-root <path>]
  verify <sha3-256:hash> [--json] [--canonfs-root <path>]
  snapshot [--json] [--canonfs-root <path>]
  snapshot-diff <lhs> <rhs> [--json] [--canonfs-root <path>]
  rollback --to <hash> [--dry-run] [--json] [--canonfs-root <path>]
  gc [--dry-run] [--json] [--canonfs-root <path>]  Remove unreferenced objects from the store
  fsck [--json] [--canonfs-root <path>]            Verify object readability and snapshot manifests
  repair [--dry-run] [--json] [--canonfs-root <path>]
                                                   Remove legacy snapshot-local object copies

Examples:
  t81 canonfs put-file README.md
  t81 canonfs stat sha3-256:... --json
  t81 canonfs snapshot
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
  t81 ir show examples/hello_world.t81
  t81 ir validate examples/hello_world.t81 --json
  t81 ir export examples/hello_world.t81 --json
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
)";}

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
)";}

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
)";}

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
  - minimal scalar Rust subset is implemented experimentally
  - build requires -DT81_ENABLE_RUST_FRONTEND=ON -DT81_ENABLE_C_FRONTEND=ON -DT81_ENABLE_MLIR=ON -DT81_ENABLE_LLVM=ON
  - a Rust toolchain with rustc on PATH is required
  - accepted subset v0: fn main() -> i32, i32 helpers, let bindings, assignment, arithmetic/bitwise/comparison/logical expressions, if/else, return
  - unsupported constructs fail closed with explicit diagnostics
)";}

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
    assignment, arithmetic/bitwise/comparison/logical expressions,
    if/else, while, same-file helper calls, and return
  - unsupported constructs fail closed with explicit diagnostics
)";}

void print_help_advanced() {
  std::cerr << R"(
Advanced commands (supported expert workflows):
  weights <subcommand> [args]         Model weights import/info/verify/quantize tools
  policy <subcommand> [args]          Axion policy compile/run/test tools
  axion <subcommand> [args]           Axion governor: status/optimize/simulate/explain/snapshot/snapshot-diff/rollback
  trace <subcommand> [args]           Trace inspection, diff, replay, canonicalize, export
  determinism <action> [args]         Determinism verification and hashing
  canonfs <action> [args]             CanonFS inspection and snapshot tooling
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
  t81 help policy
  t81 help axion
  t81 help trace
  t81 help determinism
  t81 help canonfs
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
Usage: t81 weights import <file> [-o <out>] [--format <safetensors|gguf>]

Imports model weights into native `.t81w`.
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

Shows the current Axion state and CanonFS audit trail.
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
Usage: t81 canonfs fsck [--json] [--canonfs-root <path>]

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
  t81 lang build examples/hello_world.t81 -o build/hello.tisc
  t81 lang validate examples/hello_world.t81 --json
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

void print_usage(const char* prog) {
  std::cerr << R"(T81 Foundation - Ternary-Native Computing Stack
Version )" << T81_VERSION
            << R"(


Usage: )" << prog
            << R"( <command> [options] [args]


Commands:
  code    <action> [args]               Primary code workflow commands
  lang    <action> [args]               T81 language frontend workflow commands
  project <action> [args]               Project lifecycle commands
  env     <action> [args]               Environment/toolchain diagnostics
  internal <action> [args]              Internal/experimental command group
  canonfs <action> [args]               CanonFS inspection and snapshot commands
  determinism <action> [args]           Determinism verification and hashing commands
  vm <action> [args]                    VM execution and state inspection
  tisc <action> [args]                  TISC artifact inspection and validation
  ir <action> [args]                    IR inspection for frontend lowering
  tier   <action> [args]                Cognitive tier inspection and gating
  tensor <action> [args]                Tensor artifact canonicalization and inspection
  weights <action> [args]               Model weights import, inspect, and verify
  policy <action> [args]                Axion policy compile, validate, and test
  axion <action> [args]                 Axion governor and policy-facing operations
  trace <action> [args]                 Trace inspection, replay, export, canonicalization
  c     <subcommand> [args]             Experimental C-subset frontend to MLIR
  rust  <subcommand> [args]             Experimental Rust-subset frontend to MLIR
  python <subcommand> [args]            Experimental Python-subset frontend to MLIR
  llvm  <subcommand> [args]             LLVM IR backend: translate TISC to LLVM IR/bitcode
  mlir  <subcommand> [args]             MLIR frontend: translate TISC to MLIR / LLVM IR
  repl                                  Start interactive REPL
  studio                                Human Operator TUI (RFC-0033)
  agent   [--resume <f>] [--session <f>] AI-Native / Agentic TUI (RFC-0033)
  ui                                    Interactive TUI launcher (choose studio/agent)
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
  t81 help internal
  t81 help advanced
  t81 help canonfs
  t81 help determinism
  t81 help vm
  t81 help tisc
  t81 help ir
  t81 help tier
  t81 help weights
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
  if (topic == "compile") {
    print_help_compile();
    return true;
  }
  if (topic == "run") {
    print_help_run();
    return true;
  }
  if (topic == "profile") {
    print_help_profile();
    return true;
  }
  if (topic == "disasm") {
    print_help_disasm();
    return true;
  }
  if (topic == "debug") {
    print_help_debug();
    return true;
  }
  if (topic == "check" || topic == "lint") {
    print_help_check();
    return true;
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
  if (topic == "tensor") {
    print_help_tensor();
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
      std::cerr << "Usage: t81 vm until <file.tisc> --pc <n> [--policy <file>] [--steps N] [--json]\n";
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
      std::cerr << "Usage: t81 vm stack <file.tisc> [--policy <file>] [--steps N] [--limit N] [--json]\n";
      return true;
    }
    if (sub == "mem") {
      std::cerr << "Usage: t81 vm mem <file.tisc> --addr <n> [--policy <file>] [--steps N] [--json]\n";
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
      std::cerr << "Usage: t81 determinism multi-run <file.tisc> --count <n> [--policy <file>] [--json]\n";
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
      std::cerr << "Usage: t81 canonfs get <sha3-256:hash> [-o <file>] [--json] [--canonfs-root <path>]\n";
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
      std::cerr << "Usage: t81 canonfs snapshot-diff <lhs> <rhs> [--json] [--canonfs-root <path>]\n";
      return true;
    }
    if (sub == "rollback") {
      std::cerr << "Usage: t81 canonfs rollback --to <hash> [--dry-run] [--json] [--canonfs-root <path>]\n";
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
    if (command_args.size() >= 2 && print_family_subcommand_help(command_args[0], command_args[1])) {
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

std::string json_escape(std::string_view text);
std::string trap_explanation_text(t81::vm::Trap trap);
std::string render_trace_for_determinism(const t81::vm::State& state);
std::string sha3_512_text(std::string_view text);
std::string sha3_512_file(const fs::path& path);
std::optional<fs::path> find_repo_root(fs::path start);
fs::path discover_repo_root();
fs::path discover_build_dir();
fs::path discover_canonfs_root();
struct ProcessCaptureResult {
  int exit_code = -1;
  std::string stdout_text;
  std::string stderr_text;
};
ProcessCaptureResult run_process_capture(
    const std::vector<std::string>& argv, const std::optional<fs::path>& cwd = std::nullopt,
    const std::vector<std::pair<std::string, std::string>>& env_overrides = {});
const char* ir_opcode_name(t81::tisc::ir::Opcode op);
const char* ir_primitive_name(t81::tisc::ir::PrimitiveKind kind);
const char* ir_relation_name(t81::tisc::ir::ComparisonRelation relation);
std::string format_ir_operand(const t81::tisc::ir::Operand& operand);

// ──────────────────────────────────────────────────────────────
// VM Trap → Exit Code
// ──────────────────────────────────────────────────────────────
// ──────────────────────────────────────────────────────────────
// Core Commands
// ──────────────────────────────────────────────────────────────
// Implemented in tooling/cli/driver.cpp

std::string shell_escape(std::string_view arg) {
  if (arg.empty()) {
    return "''";
  }
  bool needs_quote = false;
  for (char c : arg) {
    if (std::isspace(static_cast<unsigned char>(c)) || c == '"' || c == '\'' || c == '\\' ||
        c == '$' || c == '&' || c == '|' || c == ';' || c == '<' || c == '>' || c == '*' ||
        c == '?' || c == '~' || c == '`' || c == '(' || c == ')' || c == '[' || c == ']' ||
        c == '{' || c == '}') {
      needs_quote = true;
      break;
    }
  }
  if (!needs_quote) {
    return std::string(arg);
  }
  std::string escaped = "'";
  for (char c : arg) {
    if (c == '\'') {
      escaped += "'\\''";
    } else {
      escaped.push_back(c);
    }
  }
  escaped.push_back('\'');
  return escaped;
}

#if !defined(_WIN32)
int decode_system_status(int status) {
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  return status;
}
#else
int decode_system_status(int status) { return status; }
#endif

std::optional<fs::path> find_benchmark_runner(const fs::path& exe_path) {
  std::vector<fs::path> candidates;
  auto exe_dir = exe_path.parent_path();
  if (exe_dir.empty()) {
    exe_dir = ".";
  }
  candidates.emplace_back(exe_dir / "benchmarks/benchmark_runner");
  candidates.emplace_back(exe_dir.parent_path() / "benchmarks/benchmark_runner");
  candidates.emplace_back(fs::path("build/benchmarks/benchmark_runner"));
  candidates.emplace_back(fs::path("benchmarks/benchmark_runner"));
  for (auto& candidate : candidates) {
    if (fs::exists(candidate)) {
      return candidate;
    }
  }
  return std::nullopt;
}

// ──────────────────────────────────────────────────────────────
// Argument Parsing (clean & single-pass)
// ──────────────────────────────────────────────────────────────
struct Args {
  std::string command;
  fs::path input;
  std::optional<fs::path> output;
  std::optional<fs::path> trace_output;
  bool need_help = false;
  bool need_version = false;
  std::vector<std::string> benchmark_args;
  std::vector<std::string> command_args;
  std::optional<fs::path> weights_model;
  std::optional<fs::path> policy;
  bool trace = false;
};

struct CliUsageError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

[[noreturn]] void throw_usage_error(const std::string& message) { throw CliUsageError(message); }

Args parse_args(int argc, char* argv[]) {
  Args a;
  if (argc < 2) {
    a.need_help = true;
    return a;
  }

  int i = 1;
  for (; i < argc; ++i) {
    std::string_view token = argv[i];
    if (token == "-v" || token == "--verbose") {
      g_flags.verbose = true;
      continue;
    }
    if (token == "-q" || token == "--quiet") {
      g_flags.quiet = true;
      continue;
    }
    if (token == "-h" || token == "--help") {
      a.need_help = true;
      continue;
    }
    if (token == "-V" || token == "--version") {
      a.need_version = true;
      continue;
    }
    if (token.starts_with('-')) {
      throw_usage_error("Unknown option: " + std::string(token));
    }
    a.command = std::string(token);
    ++i;
    break;
  }

  if (a.command.empty()) {
    if (!a.need_version) {
      a.need_help = true;
    }
    return a;
  }

  if (a.command == "version") {
    a.need_version = true;
  }
  for (; i < argc; ++i) {
    std::string_view arg = argv[i];

    if (arg == "-v" || arg == "--verbose")
      g_flags.verbose = true;
    else if (arg == "-q" || arg == "--quiet")
      g_flags.quiet = true;
    else if (arg == "-o" || arg == "--output" || arg == "--out") {
      if (++i >= argc) {
        throw_usage_error("Missing argument after -o");
      }
      a.output = fs::path(argv[i]);
    } else if (arg == "--weights-model") {
      if (++i >= argc) {
        throw_usage_error("Missing argument after --weights-model");
      }
      a.weights_model = fs::path(argv[i]);
    } else if (arg == "--policy") {
      if (++i >= argc) {
        throw_usage_error("Missing argument after --policy");
      }
      a.policy = fs::path(argv[i]);
    } else if (arg == "--trace") {
      a.trace = true;
    } else if (arg == "--trace-out") {
      if (++i >= argc) {
        throw_usage_error("Missing argument after --trace-out");
      }
      a.trace = true;
      a.trace_output = fs::path(argv[i]);
    } else if (arg == "-h" || arg == "--help") {
      a.need_help = true;
    } else if (arg == "-V" || arg == "--version") {
      if (a.command == "fmt" || a.command == "code" || a.command == "project" ||
          a.command == "env" || a.command == "internal" || a.command == "completion" ||
          a.command == "man" || a.command == "feedback" || a.command == "c" || a.command == "rust" || a.command == "python" ||
          a.command == "llvm" || a.command == "mlir") {
        a.command_args.emplace_back(argv[i]);
      } else {
        a.need_version = true;
      }
    } else if (arg.starts_with('-')) {
      // Subcommands under these top-level commands own additional flags.
      if (a.command == "benchmark" || a.command == "bench") {
        a.benchmark_args.emplace_back(argv[i]);
      } else if (a.command == "weights" || a.command == "help" || a.command == "init" ||
                 a.command == "pkg" || a.command == "repro-hash" || a.command == "policy" ||
                 a.command == "axion" || a.command == "canonfs" || a.command == "determinism" ||
                 a.command == "trace" || a.command == "vm" || a.command == "tisc" ||
                 a.command == "ir" || a.command == "llama-run" || a.command == "test" ||
                 a.command == "doctor" || a.command == "fmt" || a.command == "code" ||
                 a.command == "lang" || a.command == "model" || a.command == "tensor" ||
                 a.command == "project" || a.command == "env" || a.command == "internal" ||
                 a.command == "completion" || a.command == "man" || a.command == "feedback" ||
                 a.command == "canonize-tensor" || a.command == "canonize-file" ||
                 a.command == "memory-stats" || a.command == "tier" || a.command == "profile" ||
                 a.command == "c" || a.command == "rust" || a.command == "python" || a.command == "llvm" || a.command == "mlir") {
        a.command_args.emplace_back(argv[i]);
      } else {
        throw_usage_error("Unknown option: " + std::string(arg));
      }
    } else {
      if (a.command == "benchmark" || a.command == "bench") {
        a.benchmark_args.emplace_back(argv[i]);
      } else if (a.command == "weights" || a.command == "help" || a.command == "init" ||
                 a.command == "pkg" || a.command == "repro-hash" || a.command == "policy" ||
                 a.command == "axion" || a.command == "canonfs" || a.command == "determinism" ||
                 a.command == "trace" || a.command == "vm" || a.command == "tisc" ||
                 a.command == "ir" || a.command == "llama-run" || a.command == "test" ||
                 a.command == "doctor" || a.command == "fmt" || a.command == "code" ||
                 a.command == "lang" || a.command == "model" || a.command == "tensor" ||
                 a.command == "project" || a.command == "env" || a.command == "internal" ||
                 a.command == "completion" || a.command == "man" || a.command == "feedback" ||
                 a.command == "canonize-tensor" || a.command == "canonize-file" ||
                 a.command == "memory-stats" || a.command == "tier" ||
                 a.command == "c" || a.command == "rust" || a.command == "python" || a.command == "llvm" || a.command == "mlir") {
        a.command_args.emplace_back(argv[i]);
      } else {
        if (!a.input.empty()) {
          throw_usage_error("Multiple input files not supported");
        }
        a.input = fs::path(arg);
      }
    }
  }

  return a;
}

std::shared_ptr<t81::weights::ModelFile> load_weights_model_optional(
    const std::optional<fs::path>& path) {
  if (!path) return nullptr;
  try {
    auto mf = t81::weights::load_t81w(*path);
    return std::make_shared<t81::weights::ModelFile>(std::move(mf));
  } catch (const std::exception& e) {
    error(e.what());
    return nullptr;
  }
}

int run_benchmark(const char* command_name, const Args& args) {
  auto exe_path = fs::path(command_name);
  [[maybe_unused]] [[maybe_unused]] auto runner_path = find_benchmark_runner(exe_path);

  if (!runner_path) {
    error(
        "Could not locate benchmark_runner (looked next to the CLI and under ./build/benchmarks)");
    return 1;
  }

  std::vector<std::string> benchmark_args = args.benchmark_args;
  if (!benchmark_args.empty() && !benchmark_args.front().empty() &&
      benchmark_args.front()[0] != '-') {
    const std::string selector = benchmark_args.front();
    std::optional<std::string> selector_filter;
    if (selector == "vm") {
      selector_filter = "BM_VMSimulation.*";
    } else if (selector == "canonfs") {
      selector_filter = "BM_CanonFS.*";
    } else if (selector == "weights") {
      selector_filter = "BM_TensorPromotion.*";
    } else if (selector == "determinism") {
      selector_filter = "BM_T81LangCompile.*|BM_OverflowDetection.*";
    } else {
      error("benchmark: unknown selector '" + selector +
            "'. Expected vm, canonfs, weights, determinism, or runner flags.");
      return 1;
    }

    bool has_filter = false;
    for (const auto& extra : benchmark_args) {
      if (extra.rfind("--benchmark_filter=", 0) == 0 || extra == "--benchmark_filter") {
        has_filter = true;
        break;
      }
    }
    benchmark_args.erase(benchmark_args.begin());
    if (selector_filter && !has_filter) {
      benchmark_args.insert(benchmark_args.begin(), "--benchmark_filter=" + *selector_filter);
    }
  }

  std::vector<std::string> runner_argv;
  runner_argv.push_back(runner_path->string());
  for (const auto& extra : benchmark_args) {
    runner_argv.push_back(extra);
  }

  info("Running benchmarks via " + runner_path->string());
#if defined(_WIN32)
  std::string cmd = "set T81_AXION_TRAP_STDERR=0&& set T81_BENCHMARK_WRITE_REPORTS=0&& " +
                    shell_escape(runner_path->string());
  for (const auto& extra : benchmark_args) {
    cmd += ' ';
    cmd += shell_escape(extra);
  }
  int status = std::system(cmd.c_str());
  if (status == -1) {
    error("Failed to execute benchmark_runner");
    return 1;
  }
  return decode_system_status(status);
#else
  auto format_elapsed = [](std::chrono::seconds elapsed) {
    const auto total = elapsed.count();
    const auto minutes = total / 60;
    const auto seconds = total % 60;
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << minutes << ":" << std::setw(2) << seconds;
    return oss.str();
  };

  const auto start = std::chrono::steady_clock::now();
  pid_t child = fork();
  if (child == 0) {
    setenv("T81_AXION_TRAP_STDERR", "0", 1);
    setenv("T81_BENCHMARK_WRITE_REPORTS", "0", 1);
    std::vector<std::string> argv_storage = runner_argv;
    std::vector<char*> argv_ptrs;
    argv_ptrs.reserve(argv_storage.size() + 1);
    for (auto& item : argv_storage) {
      argv_ptrs.push_back(item.data());
    }
    argv_ptrs.push_back(nullptr);
    execv(argv_ptrs[0], argv_ptrs.data());
    _exit(127);
  }
  if (child < 0) {
    error("Failed to spawn benchmark_runner");
    return 1;
  }

  int status = 0;
  auto last_tick = start;
  while (true) {
    const pid_t wait_rc = waitpid(child, &status, WNOHANG);
    if (wait_rc == child) {
      break;
    }
    if (wait_rc < 0) {
      error("Failed while waiting for benchmark_runner");
      return 1;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    const auto now = std::chrono::steady_clock::now();
    if (now - last_tick >= std::chrono::seconds(10)) {
      const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start);
      info("Benchmark still running... elapsed " + format_elapsed(elapsed));
      last_tick = now;
    }
  }

  const auto elapsed =
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start);
  info("Benchmark finished in " + format_elapsed(elapsed));
  return decode_system_status(status);
#endif
}

struct WeightsImportOptions {
  fs::path input;
  std::optional<fs::path> output;
  std::string format = "safetensors";
};

bool has_extension_ci(const fs::path& path, std::string_view expected_ext) {
  auto lower = [](std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
      out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
  };
  std::string ext = lower(path.extension().string());
  std::string expected = lower(expected_ext);
  return ext == expected;
}

std::vector<int8_t> unpack_native_trits(const t81::weights::NativeTensor& tensor) {
  std::vector<int8_t> out;
  const uint64_t total_trits = tensor.num_trits();
  out.reserve(total_trits);
  constexpr std::array<int8_t, 3> kMap = {-1, 0, 1};
  for (std::size_t limb_index = 0; limb_index < tensor.data.size() && out.size() < total_trits;
       ++limb_index) {
    uint64_t limb = tensor.data[limb_index];
    std::array<int8_t, 48> decoded{};
    for (int i = 47; i >= 0; --i) {
      decoded[static_cast<std::size_t>(i)] = kMap[limb % 3];
      limb /= 3;
    }
    for (int i = 0; i < 48 && out.size() < total_trits; ++i) {
      out.push_back(decoded[static_cast<std::size_t>(i)]);
    }
  }
  return out;
}

int run_weights_import(const Args& args) {
  if (args.command_args.size() < 2) {
    error("weights import requires an input file. Run 't81 help weights import'.");
    return 1;
  }
  WeightsImportOptions opts;
  opts.input = fs::path(args.command_args[1]);
  opts.output = args.output;
  size_t idx = 2;
  while (idx < args.command_args.size()) {
    const auto& token = args.command_args[idx++];
    if (token == "--format") {
      if (idx >= args.command_args.size()) {
        error("weights import: missing argument for --format. Run 't81 help weights import'.");
        return 1;
      }
      opts.format = args.command_args[idx++];
    } else if (token == "-o" || token == "--out" || token == "--output") {
      if (idx >= args.command_args.size()) {
        error("weights import: missing argument for " + token + ". Run 't81 help weights import'.");
        return 1;
      }
      opts.output = fs::path(args.command_args[idx++]);
    } else if (opts.input.empty()) {
      opts.input = fs::path(token);
    } else {
      error("weights import: unexpected argument '" + token + "'. Run 't81 help weights import'.");
      return 1;
    }
  }
  if (opts.input.empty()) {
    error("weights import requires an input file. Run 't81 help weights import'.");
    return 1;
  }
  if (!opts.output) {
    opts.output = opts.input.stem();
    opts.output->replace_extension(".t81w");
  }
  if (!has_extension_ci(*opts.output, ".t81w")) {
    error("weights import: output must use .t81w extension. Run 't81 help weights import'.");
    return 1;
  }

  t81::weights::ModelFile mf;
  try {
    if (opts.format == "safetensors") {
      if (!has_extension_ci(opts.input, ".safetensors")) {
        error("weights import: --format safetensors requires a .safetensors input file. Run 't81 help "
              "weights import'.");
        return 1;
      }
      mf = t81::weights::load_safetensors(opts.input);
    } else if (opts.format == "gguf") {
      if (!has_extension_ci(opts.input, ".gguf")) {
        error("weights import: --format gguf requires a .gguf input file. Run 't81 help weights "
              "import'.");
        return 1;
      }
      mf = t81::weights::load_gguf(opts.input);
    } else {
      error("weights import: unsupported format '" + opts.format +
            "'. Supported: safetensors, gguf. Run 't81 help weights import'.");
      return 1;
    }
  } catch (const std::exception& e) {
    error(e.what());
    return 1;
  }
  t81::weights::print_info(mf);
  if (mf.native.empty()) {
    error("weights import: loader produced no native tensors");
    return 1;
  }
  t81::weights::save_t81w(mf.native, *opts.output);
  info("Saved " + opts.output->string());
  return 0;
}

int run_weights_info(const Args& args) {
  if (args.command_args.size() < 2) {
    error("weights info requires a .t81w file path. Run 't81 help weights info'.");
    return 1;
  }
  fs::path path;
  bool as_json = false;
  for (size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (!token.empty() && token[0] == '-') {
      error("weights info: unknown option '" + token + "'. Run 't81 help weights info'.");
      return 1;
    } else if (path.empty()) {
      path = fs::path(token);
    } else {
      error("weights info: unexpected argument '" + token + "'. Run 't81 help weights info'.");
      return 1;
    }
  }
  if (path.empty()) {
    error("weights info requires a .t81w file path. Run 't81 help weights info'.");
    return 1;
  }
  if (!has_extension_ci(path, ".t81w")) {
    error("weights info expects a .t81w file path. Run 't81 help weights info'.");
    return 1;
  }
  auto emit_json_error = [&](std::string_view message) {
    auto escape = [](std::string_view text) {
      std::string out;
      out.reserve(text.size() + 8);
      for (char c : text) {
        if (c == '\\' || c == '"') out.push_back('\\');
        if (c == '\n') {
          out += "\\n";
          continue;
        }
        if (c == '\r') {
          out += "\\r";
          continue;
        }
        if (c == '\t') {
          out += "\\t";
          continue;
        }
        out.push_back(c);
      }
      return out;
    };
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.weights-info.v1\",\n";
    std::cout << "  \"ok\": false,\n";
    std::cout << "  \"model\": \"" << escape(path.string()) << "\",\n";
    std::cout << "  \"error\": \"" << escape(message) << "\"\n";
    std::cout << "}\n";
  };
  try {
    auto mf = t81::weights::load_t81w(path);
    if (as_json) {
      std::cout << "{\n";
      std::cout << "  \"schema\": \"t81.weights-info.v1\",\n";
      std::cout << "  \"ok\": true,\n";
      std::cout << "  \"model\": \"" << path.string() << "\",\n";
      std::cout << "  \"parameters\": " << mf.total_parameters << ",\n";
      std::cout << "  \"trits\": " << mf.total_trits << ",\n";
      std::cout << "  \"file_size_bytes\": " << mf.file_size << ",\n";
      std::cout << "  \"bits_per_trit\": " << std::fixed << std::setprecision(6) << mf.bits_per_trit
                << ",\n";
      std::cout << "  \"sparsity\": " << std::fixed << std::setprecision(6) << mf.sparsity << ",\n";
      std::cout << "  \"format\": \"" << mf.format << "\",\n";
      std::cout << "  \"checksum_sha3_512\": \"" << mf.checksum << "\"\n";
      std::cout << "}\n";
      std::cout << std::defaultfloat;
    } else {
      std::cout << "Model:        " << path << "\n";
      std::cout << "Parameters:   " << t81::weights::format_count(mf.total_parameters) << "\n";
      std::cout << "Trits:        " << t81::weights::format_count(mf.total_trits) << " trits\n";
      std::cout << "Storage:      " << t81::weights::format_bytes(mf.file_size) << " ("
                << std::fixed << std::setprecision(3) << mf.bits_per_trit << " bits/trit avg)\n";
      std::cout << std::fixed << std::setprecision(1) << "Sparsity:     " << (mf.sparsity * 100.0)
                << "% zeros\n";
      std::cout << std::defaultfloat;
      std::cout << "Format:       " << mf.format << "\n";
      std::cout << "Checksum:     sha3-512:" << mf.checksum << " (CanonFS-ready)\n";
    }
  } catch (const std::exception& e) {
    if (as_json) {
      emit_json_error(e.what());
      return 1;
    }
    error(e.what());
    return 1;
  }
  return 0;
}

int run_weights_verify(const Args& args) {
  if (args.command_args.size() < 2) {
    error("weights verify requires a .t81w file path. Run 't81 help weights verify'.");
    return 1;
  }
  fs::path path;
  bool as_json = false;
  for (size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (!token.empty() && token[0] == '-') {
      error("weights verify: unknown option '" + token + "'. Run 't81 help weights verify'.");
      return 1;
    } else if (path.empty()) {
      path = fs::path(token);
    } else {
      error("weights verify: unexpected argument '" + token + "'. Run 't81 help weights verify'.");
      return 1;
    }
  }
  if (path.empty()) {
    error("weights verify requires a .t81w file path. Run 't81 help weights verify'.");
    return 1;
  }
  if (!has_extension_ci(path, ".t81w")) {
    error("weights verify expects a .t81w file path. Run 't81 help weights verify'.");
    return 1;
  }

  try {
    auto mf = t81::weights::load_t81w(path);
    const bool ok = !mf.native.empty() && !mf.checksum.empty() && mf.file_size > 0;
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.weights-verify.v1\",\n"
                << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
                << "  \"model\": \"" << json_escape(path.string()) << "\",\n"
                << "  \"tensors\": " << mf.native.size() << ",\n"
                << "  \"parameters\": " << mf.total_parameters << ",\n"
                << "  \"file_size_bytes\": " << mf.file_size << ",\n"
                << "  \"checksum_sha3_512\": \"" << json_escape(mf.checksum) << "\"\n"
                << "}\n";
    } else {
      std::cout << "Verified model: " << path.string() << "\n";
      std::cout << "Tensors:        " << mf.native.size() << "\n";
      std::cout << "Parameters:     " << t81::weights::format_count(mf.total_parameters) << "\n";
      std::cout << "File size:      " << t81::weights::format_bytes(mf.file_size) << "\n";
      std::cout << "Checksum:       sha3-512:" << mf.checksum << "\n";
    }
    return ok ? 0 : 1;
  } catch (const std::exception& e) {
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.weights-verify.v1\",\n"
                << "  \"ok\": false,\n"
                << "  \"model\": \"" << json_escape(path.string()) << "\",\n"
                << "  \"error\": \"" << json_escape(e.what()) << "\"\n"
                << "}\n";
    } else {
      error(e.what());
    }
    return 1;
  }
}

int run_weights_quantize(const Args& args) {
  if (args.command_args.size() != 4 || args.command_args[2] != "--to-gguf") {
    error("weights quantize requires: quantize <input> --to-gguf <output>. Run 't81 help weights "
          "quantize'.");
    return 1;
  }
  fs::path input = args.command_args[1];
  fs::path output = args.command_args[3];
  if (!has_extension_ci(input, ".safetensors")) {
    error("weights quantize expects a .safetensors input. Run 't81 help weights quantize'.");
    return 1;
  }
  if (!has_extension_ci(output, ".gguf")) {
    error("weights quantize expects a .gguf output path. Run 't81 help weights quantize'.");
    return 1;
  }
  try {
    t81::weights::quantize_safetensors_to_gguf(input, output);
  } catch (const std::exception& e) {
    error(e.what());
    return 1;
  }
  return 0;
}

int run_weights_export(const Args& args) {
  if (args.command_args.size() < 4 || args.command_args[2] != "--to-safetensors") {
    error("weights export requires: export <model.t81w> --to-safetensors <output>. Run 't81 help weights export'.");
    return 1;
  }
  const fs::path input = args.command_args[1];
  const fs::path output = args.command_args[3];
  bool as_json = false;
  for (std::size_t i = 4; i < args.command_args.size(); ++i) {
    if (args.command_args[i] == "--json") {
      as_json = true;
    } else {
      error("weights export: unexpected argument '" + args.command_args[i] +
            "'. Run 't81 help weights export'.");
      return 1;
    }
  }
  if (!has_extension_ci(input, ".t81w")) {
    error("weights export expects a .t81w input file.");
    return 1;
  }
  if (!has_extension_ci(output, ".safetensors")) {
    error("weights export expects a .safetensors output file.");
    return 1;
  }

  t81::weights::ModelFile mf;
  try {
    mf = t81::weights::load_t81w(input);
  } catch (const std::exception& e) {
    error(e.what());
    return 1;
  }

  struct ExportTensor {
    std::string name;
    std::vector<uint64_t> shape;
    uint64_t trits = 0;
    std::vector<int8_t> raw;
  };
  std::vector<ExportTensor> tensors;
  tensors.reserve(mf.native.size());
  for (const auto& [name, tensor] : mf.native) {
    tensors.push_back(ExportTensor{name, tensor.shape, tensor.num_trits(), unpack_native_trits(tensor)});
  }

  std::string header_text;
  std::uint64_t header_len = 0;
  for (int iter = 0; iter < 4; ++iter) {
    std::ostringstream header;
    header << "{";
    std::uint64_t offset = sizeof(std::uint64_t) + header_len;
    for (std::size_t index = 0; index < tensors.size(); ++index) {
      const auto& tensor = tensors[index];
      header << "\"" << json_escape(tensor.name) << "\":{"
             << "\"dtype\":\"I8\","
             << "\"shape\":[";
      const uint64_t shape_product =
          std::accumulate(tensor.shape.begin(), tensor.shape.end(), uint64_t{1}, std::multiplies<uint64_t>());
      if (!tensor.shape.empty() && shape_product == tensor.trits) {
        for (std::size_t i = 0; i < tensor.shape.size(); ++i) {
          header << tensor.shape[i];
          if (i + 1 < tensor.shape.size()) {
            header << ",";
          }
        }
      } else {
        header << tensor.trits;
      }
      header << "],\"data_offsets\":[" << offset << "],\"data_lengths\":[" << tensor.raw.size() << "]}";
      offset += tensor.raw.size();
      if (index + 1 < tensors.size()) {
        header << ",";
      }
    }
    header << "}";
    header_text = header.str();
    if (header_text.size() == header_len) {
      break;
    }
    header_len = static_cast<std::uint64_t>(header_text.size());
  }
  header_len = static_cast<std::uint64_t>(header_text.size());

  std::ofstream out(output, std::ios::binary | std::ios::trunc);
  if (!out) {
    error("weights export: could not open output file: " + output.string());
    return 1;
  }
  out.write(reinterpret_cast<const char*>(&header_len), sizeof(header_len));
  out.write(header_text.data(), static_cast<std::streamsize>(header_len));
  for (const auto& tensor : tensors) {
    out.write(reinterpret_cast<const char*>(tensor.raw.data()),
              static_cast<std::streamsize>(tensor.raw.size()));
  }
  if (!out.good()) {
    error("weights export: failed writing output file: " + output.string());
    return 1;
  }

  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.weights-export.v1\",\n"
              << "  \"ok\": true,\n"
              << "  \"input\": \"" << json_escape(input.string()) << "\",\n"
              << "  \"output\": \"" << json_escape(output.string()) << "\",\n"
              << "  \"tensors\": " << mf.native.size() << "\n"
              << "}\n";
    return 0;
  }

  std::cout << "Exported " << mf.native.size() << " tensor(s) to " << output.string() << "\n";
  return 0;
}

int run_weights(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    return emit_help([&] { print_help_weights(); });
  }
  if (args.command_args.size() >= 2 &&
      (args.command_args[1] == "-h" || args.command_args[1] == "--help")) {
    const std::string sub = args.command_args[0];
    if (sub == "import") {
      print_help_weights_import();
      return 0;
    }
    if (sub == "info") {
      print_help_weights_info();
      return 0;
    }
    if (sub == "verify") {
      print_help_weights_verify();
      return 0;
    }
    if (sub == "quantize") {
      print_help_weights_quantize();
      return 0;
    }
    if (sub == "export") {
      return emit_help([&] {
        std::cerr << "Usage: t81 weights export <model.t81w> --to-safetensors <out> [--json]\n";
      });
    }
    error("weights: unknown subcommand '" + sub + "'. Run 't81 help weights'.");
    return 1;
  }
  const std::string sub = args.command_args[0];
  if (sub == "import") {
    return run_weights_import(args);
  } else if (sub == "info") {
    return run_weights_info(args);
  } else if (sub == "verify") {
    return run_weights_verify(args);
  } else if (sub == "quantize") {
    return run_weights_quantize(args);
  } else if (sub == "export") {
    return run_weights_export(args);
  }
  error("weights: unknown subcommand '" + sub + "'. Run 't81 help weights'.");
  return 1;
}

int run_policy_compile(const Args& args) {
  if (args.command_args.size() < 2) {
    error("policy compile requires an input .apl file. Run 't81 help policy compile'.");
    return 1;
  }
  fs::path input = args.command_args[1];
  fs::path output = args.output.value_or(input.stem().string() + ".axionb");
  std::ifstream ifs(input);
  if (!ifs) {
    error("Could not open input file: " + input.string());
    return 1;
  }
  std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  auto policy_res = t81::axion::parse_policy(content);
  if (!policy_res) {
    error("Policy parse error: " + policy_res.error());
    return 1;
  }
  auto& policy = policy_res.value();
  policy.compile_to_bytecode();
  std::ofstream ofs(output, std::ios::binary);
  if (!ofs) {
    error("Could not open output file: " + output.string());
    return 1;
  }
  policy.serialize(ofs);
  info("Compiled policy to " + output.string());
  return 0;
}

int run_policy_run(const Args& args) {
  if (args.command_args.size() < 2) {
    error("policy run requires an input .apl or .axionb file. Run 't81 help policy run'.");
    return 1;
  }
  fs::path input;
  bool as_json = false;
  for (size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (!token.empty() && token[0] == '-') {
      error("policy run: unknown option '" + token + "'. Run 't81 help policy run'.");
      return 1;
    } else if (input.empty()) {
      input = fs::path(token);
    } else {
      error("policy run: unexpected argument '" + token + "'. Run 't81 help policy run'.");
      return 1;
    }
  }
  if (input.empty()) {
    error("policy run requires an input .apl or .axionb file. Run 't81 help policy run'.");
    return 1;
  }
  t81::axion::Policy policy;
  if (input.extension() == ".axionb") {
    std::ifstream ifs(input, std::ios::binary);
    if (!ifs) {
      error("Could not open policy file: " + input.string());
      return 1;
    }
    auto res = t81::axion::Policy::deserialize(ifs);
    if (!res) {
      error("Policy deserialization error: " + res.error());
      return 1;
    }
    policy = std::move(res.value());
  } else {
    std::ifstream ifs(input);
    if (!ifs) {
      error("Could not open policy file: " + input.string());
      return 1;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    auto res = t81::axion::parse_policy(content);
    if (!res) {
      error("Policy parse error: " + res.error());
      return 1;
    }
    policy = std::move(res.value());
  }
  if (as_json) {
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.policy-run.v1\",\n";
    std::cout << "  \"valid\": true,\n";
    std::cout << "  \"tier\": " << policy.tier << ",\n";
    if (policy.max_instructions) {
      std::cout << "  \"max_instructions\": " << *policy.max_instructions << ",\n";
    } else {
      std::cout << "  \"max_instructions\": null,\n";
    }
    if (policy.max_stack) {
      std::cout << "  \"max_stack\": " << *policy.max_stack << ",\n";
    } else {
      std::cout << "  \"max_stack\": null,\n";
    }
    if (policy.max_recursion) {
      std::cout << "  \"max_recursion\": " << *policy.max_recursion << "\n";
    } else {
      std::cout << "  \"max_recursion\": null\n";
    }
    std::cout << "}\n";
  } else {
    info("Policy validated successfully.");
    if (g_flags.verbose) {
      std::cout << "Tier: " << policy.tier << "\n";
      if (policy.max_instructions)
        std::cout << "Max Instructions: " << *policy.max_instructions << "\n";
      if (policy.max_stack) std::cout << "Max Stack: " << *policy.max_stack << "\n";
      if (policy.max_recursion) std::cout << "Max Recursion: " << *policy.max_recursion << "\n";
    }
  }
  return 0;
}

int run_policy_validate(const Args& args) {
  if (args.command_args.size() < 2) {
    error("policy validate requires an input .apl or .axionb file. Run 't81 help policy validate'.");
    return 1;
  }
  fs::path input;
  bool as_json = false;
  for (size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (!token.empty() && token[0] == '-') {
      error("policy validate: unknown option '" + token + "'. Run 't81 help policy validate'.");
      return 1;
    } else if (input.empty()) {
      input = fs::path(token);
    } else {
      error("policy validate: unexpected argument '" + token + "'. Run 't81 help policy validate'.");
      return 1;
    }
  }
  if (input.empty()) {
    error("policy validate requires an input .apl or .axionb file. Run 't81 help policy validate'.");
    return 1;
  }
  t81::axion::Policy policy;
  if (input.extension() == ".axionb") {
    std::ifstream ifs(input, std::ios::binary);
    if (!ifs) {
      error("Could not open policy file: " + input.string());
      return 1;
    }
    auto res = t81::axion::Policy::deserialize(ifs);
    if (!res) {
      error("Policy deserialization error: " + res.error());
      return 1;
    }
    policy = std::move(res.value());
  } else {
    std::ifstream ifs(input);
    if (!ifs) {
      error("Could not open policy file: " + input.string());
      return 1;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    auto res = t81::axion::parse_policy(content);
    if (!res) {
      error("Policy parse error: " + res.error());
      return 1;
    }
    policy = std::move(res.value());
  }
  if (as_json) {
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.policy-validate.v1\",\n";
    std::cout << "  \"valid\": true,\n";
    std::cout << "  \"policy\": \"" << json_escape(input.string()) << "\",\n";
    std::cout << "  \"tier\": " << policy.tier << ",\n";
    if (policy.max_instructions) {
      std::cout << "  \"max_instructions\": " << *policy.max_instructions << ",\n";
    } else {
      std::cout << "  \"max_instructions\": null,\n";
    }
    if (policy.max_stack) {
      std::cout << "  \"max_stack\": " << *policy.max_stack << ",\n";
    } else {
      std::cout << "  \"max_stack\": null,\n";
    }
    if (policy.max_recursion) {
      std::cout << "  \"max_recursion\": " << *policy.max_recursion << "\n";
    } else {
      std::cout << "  \"max_recursion\": null\n";
    }
    std::cout << "}\n";
  } else {
    std::cout << "Policy valid: " << input.string() << "\n";
    if (g_flags.verbose) {
      std::cout << "Tier: " << policy.tier << "\n";
      if (policy.max_instructions) std::cout << "Max Instructions: " << *policy.max_instructions << "\n";
      if (policy.max_stack) std::cout << "Max Stack: " << *policy.max_stack << "\n";
      if (policy.max_recursion) std::cout << "Max Recursion: " << *policy.max_recursion << "\n";
    }
  }
  return 0;
}

int run_policy(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    return emit_help([&] { print_help_policy(); });
  }
  if (args.command_args.size() >= 2 &&
      (args.command_args[1] == "-h" || args.command_args[1] == "--help")) {
    const std::string sub = args.command_args[0];
    if (sub == "compile") {
      print_help_policy_compile();
      return 0;
    }
    if (sub == "validate") {
      print_help_policy_validate();
      return 0;
    }
    if (sub == "run") {
      print_help_policy_run();
      return 0;
    }
    if (sub == "test") {
      print_help_policy_test();
      return 0;
    }
    if (sub == "list") {
      print_help_policy_list();
      return 0;
    }
    error("policy: unknown subcommand '" + sub + "'. Run 't81 help policy'.");
    return 1;
  }
  if (args.command_args.empty()) {
    error("policy requires a subcommand (compile|validate|run|test|list). Run 't81 help policy'.");
    return 1;
  }
  const std::string sub = args.command_args[0];
  if (sub == "compile") return run_policy_compile(args);
  if (sub == "validate") return run_policy_validate(args);
  if (sub == "run") return run_policy_run(args);
  if (sub == "test") {
    if (args.command_args.size() < 2) {
      error("policy test requires an input .apl or .axionb file. Run 't81 help policy test'.");
      return 1;
    }
    fs::path input;
    std::string model_hash;
    bool as_json = false;
    for (std::size_t i = 1; i < args.command_args.size(); ++i) {
      const std::string& token = args.command_args[i];
      if (token == "--json") {
        as_json = true;
      } else if (token == "--model-hash") {
        if (++i >= args.command_args.size()) {
          error("policy test: missing value for --model-hash. Run 't81 help policy test'.");
          return 1;
        }
        model_hash = args.command_args[i];
      } else if (!token.empty() && token[0] == '-') {
        error("policy test: unknown option '" + token + "'. Run 't81 help policy test'.");
        return 1;
      } else if (input.empty()) {
        input = fs::path(token);
      } else {
        error("policy test: unexpected argument '" + token + "'. Run 't81 help policy test'.");
        return 1;
      }
    }
    if (input.empty() || model_hash.empty()) {
      error("policy test requires <policy> and --model-hash <hash>. Run 't81 help policy test'.");
      return 1;
    }
    std::ifstream ifs(input, input.extension() == ".axionb" ? std::ios::binary : std::ios::in);
    if (!ifs) {
      error("Could not open policy file: " + input.string());
      return 1;
    }
    std::string policy_text;
    if (input.extension() == ".axionb") {
      auto res = t81::axion::Policy::deserialize(ifs);
      if (!res) {
        error("Policy deserialization error: " + res.error());
        return 1;
      }
      const auto& policy = res.value();
      if (std::find(policy.allowed_tensor_hashes.begin(), policy.allowed_tensor_hashes.end(),
                    model_hash) == policy.allowed_tensor_hashes.end()) {
        if (as_json) {
          std::cout << "{\n  \"schema\": \"t81.policy-test.v1\",\n  \"ok\": false,\n  \"model_hash\": \""
                    << json_escape(model_hash) << "\",\n  \"rule\": \"hash_not_allowed\"\n}\n";
        } else {
          error("Policy test failed: model hash is not allowed.");
        }
        return 1;
      }
      if (as_json) {
        std::cout << "{\n  \"schema\": \"t81.policy-test.v1\",\n  \"ok\": true,\n  \"model_hash\": \""
                  << json_escape(model_hash) << "\",\n  \"rule\": \"hash_valid\"\n}\n";
      } else {
        info("Policy test passed.");
      }
      return 0;
    }
    policy_text.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    auto violation = t81::axion::PolicyValidator::validate_tensor_hash(model_hash, policy_text);
    const bool ok = violation.has_value() && !violation->is_violation;
    if (as_json) {
      std::cout << "{\n  \"schema\": \"t81.policy-test.v1\",\n  \"ok\": "
                << (ok ? "true" : "false") << ",\n  \"model_hash\": \"" << json_escape(model_hash)
                << "\",\n  \"rule\": \"" << json_escape(violation ? violation->rule : "unknown")
                << "\",\n  \"reason\": \""
                << json_escape(violation ? violation->reason : "validation unavailable") << "\"\n}\n";
    } else if (violation) {
      std::cout << t81::axion::PolicyValidator::generate_policy_report(*violation) << "\n";
    }
    return ok ? 0 : 1;
  }
  if (sub == "list") {
    bool as_json = false;
    fs::path search_dir = discover_repo_root();
    for (std::size_t i = 1; i < args.command_args.size(); ++i) {
      if (args.command_args[i] == "--json") { as_json = true; }
      else if (args.command_args[i] == "--dir" && i + 1 < args.command_args.size()) {
        search_dir = fs::path(args.command_args[++i]);
      }
    }
    std::vector<fs::path> found;
    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(search_dir, ec)) {
      if (ec) break;
      if (entry.is_regular_file() && entry.path().extension() == ".apl") {
        found.push_back(entry.path());
      }
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.policy-list.v1\",\n"
                << "  \"count\": " << found.size() << ",\n"
                << "  \"policies\": [\n";
      for (std::size_t i = 0; i < found.size(); ++i) {
        std::cout << "    \"" << json_escape(found[i].string()) << "\"";
        if (i + 1 < found.size()) std::cout << ",";
        std::cout << "\n";
      }
      std::cout << "  ]\n}\n";
    } else {
      if (found.empty()) {
        std::cout << "No .apl policy files found in: " << search_dir.string() << "\n";
      } else {
        for (const auto& p : found) std::cout << p.string() << "\n";
      }
    }
    return 0;
  }

  error("policy: unknown subcommand '" + sub + "'. Run 't81 help policy'.");
  return 1;
}

// RFC-0000 §7: Axion command surface — status|optimize|simulate|snapshot|rollback.
int run_repro_hash(const char* command_name, const Args& args);

bool is_valid_snapshot_hash_text(std::string_view text) {
  if (text.empty()) {
    return false;
  }
  try {
    (void)t81::hash::CanonHash81::from_string(std::string(text));
    return true;
  } catch (...) {
    return false;
  }
}

int run_axion(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    return emit_help([&] { print_help_axion(); });
  }

  const std::string sub = args.command_args[0];
  bool json_out = false;
  int target_tier = 1;
  bool tier_provided = false;
  bool tier_invalid = false;
  std::size_t log_tail_n = 10;
  std::string rollback_to;
  std::string audit_from;
  std::string audit_to;
  std::string simulate_file;
  std::string explain_file;
  std::vector<std::string> snapshot_diff_hashes;
  const fs::path canonfs_root = discover_canonfs_root();

  auto count_snapshot_dirs = [&](const fs::path& root) {
    std::size_t count = 0;
    std::error_code ec;
    const fs::path snapshots_root = root / "snapshots";
    if (!fs::exists(snapshots_root, ec)) {
      return count;
    }
    for (const auto& entry : fs::directory_iterator(snapshots_root, ec)) {
      if (ec) break;
      if (entry.is_directory()) {
        ++count;
      }
    }
    return count;
  };

  auto count_canonfs_objects = [&](const fs::path& root) {
    std::size_t count = 0;
    std::error_code ec;
    const fs::path objects_root = root / "objects";
    if (!fs::exists(objects_root, ec)) {
      return count;
    }
    for (const auto& entry : fs::directory_iterator(objects_root, ec)) {
      if (ec) break;
      if (entry.is_regular_file() && entry.path().extension() == ".blk") {
        ++count;
      }
    }
    return count;
  };

  auto latest_snapshot_hash = [&](const fs::path& root) -> std::optional<std::string> {
    std::error_code ec;
    const fs::path snapshots_root = root / "snapshots";
    if (!fs::exists(snapshots_root, ec)) {
      return std::nullopt;
    }
    fs::file_time_type latest_time;
    std::optional<std::string> latest_hash;
    for (const auto& entry : fs::directory_iterator(snapshots_root, ec)) {
      if (ec || !entry.is_directory()) {
        continue;
      }
      const auto ts = entry.last_write_time(ec);
      if (ec) {
        continue;
      }
      if (!latest_hash || ts > latest_time) {
        latest_time = ts;
        latest_hash = entry.path().filename().string();
      }
    }
    return latest_hash;
  };

  auto recent_snapshot_hashes = [&](const fs::path& root, std::size_t limit) {
    std::vector<std::pair<fs::file_time_type, std::string>> snapshots;
    std::error_code ec;
    const fs::path snapshots_root = root / "snapshots";
    if (!fs::exists(snapshots_root, ec)) {
      return std::vector<std::string>{};
    }
    for (const auto& entry : fs::directory_iterator(snapshots_root, ec)) {
      if (ec || !entry.is_directory()) {
        continue;
      }
      const auto ts = entry.last_write_time(ec);
      if (ec) {
        continue;
      }
      snapshots.emplace_back(ts, entry.path().filename().string());
    }
    std::sort(snapshots.begin(), snapshots.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.first > rhs.first; });
    std::vector<std::string> hashes;
    const std::size_t count = std::min(limit, snapshots.size());
    hashes.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
      hashes.push_back(std::move(snapshots[i].second));
    }
    return hashes;
  };

  auto axion_state_path = [&](const fs::path& root) { return root / "axion" / "state.json"; };
  auto json_nullable_axion = [&](const std::optional<std::string>& value) {
    return value ? ("\"" + json_escape(*value) + "\"") : std::string("null");
  };

  struct AxionStateSnapshot {
    bool present = false;
    int tier = 1;
    std::optional<std::string> active_snapshot;
    std::optional<std::time_t> updated;
  };

  auto read_axion_state = [&](const fs::path& root) {
    AxionStateSnapshot snapshot;
    std::ifstream state_in(axion_state_path(root));
    if (!state_in) {
      return snapshot;
    }
    snapshot.present = true;
    const std::string state_text((std::istreambuf_iterator<char>(state_in)),
                                 std::istreambuf_iterator<char>());
    std::smatch match;
    if (std::regex_search(state_text, match, std::regex(R"("tier"\s*:\s*([0-9]+))")) &&
        match.size() == 2) {
      snapshot.tier = std::stoi(match[1].str());
    }
    if (std::regex_search(state_text, match,
                          std::regex("\"active_snapshot\"\\s*:\\s*\"([^\"]+)\"")) &&
        match.size() == 2) {
      snapshot.active_snapshot = match[1].str();
    }
    if (std::regex_search(state_text, match, std::regex(R"("updated"\s*:\s*([0-9]+))")) &&
        match.size() == 2) {
      try {
        snapshot.updated = static_cast<std::time_t>(std::stoll(match[1].str()));
      } catch (...) {
      }
    }
    return snapshot;
  };

  auto read_manifest_lines = [&](const std::string& hash) {
    std::vector<std::string> lines;
    if (hash.empty()) {
      return lines;
    }
    const fs::path manifest = canonfs_root / "snapshots" / hash / "MANIFEST.txt";
    std::ifstream in(manifest, std::ios::binary);
    if (!in) {
      return lines;
    }
    std::string line;
    while (std::getline(in, line)) {
      if (!line.empty()) {
        lines.push_back(line);
      }
    }
    std::sort(lines.begin(), lines.end());
    return lines;
  };

  auto write_axion_state = [&](int tier, std::string_view active_snapshot) -> bool {
    std::error_code ec;
    fs::create_directories(axion_state_path(canonfs_root).parent_path(), ec);
    if (ec) {
      error("axion optimize: could not create state directory under " + canonfs_root.string());
      return false;
    }
    std::ofstream out(axion_state_path(canonfs_root), std::ios::binary | std::ios::trunc);
    if (!out) {
      error("axion optimize: could not write axion state file.");
      return false;
    }
    out << "{\n"
        << "  \"schema\": \"t81.axion-state.v1\",\n"
        << "  \"tier\": " << tier << ",\n"
        << "  \"active_snapshot\": ";
    if (active_snapshot.empty()) {
      out << "null,\n";
    } else {
      out << "\"" << json_escape(active_snapshot) << "\",\n";
    }
    out << "  \"updated\": " << std::time(nullptr) << "\n"
        << "}\n";
    return static_cast<bool>(out);
  };

  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const auto& tok = args.command_args[i];
    if (tok == "--json") {
      json_out = true;
    } else if (tok == "--tier" && i + 1 < args.command_args.size()) {
      tier_provided = true;
      try {
        target_tier = std::stoi(args.command_args[++i]);
        if (target_tier < 1 || target_tier > 9) {
          tier_invalid = true;
        }
      } catch (...) {
        tier_invalid = true;
      }
    } else if (tok == "--to" && i + 1 < args.command_args.size()) {
      if (sub == "rollback") {
        rollback_to = args.command_args[++i];
      } else if (sub == "audit") {
        audit_to = args.command_args[++i];
      } else {
        error("axion: unknown argument '" + tok + "'. Run 't81 help axion'.");
        return 1;
      }
    } else if (tok == "--from" && i + 1 < args.command_args.size()) {
      audit_from = args.command_args[++i];
    } else if (tok == "--tail" && i + 1 < args.command_args.size() && sub == "log") {
      try {
        log_tail_n = static_cast<std::size_t>(std::stoull(args.command_args[++i]));
      } catch (...) {
        error("axion log: invalid --tail value.");
        return 1;
      }
    } else if (explain_file.empty() && sub == "explain") {
      explain_file = tok;
    } else if (simulate_file.empty() && sub == "simulate") {
      simulate_file = tok;
    } else if (sub == "snapshot-diff" && snapshot_diff_hashes.size() < 2) {
      snapshot_diff_hashes.push_back(tok);
    } else {
      error("axion: unknown argument '" + tok + "'. Run 't81 help axion'.");
      return 1;
    }
  }

  if (tier_provided && tier_invalid) {
    error("axion: --tier must be an integer in the range 1..9.");
    return 1;
  }
  if (!rollback_to.empty() && !is_valid_snapshot_hash_text(rollback_to)) {
    error("axion rollback: invalid snapshot hash.");
    return 1;
  }
  if (!audit_from.empty() && !is_valid_snapshot_hash_text(audit_from)) {
    error("axion audit: invalid --from snapshot hash.");
    return 1;
  }
  if (!audit_to.empty() && !is_valid_snapshot_hash_text(audit_to)) {
    error("axion audit: invalid --to snapshot hash.");
    return 1;
  }
  for (const auto& hash : snapshot_diff_hashes) {
    if (!is_valid_snapshot_hash_text(hash)) {
      error("axion snapshot-diff: invalid snapshot hash.");
      return 1;
    }
  }

  if (sub == "status") {
    const auto snapshot_hash = latest_snapshot_hash(canonfs_root);
    const auto snapshot_count = count_snapshot_dirs(canonfs_root);
    const auto object_count = count_canonfs_objects(canonfs_root);
    const bool canonfs_ready = fs::exists(canonfs_root);
    auto state = read_axion_state(canonfs_root);
    if (!state.present && tier_provided) {
      state.tier = target_tier;
    }
    const bool state_snapshot_present =
        state.active_snapshot &&
        fs::exists(canonfs_root / "snapshots" / *state.active_snapshot / "MANIFEST.txt");
    std::vector<std::string> issues;
    if (!canonfs_ready) {
      issues.emplace_back("canonfs_root_missing");
    }
    if (!state.present) {
      issues.emplace_back("axion_state_missing");
    }
    if (state.active_snapshot && !state_snapshot_present) {
      issues.emplace_back("active_snapshot_missing");
    }
    if (json_out) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.axion-status.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"axion\": \"active\",\n"
                << "  \"canonfs_ready\": " << (canonfs_ready ? "true" : "false") << ",\n"
                << "  \"state_present\": " << (state.present ? "true" : "false") << ",\n"
                << "  \"object_count\": " << object_count << ",\n"
                << "  \"snapshot_count\": " << snapshot_count << ",\n"
                << "  \"active_snapshot\": ";
      if (snapshot_hash) {
        std::cout << "\"" << json_escape(*snapshot_hash) << "\",\n";
      } else {
        std::cout << "null,\n";
      }
      std::cout << "  \"state_snapshot\": "
                << json_nullable_axion(state.active_snapshot) << ",\n"
                << "  \"state_snapshot_present\": " << (state_snapshot_present ? "true" : "false")
                << ",\n"
                << "  \"state_updated\": ";
      if (state.updated) {
        std::cout << *state.updated << ",\n";
      } else {
        std::cout << "null,\n";
      }
      std::cout << "  \"tier\": " << state.tier << ",\n"
                << "  \"issues\": [\n";
      for (std::size_t i = 0; i < issues.size(); ++i) {
        std::cout << "    \"" << json_escape(issues[i]) << "\"";
        if (i + 1 < issues.size()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      std::cout << "  ]\n}\n";
    } else {
      std::cout << "Axion Governor: active\n"
                << "CanonFS root: " << canonfs_root.string() << "\n"
                << "CanonFS ready: " << (canonfs_ready ? "yes" : "no") << "\n"
                << "State file: " << (state.present ? "present" : "missing") << "\n"
                << "Stored objects: " << object_count << "\n"
                << "Snapshots: " << snapshot_count << "\n"
                << "Active snapshot: " << (snapshot_hash ? *snapshot_hash : "<none>") << "\n"
                << "State snapshot: "
                << (state.active_snapshot ? *state.active_snapshot : "<none>") << "\n"
                << "State snapshot present: " << (state_snapshot_present ? "yes" : "no") << "\n"
                << "Active tier: " << state.tier << "\n";
      if (!issues.empty()) {
        std::cout << "Issues:\n";
        for (const auto& issue : issues) {
          std::cout << "  " << issue << "\n";
        }
      }
    }
    return 0;
  }

  if (sub == "optimize") {
    const auto previous_state = read_axion_state(canonfs_root);
    std::string snapshot_error;
    const auto snapshot_hash = t81::cli::canonfs_capture_snapshot_hash(canonfs_root, &snapshot_error);
    if (!snapshot_hash) {
      error("axion optimize: failed to capture current CanonFS snapshot: " + snapshot_error);
      return 1;
    }
    std::vector<std::string> only_previous;
    std::vector<std::string> only_current;
    if (previous_state.active_snapshot && *previous_state.active_snapshot != *snapshot_hash) {
      const auto previous_lines = read_manifest_lines(*previous_state.active_snapshot);
      const auto current_lines = read_manifest_lines(*snapshot_hash);
      std::set_difference(previous_lines.begin(), previous_lines.end(), current_lines.begin(),
                          current_lines.end(), std::back_inserter(only_previous));
      std::set_difference(current_lines.begin(), current_lines.end(), previous_lines.begin(),
                          previous_lines.end(), std::back_inserter(only_current));
    }
    if (!write_axion_state(target_tier, snapshot_hash.value_or(""))) {
      return 1;
    }
    if (json_out) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.axion-optimize.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"status\": \"ok\",\n"
                << "  \"action\": \"optimize\",\n"
                << "  \"tier\": " << target_tier << ",\n"
                << "  \"previous_snapshot\": "
                << json_nullable_axion(previous_state.active_snapshot) << ",\n"
                << "  \"active_snapshot\": ";
      if (snapshot_hash) {
        std::cout << "\"" << json_escape(*snapshot_hash) << "\",\n";
      } else {
        std::cout << "null,\n";
      }
      std::cout << "  \"only_previous_count\": " << only_previous.size() << ",\n"
                << "  \"only_current_count\": " << only_current.size() << "\n"
                << "}\n";
    } else {
      info("Axion optimize: captured CanonFS snapshot, diffed it against the previous state, and "
           "recorded requested tier " + std::to_string(target_tier) + ".");
      std::cout << "Optimization receipt recorded.\n";
      std::cout << "Tier: " << target_tier << "\n";
      std::cout << "Previous snapshot: "
                << (previous_state.active_snapshot ? *previous_state.active_snapshot : "<none>") << "\n";
      std::cout << "Snapshot: " << (snapshot_hash ? *snapshot_hash : "<none>") << "\n";
      std::cout << "Only previous: " << only_previous.size() << "\n";
      std::cout << "Only current:  " << only_current.size() << "\n";
    }
    return 0;
  }

  if (sub == "simulate") {
    if (simulate_file.empty()) {
      error("axion simulate requires a .tisc file argument.");
      return 1;
    }
    if (!fs::exists(simulate_file)) {
      error("axion simulate: file not found: " + simulate_file);
      return 1;
    }
    if (fs::path(simulate_file).extension() != ".tisc") {
      error("axion simulate requires a .tisc input file.");
      return 1;
    }
    auto program = t81::tisc::load_program(simulate_file);
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    const bool ok = static_cast<bool>(result);
    if (json_out) {
      std::cout << "{"
                << "\"schema\":\"t81.axion-simulate.v1\","
                << "\"ok\":" << (ok ? "true" : "false") << ","
                << "\"status\":\"" << (ok ? "ok" : "trap") << "\","
                << "\"action\":\"simulate\","
                << "\"program\":\"" << json_escape(simulate_file) << "\","
                << "\"trace_entries\":" << vm->state().trace.size() << ","
                << "\"axion_events\":" << vm->state().axion_log.size();
      if (ok) {
        std::cout << ",\"trap\":null";
      } else {
        std::cout << ",\"trap\":\"" << t81::vm::to_string(result.error()) << "\"";
      }
      std::cout << "}\n";
    } else if (ok) {
      info("Axion simulate: executed '" + simulate_file + "' under current policy state.");
      std::cout << "Simulation complete. Trace entries=" << vm->state().trace.size()
                << " axion_events=" << vm->state().axion_log.size() << "\n";
    } else {
      error("Axion simulation trapped: " + std::string(t81::vm::to_string(result.error())));
      return 1;
    }
    return 0;
  }

  if (sub == "explain") {
    if (explain_file.empty()) {
      error("axion explain requires a policy file argument.");
      return 1;
    }
    std::ifstream ifs(explain_file, fs::path(explain_file).extension() == ".axionb" ? std::ios::binary
                                                                                     : std::ios::in);
    if (!ifs) {
      error("axion explain: could not open file: " + explain_file);
      return 1;
    }
    t81::axion::Policy policy;
    std::optional<t81::axion::PolicyViolation> violation;
    if (fs::path(explain_file).extension() == ".axionb") {
      auto res = t81::axion::Policy::deserialize(ifs);
      if (!res) {
        error("axion explain: policy deserialization error: " + res.error());
        return 1;
      }
      policy = std::move(res.value());
    } else {
      std::string policy_text((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
      auto parsed = t81::axion::parse_policy(policy_text);
      if (!parsed) {
        error("axion explain: policy parse error: " + parsed.error());
        return 1;
      }
      policy = std::move(parsed.value());
      violation = t81::axion::PolicyValidator::validate_policy(policy_text);
    }
    const bool ok = !violation || !violation->is_violation;
    if (json_out) {
      std::cout << "{"
                << "\"schema\":\"t81.axion-explain.v1\","
                << "\"ok\":" << (ok ? "true" : "false") << ","
                << "\"policy\":\"" << json_escape(explain_file) << "\","
                << "\"tier\":" << policy.tier << ","
                << "\"allowed_tensor_hashes\":" << policy.allowed_tensor_hashes.size() << ","
                << "\"loop_hints\":" << policy.loops.size() << ","
                << "\"match_guards\":" << policy.match_guards.size();
      if (violation) {
        std::cout << ",\"rule\":\"" << json_escape(violation->rule) << "\","
                  << "\"reason\":\"" << json_escape(violation->reason) << "\"";
      }
      std::cout << "}\n";
    } else {
      std::cout << "Policy:                " << explain_file << "\n";
      std::cout << "Tier:                  " << policy.tier << "\n";
      std::cout << "Allowed tensor hashes: " << policy.allowed_tensor_hashes.size() << "\n";
      std::cout << "Loop hints:            " << policy.loops.size() << "\n";
      std::cout << "Match guards:          " << policy.match_guards.size() << "\n";
      if (violation) {
        std::cout << "Validation:            " << (violation->is_violation ? "VIOLATION" : "VALID")
                  << "\n";
        std::cout << "Rule:                  " << violation->rule << "\n";
        std::cout << "Reason:                " << violation->reason << "\n";
      }
    }
    return ok ? 0 : 1;
  }

  if (sub == "snapshot") {
    return t81::cli::canonfs_snapshot(canonfs_root, json_out);
  }

  if (sub == "snapshot-diff") {
    if (snapshot_diff_hashes.size() != 2) {
      error("axion snapshot-diff requires two snapshot hashes.");
      return 1;
    }
    return t81::cli::canonfs_snapshot_diff(snapshot_diff_hashes[0], snapshot_diff_hashes[1],
                                           canonfs_root, json_out);
  }

  if (sub == "rollback") {
    if (rollback_to.empty()) {
      error("axion rollback requires --to <hash>.");
      return 1;
    }
    return t81::cli::canonfs_rollback(rollback_to, canonfs_root, json_out);
  }

  if (sub == "log") {
    const fs::path state_path = canonfs_root / "axion" / "state.json";
    const auto state = read_axion_state(canonfs_root);
    const std::size_t snap_count = count_snapshot_dirs(canonfs_root);
    const std::size_t obj_count = count_canonfs_objects(canonfs_root);
    const auto recent = recent_snapshot_hashes(canonfs_root, log_tail_n);
    if (json_out) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.axion-log.v1\",\n"
                << "  \"canonfs_root\": \"" << json_escape(canonfs_root.string()) << "\",\n"
                << "  \"state_present\": " << (state.present ? "true" : "false") << ",\n"
                << "  \"tier\": " << state.tier << ",\n"
                << "  \"active_snapshot\": " << json_nullable_axion(state.active_snapshot) << ",\n"
                << "  \"snapshots\": " << snap_count << ",\n"
                << "  \"objects\": " << obj_count << ",\n"
                << "  \"tail\": " << log_tail_n << ",\n"
                << "  \"recent_snapshots\": [\n";
      for (std::size_t i = 0; i < recent.size(); ++i) {
        std::cout << "    \"" << json_escape(recent[i]) << "\"";
        if (i + 1 < recent.size()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      std::cout << "  ]\n"
                << "}\n";
    } else {
      std::cout << "CanonFS root:  " << canonfs_root.string() << "\n";
      std::cout << "State file:    " << (state.present ? "present" : "missing") << "\n";
      std::cout << "Tier:          " << state.tier << "\n";
      std::cout << "State snap:    "
                << (state.active_snapshot ? *state.active_snapshot : "<none>") << "\n";
      std::cout << "Snapshots:     " << snap_count << "\n";
      std::cout << "Objects:       " << obj_count << "\n";
      std::cout << "Recent snaps:  " << recent.size() << " (tail=" << log_tail_n << ")\n";
      for (const auto& hash : recent) {
        std::cout << "  " << hash << "\n";
      }
      if (state.present) {
        std::ifstream sf(state_path);
        if (sf) {
          std::cout << "\nAxion state:\n";
          std::string sl;
          while (std::getline(sf, sl)) std::cout << "  " << sl << "\n";
        }
      }
    }
    return 0;
  }

  if (sub == "audit") {
    const auto latest = latest_snapshot_hash(canonfs_root);
    const auto state = read_axion_state(canonfs_root);
    const std::string from_hash =
        audit_from.empty()
            ? (state.active_snapshot ? *state.active_snapshot : (latest ? *latest : ""))
            : audit_from;
    const std::string to_hash = audit_to.empty() ? (latest ? *latest : "") : audit_to;
    const std::size_t snap_count = count_snapshot_dirs(canonfs_root);
    const std::size_t obj_count = count_canonfs_objects(canonfs_root);
    const bool state_ok = state.present;
    std::vector<std::string> only_from;
    std::vector<std::string> only_to;
    if (!from_hash.empty() && !to_hash.empty()) {
      const auto from_lines = read_manifest_lines(from_hash);
      const auto to_lines = read_manifest_lines(to_hash);
      std::set_difference(from_lines.begin(), from_lines.end(), to_lines.begin(), to_lines.end(),
                          std::back_inserter(only_from));
      std::set_difference(to_lines.begin(), to_lines.end(), from_lines.begin(), from_lines.end(),
                          std::back_inserter(only_to));
    }
    if (json_out) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.axion-audit.v1\",\n"
                << "  \"canonfs_root\": \"" << json_escape(canonfs_root.string()) << "\",\n"
                << "  \"state_present\": " << (state_ok ? "true" : "false") << ",\n"
                << "  \"snapshots\": " << snap_count << ",\n"
                << "  \"objects\": " << obj_count << ",\n"
                << "  \"tier\": " << state.tier << ",\n"
                << "  \"from\": " << (from_hash.empty() ? "null" : "\"" + json_escape(from_hash) + "\"") << ",\n"
                << "  \"to\": " << (to_hash.empty() ? "null" : "\"" + json_escape(to_hash) + "\"") << ",\n"
                << "  \"only_from_count\": " << only_from.size() << ",\n"
                << "  \"only_to_count\": " << only_to.size() << "\n"
                << "}\n";
      return 0;
    }
    std::cout << "CanonFS root:  " << canonfs_root.string() << "\n";
    std::cout << "State file:    " << (state_ok ? "present" : "missing") << "\n";
    std::cout << "Tier:          " << state.tier << "\n";
    std::cout << "Snapshots:     " << snap_count << "\n";
    std::cout << "Objects:       " << obj_count << "\n";
    std::cout << "From snapshot: " << (from_hash.empty() ? "<none>" : from_hash) << "\n";
    std::cout << "To snapshot:   " << (to_hash.empty() ? "<none>" : to_hash) << "\n";
    std::cout << "Only from:     " << only_from.size() << "\n";
    std::cout << "Only to:       " << only_to.size() << "\n";
    return 0;
  }

  error("axion: unknown subcommand '" + sub +
        "'. Run 't81 axion --help' for usage.");
  return 1;
}

int run_canonfs_command(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" || args.command_args[0] == "--help") {
    return emit_help([&] { print_help_canonfs(); });
  }

  const std::string action = args.command_args[0];
  fs::path canonfs_root = discover_canonfs_root();
  bool as_json = false;
  bool dry_run = false;
  std::optional<fs::path> output_path = args.output;
  std::vector<std::string> positional;
  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--canonfs-root") {
      if (++i >= args.command_args.size()) {
        error("canonfs: missing value for --canonfs-root.");
        return 1;
      }
      canonfs_root = args.command_args[i];
    } else if (token == "--json") {
      as_json = true;
    } else if (token == "--dry-run") {
      dry_run = true;
    } else if (token == "--out" || token == "--output" || token == "-o") {
      if (++i >= args.command_args.size()) {
        error("canonfs get: missing value for " + token + ".");
        return 1;
      }
      output_path = fs::path(args.command_args[i]);
    } else if (token == "--to" && action == "rollback") {
      if (++i >= args.command_args.size()) {
        error("canonfs rollback: missing value for --to.");
        return 1;
      }
      positional.push_back(args.command_args[i]);
    } else if (!token.empty() && token[0] == '-') {
      error("canonfs: unknown option '" + token + "'. Run 't81 help canonfs'.");
      return 1;
    } else {
      positional.push_back(token);
    }
  }

  if (action == "put-file") {
    if (positional.size() != 1) {
      error("canonfs put-file requires exactly one input file.");
      return 1;
    }
    return t81::cli::canonfs_put_file(positional[0], canonfs_root);
  }
  if (action == "put-tensor") {
    if (positional.size() != 1) {
      error("canonfs put-tensor requires exactly one input file.");
      return 1;
    }
    return t81::cli::canonize_tensor(positional[0], canonfs_root);
  }
  if (action == "ls") {
    if (!positional.empty()) {
      error("canonfs ls does not accept positional arguments.");
      return 1;
    }
    return t81::cli::canonfs_list(canonfs_root, as_json);
  }
  if (action == "get") {
    if (positional.size() != 1) {
      error("canonfs get requires exactly one canonical hash.");
      return 1;
    }
    return t81::cli::canonfs_get(positional[0], output_path, canonfs_root, as_json);
  }
  if (action == "stat") {
    if (positional.size() != 1) {
      error("canonfs stat requires exactly one canonical hash.");
      return 1;
    }
    return t81::cli::canonfs_stat(positional[0], canonfs_root, as_json);
  }
  if (action == "verify") {
    if (positional.size() != 1) {
      error("canonfs verify requires exactly one canonical hash.");
      return 1;
    }
    return t81::cli::canonfs_verify(positional[0], canonfs_root, as_json);
  }
  if (action == "snapshot") {
    if (!positional.empty()) {
      error("canonfs snapshot does not accept positional arguments.");
      return 1;
    }
    return t81::cli::canonfs_snapshot(canonfs_root, as_json);
  }
  if (action == "snapshot-diff") {
    if (positional.size() != 2) {
      error("canonfs snapshot-diff requires exactly two snapshot hashes.");
      return 1;
    }
    return t81::cli::canonfs_snapshot_diff(positional[0], positional[1], canonfs_root, as_json);
  }
  if (action == "rollback") {
    if (positional.size() != 1) {
      error("canonfs rollback requires --to <hash>.");
      return 1;
    }
    if (dry_run) {
      const fs::path snapshot_dir = canonfs_root / "snapshots" / positional[0];
      const bool exists = fs::exists(snapshot_dir);
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.canonfs-rollback.v1\",\n"
                  << "  \"ok\": " << (exists ? "true" : "false") << ",\n"
                  << "  \"dry_run\": true,\n"
                  << "  \"snapshot\": \"" << json_escape(positional[0]) << "\",\n"
                  << "  \"canonfs_root\": \"" << json_escape(canonfs_root.string()) << "\"\n"
                  << "}\n";
      } else {
        std::cout << "Rollback dry-run\n";
        std::cout << "Snapshot:     " << positional[0] << "\n";
        std::cout << "CanonFS root: " << canonfs_root.string() << "\n";
        std::cout << "Status:       " << (exists ? "ready" : "snapshot not found") << "\n";
      }
      return exists ? 0 : 1;
    }
    return t81::cli::canonfs_rollback(positional[0], canonfs_root, as_json);
  }

  if (action == "gc") {
    // Collect all objects
    const fs::path objects_dir = canonfs_root / "objects";
    // Collect hashes referenced by any snapshot
    std::unordered_set<std::string> referenced;
    std::error_code ec;
    const fs::path snaps_dir = canonfs_root / "snapshots";
    if (fs::exists(snaps_dir)) {
      for (const auto& snap_entry : fs::directory_iterator(snaps_dir, ec)) {
        if (ec) break;
        if (!snap_entry.is_directory()) continue;
        for (const auto& f : fs::directory_iterator(snap_entry.path(), ec)) {
          if (ec) break;
          if (f.is_regular_file()) referenced.insert(f.path().stem().string());
        }
      }
    }
    std::size_t removed = 0;
    std::uintmax_t bytes_freed = 0;
    std::vector<std::string> removable;
    if (fs::exists(objects_dir)) {
      for (const auto& obj : fs::directory_iterator(objects_dir, ec)) {
        if (ec) break;
        if (!obj.is_regular_file()) continue;
        const std::string stem = obj.path().stem().string();
        if (referenced.find(stem) == referenced.end()) {
          const std::uintmax_t sz = fs::file_size(obj.path(), ec);
          if (dry_run) {
            removable.push_back(stem);
            ++removed;
            bytes_freed += sz;
          } else {
            fs::remove(obj.path(), ec);
            if (!ec) {
              ++removed;
              bytes_freed += sz;
            }
          }
        }
      }
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.canonfs-gc.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"dry_run\": " << (dry_run ? "true" : "false") << ",\n"
                << "  \"removed\": " << removed << ",\n"
                << "  \"bytes_freed\": " << bytes_freed;
      if (dry_run) {
        std::cout << ",\n  \"objects\": [\n";
        for (std::size_t i = 0; i < removable.size(); ++i) {
          std::cout << "    \"" << json_escape(removable[i]) << "\"";
          if (i + 1 < removable.size()) {
            std::cout << ",";
          }
          std::cout << "\n";
        }
        std::cout << "  ]\n";
      } else {
        std::cout << "\n";
      }
      std::cout
                << "}\n";
    } else {
      std::cout << "GC" << (dry_run ? " dry-run" : "") << ": "
                << (dry_run ? "would remove " : "removed ") << removed
                << " object(s), " << (dry_run ? "would free " : "freed ") << bytes_freed
                << " byte(s)\n";
    }
    return 0;
  }

  if (action == "fsck") {
    const fs::path objects_dir = canonfs_root / "objects";
    const fs::path snapshots_dir = canonfs_root / "snapshots";
    std::size_t checked_objects = 0;
    std::size_t checked_snapshots = 0;
    std::vector<std::string> errors;
    std::error_code ec;

    if (fs::exists(objects_dir)) {
      for (const auto& obj : fs::directory_iterator(objects_dir, ec)) {
        if (ec) {
          errors.push_back("failed reading objects directory");
          break;
        }
        if (fs::is_symlink(obj.symlink_status())) {
          errors.push_back(obj.path().filename().string() + ": symlinked objects are not permitted");
          continue;
        }
        if (!obj.is_regular_file() || obj.path().extension() != ".blk") {
          continue;
        }
        ++checked_objects;
        std::ifstream in(obj.path(), std::ios::binary);
        if (!in) {
          errors.push_back(obj.path().filename().string() + ": object unreadable");
          continue;
        }
        in.peek();
        if (in.bad()) {
          errors.push_back(obj.path().filename().string() + ": object read failed");
        }
      }
    }

    if (fs::exists(snapshots_dir)) {
      for (const auto& snap : fs::directory_iterator(snapshots_dir, ec)) {
        if (ec) {
          errors.push_back("failed reading snapshots directory");
          break;
        }
        if (fs::is_symlink(snap.symlink_status())) {
          errors.push_back("snapshot entry " + snap.path().filename().string() +
                           ": symlinked snapshots are not permitted");
          continue;
        }
        if (!snap.is_directory()) {
          continue;
        }
        ++checked_snapshots;
        const fs::path manifest = snap.path() / "MANIFEST.txt";
        if (!fs::exists(manifest)) {
          errors.push_back("snapshot " + snap.path().filename().string() + ": missing MANIFEST.txt");
          continue;
        }
        for (auto it = fs::recursive_directory_iterator(snap.path(), ec);
             it != fs::recursive_directory_iterator(); ++it) {
          if (ec) {
            errors.push_back("snapshot " + snap.path().filename().string() + ": scan failed");
            break;
          }
          if (fs::is_symlink(it->symlink_status())) {
            errors.push_back("snapshot " + snap.path().filename().string() +
                             ": symlinked content is not permitted");
            break;
          }
        }
        std::ifstream in(manifest, std::ios::binary);
        if (!in) {
          errors.push_back("snapshot " + snap.path().filename().string() + ": manifest unreadable");
        }
      }
    }

    const bool ok = errors.empty();
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.canonfs-fsck.v1\",\n"
                << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
                << "  \"canonfs_root\": \"" << json_escape(canonfs_root.string()) << "\",\n"
                << "  \"checked_objects\": " << checked_objects << ",\n"
                << "  \"checked_snapshots\": " << checked_snapshots << ",\n"
                << "  \"errors\": [\n";
      for (std::size_t i = 0; i < errors.size(); ++i) {
        std::cout << "    \"" << json_escape(errors[i]) << "\"";
        if (i + 1 < errors.size()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      std::cout << "  ]\n}\n";
    } else {
      std::cout << "CanonFS fsck\n";
      std::cout << "Root:       " << canonfs_root.string() << "\n";
      std::cout << "Objects:    " << checked_objects << "\n";
      std::cout << "Snapshots:  " << checked_snapshots << "\n";
      std::cout << "Status:     " << (ok ? "ok" : "errors") << "\n";
      for (const auto& msg : errors) {
        std::cout << "  " << msg << "\n";
      }
    }
    return ok ? 0 : 1;
  }

  if (action == "repair") {
    const fs::path snapshots_dir = canonfs_root / "snapshots";
    std::size_t repaired_snapshots = 0;
    std::uintmax_t bytes_reclaimed = 0;
    std::vector<std::string> repaired_paths;
    std::error_code ec;

    if (fs::exists(snapshots_dir)) {
      for (const auto& snap : fs::directory_iterator(snapshots_dir, ec)) {
        if (ec) {
          error("canonfs repair: failed reading snapshots directory.");
          return 1;
        }
        if (!snap.is_directory()) {
          continue;
        }
        const fs::path legacy_objects = snap.path() / "objects";
        if (!fs::exists(legacy_objects)) {
          continue;
        }
        if (fs::is_symlink(fs::symlink_status(legacy_objects, ec))) {
          error("canonfs repair: refusing to operate on symlinked path " + legacy_objects.string());
          return 1;
        }
        std::uintmax_t dir_bytes = 0;
        for (auto it = fs::recursive_directory_iterator(legacy_objects, ec);
             it != fs::recursive_directory_iterator(); ++it) {
          if (ec) break;
          if (it->is_regular_file()) {
            dir_bytes += fs::file_size(it->path(), ec);
            if (ec) break;
          }
        }
        if (ec) {
          error("canonfs repair: failed scanning " + legacy_objects.string());
          return 1;
        }
        repaired_paths.push_back(fs::relative(legacy_objects, canonfs_root).string());
        ++repaired_snapshots;
        bytes_reclaimed += dir_bytes;
        if (!dry_run) {
          fs::remove_all(legacy_objects, ec);
          if (ec) {
            error("canonfs repair: failed removing " + legacy_objects.string());
            return 1;
          }
        }
      }
    }

    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.canonfs-repair.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"dry_run\": " << (dry_run ? "true" : "false") << ",\n"
                << "  \"canonfs_root\": \"" << json_escape(canonfs_root.string()) << "\",\n"
                << "  \"repaired_snapshots\": " << repaired_snapshots << ",\n"
                << "  \"bytes_reclaimed\": " << bytes_reclaimed << ",\n"
                << "  \"paths\": [\n";
      for (std::size_t i = 0; i < repaired_paths.size(); ++i) {
        std::cout << "    \"" << json_escape(repaired_paths[i]) << "\"";
        if (i + 1 < repaired_paths.size()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      std::cout << "  ]\n}\n";
    } else {
      std::cout << "CanonFS repair" << (dry_run ? " dry-run" : "") << "\n";
      std::cout << "Root:              " << canonfs_root.string() << "\n";
      std::cout << "Repaired snapshots:" << repaired_snapshots << "\n";
      std::cout << "Bytes reclaimed:   " << bytes_reclaimed << "\n";
      for (const auto& path : repaired_paths) {
        std::cout << "  " << path << "\n";
      }
    }
    return 0;
  }

  error("canonfs: unknown action '" + action + "'. Run 't81 help canonfs'.");
  return 1;
}

struct DeterminismRunRecord {
  bool ok = false;
  std::optional<t81::vm::Trap> trap;
  std::string artifact_sha3_512;
  std::string output_sha3_512;
  std::string trace_sha3_512;
};

struct DeterminismBaselineEntry {
  std::string relative_path;
  DeterminismRunRecord record;
};

bool capture_determinism_run_record(const fs::path& input, const std::optional<fs::path>& policy_path,
                                    DeterminismRunRecord& record, std::string& error_message) {
  try {
    auto program = t81::tisc::load_program(input);
    if (policy_path) {
      std::ifstream ifs(*policy_path);
      if (!ifs) {
        error_message = "Could not open policy file: " + policy_path->string();
        return false;
      }
      std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
      program.axion_policy_text = content;
    }
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    std::string printed;
    for (const auto& line : vm->state().printed_output) {
      printed += line;
      printed.push_back('\n');
    }

    record.ok = static_cast<bool>(result);
    if (!result) {
      record.trap = result.error();
    }
    record.artifact_sha3_512 = sha3_512_file(input);
    record.output_sha3_512 = sha3_512_text(printed);
    record.trace_sha3_512 = sha3_512_text(render_trace_for_determinism(vm->state()));
    return true;
  } catch (const std::exception& e) {
    error_message = e.what();
    return false;
  }
}

std::vector<DeterminismBaselineEntry> collect_determinism_baseline_entries(
    const fs::path& source_dir, const std::optional<fs::path>& policy_path, std::string& error_message) {
  std::vector<DeterminismBaselineEntry> entries;
  std::error_code ec;
  for (const auto& entry : fs::recursive_directory_iterator(source_dir, ec)) {
    if (ec) {
      error_message = "determinism baseline: failed scanning " + source_dir.string();
      return {};
    }
    if (!entry.is_regular_file() || entry.path().extension() != ".tisc") {
      continue;
    }
    DeterminismBaselineEntry item;
    item.relative_path = fs::relative(entry.path(), source_dir).generic_string();
    if (!capture_determinism_run_record(entry.path(), policy_path, item.record, error_message)) {
      return {};
    }
    entries.push_back(std::move(item));
  }
  std::sort(entries.begin(), entries.end(),
            [](const auto& lhs, const auto& rhs) { return lhs.relative_path < rhs.relative_path; });
  return entries;
}

std::string json_nullable_string(const std::optional<std::string>& value) {
  if (!value) {
    return "null";
  }
  return "\"" + json_escape(*value) + "\"";
}

bool parse_determinism_baseline_file(const fs::path& baseline_file, fs::path& source_dir,
                                     std::vector<DeterminismBaselineEntry>& entries,
                                     std::string& error_message) {
  std::ifstream in(baseline_file, std::ios::binary);
  if (!in) {
    error_message = "Could not open baseline file: " + baseline_file.string();
    return false;
  }
  const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::smatch source_match;
  if (!std::regex_search(text, source_match,
                         std::regex(R"json("source_dir"\s*:\s*"([^"]+)")json")) ||
      source_match.size() != 2) {
    error_message = "determinism baseline: missing source_dir in baseline.json";
    return false;
  }
  source_dir = fs::path(source_match[1].str());

  const std::regex entry_re(
      R"json(\{"relative_path":\s*"([^"]+)",\s*"ok":\s*(true|false),\s*"trap":\s*(null|"([^"]*)"),\s*"artifact_sha3_512":\s*"([^"]+)",\s*"output_sha3_512":\s*"([^"]+)",\s*"trace_sha3_512":\s*"([^"]+)"\})json");
  auto begin = std::sregex_iterator(text.begin(), text.end(), entry_re);
  auto end = std::sregex_iterator();
  for (auto it = begin; it != end; ++it) {
    const std::smatch& match = *it;
    DeterminismBaselineEntry entry;
    entry.relative_path = match[1].str();
    entry.record.ok = (match[2].str() == "true");
    if (match[4].matched && !match[4].str().empty()) {
      const std::string trap_name = match[4].str();
      if (trap_name == "DecodeFault") entry.record.trap = t81::vm::Trap::DecodeFault;
      else if (trap_name == "TypeFault") entry.record.trap = t81::vm::Trap::TypeFault;
      else if (trap_name == "BoundsFault") entry.record.trap = t81::vm::Trap::BoundsFault;
      else if (trap_name == "StackFault") entry.record.trap = t81::vm::Trap::StackFault;
      else if (trap_name == "DivisionFault") entry.record.trap = t81::vm::Trap::DivisionFault;
      else if (trap_name == "SecurityFault") entry.record.trap = t81::vm::Trap::SecurityFault;
      else if (trap_name == "TierFault") entry.record.trap = t81::vm::Trap::TierFault;
      else if (trap_name == "ShapeFault") entry.record.trap = t81::vm::Trap::ShapeFault;
      else if (trap_name == "TrapInstruction") entry.record.trap = t81::vm::Trap::TrapInstruction;
      else if (trap_name == "Unimplemented") entry.record.trap = t81::vm::Trap::Unimplemented;
      else if (trap_name == "AssertionFailed") entry.record.trap = t81::vm::Trap::AssertionFailed;
      else if (trap_name == "EthicsViolation") entry.record.trap = t81::vm::Trap::EthicsViolation;
      else if (trap_name == "CapabilityDenied") entry.record.trap = t81::vm::Trap::CapabilityDenied;
      else if (trap_name == "None") entry.record.trap = t81::vm::Trap::None;
      else {
        error_message = "determinism baseline: unknown trap value '" + trap_name + "'";
        return false;
      }
    }
    entry.record.artifact_sha3_512 = match[5].str();
    entry.record.output_sha3_512 = match[6].str();
    entry.record.trace_sha3_512 = match[7].str();
    entries.push_back(std::move(entry));
  }

  if (entries.empty()) {
    error_message = "determinism baseline: baseline.json does not contain any entries";
    return false;
  }
  return true;
}

int run_determinism_command(const char* command_name, const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" || args.command_args[0] == "--help") {
    return emit_help([&] { print_help_determinism(); });
  }

  const std::string action = args.command_args[0];
  bool as_json = false;
  std::optional<fs::path> policy_path;
  std::optional<fs::path> source_dir_opt;
  std::size_t multi_run_count = 0;
  std::vector<std::string> positional;
  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (token == "--policy") {
      if (++i >= args.command_args.size()) {
        error("determinism: missing value for --policy.");
        return 1;
      }
      policy_path = fs::path(args.command_args[i]);
    } else if (token == "--source-dir") {
      if (++i >= args.command_args.size()) {
        error("determinism: missing value for --source-dir.");
        return 1;
      }
      source_dir_opt = fs::path(args.command_args[i]);
    } else if (token == "--count") {
      if (++i >= args.command_args.size()) {
        error("determinism: missing value for --count.");
        return 1;
      }
      try {
        multi_run_count = static_cast<std::size_t>(std::stoull(args.command_args[i]));
      } catch (...) {
        error("determinism: invalid value for --count.");
        return 1;
      }
    } else if (!token.empty() && token[0] == '-') {
      error("determinism: unknown option '" + token + "'. Run 't81 help determinism'.");
      return 1;
    } else {
      positional.push_back(token);
    }
  }

  if (action == "verify") {
    if (!positional.empty()) {
      const fs::path baseline_dir = fs::path(positional[0]);
      const fs::path baseline_file = baseline_dir / "baseline.json";
      if (fs::exists(baseline_file)) {
        fs::path source_dir;
        std::vector<DeterminismBaselineEntry> baseline_entries;
        std::string parse_error;
        if (!parse_determinism_baseline_file(baseline_file, source_dir, baseline_entries, parse_error)) {
          error(parse_error);
          return 1;
        }

        std::size_t mismatches = 0;
        std::vector<std::string> mismatch_paths;
        for (const auto& entry : baseline_entries) {
          const fs::path program_path = source_dir / fs::path(entry.relative_path);
          if (!fs::exists(program_path)) {
            ++mismatches;
            mismatch_paths.push_back(entry.relative_path + ": missing");
            continue;
          }
          DeterminismRunRecord current;
          std::string run_error;
          if (!capture_determinism_run_record(program_path, policy_path, current, run_error)) {
            ++mismatches;
            mismatch_paths.push_back(entry.relative_path + ": " + run_error);
            continue;
          }
          if (current.ok != entry.record.ok || current.trap != entry.record.trap ||
              current.artifact_sha3_512 != entry.record.artifact_sha3_512 ||
              current.output_sha3_512 != entry.record.output_sha3_512 ||
              current.trace_sha3_512 != entry.record.trace_sha3_512) {
            ++mismatches;
            mismatch_paths.push_back(entry.relative_path);
          }
        }

        const bool ok = (mismatches == 0);
        if (as_json) {
          std::cout << "{\n"
                    << "  \"schema\": \"t81.determinism-verify.v1\",\n"
                    << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
                    << "  \"baseline\": \"" << json_escape(baseline_file.string()) << "\",\n"
                    << "  \"source_dir\": \"" << json_escape(source_dir.string()) << "\",\n"
                    << "  \"checked\": " << baseline_entries.size() << ",\n"
                    << "  \"mismatches\": " << mismatches << ",\n"
                    << "  \"paths\": [\n";
          for (std::size_t i = 0; i < mismatch_paths.size(); ++i) {
            std::cout << "    \"" << json_escape(mismatch_paths[i]) << "\"";
            if (i + 1 < mismatch_paths.size()) {
              std::cout << ",";
            }
            std::cout << "\n";
          }
          std::cout << "  ]\n}\n";
        } else {
          std::cout << "Baseline:   " << baseline_file.string() << "\n";
          std::cout << "Source dir: " << source_dir.string() << "\n";
          std::cout << "Checked:    " << baseline_entries.size() << "\n";
          std::cout << "Mismatches: " << mismatches << "\n";
          for (const auto& mismatch : mismatch_paths) {
            std::cout << "  " << mismatch << "\n";
          }
        }
        return ok ? 0 : 1;
      }
    }
    Args repro_args = args;
    repro_args.command_args = positional;
    return run_repro_hash(command_name, repro_args);
  }
  if (action == "verify-run" || action == "compare-run" || action == "certify" || action == "explain") {
    if (positional.size() != 1) {
      error("determinism " + action + " requires exactly one .tisc input file.");
      return 1;
    }
    const fs::path input = positional[0];
    if (input.extension() != ".tisc") {
      error("determinism " + action + " expects a .tisc input file.");
      return 1;
    }
    struct RunSnapshot {
      bool ok = false;
      std::optional<t81::vm::Trap> trap;
      std::string printed;
      std::string output_hash;
      std::string trace_text;
      std::string trace_hash;
      std::vector<std::string> axion_reasons;
    };
    auto run_once = [&]() -> RunSnapshot {
      RunSnapshot snapshot;
      auto program = t81::tisc::load_program(input.string());
      if (policy_path) {
        std::ifstream ifs(*policy_path);
        if (!ifs) {
          throw std::runtime_error("Could not open policy file: " + policy_path->string());
        }
        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        program.axion_policy_text = content;
      }
      auto vm = t81::vm::make_interpreter_vm();
      vm->load_program(program);
      auto result = vm->run_to_halt();
      if (!result) {
        snapshot.trap = result.error();
      } else {
        snapshot.trap.reset();
      }
      for (const auto& line : vm->state().printed_output) {
        snapshot.printed += line;
        snapshot.printed.push_back('\n');
      }
      snapshot.trace_text = render_trace_for_determinism(vm->state());
      snapshot.trace_hash = sha3_512_text(snapshot.trace_text);
      snapshot.output_hash = sha3_512_text(snapshot.printed);
      for (const auto& event : vm->state().axion_log) {
        snapshot.axion_reasons.push_back(event.verdict.reason);
      }
      snapshot.ok = static_cast<bool>(result);
      return snapshot;
    };
    const RunSnapshot run1 = run_once();
    const RunSnapshot run2 = run_once();
    const bool identical = (run1.ok == run2.ok) && (run1.trap == run2.trap) &&
                           (run1.output_hash == run2.output_hash) &&
                           (run1.trace_hash == run2.trace_hash) &&
                           (run1.axion_reasons == run2.axion_reasons);
    std::string reason = "identical";
    std::optional<std::size_t> mismatch_index;
    std::optional<std::string> lhs_detail;
    std::optional<std::string> rhs_detail;
    if (run1.ok != run2.ok) {
      reason = "status_mismatch";
      lhs_detail = run1.ok ? "ok" : "trap";
      rhs_detail = run2.ok ? "ok" : "trap";
    } else if (run1.trap != run2.trap) {
      reason = "trap_mismatch";
      lhs_detail = run1.trap ? t81::vm::to_string(*run1.trap) : "null";
      rhs_detail = run2.trap ? t81::vm::to_string(*run2.trap) : "null";
    } else if (run1.output_hash != run2.output_hash) {
      reason = "output_mismatch";
      lhs_detail = run1.printed;
      rhs_detail = run2.printed;
    } else {
      auto split_lines_local = [](std::string_view text) {
        std::vector<std::string> lines;
        std::string current;
        for (char ch : text) {
          if (ch == '\n') {
            lines.push_back(current);
            current.clear();
          } else {
            current.push_back(ch);
          }
        }
        if (!current.empty()) {
          lines.push_back(current);
        }
        return lines;
      };
      const auto trace_lines_1 = split_lines_local(run1.trace_text);
      const auto trace_lines_2 = split_lines_local(run2.trace_text);
      const std::size_t shared = std::min(trace_lines_1.size(), trace_lines_2.size());
      for (std::size_t i = 0; i < shared; ++i) {
        if (trace_lines_1[i] != trace_lines_2[i]) {
          reason = "trace_entry_mismatch";
          mismatch_index = i;
          lhs_detail = trace_lines_1[i];
          rhs_detail = trace_lines_2[i];
          break;
        }
      }
      if (reason == "identical" && trace_lines_1.size() != trace_lines_2.size()) {
        reason = "trace_size_mismatch";
        mismatch_index = shared;
        lhs_detail = std::to_string(trace_lines_1.size());
        rhs_detail = std::to_string(trace_lines_2.size());
      }
      if (reason == "identical") {
        const std::size_t axion_shared =
            std::min(run1.axion_reasons.size(), run2.axion_reasons.size());
        for (std::size_t i = 0; i < axion_shared; ++i) {
          if (run1.axion_reasons[i] != run2.axion_reasons[i]) {
            reason = "axion_reason_mismatch";
            mismatch_index = i;
            lhs_detail = run1.axion_reasons[i];
            rhs_detail = run2.axion_reasons[i];
            break;
          }
        }
        if (reason == "identical" && run1.axion_reasons.size() != run2.axion_reasons.size()) {
          reason = "axion_event_count_mismatch";
          mismatch_index = axion_shared;
          lhs_detail = std::to_string(run1.axion_reasons.size());
          rhs_detail = std::to_string(run2.axion_reasons.size());
        }
      }
    }

    if (action == "certify") {
      const std::string program_hash = sha3_512_file(input);
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.determinism-certificate.v1\",\n"
                  << "  \"ok\": " << (identical ? "true" : "false") << ",\n"
                  << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                  << "  \"program_sha3_512\": \"" << program_hash << "\",\n"
                  << "  \"run1_output_sha3_512\": \"" << run1.output_hash << "\",\n"
                  << "  \"run2_output_sha3_512\": \"" << run2.output_hash << "\",\n"
                  << "  \"run1_trace_sha3_512\": \"" << run1.trace_hash << "\",\n"
                  << "  \"run2_trace_sha3_512\": \"" << run2.trace_hash << "\",\n"
                  << "  \"deterministic\": " << (identical ? "true" : "false") << ",\n"
                  << "  \"trap\": ";
        if (run1.trap) {
          std::cout << "\"" << json_escape(t81::vm::to_string(*run1.trap)) << "\"\n";
        } else {
          std::cout << "null\n";
        }
        std::cout << "}\n";
      } else {
        std::cout << "program_sha3_512=" << program_hash << "\n";
        std::cout << "output_sha3_512=" << run1.output_hash << "\n";
        std::cout << "trace_sha3_512=" << run1.trace_hash << "\n";
        std::cout << "deterministic=" << (identical ? "true" : "false") << "\n";
        if (run1.trap) {
          std::cout << "trap=" << t81::vm::to_string(*run1.trap) << "\n";
        }
      }
      return identical ? 0 : 1;
    }
    if (action == "verify-run") {
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.determinism-verify-run.v1\",\n"
                  << "  \"ok\": " << (identical ? "true" : "false") << ",\n"
                  << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                  << "  \"run1_output_sha3_512\": \"" << run1.output_hash << "\",\n"
                  << "  \"run2_output_sha3_512\": \"" << run2.output_hash << "\",\n"
                  << "  \"run1_trace_sha3_512\": \"" << run1.trace_hash << "\",\n"
                  << "  \"run2_trace_sha3_512\": \"" << run2.trace_hash << "\"";
        if (run1.trap || run2.trap) {
          std::cout << ",\n  \"run1_trap\": ";
          if (run1.trap) std::cout << "\"" << json_escape(t81::vm::to_string(*run1.trap)) << "\"";
          else std::cout << "null";
          std::cout << ",\n  \"run2_trap\": ";
          if (run2.trap) std::cout << "\"" << json_escape(t81::vm::to_string(*run2.trap)) << "\"";
          else std::cout << "null";
        }
        std::cout << "\n}\n";
      } else if (identical) {
        std::cout << "deterministic-run-verified\n";
      } else {
        std::cout << "deterministic-run-mismatch\n";
      }
      return identical ? 0 : 1;
    }
    if (action == "compare-run") {
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.determinism-compare-run.v1\",\n"
                  << "  \"ok\": " << (identical ? "true" : "false") << ",\n"
                  << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                  << "  \"reason\": \"" << json_escape(reason) << "\",\n"
                  << "  \"run1_output_sha3_512\": \"" << run1.output_hash << "\",\n"
                  << "  \"run2_output_sha3_512\": \"" << run2.output_hash << "\",\n"
                  << "  \"run1_trace_sha3_512\": \"" << run1.trace_hash << "\",\n"
                  << "  \"run2_trace_sha3_512\": \"" << run2.trace_hash << "\",\n"
                  << "  \"mismatch_index\": ";
        if (mismatch_index) std::cout << *mismatch_index << ",\n";
        else std::cout << "null,\n";
        std::cout << "  \"lhs_detail\": ";
        if (lhs_detail) std::cout << "\"" << json_escape(*lhs_detail) << "\",\n";
        else std::cout << "null,\n";
        std::cout << "  \"rhs_detail\": ";
        if (rhs_detail) std::cout << "\"" << json_escape(*rhs_detail) << "\",\n";
        else std::cout << "null,\n";
        std::cout << "  \"run1_trap\": ";
        if (run1.trap) std::cout << "\"" << json_escape(t81::vm::to_string(*run1.trap)) << "\",\n";
        else std::cout << "null,\n";
        std::cout << "  \"run2_trap\": ";
        if (run2.trap) std::cout << "\"" << json_escape(t81::vm::to_string(*run2.trap)) << "\"\n";
        else std::cout << "null\n";
        std::cout << "}\n";
      } else {
        std::cout << "Program:   " << input.string() << "\n";
        std::cout << "Result:    " << (identical ? "identical" : reason) << "\n";
        if (mismatch_index) std::cout << "Index:     " << *mismatch_index << "\n";
        if (lhs_detail) std::cout << "Run1:      " << *lhs_detail << "\n";
        if (rhs_detail) std::cout << "Run2:      " << *rhs_detail << "\n";
      }
      return identical ? 0 : 1;
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.determinism-explain.v1\",\n"
                << "  \"ok\": " << (identical ? "true" : "false") << ",\n"
                << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"reason\": \"" << json_escape(reason) << "\",\n"
                << "  \"run1_output_sha3_512\": \"" << run1.output_hash << "\",\n"
                << "  \"run2_output_sha3_512\": \"" << run2.output_hash << "\",\n"
                << "  \"run1_trace_sha3_512\": \"" << run1.trace_hash << "\",\n"
                << "  \"run2_trace_sha3_512\": \"" << run2.trace_hash << "\",\n"
                << "  \"mismatch_index\": ";
      if (mismatch_index) std::cout << *mismatch_index << ",\n";
      else std::cout << "null,\n";
      std::cout << "  \"lhs_detail\": ";
      if (lhs_detail) std::cout << "\"" << json_escape(*lhs_detail) << "\",\n";
      else std::cout << "null,\n";
      std::cout << "  \"rhs_detail\": ";
      if (rhs_detail) std::cout << "\"" << json_escape(*rhs_detail) << "\",\n";
      else std::cout << "null,\n";
      std::cout << "  \"run1_trap\": ";
      if (run1.trap) std::cout << "\"" << json_escape(t81::vm::to_string(*run1.trap)) << "\",\n";
      else std::cout << "null,\n";
      std::cout << "  \"run2_trap\": ";
      if (run2.trap) std::cout << "\"" << json_escape(t81::vm::to_string(*run2.trap)) << "\"\n";
      else std::cout << "null\n";
      std::cout << "}\n";
    } else if (identical) {
      std::cout << "determinism-explain: runs are identical\n";
    } else {
      std::cout << "determinism-explain: " << reason << "\n";
      if (mismatch_index) {
        std::cout << "Mismatch index: " << *mismatch_index << "\n";
      }
      if (lhs_detail) {
        std::cout << "Run1: " << *lhs_detail << "\n";
      }
      if (rhs_detail) {
        std::cout << "Run2: " << *rhs_detail << "\n";
      }
    }
    return identical ? 0 : 1;
  }
  if (action == "multi-run") {
    if (positional.size() != 1) {
      error("determinism multi-run requires exactly one .tisc input file.");
      return 1;
    }
    if (multi_run_count == 0) {
      error("determinism multi-run requires --count <n> with n >= 1.");
      return 1;
    }
    const fs::path input = positional[0];
    if (input.extension() != ".tisc") {
      error("determinism multi-run expects a .tisc input file.");
      return 1;
    }

    std::vector<std::string> signatures;
    std::optional<std::string> baseline;
    bool ok = true;
    std::size_t mismatch_run = 0;
    for (std::size_t run = 0; run < multi_run_count; ++run) {
      auto program = t81::tisc::load_program(input);
      if (policy_path) {
        std::ifstream ifs(*policy_path);
        if (!ifs) {
          error("Could not open policy file: " + policy_path->string());
          return 1;
        }
        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        program.axion_policy_text = content;
      }
      auto vm = t81::vm::make_interpreter_vm();
      vm->load_program(program);
      auto result = vm->run_to_halt();
      std::string printed;
      for (const auto& line : vm->state().printed_output) {
        printed += line;
        printed.push_back('\n');
      }
      const auto signature = sha3_512_text(render_trace_for_determinism(vm->state()) + "\n--\n" + printed);
      signatures.push_back(signature);
      if (!result) {
        ok = false;
        mismatch_run = run + 1;
        break;
      }
      if (!baseline) {
        baseline = signature;
      } else if (*baseline != signature) {
        ok = false;
        mismatch_run = run + 1;
        break;
      }
    }

    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.determinism-multi-run.v1\",\n"
                << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
                << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"count\": " << multi_run_count << ",\n"
                << "  \"completed\": " << signatures.size() << ",\n"
                << "  \"mismatch_run\": " << (mismatch_run == 0 ? 0 : mismatch_run) << ",\n"
                << "  \"signatures\": [\n";
      for (std::size_t i = 0; i < signatures.size(); ++i) {
        std::cout << "    \"" << json_escape(signatures[i]) << "\"";
        if (i + 1 < signatures.size()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      std::cout << "  ]\n}\n";
    } else {
      std::cout << "Program:    " << input.string() << "\n";
      std::cout << "Runs:       " << multi_run_count << "\n";
      std::cout << "Completed:  " << signatures.size() << "\n";
      std::cout << "Determinism:" << (ok ? " stable" : " diverged") << "\n";
      if (mismatch_run != 0) {
        std::cout << "Mismatch:   run " << mismatch_run << "\n";
      }
    }
    return ok ? 0 : 1;
  }
  if (action == "hash") {
    if (positional.size() != 1) {
      error("determinism hash requires exactly one input file.");
      return 1;
    }
    return t81::cli::determinism_hash_file(positional[0], as_json);
  }
  if (action == "trace-hash") {
    if (positional.size() != 1) {
      error("determinism trace-hash requires exactly one trace file.");
      return 1;
    }
    return t81::cli::determinism_hash_trace(positional[0], as_json);
  }
  if (action == "diff") {
    if (positional.size() != 2) {
      error("determinism diff requires exactly two input files.");
      return 1;
    }
    return t81::cli::determinism_diff_files(positional[0], positional[1], as_json);
  }
  if (action == "diff-trace") {
    if (positional.size() != 2) {
      error("determinism diff-trace requires exactly two trace files.");
      return 1;
    }
    return t81::cli::determinism_diff_trace_files(positional[0], positional[1], as_json);
  }

  if (action == "baseline") {
    if (positional.empty()) {
      error("determinism baseline requires an output directory. Run 't81 help determinism baseline'.");
      return 1;
    }
    const fs::path out_dir = fs::path(positional[0]);
    const fs::path source_dir = source_dir_opt.value_or(out_dir);
    std::error_code ec;
    fs::create_directories(out_dir, ec);
    std::string collect_error;
    const auto entries = collect_determinism_baseline_entries(source_dir, policy_path, collect_error);
    if (!collect_error.empty()) {
      error(collect_error);
      return 1;
    }
    const fs::path baseline_file = out_dir / "baseline.json";
    std::ofstream ofs(baseline_file, std::ios::trunc);
    if (!ofs) {
      error("determinism baseline: could not open output file: " + baseline_file.string());
      return 1;
    }
    ofs << "{\n"
        << "  \"schema\": \"t81.determinism-baseline.v1\",\n"
        << "  \"source_dir\": \"" << json_escape(source_dir.string()) << "\",\n"
        << "  \"entries\": [\n";
    for (std::size_t i = 0; i < entries.size(); ++i) {
      ofs << "    {\"relative_path\": \"" << json_escape(entries[i].relative_path)
          << "\", \"ok\": " << (entries[i].record.ok ? "true" : "false")
          << ", \"trap\": "
          << json_nullable_string(entries[i].record.trap
                                      ? std::optional<std::string>{t81::vm::to_string(*entries[i].record.trap)}
                                      : std::nullopt)
          << ", \"artifact_sha3_512\": \"" << entries[i].record.artifact_sha3_512
          << "\", \"output_sha3_512\": \"" << entries[i].record.output_sha3_512
          << "\", \"trace_sha3_512\": \"" << entries[i].record.trace_sha3_512 << "\"}";
      if (i + 1 < entries.size()) ofs << ",";
      ofs << "\n";
    }
    ofs << "  ]\n}\n";
    if (!ofs.good()) {
      error("determinism baseline: failed writing " + baseline_file.string());
      return 1;
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.determinism-baseline.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"baseline\": \"" << json_escape(baseline_file.string()) << "\",\n"
                << "  \"source_dir\": \"" << json_escape(source_dir.string()) << "\",\n"
                << "  \"count\": " << entries.size() << "\n"
                << "}\n";
    } else {
      std::cout << "Baseline written: " << baseline_file.string() << " (" << entries.size() << " entries)\n";
    }
    return 0;
  }

  if (action == "bisect") {
    if (positional.size() != 1) {
      error("determinism bisect requires a baseline directory. Run 't81 help determinism bisect'.");
      return 1;
    }
    const fs::path baseline_dir = fs::path(positional[0]);
    const fs::path baseline_file = baseline_dir / "baseline.json";
    if (!fs::exists(baseline_file)) {
      error("determinism bisect: baseline.json not found under " + baseline_dir.string());
      return 1;
    }
    fs::path source_dir;
    std::vector<DeterminismBaselineEntry> baseline_entries;
    std::string parse_error;
    if (!parse_determinism_baseline_file(baseline_file, source_dir, baseline_entries, parse_error)) {
      error(parse_error);
      return 1;
    }
    bool ok = true;
    std::optional<std::string> first_path;
    std::optional<std::string> reason;
    std::optional<DeterminismRunRecord> current;
    std::optional<DeterminismRunRecord> expected;
    for (const auto& entry : baseline_entries) {
      const fs::path current_file = source_dir / entry.relative_path;
      if (!fs::exists(current_file)) {
        ok = false;
        first_path = entry.relative_path;
        reason = "missing_artifact";
        expected = entry.record;
        break;
      }
      DeterminismRunRecord current_record;
      std::string capture_error;
      if (!capture_determinism_run_record(current_file, policy_path, current_record, capture_error)) {
        ok = false;
        first_path = entry.relative_path;
        reason = "execution_error";
        expected = entry.record;
        break;
      }
      if (current_record.ok != entry.record.ok) {
        ok = false;
        first_path = entry.relative_path;
        reason = "status_mismatch";
        current = current_record;
        expected = entry.record;
        break;
      }
      if (current_record.trap != entry.record.trap) {
        ok = false;
        first_path = entry.relative_path;
        reason = "trap_mismatch";
        current = current_record;
        expected = entry.record;
        break;
      }
      if (current_record.artifact_sha3_512 != entry.record.artifact_sha3_512) {
        ok = false;
        first_path = entry.relative_path;
        reason = "artifact_hash_mismatch";
        current = current_record;
        expected = entry.record;
        break;
      }
      if (current_record.output_sha3_512 != entry.record.output_sha3_512) {
        ok = false;
        first_path = entry.relative_path;
        reason = "output_hash_mismatch";
        current = current_record;
        expected = entry.record;
        break;
      }
      if (current_record.trace_sha3_512 != entry.record.trace_sha3_512) {
        ok = false;
        first_path = entry.relative_path;
        reason = "trace_hash_mismatch";
        current = current_record;
        expected = entry.record;
        break;
      }
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.determinism-bisect.v1\",\n"
                << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
                << "  \"baseline\": \"" << json_escape(baseline_file.string()) << "\",\n"
                << "  \"first_mismatch\": ";
      if (first_path) std::cout << "\"" << json_escape(*first_path) << "\",\n";
      else std::cout << "null,\n";
      std::cout << "  \"reason\": ";
      if (reason) std::cout << "\"" << json_escape(*reason) << "\",\n";
      else std::cout << "null,\n";
      std::cout << "  \"expected_trace_sha3_512\": "
                << json_nullable_string(expected ? std::optional<std::string>{expected->trace_sha3_512} : std::nullopt)
                << ",\n";
      std::cout << "  \"current_trace_sha3_512\": "
                << json_nullable_string(current ? std::optional<std::string>{current->trace_sha3_512} : std::nullopt)
                << "\n}\n";
    } else if (ok) {
      std::cout << "No baseline mismatch found.\n";
    } else {
      std::cout << "First mismatch: " << *first_path << "\n";
      std::cout << "Reason:         " << *reason << "\n";
    }
    return ok ? 0 : 1;
  }

  error("determinism: unknown action '" + action + "'. Run 't81 help determinism'.");
  return 1;
}

int run_tensor_command(const char* command_name, const Args& args) {
  (void)command_name;
  if (args.command_args.empty() || args.command_args[0] == "-h" || args.command_args[0] == "--help") {
    print_help_tensor();
    return 0;
  }

  const std::string action = args.command_args[0];
  if (action == "canonize") {
    if (args.command_args.size() != 2) {
      error("tensor canonize requires exactly one input file.");
      return 1;
    }
    return t81::cli::canonize_tensor(args.command_args[1], discover_canonfs_root());
  }
  if (action == "hash") {
    Args hash_args = args;
    hash_args.command = "determinism";
    return run_determinism_command(command_name, hash_args);
  }
  if (action == "inspect") {
    fs::path path;
    bool as_json = false;
    for (std::size_t i = 1; i < args.command_args.size(); ++i) {
      const std::string& token = args.command_args[i];
      if (token == "--json") {
        as_json = true;
      } else if (!token.empty() && token[0] == '-') {
        error("tensor inspect: unknown option '" + token + "'. Run 't81 help tensor inspect'.");
        return 1;
      } else if (path.empty()) {
        path = fs::path(token);
      } else {
        error("tensor inspect: unexpected argument '" + token + "'. Run 't81 help tensor inspect'.");
        return 1;
      }
    }
    if (path.empty()) {
      error("tensor inspect requires a .t81w file path.");
      return 1;
    }
    if (!has_extension_ci(path, ".t81w")) {
      error("tensor inspect currently supports .t81w artifacts. Use 't81 tensor hash' for raw files.");
      return 1;
    }
    Args info_args = args;
    info_args.command = "weights";
    info_args.command_args = {"info", path.string()};
    if (as_json) {
      info_args.command_args.emplace_back("--json");
    }
    return run_weights_info(info_args);
  }

  error("tensor: unknown subcommand '" + action + "'. Run 't81 help tensor'.");
  return 1;
}

int run_vm_command(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" || args.command_args[0] == "--help") {
    return emit_help([&] { print_help_vm(); });
  }

  const std::string action = args.command_args[0];

  // Validate action before parsing further to give a clear "unknown action" error
  // rather than a misleading "requires a .tisc input file" message.
  static constexpr std::string_view kVmActions[] = {
      "run", "debug", "trace", "until", "step", "regs", "stack", "mem",
      "state", "profile", "explain-trap"};
  const bool known_action =
      std::any_of(std::begin(kVmActions), std::end(kVmActions),
                  [&](std::string_view a) { return a == action; });
  if (!known_action) {
    error("vm: unknown action '" + action + "'. Run 't81 help vm'.");
    return 1;
  }

  std::optional<fs::path> policy_path;
  std::optional<fs::path> trace_path = args.output;
  bool as_json = false;
  std::size_t step_count = 1;
  std::size_t stack_limit = 10;
  std::optional<std::size_t> memory_addr;
  std::optional<std::size_t> target_pc;
  fs::path input;

  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--policy") {
      if (++i >= args.command_args.size()) {
        error("vm: missing value for --policy.");
        return 1;
      }
      policy_path = fs::path(args.command_args[i]);
    } else if (token == "--json") {
      as_json = true;
    } else if ((token == "--count" || token == "--steps" || token == "--limit" ||
                token == "--addr" || token == "--pc") &&
               i + 1 < args.command_args.size()) {
      std::size_t parsed = 0;
      try {
        parsed = static_cast<std::size_t>(std::stoull(args.command_args[++i]));
      } catch (...) {
        error("vm: invalid numeric value for " + token + ".");
        return 1;
      }
      if (token == "--count" || token == "--steps") {
        step_count = parsed;
      } else if (token == "--limit") {
        stack_limit = parsed;
      } else if (token == "--pc") {
        target_pc = parsed;
      } else {
        memory_addr = parsed;
      }
    } else if (token == "--out" || token == "--output" || token == "--trace-out" || token == "-o") {
      if (++i >= args.command_args.size()) {
        error("vm trace: missing value for " + token + ".");
        return 1;
      }
      trace_path = fs::path(args.command_args[i]);
    } else if (!token.empty() && token[0] == '-') {
      error("vm: unknown option '" + token + "'. Run 't81 help vm'.");
      return 1;
    } else if (input.empty()) {
      input = fs::path(token);
    } else {
      error("vm: unexpected argument '" + token + "'. Run 't81 help vm'.");
      return 1;
    }
  }

  if (input.empty()) {
    error("vm " + action + " requires a .tisc input file.");
    return 1;
  }
  if (input.extension() != ".tisc") {
    error("vm " + action + " expects a .tisc input file.");
    return 1;
  }

  if (action == "run") {
    if (trace_path) {
      return t81::cli::run_tisc(input, policy_path, true, trace_path);
    }
    return t81::cli::run_tisc(input, policy_path, false, std::nullopt);
  }
  if (action == "debug") {
    return t81::cli::debug_tisc(input, policy_path);
  }
  if (action == "trace") {
    if (!trace_path) {
      error("vm trace requires -o <trace.txt> or --out <trace.txt>.");
      return 1;
    }
    return t81::cli::run_tisc(input, policy_path, true, trace_path);
  }
  if (action == "until" || action == "step" || action == "regs" || action == "stack" || action == "mem") {
    auto program = t81::tisc::load_program(input);
    if (policy_path) {
      std::ifstream ifs(*policy_path);
      if (!ifs) {
        error("Could not open policy file: " + policy_path->string());
        return 1;
      }
      std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
      program.axion_policy_text = content;
    }
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    std::optional<t81::vm::Trap> trap;
    std::size_t executed_steps = 0;
    bool reached_target_pc = false;
    if (action == "until" && target_pc && vm->state().contexts[0].pc == *target_pc) {
      reached_target_pc = true;
    }
    for (; executed_steps < step_count && !vm->state().halted && !reached_target_pc; ++executed_steps) {
      auto result = vm->step();
      if (!result) {
        trap = result.error();
        break;
      }
      if (action == "until" && target_pc && vm->state().contexts[0].pc == *target_pc) {
        reached_target_pc = true;
      }
    }
    const auto& state = vm->state();
    const auto& ctx = state.contexts[0];
    const bool ok = !trap.has_value();

    if (action == "until") {
      if (!target_pc) {
        error("vm until requires --pc <n>.");
        return 1;
      }
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.vm-until.v1\",\n"
                  << "  \"ok\": " << ((ok && reached_target_pc) ? "true" : "false") << ",\n"
                  << "  \"target_pc\": " << *target_pc << ",\n"
                  << "  \"pc\": " << ctx.pc << ",\n"
                  << "  \"steps_requested\": " << step_count << ",\n"
                  << "  \"steps_executed\": " << executed_steps << ",\n"
                  << "  \"reached_target_pc\": " << (reached_target_pc ? "true" : "false") << ",\n"
                  << "  \"halted\": " << (state.halted ? "true" : "false") << ",\n"
                  << "  \"trap\": ";
        if (trap) std::cout << "\"" << json_escape(t81::vm::to_string(*trap)) << "\"\n";
        else std::cout << "null\n";
        std::cout << "}\n";
      } else {
        std::cout << "Target PC:      " << *target_pc << "\n";
        std::cout << "Steps executed: " << executed_steps << "\n";
        std::cout << "PC:             " << ctx.pc << "\n";
        std::cout << "Reached target: " << (reached_target_pc ? "yes" : "no") << "\n";
        std::cout << "Halted:         " << (state.halted ? "yes" : "no") << "\n";
        if (trap) {
          std::cout << "Trap:           " << t81::vm::to_string(*trap) << "\n";
        }
      }
      return (ok && reached_target_pc) ? 0 : 1;
    }

    if (action == "step") {
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.vm-step.v1\",\n"
                  << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
                  << "  \"steps_requested\": " << step_count << ",\n"
                  << "  \"steps_executed\": " << executed_steps << ",\n"
                  << "  \"pc\": " << ctx.pc << ",\n"
                  << "  \"halted\": " << (state.halted ? "true" : "false") << ",\n"
                  << "  \"trap\": ";
        if (trap) std::cout << "\"" << json_escape(t81::vm::to_string(*trap)) << "\"\n";
        else std::cout << "null\n";
        std::cout << "}\n";
      } else {
        std::cout << "Steps executed: " << executed_steps << "\n";
        std::cout << "PC:             " << ctx.pc << "\n";
        std::cout << "Halted:         " << (state.halted ? "yes" : "no") << "\n";
        if (trap) {
          std::cout << "Trap:           " << t81::vm::to_string(*trap) << "\n";
        }
      }
      return ok ? 0 : 1;
    }

    if (action == "regs") {
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.vm-regs.v1\",\n"
                  << "  \"pc\": " << ctx.pc << ",\n"
                  << "  \"steps_executed\": " << executed_steps << ",\n"
                  << "  \"registers\": [\n";
        for (std::size_t reg = 0; reg < ctx.registers.size(); ++reg) {
          std::cout << "    {\"index\": " << reg << ", \"value\": " << ctx.registers[reg]
                    << ", \"tag\": " << static_cast<int>(ctx.register_tags[reg]) << "}";
          if (reg + 1 < ctx.registers.size()) std::cout << ",";
          std::cout << "\n";
        }
        std::cout << "  ],\n  \"trap\": ";
        if (trap) std::cout << "\"" << json_escape(t81::vm::to_string(*trap)) << "\"\n";
        else std::cout << "null\n";
        std::cout << "}\n";
      } else {
        for (std::size_t reg = 0; reg < ctx.registers.size(); ++reg) {
          if (reg < 9 || ctx.registers[reg] != 0) {
            std::cout << "R" << reg << ": " << ctx.registers[reg] << " [tag="
                      << static_cast<int>(ctx.register_tags[reg]) << "]\n";
          }
        }
        std::cout << "PC: " << ctx.pc << "\n";
      }
      return ok ? 0 : 1;
    }

    if (action == "stack") {
      const std::size_t used = ctx.stack_base >= ctx.sp ? (ctx.stack_base - ctx.sp) : 0;
      if (as_json) {
        std::cout << "{\n"
                  << "  \"schema\": \"t81.vm-stack.v1\",\n"
                  << "  \"pc\": " << ctx.pc << ",\n"
                  << "  \"steps_executed\": " << executed_steps << ",\n"
                  << "  \"stack_used\": " << used << ",\n"
                  << "  \"entries\": [\n";
        std::size_t emitted = 0;
        for (std::size_t addr = ctx.sp; addr < ctx.stack_base && emitted < stack_limit; ++addr, ++emitted) {
          std::cout << "    {\"addr\": " << addr << ", \"value\": " << state.memory[addr] << "}";
          if (addr + 1 < ctx.stack_base && emitted + 1 < stack_limit) std::cout << ",";
          std::cout << "\n";
        }
        std::cout << "  ],\n  \"trap\": ";
        if (trap) std::cout << "\"" << json_escape(t81::vm::to_string(*trap)) << "\"\n";
        else std::cout << "null\n";
        std::cout << "}\n";
      } else {
        std::cout << "Stack used: " << used << "\n";
        std::size_t emitted = 0;
        for (std::size_t addr = ctx.sp; addr < ctx.stack_base && emitted < stack_limit; ++addr, ++emitted) {
          std::cout << "[" << addr << "] " << state.memory[addr] << "\n";
        }
      }
      return ok ? 0 : 1;
    }

    if (!memory_addr) {
      error("vm mem requires --addr <n>.");
      return 1;
    }
    if (*memory_addr >= state.memory.size()) {
      error("vm mem: address out of bounds.");
      return 1;
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.vm-mem.v1\",\n"
                << "  \"pc\": " << ctx.pc << ",\n"
                << "  \"steps_executed\": " << executed_steps << ",\n"
                << "  \"addr\": " << *memory_addr << ",\n"
                << "  \"value\": " << state.memory[*memory_addr] << ",\n"
                << "  \"tag\": " << static_cast<int>(state.memory_tags[*memory_addr]) << ",\n"
                << "  \"trap\": ";
      if (trap) std::cout << "\"" << json_escape(t81::vm::to_string(*trap)) << "\"\n";
      else std::cout << "null\n";
      std::cout << "}\n";
    } else {
      std::cout << "Memory[" << *memory_addr << "]: " << state.memory[*memory_addr]
                << " [tag=" << static_cast<int>(state.memory_tags[*memory_addr]) << "]\n";
    }
    return ok ? 0 : 1;
  }
  if (action == "state") {
    auto program = t81::tisc::load_program(input);
    if (policy_path) {
      std::ifstream ifs(*policy_path);
      if (!ifs) {
        error("Could not open policy file: " + policy_path->string());
        return 1;
      }
      std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
      program.axion_policy_text = content;
    }
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    const auto& state = vm->state();
    const auto& ctx = state.contexts.empty() ? t81::vm::ThreadContext{} : state.contexts.front();
    const bool ok = static_cast<bool>(result);
    if (as_json) {
      std::cout << "{\n";
      std::cout << "  \"schema\": \"t81.vm-state.v1\",\n";
      std::cout << "  \"ok\": " << (ok ? "true" : "false") << ",\n";
      std::cout << "  \"program\": \"" << json_escape(input.string()) << "\",\n";
      std::cout << "  \"halted\": " << (state.halted ? "true" : "false") << ",\n";
      std::cout << "  \"pc\": " << ctx.pc << ",\n";
      std::cout << "  \"sp\": " << ctx.sp << ",\n";
      std::cout << "  \"memory_words\": " << state.memory.size() << ",\n";
      std::cout << "  \"trace_entries\": " << state.trace.size() << ",\n";
      std::cout << "  \"axion_events\": " << state.axion_log.size() << ",\n";
      std::cout << "  \"printed_lines\": " << state.printed_output.size() << ",\n";
      if (ok) {
        std::cout << "  \"trap\": null\n";
      } else {
        std::cout << "  \"trap\": \"" << json_escape(t81::vm::to_string(result.error())) << "\"\n";
      }
      std::cout << "}\n";
      return ok ? 0 : 1;
    }
    std::cout << "Program:       " << input.string() << "\n";
    std::cout << "Status:        " << (ok ? "ok" : "trap") << "\n";
    std::cout << "Halted:        " << (state.halted ? "yes" : "no") << "\n";
    std::cout << "PC:            " << ctx.pc << "\n";
    std::cout << "SP:            " << ctx.sp << "\n";
    std::cout << "Memory words:  " << state.memory.size() << "\n";
    std::cout << "Trace entries: " << state.trace.size() << "\n";
    std::cout << "Axion events:  " << state.axion_log.size() << "\n";
    std::cout << "Printed lines: " << state.printed_output.size() << "\n";
    if (!ok) {
      std::cout << "Trap:          " << t81::vm::to_string(result.error()) << "\n";
      return 1;
    }
    return 0;
  }
  if (action == "profile" || action == "explain-trap") {
    auto program = t81::tisc::load_program(input);
    if (policy_path) {
      std::ifstream ifs(*policy_path);
      if (!ifs) {
        error("Could not open policy file: " + policy_path->string());
        return 1;
      }
      std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
      program.axion_policy_text = content;
    }
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    const auto started = std::chrono::steady_clock::now();
    auto result = vm->run_to_halt();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() -
                                                              started);
    const auto& state = vm->state();
    const auto& ctx = state.contexts.empty() ? t81::vm::ThreadContext{} : state.contexts.front();
    const bool ok = static_cast<bool>(result);

    if (action == "profile") {
      if (as_json) {
        std::cout << "{\n";
        std::cout << "  \"schema\": \"t81.vm-profile.v1\",\n";
        std::cout << "  \"ok\": " << (ok ? "true" : "false") << ",\n";
        std::cout << "  \"program\": \"" << json_escape(input.string()) << "\",\n";
        std::cout << "  \"elapsed_us\": " << elapsed.count() << ",\n";
        std::cout << "  \"trace_entries\": " << state.trace.size() << ",\n";
        std::cout << "  \"axion_events\": " << state.axion_log.size() << ",\n";
        std::cout << "  \"memory_words\": " << state.memory.size() << ",\n";
        std::cout << "  \"stack_peak_usage\": " << state.memory_stats.stack_peak_usage << ",\n";
        std::cout << "  \"heap_peak_usage\": " << state.memory_stats.heap_peak_usage << ",\n";
        std::cout << "  \"tensor_peak_usage\": " << state.memory_stats.tensor_peak_usage << ",\n";
        std::cout << "  \"meta_peak_usage\": " << state.memory_stats.meta_peak_usage << ",\n";
        std::cout << "  \"pc\": " << ctx.pc << ",\n";
        std::cout << "  \"sp\": " << ctx.sp << ",\n";
        if (ok) {
          std::cout << "  \"trap\": null\n";
        } else {
          std::cout << "  \"trap\": \"" << json_escape(t81::vm::to_string(result.error())) << "\"\n";
        }
        std::cout << "}\n";
        return ok ? 0 : 1;
      }
      std::cout << "Program:          " << input.string() << "\n";
      std::cout << "Elapsed:          " << elapsed.count() << " us\n";
      std::cout << "Trace entries:    " << state.trace.size() << "\n";
      std::cout << "Axion events:     " << state.axion_log.size() << "\n";
      std::cout << "Memory words:     " << state.memory.size() << "\n";
      std::cout << "Stack peak usage: " << state.memory_stats.stack_peak_usage << "\n";
      std::cout << "Heap peak usage:  " << state.memory_stats.heap_peak_usage << "\n";
      std::cout << "Tensor peak:      " << state.memory_stats.tensor_peak_usage << "\n";
      std::cout << "Meta peak:        " << state.memory_stats.meta_peak_usage << "\n";
      if (!ok) {
        std::cout << "Trap:             " << t81::vm::to_string(result.error()) << "\n";
        return 1;
      }
      return 0;
    }

    const auto trap = ok ? t81::vm::Trap::None : result.error();
    const std::string explanation = trap_explanation_text(trap);
    if (as_json) {
      std::cout << "{\n";
      std::cout << "  \"schema\": \"t81.vm-trap-explain.v1\",\n";
      std::cout << "  \"program\": \"" << json_escape(input.string()) << "\",\n";
      std::cout << "  \"ok\": " << (ok ? "true" : "false") << ",\n";
      if (ok) {
        std::cout << "  \"trap\": null,\n";
      } else {
        std::cout << "  \"trap\": \"" << json_escape(t81::vm::to_string(trap)) << "\",\n";
      }
      std::cout << "  \"explanation\": \"" << json_escape(explanation) << "\"\n";
      std::cout << "}\n";
      return ok ? 0 : 1;
    }
    std::cout << "Program:      " << input.string() << "\n";
    std::cout << "Result:       " << (ok ? "completed" : "trapped") << "\n";
    if (!ok) {
      std::cout << "Trap:         " << t81::vm::to_string(trap) << "\n";
    }
    std::cout << "Explanation:  " << explanation << "\n";
    return ok ? 0 : 1;
  }

  error("vm: unknown action '" + action + "'. Run 't81 help vm'.");
  return 1;
}

int run_profile_command(const Args& args) {
  if (args.input.empty()) {
    error("profile requires a .t81 or .tisc input file.");
    return 1;
  }
  if (args.input.extension() == ".t81") {
    auto weights_model_ptr = load_weights_model_optional(args.weights_model);
    if (args.weights_model && !weights_model_ptr) {
      return 1;
    }
    TempTiscFile temp(args.input.stem().string());
    int rc = t81::cli::compile(args.input, temp.path, {}, {}, weights_model_ptr);
    if (rc != 0) {
      return rc;
    }
    Args vm_args = args;
    vm_args.command = "vm";
    vm_args.command_args = {"profile", temp.path.string()};
    if (args.policy) {
      vm_args.command_args.push_back("--policy");
      vm_args.command_args.push_back(args.policy->string());
    }
    if (std::find(args.command_args.begin(), args.command_args.end(), "--json") != args.command_args.end()) {
      vm_args.command_args.push_back("--json");
    }
    return run_vm_command(vm_args);
  }
  if (args.input.extension() == ".tisc") {
    Args vm_args = args;
    vm_args.command = "vm";
    vm_args.command_args = {"profile", args.input.string()};
    if (args.policy) {
      vm_args.command_args.push_back("--policy");
      vm_args.command_args.push_back(args.policy->string());
    }
    if (std::find(args.command_args.begin(), args.command_args.end(), "--json") != args.command_args.end()) {
      vm_args.command_args.push_back("--json");
    }
    return run_vm_command(vm_args);
  }
  error("profile expects a .t81 or .tisc input file.");
  return 1;
}

int required_tier_for_program(const t81::tisc::Program& program) {
  std::smatch match;
  if (std::regex_search(program.axion_policy_text, match, std::regex(R"(\(tier\s+([0-9]+)\))")) &&
      match.size() == 2) {
    return std::stoi(match[1].str());
  }
  return 1;
}

int run_tier_command(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" || args.command_args[0] == "--help") {
    return emit_help([&] { print_help_tier(); });
  }

  const std::string action = args.command_args[0];
  bool as_json = false;
  fs::path input;
  int max_tier = 0;
  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const auto& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (token == "--max-tier") {
      if (i + 1 >= args.command_args.size()) {
        error("tier gate: missing value for --max-tier.");
        return 1;
      }
      try {
        max_tier = std::stoi(args.command_args[++i]);
      } catch (...) {
        error("tier gate: invalid value for --max-tier.");
        return 1;
      }
    } else if (!token.empty() && token[0] == '-') {
      error("tier: unknown option '" + token + "'. Run 't81 help tier'.");
      return 1;
    } else if (input.empty()) {
      input = fs::path(token);
    } else {
      error("tier: unexpected argument '" + token + "'. Run 't81 help tier'.");
      return 1;
    }
  }

  if (action == "info") {
    const std::vector<std::pair<int, std::string>> tiers = {
        {1, "Baseline symbolic execution"}, {2, "Reflective execution"},
        {3, "Recursive reasoning"},         {4, "Distributed planning"},
        {5, "Infinite refinement"},         {6, "Universal cognition"},
        {7, "Governed orchestration"},      {8, "Sovereign optimization"},
        {9, "Reserved experimental apex"}};
    if (as_json) {
      std::cout << "{\n  \"schema\": \"t81.tier-info.v1\",\n  \"tiers\": [\n";
      for (std::size_t i = 0; i < tiers.size(); ++i) {
        std::cout << "    {\"tier\":" << tiers[i].first << ",\"label\":\""
                  << json_escape(tiers[i].second) << "\"}";
        if (i + 1 < tiers.size()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      std::cout << "  ]\n}\n";
    } else {
      for (const auto& [tier, label] : tiers) {
        std::cout << "Tier " << tier << ": " << label << "\n";
      }
    }
    return 0;
  }

  if (input.empty() || input.extension() != ".tisc") {
    error("tier " + action + " expects a .tisc input file.");
    return 1;
  }
  auto program = t81::tisc::load_program(input);
  const int required_tier = required_tier_for_program(program);

  if (action == "check") {
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.tier-check.v1\",\n"
                << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"required_tier\": " << required_tier << "\n"
                << "}\n";
    } else {
      std::cout << "Program:       " << input.string() << "\n";
      std::cout << "Required tier: " << required_tier << "\n";
    }
    return 0;
  }

  if (action == "gate") {
    if (max_tier <= 0) {
      error("tier gate requires --max-tier <n>.");
      return 1;
    }
    const bool ok = required_tier <= max_tier;
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.tier-gate.v1\",\n"
                << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
                << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"required_tier\": " << required_tier << ",\n"
                << "  \"max_tier\": " << max_tier << "\n"
                << "}\n";
      return ok ? 0 : 1;
    }
    std::cout << "Program:       " << input.string() << "\n";
    std::cout << "Required tier: " << required_tier << "\n";
    std::cout << "Max tier:      " << max_tier << "\n";
    std::cout << "Result:        " << (ok ? "allowed" : "blocked") << "\n";
    return ok ? 0 : 1;
  }

  error("tier: unknown action '" + action + "'. Run 't81 help tier'.");
  return 1;
}

int run_tisc_command(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" || args.command_args[0] == "--help") {
    return emit_help([&] { print_help_tisc(); });
  }

  const std::string action = args.command_args[0];

  if (action == "diff") {
    bool as_json = false;
    fs::path input1, input2;
    for (std::size_t i = 1; i < args.command_args.size(); ++i) {
      const auto& tok = args.command_args[i];
      if (tok == "--json") as_json = true;
      else if (tok == "-h" || tok == "--help") { print_help_tisc_diff(); return 0; }
      else if (input1.empty()) input1 = fs::path(tok);
      else if (input2.empty()) input2 = fs::path(tok);
      else { error("tisc diff: unexpected argument '" + tok + "'."); return 1; }
    }
    if (input1.empty() || input2.empty()) {
      error("tisc diff requires two .tisc input files. Run 't81 help tisc diff'.");
      return 1;
    }
    const auto prog1 = t81::tisc::load_program(input1.string());
    const auto prog2 = t81::tisc::load_program(input2.string());
    auto insn_eq = [](const t81::tisc::Insn& x, const t81::tisc::Insn& y) {
      return x.opcode == y.opcode && x.a == y.a && x.b == y.b &&
             x.c == y.c && x.literal_kind == y.literal_kind;
    };
    const std::size_t max_i = std::max(prog1.insns.size(), prog2.insns.size());
    std::size_t diffs = 0;
    for (std::size_t idx = 0; idx < max_i; ++idx) {
      if (idx >= prog1.insns.size() || idx >= prog2.insns.size() ||
          !insn_eq(prog1.insns[idx], prog2.insns[idx])) {
        ++diffs;
      }
    }
    const bool identical = (diffs == 0);
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.tisc-diff.v1\",\n"
                << "  \"lhs\": \"" << json_escape(input1.string()) << "\",\n"
                << "  \"rhs\": \"" << json_escape(input2.string()) << "\",\n"
                << "  \"lhs_instructions\": " << prog1.insns.size() << ",\n"
                << "  \"rhs_instructions\": " << prog2.insns.size() << ",\n"
                << "  \"diff_count\": " << diffs << ",\n"
                << "  \"identical\": " << (identical ? "true" : "false") << "\n"
                << "}\n";
    } else {
      if (identical) {
        std::cout << "identical\n";
      } else {
        for (std::size_t idx = 0; idx < max_i; ++idx) {
          if (idx >= prog1.insns.size()) { std::cout << "+ [" << idx << "] <rhs extra>\n"; }
          else if (idx >= prog2.insns.size()) { std::cout << "- [" << idx << "] <lhs extra>\n"; }
          else if (!insn_eq(prog1.insns[idx], prog2.insns[idx])) {
            std::cout << "! [" << idx << "] differs\n";
          }
        }
        std::cout << diffs << " instruction(s) differ\n";
      }
    }
    return identical ? 0 : 1;
  }

  bool as_json = false;
  fs::path input;
  std::optional<fs::path> output = args.output;
  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (token == "--out" || token == "--output" || token == "-o") {
      if (++i >= args.command_args.size()) {
        error("tisc encode: missing value for " + token + ".");
        return 1;
      }
      output = fs::path(args.command_args[i]);
    } else if (!token.empty() && token[0] == '-') {
      error("tisc: unknown option '" + token + "'. Run 't81 help tisc'.");
      return 1;
    } else if (input.empty()) {
      input = fs::path(token);
    } else {
      error("tisc: unexpected argument '" + token + "'. Run 't81 help tisc'.");
      return 1;
    }
  }

  if (input.empty()) {
    error("tisc " + action + " requires an input file.");
    return 1;
  }
  if (action != "encode" && input.extension() != ".tisc") {
    error("tisc " + action + " expects a .tisc input file.");
    return 1;
  }

  if (action == "disasm") {
    return t81::cli::disasm_tisc(input);
  }
  if (action == "validate") {
    auto program = t81::tisc::load_program(input);
    if (as_json) {
      std::cout << "{\n";
      std::cout << "  \"schema\": \"t81.tisc-validate.v1\",\n";
      std::cout << "  \"ok\": true,\n";
      std::cout << "  \"program\": \"" << json_escape(input.string()) << "\",\n";
      std::cout << "  \"instructions\": " << program.insns.size() << ",\n";
      std::cout << "  \"float_pool\": " << program.float_pool.size() << ",\n";
      std::cout << "  \"tensor_pool\": " << program.tensor_pool.size() << "\n";
      std::cout << "}\n";
    } else {
      std::cout << "TISC artifact valid: " << input.string() << "\n";
      std::cout << "Instructions: " << program.insns.size() << "\n";
      std::cout << "Float pool:   " << program.float_pool.size() << "\n";
      std::cout << "Tensor pool:  " << program.tensor_pool.size() << "\n";
    }
    return 0;
  }
  if (action == "stats") {
    auto program = t81::tisc::load_program(input);
    std::map<std::string, std::size_t> opcode_counts;
    for (const auto& insn : program.insns) {
      ++opcode_counts[std::string(t81::tisc::opcode_name(insn.opcode))];
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.tisc-stats.v1\",\n"
                << "  \"program\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"instructions\": " << program.insns.size() << ",\n"
                << "  \"opcodes\": [\n";
      std::size_t emitted = 0;
      for (const auto& [opcode, count] : opcode_counts) {
        std::cout << "    {\"opcode\":\"" << json_escape(opcode) << "\",\"count\":" << count << "}";
        if (++emitted < opcode_counts.size()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      std::cout << "  ]\n}\n";
    } else {
      std::cout << "Program:      " << input.string() << "\n";
      std::cout << "Instructions: " << program.insns.size() << "\n";
      std::cout << "Opcodes:\n";
      for (const auto& [opcode, count] : opcode_counts) {
        std::cout << "  " << opcode << ": " << count << "\n";
      }
    }
    return 0;
  }
  if (action == "encode") {
    std::ifstream in(input, std::ios::binary);
    if (!in) {
      error("tisc encode: could not open input file: " + input.string());
      return 1;
    }
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    auto program_res = t81::tisc::base81_view::parse(text);
    if (!program_res) {
      error("tisc encode: failed to parse Base81 view: " + program_res.error().explain().str());
      return 1;
    }
    const fs::path out_path = output.value_or(input.stem().string() + ".tisc");
    t81::tisc::save_program(program_res.value(), out_path.string());
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.tisc-encode.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"input\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"output\": \"" << json_escape(out_path.string()) << "\",\n"
                << "  \"instructions\": " << program_res.value().insns.size() << "\n"
                << "}\n";
    } else {
      std::cout << "Encoded TISC artifact: " << out_path.string() << "\n";
    }
    return 0;
  }
  if (action == "decode") {
    const auto program = t81::tisc::load_program(input.string());
    const std::string rendered = t81::tisc::base81_view::render(program);
    if (output) {
      std::ofstream out(*output, std::ios::binary | std::ios::trunc);
      if (!out) {
        error("tisc decode: could not open output file: " + output->string());
        return 1;
      }
      out << rendered;
      if (!out.good()) {
        error("tisc decode: failed writing output file: " + output->string());
        return 1;
      }
    } else if (!as_json) {
      std::cout << rendered;
      if (rendered.empty() || rendered.back() != '\n') {
        std::cout << '\n';
      }
    }
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.tisc-decode.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"input\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"output\": ";
      if (output) {
        std::cout << "\"" << json_escape(output->string()) << "\",\n";
      } else {
        std::cout << "null,\n";
      }
      std::cout << "  \"instructions\": " << program.insns.size() << "\n"
                << "}\n";
    }
    return 0;
  }

  error("tisc: unknown action '" + action + "'. Run 't81 help tisc'.");
  return 1;
}

int run_ir_command(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" || args.command_args[0] == "--help") {
    return emit_help([&] { print_help_ir(); });
  }

  const std::string action = args.command_args[0];
  bool as_json = false;
  std::optional<fs::path> output = args.output;
  fs::path input;
  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (token == "--out" || token == "--output" || token == "-o") {
      if (++i >= args.command_args.size()) {
        error("ir " + action + ": missing value for " + token + ".");
        return 1;
      }
      output = fs::path(args.command_args[i]);
    } else if (!token.empty() && token[0] == '-') {
      error("ir: unknown option '" + token + "'. Run 't81 help ir'.");
      return 1;
    } else if (input.empty()) {
      input = fs::path(token);
    } else {
      error("ir: unexpected argument '" + token + "'. Run 't81 help ir'.");
      return 1;
    }
  }

  if (action != "show" && action != "dump" && action != "export" && action != "validate") {
    error("ir: unknown action '" + action + "'. Run 't81 help ir'.");
    return 1;
  }
  if (input.empty()) {
    error("ir " + action + " requires exactly one .t81 input file.");
    return 1;
  }
  if (input.extension() != ".t81") {
    error("ir " + action + " expects a .t81 input file.");
    return 1;
  }

  std::ifstream file(input);
  if (!file) {
    error("Could not open source file: " + input.string());
    return 1;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();

  t81::frontend::Lexer lexer(source);
  t81::frontend::Parser parser(lexer);
  auto stmts = parser.parse();
  if (parser.had_error()) {
    error("IR generation aborted due to parse errors.");
    return 1;
  }

  t81::frontend::SemanticAnalyzer semantic_analyzer(stmts);
  semantic_analyzer.analyze();
  if (semantic_analyzer.had_error()) {
    error("IR generation aborted due to semantic errors.");
    return 1;
  }

  if (action == "validate") {
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.ir-validate.v1\",\n"
                << "  \"ok\": true,\n"
                << "  \"source\": \"" << json_escape(input.string()) << "\",\n"
                << "  \"statements\": " << stmts.size() << "\n"
                << "}\n";
    } else {
      std::cout << "IR valid: " << input.string() << "\n";
      std::cout << "Statements: " << stmts.size() << "\n";
    }
    return 0;
  }

  t81::frontend::IRGenerator generator;
  auto program = generator.generate(stmts);
  const auto& instructions = program.instructions();

  const bool render_json = as_json || action == "export";
  if (render_json) {
    std::ostringstream rendered;
    rendered << "{\n"
             << "  \"schema\": \"t81.ir-export.v1\",\n"
             << "  \"source\": \"" << json_escape(input.string()) << "\",\n"
             << "  \"instruction_count\": " << instructions.size() << ",\n"
             << "  \"instructions\": [\n";
    for (std::size_t i = 0; i < instructions.size(); ++i) {
      const auto& inst = instructions[i];
      rendered << "    {\"index\":" << i << ",\"opcode\":\"" << ir_opcode_name(inst.opcode)
               << "\",\"primitive\":\"" << ir_primitive_name(inst.primitive) << "\",\"boolean_result\":"
               << (inst.boolean_result ? "true" : "false") << ",\"relation\":\""
               << ir_relation_name(inst.relation) << "\",\"operands\":[";
      for (std::size_t j = 0; j < inst.operands.size(); ++j) {
        rendered << "\"" << json_escape(format_ir_operand(inst.operands[j])) << "\"";
        if (j + 1 != inst.operands.size()) {
          rendered << ",";
        }
      }
      rendered << "]}";
      if (i + 1 != instructions.size()) {
        rendered << ",";
      }
      rendered << "\n";
    }
    rendered << "  ]\n"
             << "}\n";
    if (output) {
      std::ofstream out(*output, std::ios::binary | std::ios::trunc);
      if (!out) {
        error("ir export: could not open output file: " + output->string());
        return 1;
      }
      out << rendered.str();
      if (!out.good()) {
        error("ir export: failed writing output file: " + output->string());
        return 1;
      }
    } else {
      std::cout << rendered.str();
    }
    return 0;
  }

  std::cout << "IR Instructions (" << instructions.size() << " total):\n";
  for (const auto& inst : instructions) {
    std::cout << std::setw(20) << std::left << ir_opcode_name(inst.opcode);
    std::cout << " [" << ir_primitive_name(inst.primitive) << (inst.boolean_result ? " Bool" : "")
              << "]";
    if (inst.boolean_result &&
        inst.relation != t81::tisc::ir::ComparisonRelation::None) {
      std::cout << " <" << ir_relation_name(inst.relation) << ">";
    }
    if (!inst.operands.empty()) {
      std::cout << " | ";
      for (std::size_t i = 0; i < inst.operands.size(); ++i) {
        std::cout << format_ir_operand(inst.operands[i]);
        if (i + 1 < inst.operands.size()) {
          std::cout << ", ";
        }
      }
    }
    std::cout << "\n";
  }
  return 0;
}

std::optional<fs::path> find_repro_gate_script(const fs::path& exe_path) {
  std::vector<fs::path> candidates = {
      fs::current_path() / "scripts/ci/t81lang_repro_gate.py",
      exe_path.parent_path().parent_path() / "scripts/ci/t81lang_repro_gate.py",
      exe_path.parent_path() / "scripts/ci/t81lang_repro_gate.py",
  };
  for (const auto& path : candidates) {
    if (fs::exists(path)) {
      return path;
    }
  }
  return std::nullopt;
}

int run_repro_hash(const char* command_name, const Args& args) {
  fs::path exe_path = fs::absolute(fs::path(command_name));
  auto script_path = find_repro_gate_script(exe_path);
  if (!script_path) {
    error("Could not locate scripts/ci/t81lang_repro_gate.py");
    return 1;
  }

  fs::path repo_root = script_path->parent_path().parent_path().parent_path();
  const std::string fixtures_arg =
      args.command_args.empty() ? "tests/fixtures/t81lang_determinism" : args.command_args[0];
  const std::string expected_arg =
      args.command_args.empty()
          ? "tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt"
          : (fs::path(args.command_args[0]) / "t81lang_repro_hash.txt").string();

  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dist;
  fs::path workdir = fs::temp_directory_path() / ("t81-repro-hash-" + std::to_string(dist(gen)));
  fs::path hash_out = workdir / "hash.txt";

  std::vector<std::string> argv = {"python3",
                                   script_path->string(),
                                   "--t81-bin",
                                   exe_path.string(),
                                   "--fixtures-dir",
                                   fixtures_arg,
                                   "--workdir",
                                   workdir.string(),
                                   "--hash-out",
                                   hash_out.string(),
                                   "--expected-hash-file",
                                   expected_arg};

  const auto capture = run_process_capture(argv, repo_root);
  if (!capture.stdout_text.empty()) {
    std::cout << capture.stdout_text;
  }
  if (!capture.stderr_text.empty()) {
    std::cerr << capture.stderr_text;
  }
  int rc = capture.exit_code;
  if (rc == -1) {
    error("Failed to execute t81lang_repro_gate.py");
    return 1;
  }
  if (rc != 0) {
    return rc;
  }

  std::ifstream in(hash_out);
  if (!in) {
    error("Failed to read reproducibility hash output");
    return 1;
  }
  std::string hash;
  std::getline(in, hash);
  std::cout << hash << "\n";

  std::error_code ignore_ec;
  fs::remove_all(workdir, ignore_ec);
  return 0;
}

int run_memory_stats(const Args& args) {
  fs::path input = args.input;
  if (input.empty() && !args.command_args.empty()) {
    input = args.command_args[0];
  }

  if (input.empty()) {
    // Show current memory pool configuration
    std::cout << "\n=== Memory Pool Configuration ===" << std::endl;
    std::cout << "Default Stack Size: 256 words" << std::endl;
    std::cout << "Default Heap Size: 768 words" << std::endl;
    std::cout << "Default Tensor Space: 256 words" << std::endl;
    std::cout << "Default Meta Space: 256 words" << std::endl;
    std::cout << "Total Default Memory: 1,536 words" << std::endl;
    std::cout << "\nTo see memory usage for a specific program, run:" << std::endl;
    std::cout << "  t81 internal memory-stats <program.t81>" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
  }

  if (!fs::exists(input)) {
    error("File not found: " + input.string());
    return 1;
  }

  fs::path tisc_path = input;
  std::optional<TempTiscFile> temp_tisc;
  if (input.extension() == ".t81") {
    temp_tisc.emplace(input.stem().string());
    const int compile_rc = t81::cli::compile(input, temp_tisc->path);
    if (compile_rc != 0) {
      return compile_rc;
    }
    tisc_path = temp_tisc->path;
  } else if (input.extension() != ".tisc") {
    error("memory-stats expects a .t81 or .tisc input file.");
    return 1;
  }

  auto program = t81::tisc::load_program(tisc_path.string());
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  const auto& state = vm->state();
  const auto& ctx = state.contexts.empty() ? t81::vm::ThreadContext{} : state.contexts.front();

  std::cout << "\n=== Memory Pool Analysis ===" << std::endl;
  std::cout << "Program: " << input.string() << std::endl;
  std::cout << "Status: " << (result ? "completed" : "trapped") << std::endl;
  std::cout << "PC: " << ctx.pc << "  SP: " << ctx.sp << std::endl;
  std::cout << "\nRuntime footprint:" << std::endl;
  std::cout << "Memory words: " << state.memory.size() << std::endl;
  std::cout << "Tensor slots: " << state.tensors.size() << std::endl;
  std::cout << "Symbol count: " << state.symbols.size() << std::endl;
  std::cout << "Trace entries: " << state.trace.size() << std::endl;
  std::cout << "Axion events: " << state.axion_log.size() << std::endl;
  std::cout << "\nMemory stats:" << std::endl;
  std::cout << "Stack peak usage: " << state.memory_stats.stack_peak_usage << std::endl;
  std::cout << "Heap peak usage: " << state.memory_stats.heap_peak_usage << std::endl;
  std::cout << "Tensor peak usage: " << state.memory_stats.tensor_peak_usage << std::endl;
  std::cout << "Meta peak usage: " << state.memory_stats.meta_peak_usage << std::endl;
  std::cout << "Allocations: " << state.memory_stats.total_allocations << std::endl;
  std::cout << "Deallocations: " << state.memory_stats.total_deallocations << std::endl;
  std::cout << "Fragmentation events: " << state.memory_stats.fragmentation_count << std::endl;
  std::cout << "Memory efficiency: " << std::fixed << std::setprecision(3)
            << state.memory_stats.memory_efficiency << std::defaultfloat << std::endl;
  if (!result) {
    std::cout << "Trap: " << t81::vm::to_string(result.error()) << std::endl;
    std::cout << "=================================" << std::endl;
    return 1;
  }
  std::cout << "=================================" << std::endl;
  return 0;
}

int run_canonize_file(const Args& args) {
  if (args.command_args.empty()) {
    error("canonize-file requires an input file. Run 't81 help canonize-file'.");
    return 1;
  }

  fs::path input = args.command_args[0];
  fs::path canon_root = discover_canonfs_root();
  for (size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--canonfs-root") {
      if (++i >= args.command_args.size()) {
        error("canonize-file: missing value for --canonfs-root. Run 't81 help canonize-file'.");
        return 1;
      }
      canon_root = fs::path(args.command_args[i]);
    } else {
      error("canonize-file: unknown argument '" + token + "'. Run 't81 help canonize-file'.");
      return 1;
    }
  }

  std::ifstream in(input, std::ios::binary);
  if (!in) {
    error("Could not open input file: " + input.string());
    return 1;
  }
  std::vector<char> raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::vector<std::byte> bytes;
  bytes.reserve(raw.size());
  for (char c : raw) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
  }

  auto driver = t81::canonfs::make_persistent_driver(canon_root);
  auto write_res = driver->write_object(t81::canonfs::ObjectType::RawBlock, bytes);
  if (!write_res) {
    error("canonize-file: failed to write CanonFS object");
    return 1;
  }

  std::cout << "sha3-256:" << write_res->hash.h.to_string() << "\n";
  return 0;
}

int parse_int_arg(const std::string& text, const char* flag, int min_value, int max_value) {
  int value = 0;
  auto begin = text.data();
  auto end = text.data() + text.size();
  auto [ptr, ec] = std::from_chars(begin, end, value);
  if (ec != std::errc() || ptr != end || value < min_value || value > max_value) {
    error(std::string("invalid value for ") + flag + ": " + text);
    throw std::runtime_error("invalid numeric argument");
  }
  return value;
}

float parse_float_arg(const std::string& text, const char* flag, float min_value, float max_value) {
  char* tail = nullptr;
  const float value = std::strtof(text.c_str(), &tail);
  if (tail == text.c_str() || *tail != '\0' || value < min_value || value > max_value) {
    error(std::string("invalid value for ") + flag + ": " + text);
    throw std::runtime_error("invalid numeric argument");
  }
  return value;
}

std::string json_escape(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 8);
  for (char c : text) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  return out;
}

const char* ir_opcode_name(t81::tisc::ir::Opcode op) {
  using Opcode = t81::tisc::ir::Opcode;
  switch (op) {
    case Opcode::ADD: return "ADD";
    case Opcode::SUB: return "SUB";
    case Opcode::MUL: return "MUL";
    case Opcode::DIV: return "DIV";
    case Opcode::MOD: return "MOD";
    case Opcode::FADD: return "FADD";
    case Opcode::FSUB: return "FSUB";
    case Opcode::FMUL: return "FMUL";
    case Opcode::FDIV: return "FDIV";
    case Opcode::FRACADD: return "FRACADD";
    case Opcode::FRACSUB: return "FRACSUB";
    case Opcode::FRACMUL: return "FRACMUL";
    case Opcode::FRACDIV: return "FRACDIV";
    case Opcode::NEG: return "NEG";
    case Opcode::CMP: return "CMP";
    case Opcode::MOV: return "MOV";
    case Opcode::LOADI: return "LOADI";
    case Opcode::LOAD: return "LOAD";
    case Opcode::STORE: return "STORE";
    case Opcode::PUSH: return "PUSH";
    case Opcode::POP: return "POP";
    case Opcode::JMP: return "JMP";
    case Opcode::JZ: return "JZ";
    case Opcode::JNZ: return "JNZ";
    case Opcode::JN: return "JN";
    case Opcode::JP: return "JP";
    case Opcode::CALL: return "CALL";
    case Opcode::RET: return "RET";
    case Opcode::I2F: return "I2F";
    case Opcode::F2I: return "F2I";
    case Opcode::I2FRAC: return "I2FRAC";
    case Opcode::FRAC2I: return "FRAC2I";
    case Opcode::MAKE_OPTION_SOME: return "MAKE_OPTION_SOME";
    case Opcode::MAKE_OPTION_NONE: return "MAKE_OPTION_NONE";
    case Opcode::MAKE_RESULT_OK: return "MAKE_RESULT_OK";
    case Opcode::MAKE_RESULT_ERR: return "MAKE_RESULT_ERR";
    case Opcode::OPTION_IS_SOME: return "OPTION_IS_SOME";
    case Opcode::OPTION_UNWRAP: return "OPTION_UNWRAP";
    case Opcode::RESULT_IS_OK: return "RESULT_IS_OK";
    case Opcode::RESULT_UNWRAP_OK: return "RESULT_UNWRAP_OK";
    case Opcode::RESULT_UNWRAP_ERR: return "RESULT_UNWRAP_ERR";
    case Opcode::PRINT: return "PRINT";
    case Opcode::STRLEN: return "STRLEN";
    case Opcode::STREMPTY: return "STREMPTY";
    case Opcode::VECLEN: return "VECLEN";
    case Opcode::VECEMPTY: return "VECEMPTY";
    case Opcode::VECFIRST: return "VECFIRST";
    case Opcode::VECLAST: return "VECLAST";
    case Opcode::VECPUSH: return "VECPUSH";
    case Opcode::VECPOP: return "VECPOP";
    case Opcode::STRCONCAT: return "STRCONCAT";
    case Opcode::STRSTARTSWITH: return "STRSTARTSWITH";
    case Opcode::STRENDSWITH: return "STRENDSWITH";
    case Opcode::STRCONTAINS: return "STRCONTAINS";
    case Opcode::STRINDEXOF: return "STRINDEXOF";
    case Opcode::STRREPLACE: return "STRREPLACE";
    case Opcode::STRVECNEW: return "STRVECNEW";
    case Opcode::STRVECPUSH: return "STRVECPUSH";
    case Opcode::STRSPLIT: return "STRSPLIT";
    case Opcode::STRJOIN: return "STRJOIN";
    case Opcode::WEIGHTS_LOAD: return "WEIGHTS_LOAD";
    case Opcode::TGET: return "TGET";
    case Opcode::TNEW: return "TNEW";
    case Opcode::TSET: return "TSET";
    case Opcode::TSHAPE: return "TSHAPE";
    case Opcode::BITAND: return "BITAND";
    case Opcode::BITOR: return "BITOR";
    case Opcode::BITXOR: return "BITXOR";
    case Opcode::BITNOT: return "BITNOT";
    case Opcode::BITSHL: return "BITSHL";
    case Opcode::BITSHR: return "BITSHR";
    case Opcode::BITUSHR: return "BITUSHR";
    case Opcode::MAKE_COMPLEX: return "MAKE_COMPLEX";
    case Opcode::NOP: return "NOP";
    case Opcode::HALT: return "HALT";
    case Opcode::TRAP: return "TRAP";
    case Opcode::LABEL: return "LABEL";
    default: return "UNKNOWN";
  }
}

const char* ir_primitive_name(t81::tisc::ir::PrimitiveKind kind) {
  using PrimitiveKind = t81::tisc::ir::PrimitiveKind;
  switch (kind) {
    case PrimitiveKind::Integer: return "Int";
    case PrimitiveKind::Float: return "Float";
    case PrimitiveKind::Fraction: return "Frac";
    case PrimitiveKind::Boolean: return "Bool";
    default: return "Unknown";
  }
}

const char* ir_relation_name(t81::tisc::ir::ComparisonRelation relation) {
  using Relation = t81::tisc::ir::ComparisonRelation;
  switch (relation) {
    case Relation::Less: return "Less";
    case Relation::LessEqual: return "LessEqual";
    case Relation::Greater: return "Greater";
    case Relation::GreaterEqual: return "GreaterEqual";
    case Relation::Equal: return "Equal";
    case Relation::NotEqual: return "NotEqual";
    default: return "None";
  }
}

std::string format_ir_operand(const t81::tisc::ir::Operand& operand) {
  return std::visit(
      [](auto&& value) -> std::string {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, t81::tisc::ir::Register>) {
          return "R" + std::to_string(value.index);
        } else if constexpr (std::is_same_v<T, t81::tisc::ir::Immediate>) {
          return std::to_string(value.value);
        } else {
          return "Lbl" + std::to_string(value.id);
        }
      },
      operand);
}

std::string trap_explanation_text(t81::vm::Trap trap) {
  using Trap = t81::vm::Trap;
  switch (trap) {
    case Trap::DecodeFault:
      return "The VM could not decode the TISC artifact. Rebuild the program and validate it with 't81 tisc validate'.";
    case Trap::TypeFault:
      return "A runtime value had the wrong type tag for the executed instruction.";
    case Trap::BoundsFault:
      return "Execution accessed memory, tensor, or pool state outside valid bounds.";
    case Trap::StackFault:
      return "The VM encountered stack underflow, overflow, or an invalid stack frame transition.";
    case Trap::DivisionFault:
      return "Execution attempted division or modulus by an invalid divisor.";
    case Trap::SecurityFault:
      return "A governed operation violated VM security constraints.";
    case Trap::TierFault:
      return "A cognition-tier-specific constraint failed during execution.";
    case Trap::ShapeFault:
      return "Tensor or shape metadata was incompatible with the requested operation.";
    case Trap::TrapInstruction:
      return "The program executed an explicit TRAP instruction.";
    case Trap::Unimplemented:
      return "The program hit an opcode or runtime feature that is not implemented in this VM build.";
    case Trap::AssertionFailed:
      return "A program assertion failed.";
    case Trap::EthicsViolation:
      return "Axion rejected the execution path due to an ethics or policy violation.";
    case Trap::CapabilityDenied:
      return "The program attempted an operation without the required capability grant.";
    case Trap::None:
      return "Execution completed without a trap.";
  }
  return "Unknown trap state.";
}

std::string render_trace_for_determinism(const t81::vm::State& state) {
  std::ostringstream out;
  for (const auto& entry : state.trace) {
    out << "PC=" << entry.pc << ' ' << t81::tisc::opcode_name(entry.opcode);
    if (entry.trap) {
      out << " trap=" << t81::vm::to_string(*entry.trap);
    }
    out << '\n';
  }
  return out.str();
}

std::string sha3_512_text(std::string_view text) {
  std::vector<std::uint8_t> raw;
  raw.reserve(text.size());
  for (char c : text) {
    raw.push_back(static_cast<std::uint8_t>(c));
  }
  return t81::crypto::sha3_512_hex(raw);
}

std::string sha3_512_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Could not open input file: " + path.string());
  }
  std::vector<std::uint8_t> raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  return t81::crypto::sha3_512_hex(raw);
}

std::optional<fs::path> find_repo_root(fs::path start) {
  std::error_code ec;
  if (start.empty()) {
    return std::nullopt;
  }
  start = fs::absolute(start, ec);
  if (ec) {
    return std::nullopt;
  }
  if (fs::is_regular_file(start, ec)) {
    start = start.parent_path();
  }
  for (fs::path cur = start; !cur.empty(); cur = cur.parent_path()) {
    if (fs::exists(cur / "CMakeLists.txt", ec) || fs::exists(cur / ".git", ec)) {
      return cur;
    }
    if (cur == cur.root_path()) {
      break;
    }
  }
  return std::nullopt;
}

fs::path discover_repo_root() {
  const auto cwd = fs::current_path();
  if (auto found = find_repo_root(cwd)) {
    return *found;
  }
  return cwd;
}

fs::path discover_build_dir() {
  std::error_code ec;
  const fs::path cwd = fs::current_path();
  if (fs::exists(cwd / "CTestTestfile.cmake", ec)) {
    return cwd;
  }
  const fs::path repo_root = discover_repo_root();
  if (fs::exists(repo_root / "build" / "CTestTestfile.cmake", ec)) {
    return repo_root / "build";
  }
  return repo_root / "build";
}

fs::path discover_canonfs_root() {
  std::error_code ec;
  const fs::path cwd = fs::current_path();
  if (fs::exists(cwd / ".t81_canonfs", ec)) {
    return cwd / ".t81_canonfs";
  }
  const fs::path repo_root = discover_repo_root();
  return repo_root / ".t81_canonfs";
}

bool shell_command_available(const std::string& command) {
#if defined(_WIN32)
  std::string probe = "where " + command + " >nul 2>nul";
  const int status = std::system(probe.c_str());
  if (status == -1) {
    return false;
  }
  return decode_system_status(status) == 0;
#else
  if (command.empty() || command.find('/') != std::string::npos) {
    return false;
  }
  const char* path_env = std::getenv("PATH");
  if (!path_env) {
    return false;
  }
  std::stringstream ss(path_env);
  std::string dir;
  while (std::getline(ss, dir, ':')) {
    const fs::path candidate = (dir.empty() ? fs::current_path() : fs::path(dir)) / command;
    std::error_code ec;
    if (fs::exists(candidate, ec) && !fs::is_directory(candidate, ec) &&
        ::access(candidate.c_str(), X_OK) == 0) {
      return true;
    }
  }
  return false;
#endif
}

ProcessCaptureResult run_process_capture(
    const std::vector<std::string>& argv, const std::optional<fs::path>& cwd,
    const std::vector<std::pair<std::string, std::string>>& env_overrides) {
  ProcessCaptureResult result;
  if (argv.empty()) {
    return result;
  }
#if defined(_WIN32)
  std::string cmd;
  for (std::size_t i = 0; i < argv.size(); ++i) {
    if (i != 0) {
      cmd.push_back(' ');
    }
    cmd += shell_escape(argv[i]);
  }
  TempCaptureFile out_file("capture", ".out");
  TempCaptureFile err_file("capture", ".err");
  cmd += " > " + shell_escape(out_file.path.string()) + " 2> " + shell_escape(err_file.path.string());
  const int status = std::system(cmd.c_str());
  if (status == -1) {
    return result;
  }
  result.exit_code = decode_system_status(status);
  std::ifstream out_stream(out_file.path, std::ios::binary);
  std::ifstream err_stream(err_file.path, std::ios::binary);
  result.stdout_text.assign((std::istreambuf_iterator<char>(out_stream)), std::istreambuf_iterator<char>());
  result.stderr_text.assign((std::istreambuf_iterator<char>(err_stream)), std::istreambuf_iterator<char>());
  return result;
#else
  TempCaptureFile out_file("capture", ".out");
  TempCaptureFile err_file("capture", ".err");

  std::vector<std::string> argv_storage = argv;
  std::vector<char*> argv_ptrs;
  argv_ptrs.reserve(argv_storage.size() + 1);
  for (auto& item : argv_storage) {
    argv_ptrs.push_back(item.data());
  }
  argv_ptrs.push_back(nullptr);

  pid_t pid = fork();
  if (pid == 0) {
    int out_fd = open(out_file.path.c_str(), O_WRONLY | O_TRUNC);
    int err_fd = open(err_file.path.c_str(), O_WRONLY | O_TRUNC);
    if (out_fd == -1 || err_fd == -1) {
      _exit(127);
    }
    if (dup2(out_fd, STDOUT_FILENO) == -1 || dup2(err_fd, STDERR_FILENO) == -1) {
      _exit(127);
    }
    close(out_fd);
    close(err_fd);
    if (cwd && chdir(cwd->c_str()) != 0) {
      _exit(127);
    }
    for (const auto& [key, value] : env_overrides) {
      setenv(key.c_str(), value.c_str(), 1);
    }
    execvp(argv_ptrs[0], argv_ptrs.data());
    _exit(127);
  }
  if (pid < 0) {
    return result;
  }

  int status = 0;
  if (waitpid(pid, &status, 0) == -1) {
    return result;
  }
  result.exit_code = decode_system_status(status);
  std::ifstream out_stream(out_file.path, std::ios::binary);
  std::ifstream err_stream(err_file.path, std::ios::binary);
  result.stdout_text.assign((std::istreambuf_iterator<char>(out_stream)), std::istreambuf_iterator<char>());
  result.stderr_text.assign((std::istreambuf_iterator<char>(err_stream)), std::istreambuf_iterator<char>());
  return result;
#endif
}

std::optional<std::string> capture_command_stdout(const std::vector<std::string>& argv) {
  const auto result = run_process_capture(argv);
  if (result.exit_code != 0) {
    return std::nullopt;
  }
  std::string text = result.stdout_text;
  while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
    text.pop_back();
  }
  return text;
}

struct DoctorCheck {
  std::string id;
  bool ok = false;
  std::string detail;
  std::string remediation;
};

int run_env_paths(const Args& args) {
  bool as_json = false;
  for (const auto& token : args.command_args) {
    if (token == "--json") {
      as_json = true;
    } else if (token == "-h" || token == "--help") {
      print_help_env();
      return 0;
    } else {
      error("env paths: unknown option '" + token + "'. Run 't81 help env'.");
      return 1;
    }
  }

  const fs::path cwd = fs::current_path();
  const fs::path repo_root = discover_repo_root();
  const fs::path build_dir = discover_build_dir();
  const fs::path canonfs_root = discover_canonfs_root();
  const fs::path temp_dir = fs::temp_directory_path();
  const char* home = std::getenv("HOME");

  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.env-paths.v1\",\n"
              << "  \"cwd\": \"" << json_escape(cwd.string()) << "\",\n"
              << "  \"repo_root\": \"" << json_escape(repo_root.string()) << "\",\n"
              << "  \"build_dir\": \"" << json_escape(build_dir.string()) << "\",\n"
              << "  \"canonfs_root\": \"" << json_escape(canonfs_root.string()) << "\",\n"
              << "  \"temp_dir\": \"" << json_escape(temp_dir.string()) << "\",\n"
              << "  \"home\": ";
    if (home) {
      std::cout << "\"" << json_escape(home) << "\"\n";
    } else {
      std::cout << "null\n";
    }
    std::cout << "}\n";
    return 0;
  }

  std::cout << "cwd:          " << cwd.string() << "\n";
  std::cout << "repo_root:    " << repo_root.string() << "\n";
  std::cout << "build_dir:    " << build_dir.string() << "\n";
  std::cout << "canonfs_root: " << canonfs_root.string() << "\n";
  std::cout << "temp_dir:     " << temp_dir.string() << "\n";
  std::cout << "home:         " << (home ? home : "<unset>") << "\n";
  return 0;
}

int run_env_toolchain(const Args& args) {
  bool as_json = false;
  for (const auto& token : args.command_args) {
    if (token == "--json") {
      as_json = true;
    } else if (token == "-h" || token == "--help") {
      print_help_env();
      return 0;
    } else {
      error("env toolchain: unknown option '" + token + "'. Run 't81 help env'.");
      return 1;
    }
  }

  struct ToolProbe {
    std::string name;
    std::string command;
    bool available = false;
    std::optional<std::string> version;
  };

  std::vector<ToolProbe> probes = {
      {"cmake", "cmake", false, std::nullopt},
      {"ctest", "ctest", false, std::nullopt},
      {"python3", "python3", false, std::nullopt},
      {"clang++", "clang++", false, std::nullopt},
      {"g++", "g++", false, std::nullopt},
  };

  for (auto& probe : probes) {
    probe.available = shell_command_available(probe.command);
    if (probe.available) {
      probe.version = capture_command_stdout({probe.command, "--version"});
    }
  }

  bool any_compiler = false;
  for (const auto& probe : probes) {
    if (probe.name == "clang++" || probe.name == "g++") {
      any_compiler = any_compiler || probe.available;
    }
  }

  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.env-toolchain.v1\",\n"
              << "  \"ok\": " << ((probes[0].available && probes[1].available &&
                                      probes[2].available && any_compiler)
                                         ? "true"
                                         : "false")
              << ",\n"
              << "  \"tools\": [\n";
    for (std::size_t i = 0; i < probes.size(); ++i) {
      const auto& probe = probes[i];
      std::cout << "    {\"name\":\"" << json_escape(probe.name) << "\",\"available\":"
                << (probe.available ? "true" : "false") << ",\"version\":";
      if (probe.version) {
        std::cout << "\"" << json_escape(*probe.version) << "\"}";
      } else {
        std::cout << "null}";
      }
      if (i + 1 != probes.size()) {
        std::cout << ",";
      }
      std::cout << "\n";
    }
    std::cout << "  ]\n"
              << "}\n";
    return 0;
  }

  for (const auto& probe : probes) {
    std::cout << (probe.available ? "[ok]   " : "[miss] ") << probe.name;
    if (probe.version) {
      std::cout << ": " << *probe.version;
    }
    std::cout << "\n";
  }
  if (!any_compiler) {
    std::cout << "Compiler note: neither clang++ nor g++ was found in PATH.\n";
  }
  return 0;
}

int run_env_check(const Args& args) {
  bool as_json = false;
  for (const auto& token : args.command_args) {
    if (token == "--json") {
      as_json = true;
    } else if (token == "-h" || token == "--help") {
      print_help_env_check();
      return 0;
    } else {
      error("env check: unknown option '" + token + "'. Run 't81 help env check'.");
      return 1;
    }
  }

  const fs::path build_dir = discover_build_dir();
  const bool build_ready = fs::exists(build_dir / "CTestTestfile.cmake");
  const bool toolchain_ok = shell_command_available("cmake") && shell_command_available("ctest") &&
                            (shell_command_available("clang++") || shell_command_available("g++"));
  const bool ok = build_ready && toolchain_ok;

  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.env-check.v1\",\n"
              << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
              << "  \"build_ready\": " << (build_ready ? "true" : "false") << ",\n"
              << "  \"toolchain_ok\": " << (toolchain_ok ? "true" : "false") << "\n"
              << "}\n";
  } else if (!ok) {
    if (!build_ready) std::cerr << "error: build directory not ready (run cmake + ninja first)\n";
    if (!toolchain_ok) std::cerr << "error: toolchain incomplete (cmake/ctest/compiler missing)\n";
  }
  return ok ? 0 : 1;
}

int run_env_diag(const Args& args) {
  bool as_json = false;
  for (const auto& token : args.command_args) {
    if (token == "--json") {
      as_json = true;
    } else if (token == "-h" || token == "--help") {
      print_help_env_diag();
      return 0;
    } else {
      error("env diag: unknown option '" + token + "'. Run 't81 help env'.");
      return 1;
    }
  }

  const fs::path repo_root = discover_repo_root();
  const fs::path build_dir = discover_build_dir();
  const fs::path canonfs_root = discover_canonfs_root();
  const bool build_metadata_present = fs::exists(build_dir / "CTestTestfile.cmake");
  const bool canonfs_ready = fs::exists(canonfs_root);
  const bool axion_state_present = fs::exists(canonfs_root / "axion" / "state.json");
  const bool toolchain_ok = shell_command_available("cmake") && shell_command_available("ctest") &&
                            shell_command_available("python3") &&
                            (shell_command_available("clang++") || shell_command_available("g++"));
  const bool ok = build_metadata_present && toolchain_ok;

  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.env-diag.v1\",\n"
              << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
              << "  \"repo_root\": \"" << json_escape(repo_root.string()) << "\",\n"
              << "  \"build_dir\": \"" << json_escape(build_dir.string()) << "\",\n"
              << "  \"canonfs_root\": \"" << json_escape(canonfs_root.string()) << "\",\n"
              << "  \"build_metadata_present\": " << (build_metadata_present ? "true" : "false")
              << ",\n"
              << "  \"toolchain_ok\": " << (toolchain_ok ? "true" : "false") << ",\n"
              << "  \"canonfs_ready\": " << (canonfs_ready ? "true" : "false") << ",\n"
              << "  \"axion_state_present\": " << (axion_state_present ? "true" : "false") << "\n"
              << "}\n";
    return ok ? 0 : 2;
  }

  std::cout << "repo_root:             " << repo_root.string() << "\n";
  std::cout << "build_dir:             " << build_dir.string() << "\n";
  std::cout << "build metadata:        " << (build_metadata_present ? "present" : "missing") << "\n";
  std::cout << "toolchain:             " << (toolchain_ok ? "ok" : "incomplete") << "\n";
  std::cout << "canonfs_root:          " << canonfs_root.string() << "\n";
  std::cout << "canonfs_ready:         " << (canonfs_ready ? "yes" : "no") << "\n";
  std::cout << "axion_state_present:   " << (axion_state_present ? "yes" : "no") << "\n";
  return ok ? 0 : 2;
}

int run_env_clean(const Args& args) {
  bool as_json = false;
  bool force = false;
  fs::path build_dir = discover_build_dir();
  for (std::size_t i = 0; i < args.command_args.size(); ++i) {
    const auto& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (token == "--force") {
      force = true;
    } else if (token == "--build-dir") {
      if (i + 1 >= args.command_args.size()) {
        error("env clean: missing value for --build-dir.");
        return 1;
      }
      build_dir = fs::path(args.command_args[++i]);
    } else if (token == "-h" || token == "--help") {
      return emit_help(
          [&] { std::cerr << "Usage: t81 env clean [--build-dir <path>] [--force] [--json]\n"; });
    } else {
      error("env clean: unknown option '" + token + "'. Run 't81 help env clean'.");
      return 1;
    }
  }

  const fs::path repo_root = discover_repo_root();
  std::error_code ec;
  const fs::path abs_repo_root = fs::weakly_canonical(repo_root, ec);
  ec.clear();
  const fs::path abs_build_dir =
      fs::exists(build_dir, ec) ? fs::weakly_canonical(build_dir, ec) : fs::absolute(build_dir, ec);
  if (ec) {
    error("env clean: could not resolve build dir: " + build_dir.string());
    return 1;
  }
  if (abs_build_dir.empty() || abs_build_dir == abs_build_dir.root_path()) {
    error("env clean: refusing to clean an unsafe root path.");
    return 1;
  }
  if (abs_build_dir == abs_repo_root) {
    error("env clean: refusing to clean the repository root.");
    return 1;
  }
  const auto rel_to_repo = fs::relative(abs_build_dir, abs_repo_root, ec);
  const bool inside_repo = !ec && !rel_to_repo.empty() && *rel_to_repo.begin() != "..";
  if (!force && !inside_repo) {
    error("env clean: target is outside the repository root; rerun with --force to allow it.");
    return 1;
  }

  std::vector<fs::path> removed_paths;
  const std::vector<fs::path> targets = {
      abs_build_dir / "CMakeCache.txt", abs_build_dir / "CMakeFiles", abs_build_dir / "CTestTestfile.cmake",
      abs_build_dir / "Testing",        abs_build_dir / "build.ninja", abs_build_dir / ".ninja_deps",
      abs_build_dir / ".ninja_log"};

  for (const auto& target : targets) {
    std::uintmax_t count = fs::remove_all(target, ec);
    if (ec) {
      error("env clean: failed removing " + target.string());
      return 1;
    }
    if (count > 0) {
      removed_paths.push_back(target);
    }
  }

  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.env-clean.v1\",\n"
              << "  \"ok\": true,\n"
              << "  \"build_dir\": \"" << json_escape(abs_build_dir.string()) << "\",\n"
              << "  \"forced\": " << (force ? "true" : "false") << ",\n"
              << "  \"removed\": [\n";
    for (std::size_t i = 0; i < removed_paths.size(); ++i) {
      std::cout << "    \"" << json_escape(removed_paths[i].string()) << "\"";
      if (i + 1 < removed_paths.size()) {
        std::cout << ",";
      }
      std::cout << "\n";
    }
    std::cout << "  ]\n}\n";
    return 0;
  }

  std::cout << "Cleaned build artifacts under " << abs_build_dir.string() << "\n";
  for (const auto& path : removed_paths) {
    std::cout << "  removed " << path.string() << "\n";
  }
  if (removed_paths.empty()) {
    std::cout << "  nothing to remove\n";
  }
  return 0;
}

int run_doctor(const Args& args) {
  bool as_json = false;
  std::optional<std::string> scope;
  for (const auto& token : args.command_args) {
    if (token == "--json") {
      as_json = true;
    } else if (token == "-h" || token == "--help") {
      print_help_doctor();
      return 0;
    } else if (!scope) {
      scope = token;
    } else {
      error("doctor: unknown option '" + token + "'. Run 't81 help doctor'.");
      return 1;
    }
  }

  if (scope && *scope != "toolchain" && *scope != "canonfs" && *scope != "vm") {
    error("doctor: unknown scope '" + *scope + "'. Expected toolchain, canonfs, or vm.");
    return 1;
  }

  std::vector<DoctorCheck> checks;
  const fs::path build_dir = discover_build_dir();
  const fs::path canonfs_root = discover_canonfs_root();

  auto push_check = [&](std::string id, bool ok, std::string detail, std::string remediation) {
    DoctorCheck check;
    check.id = std::move(id);
    check.ok = ok;
    check.detail = std::move(detail);
    check.remediation = std::move(remediation);
    checks.push_back(std::move(check));
  };

  auto run_toolchain_checks = [&]() {
    const bool cmake_ok = shell_command_available("cmake");
    push_check("cmake_available", cmake_ok,
               cmake_ok ? "cmake found in PATH" : "cmake not found in PATH",
               "Install CMake and ensure `cmake` is in PATH.");
    const bool ctest_ok = shell_command_available("ctest");
    push_check("ctest_available", ctest_ok,
               ctest_ok ? "ctest found in PATH" : "ctest not found in PATH",
               "Install CMake/CTest and ensure `ctest` is in PATH.");
    const bool compiler_ok = shell_command_available("clang++") || shell_command_available("g++");
    push_check("compiler_available", compiler_ok,
               compiler_ok ? "C++ compiler found in PATH" : "clang++/g++ not found in PATH",
               "Install a supported C++ compiler and ensure it is in PATH.");
    const bool python_ok = shell_command_available("python3");
    push_check("python3_available", python_ok,
               python_ok ? "python3 found in PATH" : "python3 not found in PATH",
               "Install Python 3 for CI/ops utilities (repro-hash/docs gates).");
  };

  auto run_canonfs_checks = [&]() {
    const bool root_exists = fs::exists(canonfs_root);
    push_check("canonfs_root_present", root_exists,
               root_exists ? ("CanonFS root present at " + canonfs_root.string())
                           : ("CanonFS root missing at " + canonfs_root.string()),
               "Run a CanonFS command such as `t81 canonfs snapshot` or set T81_CANONFS_ROOT.");
    const fs::path objects_dir = canonfs_root / "objects";
    const bool objects_ok = !root_exists || !fs::exists(objects_dir) || fs::is_directory(objects_dir);
    push_check("canonfs_objects_dir", objects_ok,
               fs::exists(objects_dir) ? (objects_ok ? "objects directory readable"
                                                     : "objects path exists but is not a directory")
                                       : "objects directory not present yet",
               "Ensure CanonFS object storage is intact under <root>/objects.");
    const fs::path snapshots_dir = canonfs_root / "snapshots";
    const bool snapshots_ok =
        !root_exists || !fs::exists(snapshots_dir) || fs::is_directory(snapshots_dir);
    push_check("canonfs_snapshots_dir", snapshots_ok,
               fs::exists(snapshots_dir) ? (snapshots_ok ? "snapshots directory readable"
                                                         : "snapshots path exists but is not a directory")
                                         : "snapshots directory not present yet",
               "Ensure CanonFS snapshot storage is intact under <root>/snapshots.");
  };

  auto run_vm_checks = [&]() {
    const bool build_metadata_present = fs::exists(build_dir / "CTestTestfile.cmake");
    push_check("build_dir_present", build_metadata_present,
               build_metadata_present ? ("CTest metadata found in " + build_dir.string())
                                      : ("build dir is missing CTest metadata at " + build_dir.string()),
               "Run: cmake -S . -B build && cmake --build build");
    const fs::path vm_binary = build_dir / "t81";
    const bool vm_binary_present = fs::exists(vm_binary);
    push_check("vm_binary_present", vm_binary_present,
               vm_binary_present ? ("CLI binary present at " + vm_binary.string())
                                 : ("CLI binary missing at " + vm_binary.string()),
               "Build the CLI binary with `cmake --build build --target t81`.");

    bool temp_writable = false;
    std::string temp_detail;
    try {
      TempCaptureFile probe("doctor-probe", ".tmp");
      {
        std::ofstream out(probe.path);
        temp_writable = static_cast<bool>(out);
        if (temp_writable) {
          out << "ok";
        }
      }
      temp_detail = temp_writable ? "temp directory writable" : "temp directory not writable";
    } catch (...) {
      temp_writable = false;
      temp_detail = "temp directory probe failed";
    }
    push_check("temp_writable", temp_writable, temp_detail,
               "Fix filesystem permissions for your temp directory.");
  };

  if (!scope || *scope == "toolchain") {
    run_toolchain_checks();
  }
  if (!scope || *scope == "canonfs") {
    run_canonfs_checks();
  }
  if (!scope || *scope == "vm") {
    run_vm_checks();
  }

  bool all_ok = true;
  for (const auto& check : checks) {
    all_ok = all_ok && check.ok;
  }

  if (as_json) {
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.doctor.v1\",\n";
    std::cout << "  \"ok\": " << (all_ok ? "true" : "false") << ",\n";
    std::cout << "  \"scope\": ";
    if (scope) {
      std::cout << "\"" << json_escape(*scope) << "\"";
    } else {
      std::cout << "null";
    }
    std::cout << ",\n";
    std::cout << "  \"checks\": [\n";
    for (size_t i = 0; i < checks.size(); ++i) {
      const auto& check = checks[i];
      std::cout << "    {\"id\":\"" << json_escape(check.id) << "\",\"ok\":"
                << (check.ok ? "true" : "false") << ",\"detail\":\"" << json_escape(check.detail)
                << "\",\"remediation\":\"" << json_escape(check.remediation) << "\"}";
      if (i + 1 != checks.size()) {
        std::cout << ",";
      }
      std::cout << "\n";
    }
    std::cout << "  ]\n";
    std::cout << "}\n";
    return all_ok ? 0 : 2;
  }

  if (scope) {
    std::cout << "Scope: " << *scope << "\n";
  }
  for (const auto& check : checks) {
    std::cout << (check.ok ? "[ok]   " : "[fail] ") << check.id << ": " << check.detail << "\n";
    if (!check.ok) {
      std::cout << "       fix: " << check.remediation << "\n";
    }
  }
  return all_ok ? 0 : 2;
}

int run_test_command(const Args& args) {
  fs::path build_dir = "build";
  std::optional<std::string> filter;
  bool as_json = false;
  bool list_only = false;
  std::vector<std::string> passthrough;

  for (size_t i = 0; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--json") {
      as_json = true;
    } else if (token == "--list") {
      list_only = true;
    } else if (token == "--build-dir") {
      if (++i >= args.command_args.size()) {
        error("test: missing value for --build-dir. Run 't81 help test'.");
        return 1;
      }
      build_dir = fs::path(args.command_args[i]);
    } else if (token == "--filter" || token == "-R") {
      if (++i >= args.command_args.size()) {
        error("test: missing value for --filter. Run 't81 help test'.");
        return 1;
      }
      filter = args.command_args[i];
    } else if (token == "--") {
      for (size_t j = i + 1; j < args.command_args.size(); ++j) {
        passthrough.push_back(args.command_args[j]);
      }
      break;
    } else if (token == "-h" || token == "--help") {
      print_help_test();
      return 0;
    } else {
      error("test: unknown option '" + token + "'. Run 't81 help test'.");
      return 1;
    }
  }

  if (!shell_command_available("ctest")) {
    error("test: ctest not found in PATH.");
    return 2;
  }
  if (!fs::exists(build_dir / "CTestTestfile.cmake")) {
    error("test: missing CTest metadata under " + build_dir.string() +
          ". Run cmake configure/build first.");
    return 2;
  }

  std::vector<std::string> ctest_argv = {"ctest", "--test-dir", build_dir.string()};
  if (list_only) {
    ctest_argv.push_back("-N");
  } else {
    ctest_argv.push_back("--output-on-failure");
  }
  if (filter) {
    ctest_argv.push_back("-R");
    ctest_argv.push_back(*filter);
  }
  TempCaptureFile junit_file("ctest-junit", ".xml");
  bool used_junit = false;
  if (!list_only) {
    ctest_argv.push_back("--output-junit");
    ctest_argv.push_back(junit_file.path.string());
    used_junit = true;
  }
  for (const auto& token : passthrough) {
    ctest_argv.push_back(token);
  }

  auto capture = run_process_capture(ctest_argv);
  int rc = capture.exit_code;
  std::string stdout_text = capture.stdout_text;
  std::string stderr_text = capture.stderr_text;
  std::uintmax_t stdout_bytes = stdout_text.size();
  std::uintmax_t stderr_bytes = stderr_text.size();
  if (rc == -1) {
    error("test: failed to invoke ctest.");
    return 2;
  }
  if (used_junit && rc != 0) {
    std::string lowered = stderr_text;
    for (char& c : lowered) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (lowered.find("output-junit") != std::string::npos &&
        (lowered.find("unknown") != std::string::npos || lowered.find("unrecognized") != std::string::npos ||
         lowered.find("invalid") != std::string::npos)) {
      used_junit = false;
      ctest_argv.erase(std::remove(ctest_argv.begin(), ctest_argv.end(), "--output-junit"),
                       ctest_argv.end());
      ctest_argv.erase(std::remove(ctest_argv.begin(), ctest_argv.end(), junit_file.path.string()),
                       ctest_argv.end());
      capture = run_process_capture(ctest_argv);
      rc = capture.exit_code;
      stdout_text = capture.stdout_text;
      stderr_text = capture.stderr_text;
      stdout_bytes = stdout_text.size();
      stderr_bytes = stderr_text.size();
      if (rc == -1) {
        error("test: failed to invoke ctest.");
        return 2;
      }
    }
  }

  struct TestSummary {
    int total = -1;
    int passed = -1;
    int failed = -1;
    int skipped = -1;
    double duration_sec = -1.0;
  };
  auto parse_summary = [](const std::string& text) -> TestSummary {
    TestSummary summary;
    std::smatch match;
    std::regex pass_fail_re(R"((\d+)% tests passed,\s*(\d+)\s*tests failed out of\s*(\d+))");
    if (std::regex_search(text, match, pass_fail_re) && match.size() == 4) {
      summary.failed = std::stoi(match[2].str());
      summary.total = std::stoi(match[3].str());
      summary.passed = summary.total - summary.failed;
      summary.skipped = 0;
    }
    std::regex total_only_re(R"(Total Tests:\s*(\d+))");
    if (summary.total < 0 && std::regex_search(text, match, total_only_re) && match.size() == 2) {
      summary.total = std::stoi(match[1].str());
      summary.passed = summary.total;
      summary.failed = 0;
      summary.skipped = 0;
    }
    std::regex duration_re(R"(Total Test time \(real\)\s*=\s*([0-9]+(?:\.[0-9]+)?)\s*sec)");
    if (std::regex_search(text, match, duration_re) && match.size() == 2) {
      summary.duration_sec = std::stod(match[1].str());
    }
    return summary;
  };
  TestSummary summary = parse_summary(stdout_text + "\n" + stderr_text);
  if (used_junit && fs::exists(junit_file.path)) {
    std::ifstream junit_in(junit_file.path, std::ios::binary);
    const std::string xml((std::istreambuf_iterator<char>(junit_in)), std::istreambuf_iterator<char>());
    std::smatch m;
    if (std::regex_search(xml, m, std::regex("tests=\"([0-9]+)\"")) && m.size() == 2) {
      summary.total = std::atoi(m[1].str().c_str());
    }
    if (std::regex_search(xml, m, std::regex("failures=\"([0-9]+)\"")) && m.size() == 2) {
      summary.failed = std::atoi(m[1].str().c_str());
    }
    if (std::regex_search(xml, m, std::regex("disabled=\"([0-9]+)\"")) && m.size() == 2) {
      summary.skipped = std::atoi(m[1].str().c_str());
    }
    if (summary.total >= 0 && summary.failed >= 0) {
      summary.passed = summary.total - summary.failed - std::max(0, summary.skipped);
    }
    if (std::regex_search(xml, m, std::regex("time=\"([0-9]+(?:\\.[0-9]+)?)\"")) &&
        m.size() == 2) {
      summary.duration_sec = std::atof(m[1].str().c_str());
    }
  }

  if (as_json) {
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.test.v1\",\n";
    std::cout << "  \"ok\": " << (rc == 0 ? "true" : "false") << ",\n";
    std::cout << "  \"exit_code\": " << rc << ",\n";
    std::cout << "  \"mode\": \"" << (list_only ? "list" : "run") << "\",\n";
    std::cout << "  \"build_dir\": \"" << json_escape(build_dir.string()) << "\",\n";
    if (filter) {
      std::cout << "  \"filter\": \"" << json_escape(*filter) << "\",\n";
    } else {
      std::cout << "  \"filter\": null,\n";
    }
    std::cout << "  \"summary\": {\n";
    std::cout << "    \"total\": " << summary.total << ",\n";
    std::cout << "    \"passed\": " << summary.passed << ",\n";
    std::cout << "    \"failed\": " << summary.failed << ",\n";
    std::cout << "    \"skipped\": " << summary.skipped << ",\n";
    std::cout << "    \"duration_sec\": " << std::fixed << std::setprecision(3)
              << summary.duration_sec << "\n";
    std::cout << std::defaultfloat;
    std::cout << "  },\n";
    std::cout << "  \"stdout_bytes\": " << stdout_bytes << ",\n";
    std::cout << "  \"stderr_bytes\": " << stderr_bytes << ",\n";
    std::cout << "  \"summary_source\": \"" << (used_junit ? "junit_xml" : "ctest_text") << "\"\n";
    std::cout << "}\n";
    return rc == 0 ? 0 : 2;
  }

  std::cout << stdout_text;
  std::cerr << stderr_text;
  if (!g_flags.quiet) {
    std::ostringstream oss;
    oss << "test summary: total=" << summary.total << " passed=" << summary.passed
        << " failed=" << summary.failed << " skipped=" << summary.skipped;
    if (summary.duration_sec >= 0.0) {
      oss << " duration_sec=" << std::fixed << std::setprecision(3) << summary.duration_sec;
    }
    std::cout << oss.str() << "\n";
  }
  return rc == 0 ? 0 : 2;
}

std::string normalize_format_content(const std::string& input) {
  std::string out;
  out.reserve(input.size() + 2);
  std::string line;
  line.reserve(256);
  for (size_t i = 0; i < input.size(); ++i) {
    char c = input[i];
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
        line.pop_back();
      }
      out += line;
      out.push_back('\n');
      line.clear();
      continue;
    }
    line.push_back(c);
  }
  if (!line.empty()) {
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
      line.pop_back();
    }
    out += line;
    out.push_back('\n');
  }
  if (out.empty()) {
    out.push_back('\n');
  }
  return out;
}

std::string format_t81_content(std::string_view input) {
  std::istringstream in{std::string(input)};
  std::ostringstream out;
  std::string line;
  int indent_level = 0;
  while (std::getline(in, line)) {
    std::string trimmed = line;
    size_t begin = 0;
    while (begin < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[begin]))) {
      ++begin;
    }
    trimmed.erase(0, begin);
    while (!trimmed.empty() &&
           std::isspace(static_cast<unsigned char>(trimmed[trimmed.size() - 1]))) {
      trimmed.pop_back();
    }
    if (trimmed.empty()) {
      out << "\n";
      continue;
    }

    int leading_close = 0;
    while (leading_close < static_cast<int>(trimmed.size()) && trimmed[leading_close] == '}') {
      ++leading_close;
    }
    int line_indent = indent_level - leading_close;
    if (line_indent < 0) line_indent = 0;
    out << std::string(static_cast<size_t>(line_indent) * 2, ' ') << trimmed << "\n";

    int delta = 0;
    for (char c : trimmed) {
      if (c == '{') ++delta;
      if (c == '}') --delta;
    }
    indent_level += delta;
    if (indent_level < 0) indent_level = 0;
  }
  std::string formatted = out.str();
  if (formatted.empty()) formatted = "\n";
  return formatted;
}

bool validate_t81_source(std::string_view source, std::string_view diag_name, std::string& reason) {
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dist;
  fs::path tmp_path =
      fs::temp_directory_path() / ("t81-fmt-validate-" + std::to_string(dist(gen)) + ".t81");
  {
    std::ofstream out(tmp_path, std::ios::binary | std::ios::trunc);
    if (!out) {
      reason = "validator temp file write failed";
      return false;
    }
    out << source;
    if (!out.good()) {
      reason = "validator temp file write failed";
      return false;
    }
  }

  const bool old_quiet = g_flags.quiet;
  g_flags.quiet = true;
  const int rc = t81::cli::check_syntax(tmp_path);
  g_flags.quiet = old_quiet;

  std::error_code ignore_ec;
  fs::remove(tmp_path, ignore_ec);
  (void)diag_name;
  if (rc != 0) {
    reason = "parse/semantic errors";
    return false;
  }
  return true;
}

int run_fmt_command(const Args& args) {
  bool check_only = false;
  bool as_json = false;
  bool show_version = false;
  std::vector<fs::path> files;
  for (const auto& token : args.command_args) {
    if (token == "--check") {
      check_only = true;
    } else if (token == "--json") {
      as_json = true;
    } else if (token == "--version") {
      show_version = true;
    } else if (token == "-h" || token == "--help") {
      print_help_fmt();
      return 0;
    } else if (!token.empty() && token[0] == '-') {
      error("fmt: unknown option '" + token + "'. Run 't81 help fmt'.");
      return 1;
    } else {
      files.emplace_back(token);
    }
  }

  if (show_version) {
    std::cout << T81_FMT_VERSION << '\n';
    return 0;
  }

  if (files.empty()) {
    error("fmt requires at least one file path. Run 't81 help fmt'.");
    return 1;
  }

  std::vector<std::string> changed;
  std::vector<std::string> failures;
  for (const auto& path : files) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
      failures.push_back("Could not open file: " + path.string());
      continue;
    }
    std::string original((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::string normalized = normalize_format_content(original);
    std::string formatted = normalized;
    const std::string ext = path.extension().string();
    if (ext == ".t81") {
      std::string parse_reason;
      if (!validate_t81_source(normalized, path.string(), parse_reason)) {
        failures.push_back("Invalid .t81 input (cannot safely format): " + path.string() + " (" +
                           parse_reason + ")");
        continue;
      }
      formatted = format_t81_content(normalized);
      if (format_t81_content(formatted) != formatted) {
        failures.push_back("Formatter idempotence check failed: " + path.string());
        continue;
      }
      if (!validate_t81_source(formatted, path.string(), parse_reason)) {
        failures.push_back("Formatter produced invalid .t81 output: " + path.string() + " (" +
                           parse_reason + ")");
        continue;
      }
    }
    if (formatted != original) {
      changed.push_back(path.string());
      if (!check_only) {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) {
          failures.push_back("Could not write file: " + path.string());
          continue;
        }
        out << formatted;
        if (!out.good()) {
          failures.push_back("Failed writing file: " + path.string());
          continue;
        }
      }
    }
  }

  if (as_json) {
    const bool ok = failures.empty() && (!check_only || changed.empty());
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.fmt.v1\",\n";
    std::cout << "  \"ok\": " << (ok ? "true" : "false") << ",\n";
    std::cout << "  \"formatter_version\": \"" << T81_FMT_VERSION << "\",\n";
    std::cout << "  \"check_only\": " << (check_only ? "true" : "false") << ",\n";
    std::cout << "  \"files_scanned\": " << files.size() << ",\n";
    std::cout << "  \"changed\": [";
    for (size_t i = 0; i < changed.size(); ++i) {
      if (i != 0) {
        std::cout << ",";
      }
      std::cout << "\"" << json_escape(changed[i]) << "\"";
    }
    std::cout << "],\n";
    std::cout << "  \"failures\": [";
    for (size_t i = 0; i < failures.size(); ++i) {
      if (i != 0) {
        std::cout << ",";
      }
      std::cout << "\"" << json_escape(failures[i]) << "\"";
    }
    std::cout << "]\n";
    std::cout << "}\n";
  } else {
    for (const auto& file : changed) {
      std::cout << (check_only ? "would format: " : "formatted: ") << file << "\n";
    }
    for (const auto& failure : failures) {
      error("fmt: " + failure);
    }
    if (!g_flags.quiet) {
      std::cout << "fmt summary: scanned=" << files.size() << " changed=" << changed.size()
                << " failures=" << failures.size() << " mode=" << (check_only ? "check" : "write")
                << "\n";
    }
  }

  if (!failures.empty()) {
    return 1;
  }
  if (check_only && !changed.empty()) {
    return 2;
  }
  return 0;
}

bool parse_package_field(const std::string& text, const std::string& field, std::string& value_out) {
  const std::regex re("\\(" + field + "\\s+\"([^\"]+)\"\\)");
  std::smatch match;
  if (!std::regex_search(text, match, re) || match.size() < 2) {
    return false;
  }
  value_out = match[1].str();
  return true;
}

int run_pkg_check(const Args& args) {
  fs::path manifest = "package.t81";
  bool as_json = false;
  for (const auto& token : args.command_args) {
    if (token == "--json") {
      as_json = true;
    } else if (token == "-h" || token == "--help") {
      print_help_pkg();
      return 0;
    } else if (!token.empty() && token[0] == '-') {
      error("pkg check: unknown option '" + token + "'. Run 't81 help pkg'.");
      return 1;
    } else {
      manifest = fs::path(token);
    }
  }

  std::vector<std::string> errors;
  std::string name;
  std::string version;
  if (!fs::exists(manifest)) {
    errors.push_back("manifest not found: " + manifest.string());
  } else {
    std::ifstream in(manifest);
    if (!in) {
      errors.push_back("manifest not readable: " + manifest.string());
    } else {
      std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
      if (content.find("(package") == std::string::npos) {
        errors.push_back("missing top-level (package ...) form");
      }
      if (!parse_package_field(content, "name", name)) {
        errors.push_back("missing required field: (name \"...\")");
      } else {
        bool valid_name = !name.empty();
        for (char c : name) {
          if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') {
            valid_name = false;
            break;
          }
        }
        if (!valid_name) {
          errors.push_back("invalid package name: only [A-Za-z0-9_-] allowed");
        }
      }
      if (!parse_package_field(content, "version", version)) {
        errors.push_back("missing required field: (version \"x.y.z\")");
      } else {
        const std::regex semver_re(R"(^[0-9]+\.[0-9]+\.[0-9]+$)");
        if (!std::regex_match(version, semver_re)) {
          errors.push_back("invalid version: expected simple semver x.y.z");
        }
      }
    }
  }

  if (as_json) {
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.pkg-check.v1\",\n";
    std::cout << "  \"valid\": " << (errors.empty() ? "true" : "false") << ",\n";
    std::cout << "  \"manifest\": \"" << json_escape(manifest.string()) << "\",\n";
    std::cout << "  \"name\": ";
    if (name.empty()) {
      std::cout << "null,\n";
    } else {
      std::cout << "\"" << json_escape(name) << "\",\n";
    }
    std::cout << "  \"version\": ";
    if (version.empty()) {
      std::cout << "null,\n";
    } else {
      std::cout << "\"" << json_escape(version) << "\",\n";
    }
    std::cout << "  \"errors\": [";
    for (size_t i = 0; i < errors.size(); ++i) {
      if (i != 0) {
        std::cout << ",";
      }
      std::cout << "\"" << json_escape(errors[i]) << "\"";
    }
    std::cout << "]\n";
    std::cout << "}\n";
  } else if (errors.empty()) {
    std::cout << "package.t81: valid\n";
  } else {
    for (const auto& msg : errors) {
      error("pkg check: " + msg);
    }
  }
  return errors.empty() ? 0 : 2;
}

std::string build_bash_completion() {
  return R"(_t81_complete() {
  local cur prev words cword
  _init_completion || return
  local commands="code lang project env internal canonfs determinism vm tisc ir tier tensor weights policy axion trace c rust python llvm mlir repl completion man feedback version help"
  if [[ ${cword} -eq 1 ]]; then
    COMPREPLY=( $(compgen -W "${commands}" -- "${cur}") )
    return
  fi
  case "${words[1]}" in
    lang)
      COMPREPLY=( $(compgen -W "check lint fmt build run test disasm debug repl show dump export validate profile" -- "${cur}") )
      ;;
    code)
      COMPREPLY=( $(compgen -W "check lint fmt build run test disasm debug repl profile" -- "${cur}") )
      ;;
    project)
      COMPREPLY=( $(compgen -W "init build run test" -- "${cur}") )
      ;;
    env)
      COMPREPLY=( $(compgen -W "check doctor paths diag toolchain clean feedback" -- "${cur}") )
      ;;
    internal)
      COMPREPLY=( $(compgen -W "pkg benchmark repro-hash canonize-tensor canonize-file memory-stats llama-run" -- "${cur}") )
      ;;
    canonfs)
      COMPREPLY=( $(compgen -W "put-file put-tensor ls get stat verify snapshot snapshot-diff rollback gc fsck repair" -- "${cur}") )
      ;;
    determinism)
      COMPREPLY=( $(compgen -W "verify verify-run compare-run certify explain hash trace-hash diff diff-trace baseline bisect multi-run" -- "${cur}") )
      ;;
    vm)
      COMPREPLY=( $(compgen -W "run debug trace until step regs stack mem state profile explain-trap" -- "${cur}") )
      ;;
    tisc)
      COMPREPLY=( $(compgen -W "disasm validate stats encode decode diff" -- "${cur}") )
    ;;
    ir)
      COMPREPLY=( $(compgen -W "show dump export validate" -- "${cur}") )
    ;;
    tier)
      COMPREPLY=( $(compgen -W "info check gate" -- "${cur}") )
    ;;
    tensor)
      COMPREPLY=( $(compgen -W "canonize hash inspect" -- "${cur}") )
      ;;
    weights)
      COMPREPLY=( $(compgen -W "import info verify export quantize" -- "${cur}") )
      ;;
    policy)
      COMPREPLY=( $(compgen -W "compile validate run test list" -- "${cur}") )
      ;;
    axion)
      COMPREPLY=( $(compgen -W "status optimize simulate explain snapshot snapshot-diff rollback log audit" -- "${cur}") )
      ;;
    trace)
      COMPREPLY=( $(compgen -W "show diff replay summary stats filter canonicalize export" -- "${cur}") )
      ;;
    c)
      COMPREPLY=( $(compgen -W "compile help" -- "${cur}") )
      ;;
    rust)
      COMPREPLY=( $(compgen -W "compile help" -- "${cur}") )
      ;;
    python)
      COMPREPLY=( $(compgen -W "compile help" -- "${cur}") )
      ;;
    llvm)
      COMPREPLY=( $(compgen -W "compile help" -- "${cur}") )
      ;;
    mlir)
      COMPREPLY=( $(compgen -W "compile lower pipeline help" -- "${cur}") )
      ;;
    completion)
      COMPREPLY=( $(compgen -W "bash zsh fish" -- "${cur}") )
      ;;
    feedback)
      COMPREPLY=( $(compgen -W "submit report" -- "${cur}") )
      ;;
  esac
}
complete -F _t81_complete t81
)";
}

std::string build_zsh_completion() {
  return R"(#compdef t81
local context state line
typeset -A opt_args

local -a commands
commands=(
  'lang:T81Lang-oriented workflow commands'
  'code:code workflow commands'
  'project:project lifecycle commands'
  'env:environment commands'
  'internal:internal commands'
  'canonfs:CanonFS inspection and snapshot commands'
  'determinism:determinism verification and hashing commands'
  'vm:VM execution and state inspection'
  'tisc:TISC artifact inspection and validation'
  'ir:IR inspection for frontend lowering'
  'tier:cognitive tier inspection and gating'
  'tensor:tensor artifact tools'
  'weights:model weight tools'
  'policy:Axion policy tools'
  'axion:Axion governor tools'
  'trace:trace inspection tools'
  'c:Experimental C-subset frontend to MLIR'
  'rust:Experimental Rust-subset frontend to MLIR'
  'python:Experimental Python-subset frontend to MLIR'
  'llvm:LLVM IR backend — translate TISC to LLVM IR/bitcode'
  'mlir:MLIR frontend — translate TISC to MLIR or LLVM IR'
  'completion:print completion script'
  'man:show or install man page'
  'feedback:local CLI feedback loop'
  'help:show help'
  'version:show version'
)

_arguments -C \
  '1:command:->command' \
  '*::arg:->args'

case $state in
  command)
    _describe -t commands 't81 command' commands
    ;;
  args)
    case $words[2] in
      lang)
        _values 'lang action' check lint fmt build run test disasm debug repl show dump export validate profile
        ;;
      code)
        _values 'code action' check lint fmt build run test disasm debug repl profile
        ;;
      project)
        _values 'project action' init build run test
        ;;
      env)
        _values 'env action' check doctor paths diag toolchain clean feedback
        ;;
      internal)
        _values 'internal action' pkg benchmark repro-hash canonize-tensor canonize-file memory-stats llama-run
        ;;
      canonfs)
        _values 'canonfs action' put-file put-tensor ls get stat verify snapshot snapshot-diff rollback gc fsck repair
        ;;
      determinism)
        _values 'determinism action' verify verify-run compare-run certify explain hash trace-hash diff diff-trace baseline bisect multi-run
        ;;
      vm)
        _values 'vm action' run debug trace until step regs stack mem state profile explain-trap
        ;;
      tisc)
        _values 'tisc action' disasm validate stats encode decode diff
        ;;
      ir)
        _values 'ir action' show dump export validate
        ;;
      tier)
        _values 'tier action' info check gate
        ;;
      tensor)
        _values 'tensor action' canonize hash inspect
        ;;
      weights)
        _values 'weights action' import info verify export quantize
        ;;
      policy)
        _values 'policy action' compile validate run test list
        ;;
      axion)
        _values 'axion action' status optimize simulate explain snapshot snapshot-diff rollback log audit
        ;;
      trace)
        _values 'trace action' show diff replay summary stats filter canonicalize export
        ;;
      c)
        _values 'c action' compile help
        ;;
      rust)
        _values 'rust action' compile help
        ;;
      python)
        _values 'python action' compile help
        ;;
      llvm)
        _values 'llvm action' compile help
        ;;
      mlir)
        _values 'mlir action' compile lower pipeline help
        ;;
      feedback)
        _values 'feedback action' submit report
        ;;
      completion)
        _values 'shell' bash zsh fish
        ;;
    esac
    ;;
esac
)";
}

std::string build_fish_completion() {
  return R"(complete -c t81 -f -n '__fish_use_subcommand' -a 'code lang project env internal canonfs determinism vm tisc ir tier tensor weights policy axion trace c rust python llvm mlir repl completion man feedback version help'
complete -c t81 -f -n '__fish_seen_subcommand_from completion' -a 'bash zsh fish'
complete -c t81 -f -n '__fish_seen_subcommand_from lang' -a 'check lint fmt build run test disasm debug repl show dump export validate profile'
complete -c t81 -f -n '__fish_seen_subcommand_from code' -a 'check lint fmt build run test disasm debug repl profile'
complete -c t81 -f -n '__fish_seen_subcommand_from project' -a 'init build run test'
complete -c t81 -f -n '__fish_seen_subcommand_from env' -a 'check doctor paths diag toolchain clean feedback'
complete -c t81 -f -n '__fish_seen_subcommand_from internal' -a 'pkg benchmark repro-hash canonize-tensor canonize-file memory-stats llama-run'
complete -c t81 -f -n '__fish_seen_subcommand_from canonfs' -a 'put-file put-tensor ls get stat verify snapshot snapshot-diff rollback gc fsck repair'
complete -c t81 -f -n '__fish_seen_subcommand_from determinism' -a 'verify verify-run compare-run certify explain hash trace-hash diff diff-trace baseline bisect multi-run'
complete -c t81 -f -n '__fish_seen_subcommand_from vm' -a 'run debug trace until step regs stack mem state profile explain-trap'
complete -c t81 -f -n '__fish_seen_subcommand_from tisc' -a 'disasm validate stats encode decode diff'
complete -c t81 -f -n '__fish_seen_subcommand_from ir' -a 'show dump export validate'
complete -c t81 -f -n '__fish_seen_subcommand_from tier' -a 'info check gate'
complete -c t81 -f -n '__fish_seen_subcommand_from tensor' -a 'canonize hash inspect'
complete -c t81 -f -n '__fish_seen_subcommand_from weights' -a 'import info verify export quantize'
complete -c t81 -f -n '__fish_seen_subcommand_from policy' -a 'compile validate run test list'
complete -c t81 -f -n '__fish_seen_subcommand_from axion' -a 'status optimize simulate explain snapshot snapshot-diff rollback log audit'
complete -c t81 -f -n '__fish_seen_subcommand_from trace' -a 'show diff replay summary stats filter canonicalize export'
complete -c t81 -f -n '__fish_seen_subcommand_from c' -a 'compile help'
complete -c t81 -f -n '__fish_seen_subcommand_from rust' -a 'compile help'
complete -c t81 -f -n '__fish_seen_subcommand_from python' -a 'compile help'
complete -c t81 -f -n '__fish_seen_subcommand_from llvm' -a 'compile help'
complete -c t81 -f -n '__fish_seen_subcommand_from mlir' -a 'compile lower pipeline help'
complete -c t81 -f -n '__fish_seen_subcommand_from feedback' -a 'submit report'
)";
}

int run_completion_command(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" || args.command_args[0] == "--help") {
    print_help_completion();
    return args.command_args.empty() ? 1 : 0;
  }
  const std::string shell = args.command_args[0];
  if (shell == "bash") {
    std::cout << build_bash_completion();
    return 0;
  }
  if (shell == "zsh") {
    std::cout << build_zsh_completion();
    return 0;
  }
  if (shell == "fish") {
    std::cout << build_fish_completion();
    return 0;
  }
  error("completion: unsupported shell '" + shell + "'. Use bash|zsh|fish.");
  return 1;
}

std::string build_manpage_text() {
  return R"(.TH T81 1 "2026-02-26" "t81 CLI" "User Commands"
.SH NAME
t81 \- T81 Foundation CLI
.SH SYNOPSIS
t81 [global-options] <command> [args]
t81 [global-options] <domain> <action> [args]
.SH DESCRIPTION
Domain-first commands:
.TP
lang
T81Lang-oriented workflow for source compile/check/run and IR lowering.
.TP
code
Check, build, run, test, format, debug, disassemble, repl.
.TP
canonfs
Put, inspect, verify, snapshot, diff, and roll back CanonFS objects.
.TP
determinism
Hash artifacts, diff traces, and verify repeatable execution.
.TP
vm
Run, trace, debug, profile, and inspect TISC execution.
.TP
tisc
Disassemble, validate, encode, and decode TISC artifacts.
.TP
ir
Show and export lowered IR from T81 source.
.TP
tensor
Canonicalize, hash, and inspect tensor artifacts.
.TP
weights
Import, inspect, verify, and quantize model weights.
.TP
policy
Compile, run, and test Axion policies.
.TP
axion
Inspect governor state, simulate, snapshot, diff snapshots, and roll back.
.TP
trace
Show, diff, replay, canonicalize, and export execution traces.
.TP
project
Project lifecycle commands.
.TP
env
Environment checks and UX feedback.
.TP
internal
Internal and experimental operations.
.SH GLOBAL OPTIONS
-h, --help
-V, --version
-q, --quiet
-v, --verbose
.SH SEE ALSO
t81 help, t81 help code, t81 help advanced, t81 help labs
)";
}

int run_man_command(const Args& args) {
  fs::path install_dir;
  for (size_t i = 0; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "-h" || token == "--help") {
      print_help_man();
      return 0;
    }
    if (token == "--install-dir") {
      if (++i >= args.command_args.size()) {
        error("man: missing value for --install-dir. Run 't81 help man'.");
        return 1;
      }
      install_dir = fs::path(args.command_args[i]);
      continue;
    }
    error("man: unknown option '" + token + "'. Run 't81 help man'.");
    return 1;
  }
  const std::string man_text = build_manpage_text();
  if (install_dir.empty()) {
    std::cout << man_text;
    return 0;
  }
  std::error_code ec;
  fs::create_directories(install_dir, ec);
  if (ec) {
    error("man: could not create install dir: " + install_dir.string());
    return 2;
  }
  const fs::path out = install_dir / "t81.1";
  std::ofstream ofs(out, std::ios::binary | std::ios::trunc);
  if (!ofs) {
    error("man: could not open output file: " + out.string());
    return 2;
  }
  ofs << man_text;
  if (!ofs.good()) {
    error("man: failed writing output file: " + out.string());
    return 2;
  }
  std::cout << "installed man page: " << out.string() << "\n";
  return 0;
}

int run_feedback_command(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" || args.command_args[0] == "--help") {
    print_help_feedback();
    return args.command_args.empty() ? 1 : 0;
  }
  const std::string sub = args.command_args[0];
  fs::path path = ".t81_cli_feedback.jsonl";
  if (sub == "submit") {
    int rating = -1;
    std::string note;
    for (size_t i = 1; i < args.command_args.size(); ++i) {
      const std::string& token = args.command_args[i];
      if (token == "--rating") {
        if (++i >= args.command_args.size()) {
          error("feedback submit: missing value for --rating");
          return 1;
        }
        rating = std::atoi(args.command_args[i].c_str());
      } else if (token == "--note") {
        if (++i >= args.command_args.size()) {
          error("feedback submit: missing value for --note");
          return 1;
        }
        note = args.command_args[i];
      } else if (token == "--path") {
        if (++i >= args.command_args.size()) {
          error("feedback submit: missing value for --path");
          return 1;
        }
        path = fs::path(args.command_args[i]);
      } else {
        error("feedback submit: unknown option '" + token + "'");
        return 1;
      }
    }
    if (rating < 1 || rating > 5) {
      error("feedback submit: --rating must be between 1 and 5");
      return 1;
    }
    std::ofstream out(path, std::ios::binary | std::ios::app);
    if (!out) {
      error("feedback submit: could not open path: " + path.string());
      return 2;
    }
    out << "{\"schema\":\"t81.feedback.v1\",\"rating\":" << rating << ",\"note\":\""
        << json_escape(note) << "\"}\n";
    if (!out.good()) {
      error("feedback submit: write failed: " + path.string());
      return 2;
    }
    std::cout << "feedback recorded: " << path.string() << "\n";
    return 0;
  }

  if (sub == "report") {
    for (size_t i = 1; i < args.command_args.size(); ++i) {
      const std::string& token = args.command_args[i];
      if (token == "--path") {
        if (++i >= args.command_args.size()) {
          error("feedback report: missing value for --path");
          return 1;
        }
        path = fs::path(args.command_args[i]);
      } else {
        error("feedback report: unknown option '" + token + "'");
        return 1;
      }
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
      error("feedback report: could not open path: " + path.string());
      return 2;
    }
    std::string line;
    int count = 0;
    int rating_sum = 0;
    while (std::getline(in, line)) {
      std::smatch m;
      if (std::regex_search(line, m, std::regex("\"rating\":([0-9]+)")) && m.size() == 2) {
        int r = std::atoi(m[1].str().c_str());
        if (r >= 1 && r <= 5) {
          ++count;
          rating_sum += r;
        }
      }
    }
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.feedback-report.v1\",\n";
    std::cout << "  \"path\": \"" << json_escape(path.string()) << "\",\n";
    std::cout << "  \"count\": " << count << ",\n";
    if (count == 0) {
      std::cout << "  \"average_rating\": null\n";
    } else {
      std::cout << "  \"average_rating\": " << std::fixed << std::setprecision(3)
                << (static_cast<double>(rating_sum) / static_cast<double>(count)) << "\n";
      std::cout << std::defaultfloat;
    }
    std::cout << "}\n";
    return 0;
  }
  error("feedback: unknown subcommand '" + sub + "'. Run 't81 help feedback'.");
  return 1;
}

int run_llama_run(const Args& args) {
#if defined(T81_HAS_LLAMA_CPP)
  if (!args.policy) {
    error("llama-run requires --policy <policy.apl>");
    return 1;
  }

  std::vector<std::string> positional;
  int max_tokens = 64;
  uint32_t seed = 0;
  int n_threads = 1;
  int top_k = 1;
  float top_p = 1.0f;
  float temperature = 0.0f;
  std::string expected_model_hash;
  fs::path canonfs_root = fs::current_path() / ".t81_canonfs";

  for (size_t i = 0; i < args.command_args.size(); ++i) {
    const std::string& token = args.command_args[i];
    if (token == "--max-tokens") {
      if (++i >= args.command_args.size()) {
        error("llama-run: missing value for --max-tokens");
        return 1;
      }
      max_tokens = parse_int_arg(args.command_args[i], "--max-tokens", 1, 1000000);
    } else if (token == "--seed") {
      if (++i >= args.command_args.size()) {
        error("llama-run: missing value for --seed");
        return 1;
      }
      seed = static_cast<uint32_t>(parse_int_arg(args.command_args[i], "--seed", 0, INT32_MAX));
    } else if (token == "--threads") {
      if (++i >= args.command_args.size()) {
        error("llama-run: missing value for --threads");
        return 1;
      }
      n_threads = parse_int_arg(args.command_args[i], "--threads", 1, 4096);
    } else if (token == "--temperature") {
      if (++i >= args.command_args.size()) {
        error("llama-run: missing value for --temperature");
        return 1;
      }
      temperature = parse_float_arg(args.command_args[i], "--temperature", 0.0f, 10.0f);
    } else if (token == "--top-k") {
      if (++i >= args.command_args.size()) {
        error("llama-run: missing value for --top-k");
        return 1;
      }
      top_k = parse_int_arg(args.command_args[i], "--top-k", 1, INT32_MAX);
    } else if (token == "--top-p") {
      if (++i >= args.command_args.size()) {
        error("llama-run: missing value for --top-p");
        return 1;
      }
      top_p = parse_float_arg(args.command_args[i], "--top-p", 0.0f, 1.0f);
    } else if (token == "--expected-model-hash") {
      if (++i >= args.command_args.size()) {
        error("llama-run: missing value for --expected-model-hash");
        return 1;
      }
      expected_model_hash = args.command_args[i];
    } else if (token == "--canonfs-root") {
      if (++i >= args.command_args.size()) {
        error("llama-run: missing value for --canonfs-root");
        return 1;
      }
      canonfs_root = fs::path(args.command_args[i]);
    } else if (!token.empty() && token.front() == '-') {
      error("llama-run: unknown option '" + token + "'. Run 't81 help llama-run'.");
      return 1;
    } else {
      positional.push_back(token);
    }
  }

  if (positional.size() != 2) {
    error("llama-run requires exactly: <model.gguf> <prompt>");
    return 1;
  }

  std::ifstream policy_stream(*args.policy);
  if (!policy_stream) {
    error("Could not open policy file: " + args.policy->string());
    return 1;
  }
  std::string policy_text((std::istreambuf_iterator<char>(policy_stream)),
                          std::istreambuf_iterator<char>());

  fs::path model_path = positional[0];
  std::optional<TempBinaryFile> temp_model_file;
  if (positional[0].rfind("sha3-256:", 0) == 0) {
    std::string stripped = positional[0].substr(std::string("sha3-256:").size());
    t81::canonfs::CanonHash hash;
    try {
      hash.h = t81::hash::CanonHash81::from_string(stripped);
    } catch (...) {
      error("llama-run: invalid canonical hash: " + positional[0]);
      return 1;
    }

    auto driver = t81::canonfs::make_persistent_driver(canonfs_root);
    t81::canonfs::CanonRef ref{hash};
    auto read_res = driver->read_object_bytes(ref);
    if (!read_res) {
      error("llama-run: CanonFS model hash not found under " + canonfs_root.string());
      return 1;
    }

    temp_model_file.emplace("llama-model", ".gguf");
    std::ofstream out(temp_model_file->path, std::ios::binary | std::ios::trunc);
    if (!out) {
      error("llama-run: failed to create temporary model file");
      return 1;
    }
    const auto& bytes = read_res.value();
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    if (!out.good()) {
      error("llama-run: failed writing temporary model bytes");
      return 1;
    }
    model_path = temp_model_file->path;
  }

  auto adapter = t81::experimental::LlamaCppAdapter::create(model_path, policy_text);
  if (!adapter.has_value()) {
    error("llama-run: adapter init failed: " + adapter.error());
    return 1;
  }

  t81::experimental::LlamaCppInferenceRequest req;
  req.prompt = positional[1];
  req.expected_model_hash = expected_model_hash;
  req.seed = seed;
  req.max_tokens = max_tokens;
  req.top_k = top_k;
  req.top_p = top_p;
  req.temperature = temperature;
  req.n_threads = n_threads;

  auto receipt = adapter.value()->infer(req);
  if (!receipt.has_value()) {
    error("llama-run: inference failed: " + receipt.error());
    return 1;
  }

  std::cout << "model_hash: " << receipt->model_hash << "\n";
  std::cout << "prompt_hash: " << receipt->prompt_hash << "\n";
  std::cout << "policy_reason: " << receipt->policy_reason << "\n";
  std::cout << "generated_tokens: " << receipt->token_ids.size() << "\n";
  std::cout << "token_ids_csv:";
  for (size_t i = 0; i < receipt->token_ids.size(); ++i) {
    if (i != 0) std::cout << ",";
    std::cout << receipt->token_ids[i];
  }
  std::cout << "\n";
  std::cout << receipt->text << "\n";
  return 0;
#else
  (void)args;
  error("llama-run is unavailable in this build (reconfigure with -DT81_ENABLE_LLAMA_CPP=ON)");
  return 1;
#endif
}

int normalize_domain_command(Args& args) {
  auto remap_to_input_command = [&](const std::string& mapped, size_t first_tail) -> int {
    args.command = mapped;
    if (args.command_args.size() <= first_tail) {
      error(mapped + " requires an input file. Run 't81 help " + mapped + "'.");
      return 1;
    }
    fs::path input = fs::path(args.command_args[first_tail]);
    std::vector<std::string> remaining_args;
    for (size_t i = first_tail + 1; i < args.command_args.size(); ++i) {
      const std::string& extra = args.command_args[i];
      if (!extra.empty() && extra[0] == '-') {
        remaining_args.push_back(extra);
      } else {
        error("Multiple input files not supported yet");
        return 1;
      }
    }
    args.input = input;
    args.command_args = std::move(remaining_args);
    return -1;
  };

  auto remap_project_action = [&](const std::string& mapped) -> int {
    args.command = mapped;
    std::optional<fs::path> discovered_input;
    std::vector<std::string> remaining_args;
    for (std::size_t i = 1; i < args.command_args.size(); ++i) {
      const std::string& token = args.command_args[i];
      if (!token.empty() && token[0] != '-') {
        if (discovered_input) {
          error("Multiple input files not supported yet");
          return 1;
        }
        discovered_input = fs::path(token);
      } else {
        remaining_args.push_back(token);
      }
    }
    args.input = discovered_input.value_or(fs::path("main.t81"));
    args.command_args = std::move(remaining_args);
    return -1;
  };

  if (args.command == "code") {
    if (args.command_args.size() == 1 &&
        (args.command_args[0] == "-V" || args.command_args[0] == "--version")) {
      print_version();
      return 0;
    }
    if (args.command_args.empty()) {
      return emit_help([&] { print_help_code(); }, 1);
    }
    const std::string action = args.command_args[0];
    if (action == "build") {
      return remap_to_input_command("compile", 1);
    }
    if (action == "check" || action == "lint" || action == "run" || action == "disasm" ||
        action == "debug") {
      return remap_to_input_command(action, 1);
    }
    if (action == "profile") {
      return remap_to_input_command("profile", 1);
    }
    if (action == "repl" || action == "test" || action == "fmt") {
      args.command = action;
      args.command_args.erase(args.command_args.begin());
      if (action == "repl" && !args.command_args.empty()) {
        error("repl does not accept positional arguments");
        return 1;
      }
      return -1;
    }
    error("Unknown code action: " + action + ". Run 't81 help code'.");
    return 1;
  }

  if (args.command == "lang") {
    if (args.command_args.size() == 1 &&
        (args.command_args[0] == "-V" || args.command_args[0] == "--version")) {
      print_version();
      return 0;
    }
    if (args.command_args.empty()) {
      return emit_help([&] { print_help_lang(); });
    }
    const std::string action = args.command_args[0];
    if (action == "build") {
      return remap_to_input_command("compile", 1);
    }
    if (action == "check" || action == "lint" || action == "run" || action == "disasm" ||
        action == "debug") {
      return remap_to_input_command(action, 1);
    }
    if (action == "profile") {
      return remap_to_input_command("profile", 1);
    }
    if (action == "repl" || action == "test" || action == "fmt") {
      args.command = action;
      args.command_args.erase(args.command_args.begin());
      if (action == "repl" && !args.command_args.empty()) {
        error("repl does not accept positional arguments");
        return 1;
      }
      return -1;
    }
    if (action == "show" || action == "dump" || action == "export" || action == "validate") {
      args.command = "ir";
      return -1;
    }
    error("Unknown lang action: " + action + ". Run 't81 help lang'.");
    return 1;
  }

  if (args.command == "project") {
    if (args.command_args.size() == 1 &&
        (args.command_args[0] == "-V" || args.command_args[0] == "--version")) {
      print_version();
      return 0;
    }
    if (args.command_args.empty()) {
      return emit_help([&] { print_help_project(); });
    }
    const std::string action = args.command_args[0];
    if (action == "init") {
      args.command = "init";
      args.command_args.erase(args.command_args.begin());
      return -1;
    }
    if (action == "build") {
      return remap_project_action("compile");
    }
    if (action == "run") {
      return remap_project_action("run");
    }
    if (action == "test") {
      args.command = "test";
      args.command_args.erase(args.command_args.begin());
      return -1;
    }
    error("Unknown project action: " + action + ". Run 't81 help project'.");
    return 1;
  }

  if (args.command == "env") {
    if (args.command_args.size() == 1 &&
        (args.command_args[0] == "-V" || args.command_args[0] == "--version")) {
      print_version();
      return 0;
    }
    if (args.command_args.empty()) {
      return emit_help([&] { print_help_env(); });
    }
    const std::string action = args.command_args[0];
    if (action == "doctor") {
      args.command = "doctor";
      args.command_args.erase(args.command_args.begin());
      return -1;
    }
    if (action == "check") {
      args.command = "env-check";
      args.command_args.erase(args.command_args.begin());
      return -1;
    }
    if (action == "paths" || action == "diag" || action == "toolchain" || action == "clean") {
      args.command = "env-" + action;
      args.command_args.erase(args.command_args.begin());
      return -1;
    }
    if (action == "feedback") {
      args.command = "feedback";
      args.command_args.erase(args.command_args.begin());
      return -1;
    }
    error("Unknown env action: " + action + ". Run 't81 help env'.");
    return 1;
  }

  if (args.command == "internal") {
    if (args.command_args.size() == 1 &&
        (args.command_args[0] == "-V" || args.command_args[0] == "--version")) {
      print_version();
      return 0;
    }
    if (args.command_args.empty()) {
      return emit_help([&] { print_help_internal(); });
    }
    const std::string action = args.command_args[0];
    if (action == "benchmark") {
      args.command = "benchmark";
      args.benchmark_args.assign(args.command_args.begin() + 1, args.command_args.end());
      args.command_args.clear();
      return -1;
    }
    if (action == "pkg" || action == "repro-hash" || action == "canonize-tensor" ||
        action == "canonize-file" || action == "llama-run" || action == "memory-stats") {
      args.command = action;
      args.command_args.erase(args.command_args.begin());
      return -1;
    }
    error("Unknown internal action: " + action + ". Run 't81 help internal'.");
    return 1;
  }

  return -1;
}

std::optional<std::string> removed_alias_recommendation(std::string_view command) {
  if (command == "compile") return "t81 code build";
  if (command == "check") return "t81 code check";
  if (command == "lint") return "t81 code lint";
  if (command == "run") return "t81 code run";
  if (command == "profile") return "t81 code profile";
  if (command == "test") return "t81 code test";
  if (command == "fmt") return "t81 code fmt";
  if (command == "disasm") return "t81 code disasm";
  if (command == "debug") return "t81 code debug";
  if (command == "init") return "t81 project init";
  if (command == "doctor") return "t81 env doctor";
  if (command == "pkg") return "t81 internal pkg";
  if (command == "benchmark") return "t81 internal benchmark";
  if (command == "repro-hash") return "t81 internal repro-hash";
  if (command == "canonize-tensor") return "t81 internal canonize-tensor";
  if (command == "canonize-file") return "t81 internal canonize-file";
  if (command == "memory-stats") return "t81 internal memory-stats";
  if (command == "llama-run") return "t81 internal llama-run";
  if (command == "model") return "t81 weights";
  if (command == "bench") return "t81 internal benchmark";
  return std::nullopt;
}

std::optional<std::string> removed_help_topic_recommendation(std::string_view topic) {
  if (topic == "compile") return "t81 code build";
  if (topic == "check") return "t81 code check";
  if (topic == "lint") return "t81 code lint";
  if (topic == "run") return "t81 code run";
  if (topic == "profile") return "t81 code profile";
  if (topic == "disasm") return "t81 code disasm";
  if (topic == "debug") return "t81 code debug";
  if (topic == "model") return "t81 weights";
  if (topic == "bench") return "t81 internal benchmark";
  return std::nullopt;
}

// ──────────────────────────────────────────────────────────────
// Main
// ──────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
  g_json_error_mode = false;
  g_json_error_emitted = false;
  for (int i = 1; i < argc; ++i) {
    if (std::string_view(argv[i]) == "--json") {
      g_json_error_mode = true;
      break;
    }
  }
  try {
    auto args = parse_args(argc, argv);
    const std::string entered_command = args.command;

    if (entered_command != "help" && entered_command != "version") {
      auto removed = removed_alias_recommendation(entered_command);
      if (removed) {
        error("Command '" + entered_command + "' has been removed. Use '" + *removed + "'.");
        return 1;
      }
    }

    if (args.need_version) {
      print_version();
      return 0;
    }

    if (args.command == "help") {
      if (!args.command_args.empty()) {
        auto removed = removed_help_topic_recommendation(args.command_args[0]);
        if (removed) {
          error("Help topic '" + args.command_args[0] + "' has been removed. Use '" + *removed + "'.");
          return 1;
        }
      }
      bool handled = false;
      emit_help([&] { handled = handle_help_request(args.command, args.command_args, argv[0]); });
      if (handled) {
        return 0;
      }
      error("Unknown help topic: " + args.command_args[0] + ". Run 't81 --help'.");
      return 1;
    }

    if (args.need_help) {
      if (!args.command.empty()) {
        auto removed = removed_alias_recommendation(args.command);
        if (removed) {
          error("Command '" + args.command + "' has been removed. Use '" + *removed + "'.");
          return 1;
        }
      }
      bool handled = false;
      emit_help([&] { handled = handle_help_request(args.command, args.command_args, argv[0]); });
      if (handled) {
        return 0;
      }
      error("Unknown help topic: " + args.command + ". Run 't81 --help'.");
      return 1;
    }

    if (args.command == "version") {
      print_version();
      return 0;
    }

    for (;;) {
      const std::string before = args.command;
      const int normalize_rc = normalize_domain_command(args);
      if (normalize_rc != -1) {
        return normalize_rc;
      }
      if (args.command == before) {
        break;
      }
    }

    if (args.command == "run" && args.output && !args.trace_output) {
      args.trace = true;
      args.trace_output = args.output;
      args.output.reset();
    }

    bool needs_input =
        (args.command == "compile" || args.command == "run" || args.command == "disasm" ||
         args.command == "debug" || args.command == "check" || args.command == "lint" ||
         args.command == "profile");
    if (args.command.empty() || (needs_input && args.input.empty())) {
      emit_help([&] { print_usage(argv[0]); }, 0);
      return 1;
    }

    if (args.command == "repl" && !args.input.empty()) {
      error("repl does not accept an input file");
      return 1;
    }

    const auto ext = args.input.extension();
    auto weights_model_ptr = std::shared_ptr<t81::weights::ModelFile>{};
    if (args.weights_model &&
        (args.command == "compile" ||
         ((args.command == "run" || args.command == "debug") && ext == ".t81") ||
         args.command == "repl")) {
      weights_model_ptr = load_weights_model_optional(args.weights_model);
      if (!weights_model_ptr) return 1;
    }

    if (args.command == "compile") {
      if (args.policy) {
        error("compile does not support --policy yet; pass policy at runtime with 't81 run --policy'.");
        return 1;
      }
      fs::path out = args.output.value_or(args.input.stem().string() + ".tisc");
      if (ext == ".t81") {
        // If a policy is provided during compile, we should probably embed it.
        // However, our current compile function doesn't take a policy path.
        // For now, let's just pass it if we extend compile later.
        return t81::cli::compile(args.input, out, {}, {}, weights_model_ptr);
      } else if (ext == ".t81w") {
        try {
          auto model = t81::weights::load_t81w(args.input);
          auto source = t81::weights::emit_t81w_module(model, args.input.string());
          auto model_ptr = std::make_shared<t81::weights::ModelFile>(std::move(model));
          return t81::cli::compile(args.input, out, source, args.input.string(), model_ptr);
        } catch (const std::exception& e) {
          error(e.what());
          return 1;
        }
      } else {
        error("compile expects a .t81 or .t81w source file");
        return 1;
      }

    } else if (args.command == "run") {
      if (ext == ".t81") {
        TempTiscFile temp(args.input.stem().string());
        int rc = t81::cli::compile(args.input, temp.path, {}, {}, weights_model_ptr);
        if (rc != 0) return rc;
        return t81::cli::run_tisc(temp.path, args.policy, args.trace, args.trace_output);
      } else if (ext == ".tisc") {
        return t81::cli::run_tisc(args.input, args.policy, args.trace, args.trace_output);
      } else {
        error("run expects .t81 or .tisc file");
        return 1;
      }

    } else if (args.command == "disasm") {
      if (ext == ".tisc") {
        return t81::cli::disasm_tisc(args.input);
      }
      error("disasm expects a .tisc file");
      return 1;

    } else if (args.command == "debug") {
      if (ext == ".t81") {
        TempTiscFile temp(args.input.stem().string());
        int rc = t81::cli::compile(args.input, temp.path, {}, {}, weights_model_ptr);
        if (rc != 0) return rc;
        return t81::cli::debug_tisc(temp.path, args.policy);
      } else if (ext == ".tisc") {
        return t81::cli::debug_tisc(args.input, args.policy);
      } else {
        error("debug expects .t81 or .tisc file");
        return 1;
      }

    } else if (args.command == "check" || args.command == "lint") {
      if (ext != ".t81") {
        error(args.command + " expects a .t81 source file");
        return 1;
      }
      return t81::cli::check_syntax(args.input);

    } else if (args.command == "profile") {
      return run_profile_command(args);

    } else if (args.command == "benchmark") {
      return run_benchmark(argv[0], args);

    } else if (args.command == "repro-hash") {
      if (!args.command_args.empty() &&
          (args.command_args[0] == "-h" || args.command_args[0] == "--help")) {
        return emit_help([&] { print_help_repro_hash(); });
      }
      return run_repro_hash(argv[0], args);

    } else if (args.command == "canonize-tensor") {
      if (!args.command_args.empty() &&
          (args.command_args[0] == "-h" || args.command_args[0] == "--help")) {
        return emit_help([&] { print_help_canonize_tensor(); });
      }
      if (args.command_args.empty()) {
        error("canonize-tensor requires an input file. Run 't81 help canonize-tensor'.");
        return 1;
      }
      return t81::cli::canonize_tensor(args.command_args[0], discover_canonfs_root());

    } else if (args.command == "canonize-file") {
      if (!args.command_args.empty() &&
          (args.command_args[0] == "-h" || args.command_args[0] == "--help")) {
        return emit_help([&] { print_help_canonize_file(); });
      }
      return run_canonize_file(args);

    } else if (args.command == "memory-stats") {
      return run_memory_stats(args);

    } else if (args.command == "canonfs") {
      return run_canonfs_command(args);

    } else if (args.command == "determinism") {
      return run_determinism_command(argv[0], args);

    } else if (args.command == "tensor") {
      return run_tensor_command(argv[0], args);

    } else if (args.command == "vm") {
      return run_vm_command(args);

    } else if (args.command == "tisc") {
      return run_tisc_command(args);

    } else if (args.command == "ir") {
      return run_ir_command(args);

    } else if (args.command == "tier") {
      return run_tier_command(args);

    } else if (args.command == "env-paths") {
      return run_env_paths(args);

    } else if (args.command == "env-check") {
      return run_env_check(args);

    } else if (args.command == "env-diag") {
      return run_env_diag(args);

    } else if (args.command == "env-clean") {
      return run_env_clean(args);

    } else if (args.command == "env-toolchain") {
      return run_env_toolchain(args);

    } else if (args.command == "doctor") {
      return run_doctor(args);

    } else if (args.command == "test") {
      return run_test_command(args);

    } else if (args.command == "fmt") {
      return run_fmt_command(args);

    } else if (args.command == "completion") {
      return run_completion_command(args);

    } else if (args.command == "man") {
      return run_man_command(args);

    } else if (args.command == "feedback") {
      return run_feedback_command(args);

    } else if (args.command == "init") {
      if (!args.command_args.empty() &&
          (args.command_args[0] == "-h" || args.command_args[0] == "--help")) {
          return emit_help([&] { print_help_init(); });
        }
      if (args.command_args.empty()) {
        error("init requires a project name. Run 't81 help init'.");
        return 1;
      }
      return t81::cli::init_project(args.command_args[0]);

    } else if (args.command == "pkg") {
      if (!args.command_args.empty() &&
          (args.command_args[0] == "-h" || args.command_args[0] == "--help")) {
        print_help_pkg();
        return 0;
      }
      if (args.command_args.empty()) {
        error("pkg requires a subcommand (init, check). Run 't81 help pkg'.");
        return 1;
      }
      const std::string sub = args.command_args[0];
      if (sub == "init") {
        if (args.command_args.size() >= 2 &&
            (args.command_args[1] == "-h" || args.command_args[1] == "--help")) {
          return emit_help([&] { print_help_pkg(); });
        }
        std::string name = args.command_args.size() > 1 ? args.command_args[1] : "my-t81-pkg";
        return t81::cli::init_package(name);
      } else if (sub == "check") {
        Args check_args = args;
        check_args.command_args.assign(args.command_args.begin() + 1, args.command_args.end());
        return run_pkg_check(check_args);
      }
      error("pkg: unknown subcommand '" + sub + "'. Run 't81 help pkg'.");
      return 1;

    } else if (args.command == "repl") {
      return t81::cli::repl(weights_model_ptr, args.policy);

    } else if (args.command == "weights") {
      return run_weights(args);

    } else if (args.command == "policy") {
      return run_policy(args);

    } else if (args.command == "axion") {
      return run_axion(args);

    } else if (args.command == "trace") {
      if (args.command_args.empty()) {
        return emit_help([&] { print_help_trace(); });
      }
      if (!args.command_args.empty() &&
          (args.command_args[0] == "-h" || args.command_args[0] == "--help")) {
        return emit_help([&] { print_help_trace(); });
      }
      t81::cli::TraceArgs ta;
      for (size_t i = 0; i < args.command_args.size(); ++i) {
        const auto& token = args.command_args[i];
        if (token == "--no-color") {
          ta.no_color = true;
          continue;
        }
        if (ta.subcommand.empty()) {
          ta.subcommand = token;
          continue;
        }
        if ((token == "-h" || token == "--help") && ta.subcommand == "show") {
          print_help_trace_show();
          return 0;
        }
        if ((token == "-h" || token == "--help") && ta.subcommand == "diff") {
          print_help_trace_diff();
          return 0;
        }
        if ((token == "-h" || token == "--help") && ta.subcommand == "replay") {
          print_help_trace_replay();
          return 0;
        }
        if ((token == "-h" || token == "--help") &&
            (ta.subcommand == "summary" || ta.subcommand == "stats")) {
          print_help_trace_summary(ta.subcommand);
          return 0;
        }
        if ((token == "-h" || token == "--help") && ta.subcommand == "filter") {
          return emit_help([&] { print_help_trace_filter(); });
        }
        if ((token == "-h" || token == "--help") && ta.subcommand == "canonicalize") {
          print_help_trace_canonicalize();
          return 0;
        }
        if ((token == "-h" || token == "--help") && ta.subcommand == "export") {
          print_help_trace_export();
          return 0;
        }
        ta.args.push_back(token);
      }
      // Auto-disable color when stdout is not a terminal
      if (!ta.no_color && !isatty(fileno(stdout))) {
        ta.no_color = true;
      }
      return t81::cli::run_trace(ta);

    } else if (args.command == "llama-run") {
      return run_llama_run(args);

    } else if (args.command == "llvm") {
      t81::cli::LLVMArgs la;
      if (args.output) la.output = *args.output;
      if (args.command_args.empty() || args.command_args[0] == "--help" ||
          args.command_args[0] == "-h") {
        return emit_help([&] { print_help_llvm(); });
      }
      la.subcommand = args.command_args[0];
      for (size_t i = 1; i < args.command_args.size(); ++i) {
        const std::string& tok = args.command_args[i];
        if (tok == "--bitcode") {
          la.bitcode = true;
        } else if (tok == "--no-comments") {
          la.no_comments = true;
        } else if ((tok == "-o" || tok == "--output") && i + 1 < args.command_args.size()) {
          la.output = args.command_args[++i];
        } else if ((tok == "--help" || tok == "-h")) {
          return emit_help([&] { print_help_llvm(); });
        } else if (tok[0] != '-') {
          if (la.input.empty()) la.input = tok;
          else la.output = tok;
        } else {
          error("llvm: unknown option '" + tok + "'. Run 't81 help llvm'.");
          return 1;
        }
      }
      return t81::cli::run_llvm(la);

    } else if (args.command == "c") {
      t81::cli::CArgs ca;
      if (args.output) ca.output = *args.output;
      if (args.command_args.empty() || args.command_args[0] == "--help" ||
          args.command_args[0] == "-h") {
        return emit_help([&] { print_help_c(); });
      }
      ca.subcommand = args.command_args[0];
      for (size_t i = 1; i < args.command_args.size(); ++i) {
        const std::string& tok = args.command_args[i];
        if (tok == "--mode=dcp") {
          ca.dcp_floats = true;
        } else if (tok == "--mode=compat") {
          ca.dcp_floats = false;
        } else if (tok == "--dialect=t81") {
          ca.use_t81_dialect = true;
        } else if (tok == "--dialect=std") {
          ca.use_t81_dialect = false;
        } else if (tok == "--emit" && i + 1 < args.command_args.size()) {
          ca.emit = args.command_args[++i];
        } else if (tok.rfind("--emit=", 0) == 0) {
          ca.emit = tok.substr(7);
        } else if (tok == "--no-comments") {
          ca.no_comments = true;
        } else if ((tok == "-o" || tok == "--output") && i + 1 < args.command_args.size()) {
          ca.output = args.command_args[++i];
        } else if (tok == "--help" || tok == "-h") {
          return emit_help([&] { print_help_c(); });
        } else if (tok[0] != '-') {
          if (ca.input.empty()) ca.input = tok;
          else ca.output = tok;
        } else {
          error("c: unknown option '" + tok + "'. Run 't81 help c'.");
          return 1;
        }
      }
      return t81::cli::run_c(ca);

    } else if (args.command == "rust") {
      t81::cli::RustArgs ra;
      if (args.output) ra.output = *args.output;
      if (args.command_args.empty() || args.command_args[0] == "--help" ||
          args.command_args[0] == "-h") {
        return emit_help([&] { print_help_rust(); });
      }
      ra.subcommand = args.command_args[0];
      for (size_t i = 1; i < args.command_args.size(); ++i) {
        const std::string& tok = args.command_args[i];
        if (tok == "--mode=dcp") {
          ra.dcp_floats = true;
        } else if (tok == "--mode=compat") {
          ra.dcp_floats = false;
        } else if (tok == "--dialect=t81") {
          ra.use_t81_dialect = true;
        } else if (tok == "--dialect=std") {
          ra.use_t81_dialect = false;
        } else if (tok == "--emit" && i + 1 < args.command_args.size()) {
          ra.emit = args.command_args[++i];
        } else if (tok.rfind("--emit=", 0) == 0) {
          ra.emit = tok.substr(7);
        } else if (tok == "--no-comments") {
          ra.no_comments = true;
        } else if ((tok == "-o" || tok == "--output") && i + 1 < args.command_args.size()) {
          ra.output = args.command_args[++i];
        } else if (tok == "--help" || tok == "-h") {
          return emit_help([&] { print_help_rust(); });
        } else if (tok[0] != '-') {
          if (ra.input.empty()) ra.input = tok;
          else ra.output = tok;
        } else {
          error("rust: unknown option '" + tok + "'. Run 't81 help rust'.");
          return 1;
        }
      }
      return t81::cli::run_rust(ra);

    } else if (args.command == "python") {
      t81::cli::PythonArgs pa;
      if (args.output) pa.output = *args.output;
      if (args.command_args.empty() || args.command_args[0] == "--help" ||
          args.command_args[0] == "-h") {
        return emit_help([&] { print_help_python(); });
      }
      pa.subcommand = args.command_args[0];
      for (size_t i = 1; i < args.command_args.size(); ++i) {
        const std::string& tok = args.command_args[i];
        if (tok == "--mode=dcp") {
          pa.dcp_floats = true;
        } else if (tok == "--mode=compat") {
          pa.dcp_floats = false;
        } else if (tok == "--dialect=t81") {
          pa.use_t81_dialect = true;
        } else if (tok == "--dialect=std") {
          pa.use_t81_dialect = false;
        } else if (tok == "--emit" && i + 1 < args.command_args.size()) {
          pa.emit = args.command_args[++i];
        } else if (tok.rfind("--emit=", 0) == 0) {
          pa.emit = tok.substr(7);
        } else if (tok == "--no-comments") {
          pa.no_comments = true;
        } else if ((tok == "-o" || tok == "--output") && i + 1 < args.command_args.size()) {
          pa.output = args.command_args[++i];
        } else if (tok == "--help" || tok == "-h") {
          return emit_help([&] { print_help_python(); });
        } else if (tok[0] != '-') {
          if (pa.input.empty()) pa.input = tok;
          else pa.output = tok;
        } else {
          error("python: unknown option '" + tok + "'. Run 't81 help python'.");
          return 1;
        }
      }
      return t81::cli::run_python(pa);

    } else if (args.command == "mlir") {
      t81::cli::MlirArgs ma;
      if (args.output) ma.output = *args.output;
      if (args.command_args.empty() || args.command_args[0] == "--help" ||
          args.command_args[0] == "-h") {
        return emit_help([&] { print_help_mlir(); });
      }
      ma.subcommand = args.command_args[0];
      for (size_t i = 1; i < args.command_args.size(); ++i) {
        const std::string& tok = args.command_args[i];
        if (tok == "--mode=dcp") {
          ma.dcp_floats = true;
        } else if (tok == "--mode=compat") {
          ma.dcp_floats = false;
        } else if (tok == "--dialect=t81") {
          ma.use_t81_dialect = true;
        } else if (tok == "--dialect=std") {
          ma.use_t81_dialect = false;
        } else if (tok == "--no-comments") {
          ma.no_comments = true;
        } else if ((tok == "-o" || tok == "--output") && i + 1 < args.command_args.size()) {
          ma.output = args.command_args[++i];
        } else if (tok == "--help" || tok == "-h") {
          return emit_help([&] { print_help_mlir(); });
        } else if (tok[0] != '-') {
          if (ma.input.empty()) ma.input = tok;
          else ma.output = tok;
        } else {
          error("mlir: unknown option '" + tok + "'. Run 't81 help mlir'.");
          return 1;
        }
      }
      return t81::cli::run_mlir(ma);

    // ── RFC-0033: TUI frontends ────────────────────────────────────────────
    } else if (args.command == "studio" || args.command == "agent" ||
               args.command == "ui") {
#if defined(T81_HAS_TUI)
      if (args.command == "studio") {
        return t81::tui::run_studio(args.command_args);
      }
      if (args.command == "agent") {
        return t81::tui::run_agent(args.command_args);
      }
      // "ui" — interactive launcher: choose Studio or Agent
      {
        using namespace ftxui;
        std::vector<std::string> choices = {
          "Studio  —  Human Operator Interface",
          "Agent   —  AI-Native / Agentic Interface",
          "Cancel",
        };
        int selected = 0;
        auto screen  = ScreenInteractive::Fullscreen();
        auto menu_c  = Menu(&choices, &selected);
        auto root    = Renderer(menu_c, [&]() -> Element {
          return vbox({
            text("  T81 Foundation  —  Choose Interface") | bold
              | color(Color::Cyan),
            separator(),
            menu_c->Render(),
            separator(),
            text("  Arrow keys, Enter to select, Escape to cancel.")
              | color(Color::GrayDark),
          }) | border | size(WIDTH, EQUAL, 52) | center;
        });
        root = CatchEvent(root, [&](Event e) -> bool {
          if (e == Event::Escape) { screen.ExitLoopClosure()(); return true; }
          if (e == Event::Return) { screen.ExitLoopClosure()(); return true; }
          return false;
        });
        screen.Loop(root);
        if (selected == 0) return t81::tui::run_studio({});
        if (selected == 1) return t81::tui::run_agent({});
        return 0;
      }
#else
      error("TUI frontends are not available in this build. "
            "Reconfigure with -DT81_BUILD_TUI=ON and rebuild.");
      return 1;
#endif

    } else {
      error("Unknown command: " + args.command + ". Run 't81 --help' to list commands.");
      emit_help([&] { print_usage(argv[0]); });
      return 1;
    }

  } catch (const std::exception& e) {
    error(e.what());
    if (!g_flags.quiet) {
      std::cerr << "Run '" << (argc > 0 ? argv[0] : "t81") << " help' for usage.\n";
    }
    return 1;
  } catch (...) {
    error("Unknown exception");
    return 1;
  }
}
