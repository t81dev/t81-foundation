/**
 * @file main.cpp
 * @brief T81 Foundation Command-Line Interface (v1.0.0-SOVEREIGN)
 *
 * Sovereign-grade, zero-dependency, ternary-native toolchain driver.
 * MIT + GPL-3.0 dual-licensed.
 */

#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <optional>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#if !defined(_WIN32)
#include <sys/wait.h>
#endif

#include "t81/cli/driver.hpp"
#include "t81/cli/logging.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/tisc/binary_emitter.hpp"
#include "t81/tisc/binary_io.hpp"
#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

namespace fs = std::filesystem;

// ──────────────────────────────────────────────────────────────
// Version & Build Info
// ──────────────────────────────────────────────────────────────
#define STR(x) #x
#define XSTR(x) STR(x)

constexpr const char* T81_VERSION      = "1.0.0-SOVEREIGN";
constexpr const char* T81_BUILD_DATE   = __DATE__ " " __TIME__;
constexpr const char* T81_FULL_VERSION = "T81 Foundation 1.0.0-SOVEREIGN (" __DATE__ " " __TIME__ ")";
struct TempTiscFile {
    fs::path path;

    explicit TempTiscFile(const std::string& hint = "t81") {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dist;

        do {
            path = fs::temp_directory_path() /
                   ("t81-" + hint + "-" + std::to_string(dist(gen)) + ".tisc");
        } while (fs::exists(path));
    }

