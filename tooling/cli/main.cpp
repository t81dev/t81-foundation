/**
 * @file main.cpp
 * @brief T81 Foundation Command-Line Interface
 *
 * Sovereign-grade, zero-dependency, ternary-native toolchain driver.
 * MIT + GPL-3.0 dual-licensed.
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>
#if !defined(_WIN32)
#include <sys/wait.h>
#include <unistd.h>
#else
#include <io.h>
#endif

#include "internal/tooling/logging.hpp"
#include "t81/config.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/cli/driver.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/isa/binary_io.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/tracing/canonhash.hpp"
#include "t81/vm/vm.hpp"
#include "core/vm/internal/memory_segments.hpp"
#include "t81/weights.hpp"
#if defined(T81_HAS_LLAMA_CPP)
#include "t81/experimental/llama_cpp_adapter.hpp"
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

void print_help_weights() {
  std::cerr << R"(
Usage: t81 weights <subcommand> [options]

Subcommands:
  import <file> [-o <out>] [--format <fmt>] Import weights (safetensors/gguf) -> .t81w
  info <model.t81w>                          Print native model metadata
  quantize <input> --to-gguf <out>           Quantize SafeTensors -> T3_K GGUF

Options:
  --format <fmt>    Input format (safetensors, gguf). Default: safetensors
  -o, --out <file>  Output file path

Examples:
  t81 weights import model.safetensors -o model.t81w
  t81 weights info model.t81w --json
  t81 weights quantize model.safetensors --to-gguf model.gguf
)";
}

void print_help_policy() {
  std::cerr << R"(
Usage: t81 policy <subcommand> [options]

Subcommands:
  compile <file.apl> [-o <out>]   Compile Axion Policy Language -> .axionb
  run <file.apl|.axionb>          Validate and load an Axion policy

Options:
  -o <out>          Output file path

Examples:
  t81 policy compile policy.apl -o policy.axionb
  t81 policy run policy.apl --json
)";
}

void print_help_trace() {
  std::cerr << R"(
Usage: t81 trace <subcommand> [args]

Subcommands:
  show <trace.txt>                Visualize an Axion trace with color
  diff <trace1.txt> <trace2.txt>  Diff two Axion traces
  replay <file.tisc> <trace.txt>  Replay and verify trace matches
  export <trace.txt> [options]    Export trace to JSON/CSV

Options:
  --format <json|csv>             Export format (default: json)
  -o, --out <file>                Output file path (default: stdout)
  --no-color                      Disable ANSI colors in show/diff output

Examples:
  t81 trace show trace.log
  t81 trace diff run1.log run2.log --no-color
  t81 trace export trace.log --format json -o trace.json
)";
}

void print_help_canonize_file() {
  std::cerr << R"(
Usage: t81 canonize-file <file> [--canonfs-root <path>]

Writes raw file bytes into CanonFS and prints `sha3-256:<hash>`.
)";
}

void print_help_memory_stats() {
  std::cerr << R"(
Usage: t81 memory-stats [file]

Displays memory pool statistics and profiling information.
If a T81 file is provided, runs the program and shows memory usage.
Without a file, shows current memory pool configuration.

Examples:
  t81 memory-stats program.t81
  t81 memory-stats
)";
}

void print_help_llama_run() {
  std::cerr << R"(
Usage: t81 llama-run <model.gguf|sha3-256:hash> <prompt> --policy <policy.apl> [options]

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
Usage: t81 compile <file.t81|file.t81w> [-o <file.tisc>] [--weights-model <model.t81w>]

Compiles T81Lang source (or T81 weight module source) into TISC bytecode.

Example:
  t81 compile program.t81 -o program.tisc
)";
}

void print_help_run() {
  std::cerr << R"(
Usage: t81 run <file.t81|file.tisc> [--policy <policy.apl>] [--trace] [--weights-model <model.t81w>]

Compiles (if needed) and executes a program via the VM.

Example:
  t81 run program.tisc --policy policy.apl
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
Usage: t81 doctor [--json]

Runs local environment readiness checks and reports actionable diagnostics.

Examples:
  t81 doctor
  t81 doctor --json
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

Example:
  t81 project init my_project
)";
}

void print_help_env() {
  std::cerr << R"(
Usage: t81 env <action> [args]

Actions:
  doctor [--json]                     Run environment readiness checks
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

void print_help_repro_hash() {
  std::cerr << R"(
Usage: t81 repro-hash [fixtures_dir]

Runs the T81Lang reproducibility fixture hash gate.

Example:
  t81 repro-hash tests/fixtures/t81lang_determinism
)";
}

void print_help_canonize_tensor() {
  std::cerr << R"(
Usage: t81 canonize-tensor <file>

Canonicalizes tensor input into CanonFS object storage.

Example:
  t81 canonize-tensor model.t81w
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
Usage: t81 pkg <subcommand> [args]

Subcommands:
  init [package_name]
  check [package.t81] [--json]

Examples:
  t81 pkg init my_pkg
  t81 pkg check
  t81 pkg check --json
)";
}

void print_help_benchmark() {
  std::cerr << R"(
Usage: t81 benchmark [benchmark_runner_flags...]

Runs the core benchmark suite.

Example:
  t81 benchmark --benchmark_filter=BM_VMSimulation_Dispatch
)";
}

void print_help_advanced() {
  std::cerr << R"(
Advanced commands (supported expert workflows):
  weights <subcommand> [args]         Model weights import/info/quantize tools
  policy <subcommand> [args]          Axion policy compile/validation tools
  trace <subcommand> [args]           Trace inspection, diff, replay, export

Use:
  t81 help weights
  t81 help policy
  t81 help trace
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

void print_help_weights_quantize() {
  std::cerr << R"(
Usage: t81 weights quantize <input> --to-gguf <out>

Quantizes SafeTensors input into GGUF output.
)";
}

void print_help_policy_compile() {
  std::cerr << R"(
Usage: t81 policy compile <file.apl> [-o <out>]

Compiles Axion policy source into bytecode.
)";
}

void print_help_policy_run() {
  std::cerr << R"(
Usage: t81 policy run <file.apl|file.axionb> [--json]

Parses/loads policy and reports validation result.
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
Usage: t81 trace replay <file.tisc> <trace.txt>
)";
}

void print_help_trace_export() {
  std::cerr << R"(
Usage: t81 trace export <trace.txt> [--format <json|csv>] [-o <file>]
)";
}

void print_usage(const char* prog) {
  std::cerr << R"(T81 Foundation - Ternary-Native Computing Stack
Version )" << T81_VERSION
            << R"(


Usage: )" << prog
            << R"( <command> [options] [args]


Commands:
  code    <action> [args]               Domain-first code workflow commands
  project <action> [args]               Project lifecycle commands
  env     <action> [args]               Environment/toolchain diagnostics
  internal <action> [args]              Internal/experimental command group
  completion <shell>                    Print shell completion script
  man [--install-dir <dir>]             Show or install CLI manpage
  feedback <subcommand> [args]          Local CLI UX feedback loop
  ---
  check   <file.t81>                   Syntax/semantic check (no bytecode output)
  lint    <file.t81>                   Alias for `check`
  compile <file.t81|.t81w> [...]       Compile source to TISC bytecode
  run     <file.t81|.tisc> [...]       Compile (if needed) and execute
  disasm  <file.tisc>                  Print human-readable TISC disassembly
  debug   <file.t81|.tisc> [...]       Start debugger (compile if needed)
  repl                                 Enter interactive REPL
  test    [options] [-- ...]           Run project tests via CTest
  doctor  [--json]                     Check environment/toolchain readiness
  fmt     [options] <file...>          Format files (supports --check)
  init    <project_name>               Scaffold a new T81 project
  version                              Show version
  help [command]                       Show help for command/topic


Global options:
  -v, --verbose                        Verbose diagnostic output
  -q, --quiet                          Suppress non-error output
  -h, --help                           Show help

Diagnostics:
  `t81 compile` now prints any semantic or parsing errors with the originating
  source file, line, and column so you can jump directly to the issue without
  rerunning separate diagnostics.

More command groups:
  t81 help code
  t81 help project
  t81 help env
  t81 help internal
  t81 help advanced
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
  if (topic == "build") {
    print_help_compile();
    return true;
  }
  if (topic == "weights") {
    print_help_weights();
    return true;
  }
  if (topic == "policy") {
    print_help_policy();
    return true;
  }
  if (topic == "trace") {
    print_help_trace();
    return true;
  }
  if (topic == "repro-hash") {
    print_help_repro_hash();
    return true;
  }
  if (topic == "canonize-tensor") {
    print_help_canonize_tensor();
    return true;
  }
  if (topic == "canonize-file") {
    print_help_canonize_file();
    return true;
  }
  if (topic == "init") {
    print_help_init();
    return true;
  }
  if (topic == "pkg") {
    print_help_pkg();
    return true;
  }
  if (topic == "repl") {
    print_help_repl();
    return true;
  }
  if (topic == "test") {
    print_help_test();
    return true;
  }
  if (topic == "doctor") {
    print_help_doctor();
    return true;
  }
  if (topic == "fmt") {
    print_help_fmt();
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
  if (topic == "benchmark") {
    print_help_benchmark();
    return true;
  }
  if (topic == "llama-run") {
    print_help_llama_run();
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
  if (topic == "help" || topic.empty()) {
    print_usage(prog);
    return true;
  }
  return false;
}

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
  bool need_help = false;
  bool need_version = false;
  std::vector<std::string> benchmark_args;
  std::vector<std::string> command_args;
  std::optional<fs::path> weights_model;
  std::optional<fs::path> policy;
  bool trace = false;
};

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
      error("Unknown option: " + std::string(token));
      std::exit(1);
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
    else if (arg == "-o" || arg == "--output") {
      if (++i >= argc) {
        error("Missing argument after -o");
        std::exit(1);
      }
      a.output = fs::path(argv[i]);
    } else if (arg == "--weights-model") {
      if (++i >= argc) {
        error("Missing argument after --weights-model");
        std::exit(1);
      }
      a.weights_model = fs::path(argv[i]);
    } else if (arg == "--policy") {
      if (++i >= argc) {
        error("Missing argument after --policy");
        std::exit(1);
      }
      a.policy = fs::path(argv[i]);
    } else if (arg == "--trace") {
      a.trace = true;
    } else if (arg == "-h" || arg == "--help") {
      a.need_help = true;
    } else if (arg == "-V" || arg == "--version") {
      if (a.command == "fmt" || a.command == "code" || a.command == "project" ||
          a.command == "env" || a.command == "internal" || a.command == "completion" ||
          a.command == "man" || a.command == "feedback") {
        a.command_args.emplace_back(argv[i]);
      } else {
        a.need_version = true;
      }
    } else if (arg.starts_with('-')) {
      // Subcommands under these top-level commands own additional flags.
      if (a.command == "benchmark") {
        a.benchmark_args.emplace_back(argv[i]);
      } else if (a.command == "weights" || a.command == "help" || a.command == "init" ||
                 a.command == "pkg" || a.command == "repro-hash" || a.command == "policy" ||
                 a.command == "trace" || a.command == "llama-run" || a.command == "test" ||
                 a.command == "doctor" || a.command == "fmt" || a.command == "code" ||
                 a.command == "project" || a.command == "env" || a.command == "internal" ||
                 a.command == "completion" || a.command == "man" || a.command == "feedback" ||
                 a.command == "canonize-tensor" || a.command == "canonize-file") {
        a.command_args.emplace_back(argv[i]);
      } else {
        error("Unknown option: " + std::string(arg));
        std::exit(1);
      }
    } else {
      if (a.command == "benchmark") {
        a.benchmark_args.emplace_back(argv[i]);
      } else if (a.command == "weights" || a.command == "help" || a.command == "init" ||
                 a.command == "pkg" || a.command == "repro-hash" || a.command == "policy" ||
                 a.command == "trace" || a.command == "llama-run" || a.command == "test" ||
                 a.command == "doctor" || a.command == "fmt" || a.command == "code" ||
                 a.command == "project" || a.command == "env" || a.command == "internal" ||
                 a.command == "completion" || a.command == "man" || a.command == "feedback" ||
                 a.command == "canonize-tensor" || a.command == "canonize-file") {
        a.command_args.emplace_back(argv[i]);
      } else {
        if (!a.input.empty()) {
          error("Multiple input files not supported yet");
          std::exit(1);
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

  // Benchmarks intentionally trigger many overflow traps (BM_overflow_*).
  // Mute trap stderr for this subprocess so benchmark runs don't flood logs.
#if defined(_WIN32)
  std::string cmd = "set T81_AXION_TRAP_STDERR=0&& ";
#else
  std::string cmd = "T81_AXION_TRAP_STDERR=0 ";
#endif
  cmd += shell_escape(runner_path->string());
  for (const auto& extra : args.benchmark_args) {
    cmd += ' ';
    cmd += shell_escape(extra);
  }

  info("Running benchmarks via " + runner_path->string());
#if defined(_WIN32)
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
    execl("/bin/sh", "sh", "-c", cmd.c_str(), static_cast<char*>(nullptr));
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

int run_weights_import(const Args& args) {
  if (args.command_args.size() < 2) {
    error("weights import requires the input file");
    return 1;
  }
  WeightsImportOptions opts;
  opts.input = fs::path(args.command_args[1]);
  size_t idx = 2;
  while (idx < args.command_args.size()) {
    const auto& token = args.command_args[idx++];
    if (token == "--format") {
      if (idx >= args.command_args.size()) {
        error("weights import: missing argument for --format");
        return 1;
      }
      opts.format = args.command_args[idx++];
    } else if (token == "-o" || token == "--out") {
      if (idx >= args.command_args.size()) {
        error("weights import: missing argument for " + token);
        return 1;
      }
      opts.output = fs::path(args.command_args[idx++]);
    } else if (opts.input.empty()) {
      opts.input = fs::path(token);
    } else {
      error("weights import: unexpected argument '" + token + "'");
      return 1;
    }
  }
  if (opts.input.empty()) {
    error("weights import needs an input file");
    return 1;
  }
  if (!opts.output) {
    opts.output = opts.input.stem();
    opts.output->replace_extension(".t81w");
  }

  t81::weights::ModelFile mf;
  try {
    if (opts.format == "safetensors") {
      mf = t81::weights::load_safetensors(opts.input);
    } else if (opts.format == "gguf") {
      mf = t81::weights::load_gguf(opts.input);
    } else {
      error("weights import: unsupported format: " + opts.format);
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

int run_weights_quantize(const Args& args) {
  if (args.command_args.size() != 4 || args.command_args[2] != "--to-gguf") {
    error("weights quantize requires: quantize <input> --to-gguf <output>. Run 't81 help weights "
          "quantize'.");
    return 1;
  }
  fs::path input = args.command_args[1];
  fs::path output = args.command_args[3];
  try {
    t81::weights::quantize_safetensors_to_gguf(input, output);
  } catch (const std::exception& e) {
    error(e.what());
    return 1;
  }
  return 0;
}

int run_weights(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    print_help_weights();
    return args.command_args.empty() ? 1 : 0;
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
    if (sub == "quantize") {
      print_help_weights_quantize();
      return 0;
    }
    error("weights: unknown subcommand '" + sub + "'. Run 't81 help weights'.");
    return 1;
  }
  const std::string sub = args.command_args[0];
  if (sub == "import") {
    return run_weights_import(args);
  } else if (sub == "info") {
    return run_weights_info(args);
  } else if (sub == "quantize") {
    return run_weights_quantize(args);
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

int run_policy(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    print_help_policy();
    return args.command_args.empty() ? 1 : 0;
  }
  if (args.command_args.size() >= 2 &&
      (args.command_args[1] == "-h" || args.command_args[1] == "--help")) {
    const std::string sub = args.command_args[0];
    if (sub == "compile") {
      print_help_policy_compile();
      return 0;
    }
    if (sub == "run") {
      print_help_policy_run();
      return 0;
    }
    error("policy: unknown subcommand '" + sub + "'. Run 't81 help policy'.");
    return 1;
  }
  if (args.command_args.empty()) {
    error("policy requires a subcommand (compile|run). Run 't81 help policy'.");
    return 1;
  }
  const std::string sub = args.command_args[0];
  if (sub == "compile") return run_policy_compile(args);
  if (sub == "run") return run_policy_run(args);
  error("policy: unknown subcommand '" + sub + "'. Run 't81 help policy'.");
  return 1;
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

  std::string cmd = "cd " + shell_escape(repo_root.string()) + " && python3 " +
                    shell_escape(script_path->string()) + " --t81-bin " +
                    shell_escape(exe_path.string()) + " --fixtures-dir " +
                    shell_escape(fixtures_arg) + " --workdir " + shell_escape(workdir.string()) +
                    " --hash-out " + shell_escape(hash_out.string()) + " --expected-hash-file " +
                    shell_escape(expected_arg);

  int status = std::system(cmd.c_str());
  if (status == -1) {
    error("Failed to execute t81lang_repro_gate.py");
    return 1;
  }
  int rc = decode_system_status(status);
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
  if (args.command_args.empty()) {
    // Show current memory pool configuration
    std::cout << "\n=== Memory Pool Configuration ===" << std::endl;
    std::cout << "Default Stack Size: 256 words" << std::endl;
    std::cout << "Default Heap Size: 768 words" << std::endl;
    std::cout << "Default Tensor Space: 256 words" << std::endl;
    std::cout << "Default Meta Space: 256 words" << std::endl;
    std::cout << "Total Default Memory: 1,536 words" << std::endl;
    std::cout << "\nTo see memory usage for a specific program, run:" << std::endl;
    std::cout << "  t81 memory-stats <program.t81>" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
  }

  // For now, just show the configuration and indicate profiling would be integrated
  fs::path input = args.command_args[0];
  if (!fs::exists(input)) {
    error("File not found: " + input.string());
    return 1;
  }

  std::cout << "\n=== Memory Pool Analysis ===" << std::endl;
  std::cout << "Program: " << input.string() << std::endl;
  std::cout << "Status: Memory profiling integration in progress" << std::endl;
  std::cout << "\nCurrent memory pool configuration:" << std::endl;
  std::cout << "Stack: 256 words (fixed)" << std::endl;
  std::cout << "Heap: 768 words (fixed)" << std::endl;
  std::cout << "Tensor: 256 words (fixed)" << std::endl;
  std::cout << "Meta: 256 words (fixed)" << std::endl;
  std::cout << "\nOptimization opportunities:" << std::endl;
  std::cout << "- Dynamic pool sizing based on program requirements" << std::endl;
  std::cout << "- Memory compaction to reduce fragmentation" << std::endl;
  std::cout << "- Unified memory pool for better utilization" << std::endl;
  std::cout << "=================================" << std::endl;
  
  return 0;
}

int run_canonize_file(const Args& args) {
  if (args.command_args.empty()) {
    error("canonize-file requires an input file. Run 't81 help canonize-file'.");
    return 1;
  }

  fs::path input = args.command_args[0];
  fs::path canon_root = fs::current_path() / ".t81_canonfs";
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

bool shell_command_available(const std::string& command) {
#if defined(_WIN32)
  std::string probe = "where " + command + " >nul 2>nul";
#else
  std::string probe = "command -v " + shell_escape(command) + " >/dev/null 2>&1";
#endif
  const int status = std::system(probe.c_str());
  if (status == -1) {
    return false;
  }
  return decode_system_status(status) == 0;
}

struct DoctorCheck {
  std::string id;
  bool ok = false;
  std::string detail;
  std::string remediation;
};

int run_doctor(const Args& args) {
  bool as_json = false;
  for (const auto& token : args.command_args) {
    if (token == "--json") {
      as_json = true;
    } else if (token == "-h" || token == "--help") {
      print_help_doctor();
      return 0;
    } else {
      error("doctor: unknown option '" + token + "'. Run 't81 help doctor'.");
      return 1;
    }
  }

  std::vector<DoctorCheck> checks;

  DoctorCheck ctest_check;
  ctest_check.id = "ctest_available";
  ctest_check.ok = shell_command_available("ctest");
  ctest_check.detail = ctest_check.ok ? "ctest found in PATH" : "ctest not found in PATH";
  ctest_check.remediation = "Install CMake/CTest and ensure `ctest` is in PATH.";
  checks.push_back(std::move(ctest_check));

  DoctorCheck build_dir_check;
  build_dir_check.id = "build_dir_present";
  build_dir_check.ok = fs::exists("build/CTestTestfile.cmake");
  build_dir_check.detail = build_dir_check.ok ? "build/CTestTestfile.cmake found"
                                              : "build dir is missing CTest metadata";
  build_dir_check.remediation = "Run: cmake -S . -B build && cmake --build build";
  checks.push_back(std::move(build_dir_check));

  DoctorCheck write_check;
  write_check.id = "temp_writable";
  try {
    const fs::path probe =
        fs::temp_directory_path() / ("t81-doctor-probe-" + std::to_string(std::rand()) + ".tmp");
    {
      std::ofstream out(probe);
      write_check.ok = static_cast<bool>(out);
      if (write_check.ok) {
        out << "ok";
      }
    }
    std::error_code ignore_ec;
    fs::remove(probe, ignore_ec);
    write_check.detail = write_check.ok ? "temp directory writable" : "temp directory not writable";
  } catch (...) {
    write_check.ok = false;
    write_check.detail = "temp directory probe failed";
  }
  write_check.remediation = "Fix filesystem permissions for your temp directory.";
  checks.push_back(std::move(write_check));

  DoctorCheck python_check;
  python_check.id = "python3_available";
  python_check.ok = shell_command_available("python3");
  python_check.detail = python_check.ok ? "python3 found in PATH" : "python3 not found in PATH";
  python_check.remediation = "Install Python 3 for CI/ops utilities (repro-hash/docs gates).";
  checks.push_back(std::move(python_check));

  bool all_ok = true;
  for (const auto& check : checks) {
    all_ok = all_ok && check.ok;
  }

  if (as_json) {
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.doctor.v1\",\n";
    std::cout << "  \"ok\": " << (all_ok ? "true" : "false") << ",\n";
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

  std::string base_cmd = "ctest --test-dir " + shell_escape(build_dir.string());
  if (list_only) {
    base_cmd += " -N";
  } else {
    base_cmd += " --output-on-failure";
  }
  if (filter) {
    base_cmd += " -R " + shell_escape(*filter);
  }
  for (const auto& token : passthrough) {
    base_cmd += " ";
    base_cmd += shell_escape(token);
  }

  auto read_file = [](const fs::path& p) -> std::string {
    std::ifstream in(p, std::ios::binary);
    if (!in) return {};
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  };
  auto read_size = [](const fs::path& p) -> std::uintmax_t {
    std::error_code ec;
    auto sz = fs::file_size(p, ec);
    return ec ? 0 : sz;
  };

  auto run_ctest_capture = [&](const std::string& effective_cmd) {
    const fs::path out_path = fs::temp_directory_path() / "t81-test.out";
    const fs::path err_path = fs::temp_directory_path() / "t81-test.err";
    std::string cmd = effective_cmd + " > " + shell_escape(out_path.string()) + " 2> " +
                      shell_escape(err_path.string());
    const int status = std::system(cmd.c_str());
    if (status == -1) {
      return std::tuple<int, std::string, std::string, std::uintmax_t, std::uintmax_t>{
          -1, {}, {}, 0, 0};
    }
    const int rc = decode_system_status(status);
    const std::string stdout_text = read_file(out_path);
    const std::string stderr_text = read_file(err_path);
    const auto stdout_bytes = read_size(out_path);
    const auto stderr_bytes = read_size(err_path);
    std::error_code ignore_ec;
    fs::remove(out_path, ignore_ec);
    fs::remove(err_path, ignore_ec);
    return std::tuple<int, std::string, std::string, std::uintmax_t, std::uintmax_t>{
        rc, stdout_text, stderr_text, stdout_bytes, stderr_bytes};
  };

  bool used_junit = false;
  fs::path junit_path = fs::temp_directory_path() / "t81-test-junit.xml";
  std::string effective_cmd = base_cmd;
  if (!list_only) {
    effective_cmd += " --output-junit " + shell_escape(junit_path.string());
    used_junit = true;
  }
  auto [rc, stdout_text, stderr_text, stdout_bytes, stderr_bytes] = run_ctest_capture(effective_cmd);
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
      std::tie(rc, stdout_text, stderr_text, stdout_bytes, stderr_bytes) = run_ctest_capture(base_cmd);
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
  if (used_junit && fs::exists(junit_path)) {
    const std::string xml = read_file(junit_path);
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
    std::error_code ignore_ec;
    fs::remove(junit_path, ignore_ec);
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
  local commands="code project env internal completion man feedback check lint compile run disasm debug repl test doctor fmt init version help weights policy trace pkg benchmark repro-hash canonize-tensor canonize-file llama-run"
  if [[ ${cword} -eq 1 ]]; then
    COMPREPLY=( $(compgen -W "${commands}" -- "${cur}") )
    return
  fi
  case "${words[1]}" in
    code)
      COMPREPLY=( $(compgen -W "check lint fmt build run test disasm debug repl" -- "${cur}") )
      ;;
    project)
      COMPREPLY=( $(compgen -W "init" -- "${cur}") )
      ;;
    env)
      COMPREPLY=( $(compgen -W "doctor feedback" -- "${cur}") )
      ;;
    internal)
      COMPREPLY=( $(compgen -W "pkg benchmark repro-hash canonize-tensor canonize-file llama-run" -- "${cur}") )
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
local -a commands
commands=(
  'code:code workflow commands'
  'project:project lifecycle commands'
  'env:environment commands'
  'internal:internal commands'
  'completion:print completion script'
  'man:show or install man page'
  'feedback:local CLI feedback loop'
  'check:legacy alias'
  'compile:legacy alias'
  'run:legacy alias'
  'help:show help'
  'version:show version'
)
_arguments '1:command:->cmds' '*::arg:->args'
case $state in
  cmds) _describe -t commands 't81 commands' commands ;;
esac
)";
}

std::string build_fish_completion() {
  return R"(complete -c t81 -f -n '__fish_use_subcommand' -a 'code project env internal completion man feedback check lint compile run disasm debug repl test doctor fmt init version help weights policy trace pkg benchmark repro-hash canonize-tensor canonize-file llama-run'
complete -c t81 -f -n '__fish_seen_subcommand_from completion' -a 'bash zsh fish'
complete -c t81 -f -n '__fish_seen_subcommand_from code' -a 'check lint fmt build run test disasm debug repl'
complete -c t81 -f -n '__fish_seen_subcommand_from project' -a 'init'
complete -c t81 -f -n '__fish_seen_subcommand_from env' -a 'doctor feedback'
complete -c t81 -f -n '__fish_seen_subcommand_from internal' -a 'pkg benchmark repro-hash canonize-tensor canonize-file llama-run'
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
code
Check, build, run, test, format, debug, disassemble, repl.
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
    for (size_t i = first_tail + 1; i < args.command_args.size(); ++i) {
      const std::string& extra = args.command_args[i];
      if (!extra.empty() && extra[0] == '-') {
        error("Unknown option: " + extra);
      } else {
        error("Multiple input files not supported yet");
      }
      return 1;
    }
    args.input = input;
    args.command_args.clear();
    return -1;
  };

  if (args.command == "code") {
    if (args.command_args.size() == 1 &&
        (args.command_args[0] == "-V" || args.command_args[0] == "--version")) {
      print_version();
      return 0;
    }
    if (args.command_args.empty()) {
      print_help_code();
      return 1;
    }
    const std::string action = args.command_args[0];
    if (action == "build") {
      return remap_to_input_command("compile", 1);
    }
    if (action == "check" || action == "lint" || action == "run" || action == "disasm" ||
        action == "debug") {
      return remap_to_input_command(action, 1);
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

  if (args.command == "project") {
    if (args.command_args.size() == 1 &&
        (args.command_args[0] == "-V" || args.command_args[0] == "--version")) {
      print_version();
      return 0;
    }
    if (args.command_args.empty()) {
      print_help_project();
      return 1;
    }
    const std::string action = args.command_args[0];
    if (action == "init") {
      args.command = "init";
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
      print_help_env();
      return 1;
    }
    const std::string action = args.command_args[0];
    if (action == "doctor") {
      args.command = "doctor";
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
      print_help_internal();
      return 1;
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

std::optional<std::string> legacy_alias_recommendation(std::string_view command) {
  if (command == "compile") return "t81 code build";
  if (command == "check") return "t81 code check";
  if (command == "lint") return "t81 code lint";
  if (command == "run") return "t81 code run";
  if (command == "test") return "t81 code test";
  if (command == "fmt") return "t81 code fmt";
  if (command == "disasm") return "t81 code disasm";
  if (command == "debug") return "t81 code debug";
  if (command == "repl") return "t81 code repl";
  if (command == "init") return "t81 project init";
  if (command == "doctor") return "t81 env doctor";
  if (command == "pkg") return "t81 internal pkg";
  if (command == "benchmark") return "t81 internal benchmark";
  if (command == "repro-hash") return "t81 internal repro-hash";
  if (command == "canonize-tensor") return "t81 internal canonize-tensor";
  if (command == "canonize-file") return "t81 internal canonize-file";
  if (command == "llama-run") return "t81 internal llama-run";
  return std::nullopt;
}

// ──────────────────────────────────────────────────────────────
// Main
// ──────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
  try {
    auto args = parse_args(argc, argv);
    const std::string entered_command = args.command;
    const bool entered_domain_group =
        (entered_command == "code" || entered_command == "project" || entered_command == "env" ||
         entered_command == "internal");

    if (args.need_version) {
      print_version();
      return 0;
    }

    if (args.command == "help") {
      if (args.command_args.empty()) {
        print_usage(argv[0]);
        return 0;
      }
      if (args.command_args.size() >= 2) {
        const std::string& family = args.command_args[0];
        const std::string& sub = args.command_args[1];
        if (family == "weights" && sub == "import") {
          print_help_weights_import();
          return 0;
        }
        if (family == "weights" && sub == "info") {
          print_help_weights_info();
          return 0;
        }
        if (family == "weights" && sub == "quantize") {
          print_help_weights_quantize();
          return 0;
        }
        if (family == "policy" && sub == "compile") {
          print_help_policy_compile();
          return 0;
        }
        if (family == "policy" && sub == "run") {
          print_help_policy_run();
          return 0;
        }
        if (family == "trace" && sub == "show") {
          print_help_trace_show();
          return 0;
        }
        if (family == "trace" && sub == "diff") {
          print_help_trace_diff();
          return 0;
        }
        if (family == "trace" && sub == "replay") {
          print_help_trace_replay();
          return 0;
        }
        if (family == "trace" && sub == "export") {
          print_help_trace_export();
          return 0;
        }
        if (family == "code" && sub == "build") {
          print_help_compile();
          return 0;
        }
        if (family == "code" && sub == "check") {
          print_help_check();
          return 0;
        }
        if (family == "code" && sub == "lint") {
          print_help_check();
          return 0;
        }
        if (family == "code" && sub == "fmt") {
          print_help_fmt();
          return 0;
        }
        if (family == "code" && sub == "run") {
          print_help_run();
          return 0;
        }
        if (family == "code" && sub == "test") {
          print_help_test();
          return 0;
        }
        if (family == "code" && sub == "disasm") {
          print_help_disasm();
          return 0;
        }
        if (family == "code" && sub == "debug") {
          print_help_debug();
          return 0;
        }
        if (family == "code" && sub == "repl") {
          print_help_repl();
          return 0;
        }
        if (family == "project" && sub == "init") {
          print_help_init();
          return 0;
        }
        if (family == "env" && sub == "doctor") {
          print_help_doctor();
          return 0;
        }
        if (family == "env" && sub == "feedback") {
          print_help_feedback();
          return 0;
        }
        if (family == "internal" && sub == "pkg") {
          print_help_pkg();
          return 0;
        }
        if (family == "internal" && sub == "benchmark") {
          print_help_benchmark();
          return 0;
        }
        if (family == "internal" && sub == "repro-hash") {
          print_help_repro_hash();
          return 0;
        }
        if (family == "internal" && sub == "canonize-tensor") {
          print_help_canonize_tensor();
          return 0;
        }
        if (family == "internal" && sub == "canonize-file") {
          print_help_canonize_file();
          return 0;
        }
        if (family == "internal" && sub == "llama-run") {
          print_help_llama_run();
          return 0;
        }
      }
      std::string sub = args.command_args[0];
      if (print_help_topic(sub, argv[0])) {
        return 0;
      }
      error("Unknown help topic: " + sub + ". Run 't81 --help'.");
      return 1;
    }

    if (args.need_help) {
      if (args.command.empty()) {
        print_usage(argv[0]);
        return 0;
      }
      if ((args.command == "weights" || args.command == "policy" || args.command == "trace") &&
          !args.command_args.empty()) {
        const std::string& sub = args.command_args[0];
        if (args.command == "weights" && sub == "import") {
          print_help_weights_import();
          return 0;
        }
        if (args.command == "weights" && sub == "info") {
          print_help_weights_info();
          return 0;
        }
        if (args.command == "weights" && sub == "quantize") {
          print_help_weights_quantize();
          return 0;
        }
        if (args.command == "policy" && sub == "compile") {
          print_help_policy_compile();
          return 0;
        }
        if (args.command == "policy" && sub == "run") {
          print_help_policy_run();
          return 0;
        }
        if (args.command == "trace" && sub == "show") {
          print_help_trace_show();
          return 0;
        }
        if (args.command == "trace" && sub == "diff") {
          print_help_trace_diff();
          return 0;
        }
        if (args.command == "trace" && sub == "replay") {
          print_help_trace_replay();
          return 0;
        }
        if (args.command == "trace" && sub == "export") {
          print_help_trace_export();
          return 0;
        }
      }
      if ((args.command == "code" || args.command == "project" || args.command == "env" ||
           args.command == "internal") &&
          !args.command_args.empty()) {
        const std::string& sub = args.command_args[0];
        if (args.command == "code" && sub == "build") {
          print_help_compile();
          return 0;
        }
        if (args.command == "code" && (sub == "check" || sub == "lint")) {
          print_help_check();
          return 0;
        }
        if (args.command == "code" && sub == "fmt") {
          print_help_fmt();
          return 0;
        }
        if (args.command == "code" && sub == "run") {
          print_help_run();
          return 0;
        }
        if (args.command == "code" && sub == "test") {
          print_help_test();
          return 0;
        }
        if (args.command == "code" && sub == "disasm") {
          print_help_disasm();
          return 0;
        }
        if (args.command == "code" && sub == "debug") {
          print_help_debug();
          return 0;
        }
        if (args.command == "code" && sub == "repl") {
          print_help_repl();
          return 0;
        }
        if (args.command == "project" && sub == "init") {
          print_help_init();
          return 0;
        }
        if (args.command == "env" && sub == "doctor") {
          print_help_doctor();
          return 0;
        }
        if (args.command == "env" && sub == "feedback") {
          print_help_feedback();
          return 0;
        }
        if (args.command == "internal" && sub == "pkg") {
          print_help_pkg();
          return 0;
        }
        if (args.command == "internal" && sub == "benchmark") {
          print_help_benchmark();
          return 0;
        }
        if (args.command == "internal" && sub == "repro-hash") {
          print_help_repro_hash();
          return 0;
        }
        if (args.command == "internal" && sub == "canonize-tensor") {
          print_help_canonize_tensor();
          return 0;
        }
        if (args.command == "internal" && sub == "canonize-file") {
          print_help_canonize_file();
          return 0;
        }
        if (args.command == "internal" && sub == "memory-stats") {
          print_help_memory_stats();
          return 0;
        }
        if (args.command == "internal" && sub == "llama-run") {
          print_help_llama_run();
          return 0;
        }
      }
      if (print_help_topic(args.command, argv[0])) {
        return 0;
      }
      error("Unknown help topic: " + args.command + ". Run 't81 --help'.");
      return 1;
    }

    if (args.command == "version") {
      print_version();
      return 0;
    }

    {
      const int normalize_rc = normalize_domain_command(args);
      if (normalize_rc != -1) {
        return normalize_rc;
      }
    }

    if (!entered_domain_group && !entered_command.empty()) {
      auto recommendation = legacy_alias_recommendation(entered_command);
      if (recommendation && !g_flags.quiet) {
        std::cerr << "warning: '" << entered_command << "' is a legacy alias; prefer '"
                  << *recommendation << "'.\n";
      }
    }

    bool needs_input =
        (args.command == "compile" || args.command == "run" || args.command == "disasm" ||
         args.command == "debug" || args.command == "check" || args.command == "lint");
    if (args.command.empty() || (needs_input && args.input.empty())) {
      print_usage(argv[0]);
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
        return t81::cli::run_tisc(temp.path, args.policy, args.trace);
      } else if (ext == ".tisc") {
        return t81::cli::run_tisc(args.input, args.policy, args.trace);
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

    } else if (args.command == "benchmark") {
      return run_benchmark(argv[0], args);

    } else if (args.command == "repro-hash") {
      if (!args.command_args.empty() &&
          (args.command_args[0] == "-h" || args.command_args[0] == "--help")) {
        print_help_repro_hash();
        return 0;
      }
      return run_repro_hash(argv[0], args);

    } else if (args.command == "canonize-tensor") {
      if (!args.command_args.empty() &&
          (args.command_args[0] == "-h" || args.command_args[0] == "--help")) {
        print_help_canonize_tensor();
        return 0;
      }
      if (args.command_args.empty()) {
        error("canonize-tensor requires an input file. Run 't81 help canonize-tensor'.");
        return 1;
      }
      return t81::cli::canonize_tensor(args.command_args[0]);

    } else if (args.command == "canonize-file") {
      if (!args.command_args.empty() &&
          (args.command_args[0] == "-h" || args.command_args[0] == "--help")) {
        print_help_canonize_file();
        return 0;
      }
      return run_canonize_file(args);

    } else if (args.command == "memory-stats") {
      return run_memory_stats(args);

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
        print_help_init();
        return 0;
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
          print_help_pkg();
          return 0;
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

    } else if (args.command == "trace") {
      if (!args.command_args.empty() &&
          (args.command_args[0] == "-h" || args.command_args[0] == "--help")) {
        print_help_trace();
        return 0;
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
        if ((token == "-h" || token == "--help") && ta.subcommand == "export") {
          print_help_trace_export();
          return 0;
        }
        ta.args.push_back(token);
      }
      return t81::cli::run_trace(ta);

    } else if (args.command == "llama-run") {
      return run_llama_run(args);

    } else {
      error("Unknown command: " + args.command + ". Run 't81 --help' to list commands.");
      print_usage(argv[0]);
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