    ~TempTiscFile() {
        std::error_code ec;
        fs::remove(path, ec);  // best-effort cleanup
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

void print_usage(const char* prog) {
    std::cerr << R"(T81 Foundation - Ternary-Native Computing Stack
Version )" << T81_VERSION << R"(  


Usage: )" << prog << R"( <command> [options] [args]


Commands:
  compile <file.t81> [-o <file.tisc>]   Compile T81Lang → TISC bytecode
  run     <file.t81|.tisc>             Compile (if needed) and execute
  check   <file.t81>                   Syntax-check only
  repl                                 Enter interactive REPL (future)
  version                              Show version
  benchmark                            Run the core benchmark suite (build/benchmarks/benchmark_runner)
  weights import <file> [options]      Import BitNet/SafeTensors → .t81w
  weights info <model.t81w>            Print native model metadata
  weights quantize <dir|file> --to-gguf <out>  Quantize SafeTensors → T3_K GGUF
  help                                 Show this message


Global options:
  -v, --verbose                        Verbose diagnostic output
  -q, --quiet                          Suppress non-error output
  -h, --help                           Show help

Diagnostics:
  `t81 compile` now prints any semantic or parsing errors with the originating
  source file, line, and column so you can jump directly to the issue without
  rerunning separate diagnostics.

)";
}

// ──────────────────────────────────────────────────────────────
// VM Trap → Exit Code
// ──────────────────────────────────────────────────────────────
// ──────────────────────────────────────────────────────────────
// Core Commands
// ──────────────────────────────────────────────────────────────
// Implemented in src/cli/driver.cpp

std::string shell_escape(std::string_view arg) {
    if (arg.empty()) {
        return "''";
    }
    bool needs_quote = false;
    for (char c : arg) {
        if (std::isspace(static_cast<unsigned char>(c)) || c == '"' || c == '\'' || c == '\\' ||
            c == '$' || c == '&' || c == '|' || c == ';' || c == '<' || c == '>' || c == '*' ||
            c == '?' || c == '~' || c == '`' || c == '(' || c == ')' || c == '[' || c == ']' ||
            c == '{' || c == '}' ) {
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
int decode_system_status(int status) {
    return status;
}
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
    bool need_help    = false;
    bool need_version = false;
    std::vector<std::string> benchmark_args;
    std::vector<std::string> command_args;
};

Args parse_args(int argc, char* argv[]) {
    Args a;
    if (argc < 2) {
        a.need_help = true;
        return a;
    }

    std::string cmd = argv[1];
    if (cmd == "help" || cmd == "--help" || cmd == "-h") { a.need_help = true; return a; }
    if (cmd == "version" || cmd == "--version" || cmd == "-V") { a.need_version = true; return a; }

    a.command = cmd;

    for (int i = 2; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "-v" || arg == "--verbose")    g_flags.verbose = true;
        else if (arg == "-q" || arg == "--quiet") g_flags.quiet   = true;
        else if (arg == "-o" || arg == "--output") {
            if (++i >= argc) { error("Missing argument after -o"); std::exit(1); }
            a.output = fs::path(argv[i]);
        }
        else if (arg == "-h" || arg == "--help")   { a.need_help = true; }
        else if (arg.starts_with('-')) {
            error("Unknown option: " + std::string(arg));
            std::exit(1);
        }
        else {
            if (a.command == "benchmark") {
                a.benchmark_args.emplace_back(argv[i]);
            } else if (a.command == "weights") {
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

int run_benchmark(const char* command_name, const Args& args) {
    auto exe_path = fs::path(command_name);
    auto runner_path = find_benchmark_runner(exe_path);

    if (!runner_path) {
        error("Could not locate benchmark_runner (looked next to the CLI and under ./build/benchmarks)");
        return 1;
    }

    std::string cmd = runner_path->string();
    for (const auto& extra : args.benchmark_args) {
        cmd += ' ';
        cmd += shell_escape(extra);
    }

    info("Running benchmarks via " + runner_path->string());
    int status = std::system(cmd.c_str());
    if (status == -1) {
        error("Failed to execute benchmark_runner");
        return 1;
    }
    return decode_system_status(status);
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
        error("weights info requires a .t81w file path");
        return 1;
    }
    fs::path path = args.command_args[1];
    try {
        auto mf = t81::weights::load_t81w(path);
        std::cout << "Model:        " << path << "\n";
        std::cout << "Parameters:   " << t81::weights::format_count(mf.total_parameters) << "\n";
        std::cout << "Trits:        " << t81::weights::format_count(mf.total_trits) << " trits\n";
        std::cout << "Storage:      " << t81::weights::format_bytes(mf.file_size)
                  << " (" << std::fixed << std::setprecision(3) << mf.bits_per_trit
                  << " bits/trit avg)\n";
        std::cout << std::fixed << std::setprecision(1)
                  << "Sparsity:     " << (mf.sparsity * 100.0) << "% zeros\n";
        std::cout << std::defaultfloat;
        std::cout << "Format:       " << mf.format << "\n";
        std::cout << "Checksum:     sha3-512:" << mf.checksum << " (CanonFS-ready)\n";
    } catch (const std::exception& e) {
        error(e.what());
        return 1;
    }
    return 0;
}

int run_weights_quantize(const Args& args) {
    if (args.command_args.size() != 4 || args.command_args[2] != "--to-gguf") {
        error("weights quantize requires: quantize <input> --to-gguf <output>");
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
    if (args.command_args.empty()) {
        error("weights requires a subcommand (import|info)");
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
    error("weights: unknown subcommand '" + sub + "'");
    return 1;
}

// ──────────────────────────────────────────────────────────────
// Main
// ──────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    try {
        auto args = parse_args(argc, argv);

        if (args.need_help)    { print_usage(argv[0]); return 0; }
        if (args.need_version) { print_version();    return 0; }

        bool needs_input = (args.command == "compile" || args.command == "run" || args.command == "check");
        if (args.command.empty() || (needs_input && args.input.empty())) {
            print_usage(argv[0]);
            return 1;
        }

        const auto ext = args.input.extension();

        if (args.command == "compile") {
            if (ext != ".t81") {
                error("compile expects a .t81 source file");
                return 1;
            }
            fs::path out = args.output.value_or(args.input.stem().string() + ".tisc");
            return t81::cli::compile(args.input, out);

        } else if (args.command == "run") {
            if (ext == ".t81") {
                TempTiscFile temp(args.input.stem().string());
                int rc = t81::cli::compile(args.input, temp.path);
                if (rc != 0) return rc;
                return t81::cli::run_tisc(temp.path);
            } else if (ext == ".tisc") {
                return t81::cli::run_tisc(args.input);
            } else {
                error("run expects .t81 or .tisc file");
                return 1;
            }

        } else if (args.command == "check") {
            if (ext != ".t81") {
                error("check expects a .t81 source file");
                return 1;
            }
            return t81::cli::check_syntax(args.input);

        } else if (args.command == "benchmark") {
            return run_benchmark(argv[0], args);

        } else if (args.command == "weights") {
            return run_weights(args);

        } else {
            error("Unknown command: " + args.command);
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
