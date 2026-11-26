/**
 * @file main.cpp
 * @brief T81 Foundation Command-Line Interface
 *
 * Sovereign-grade, zero-dependency, ternary-native toolchain driver.
 * Licensed under Apache-2.0.
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
#include <map>
#include <memory>
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
#include "crypto/ed25519_bundle.hpp"
#include "tools/cli/core/cli_docs_gen.hpp"
#include "tools/cli/core/cmd_axion_canonfs.hpp"
#include "tools/cli/core/cmd_determinism.hpp"
#include "tools/cli/core/cmd_policy.hpp"
#include "tools/cli/core/cmd_vm_tisc_ir.hpp"
#include "tools/cli/core/cmd_weights_model.hpp"
#include "tools/cli/core/execution_policy.hpp"
#include "tools/cli/core/help_text.hpp"
#include "tools/cli/core/mcp_server.hpp"
#include "tools/cli/core/process_capture.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// clang-format off
#include <windows.h>
#include <shellapi.h>
// clang-format on
// Windows GDI's ERROR macro collides with monitoring::AlertLevel::ERROR.
#ifdef ERROR
#undef ERROR
#endif
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

#if !defined(_WIN32)
#include <sys/wait.h>
#include <unistd.h>
#else
#include <io.h>
#endif

#include "t81/axion/policy_validator.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/canonfs/interchange_ops.hpp"
#include "t81/canonfs/performance_cli.hpp"
#include "t81/canonfs/storage_key.hpp"
#include "t81/cli/artifact_family.hpp"
#include "t81/cli/driver.hpp"
#include "t81/config.hpp"
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
#include "t81/weights.hpp"
#include "tools/cli/ai/ai_cli_shared.hpp"
#include "tools/cli/logging.hpp"
#include "vm/internal/memory_segments.hpp"
#if defined(T81_HAS_LLAMA_CPP)
#include "t81/experimental/llama_cpp_adapter.hpp"
#endif
#if defined(T81_HAS_TUI)
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include "tools/tui/agent.hpp"
#include "tools/tui/common.hpp"
#include "tools/tui/studio.hpp"
#endif

namespace fs = std::filesystem;

// ──────────────────────────────────────────────────────────────
// Version & Build Info
// ──────────────────────────────────────────────────────────────
#define STR(x) #x
#define XSTR(x) STR(x)

[[maybe_unused]] constexpr const char* T81_VERSION = T81_VERSION_STR;
constexpr const char* T81_FULL_VERSION = "T81 Foundation " T81_VERSION_STR;
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
Copyright © 2026 T81 Foundation
Licensed under Apache-2.0
)";
}

// ScopedStreamRedirect and emit_help defined in tools/cli/core/cli_args.hpp
// Help text & topic dispatch extracted to tools/cli/core/help_text.{hpp,cpp}

std::string json_escape(std::string_view text);
std::string trap_explanation_text(t81::vm::Trap trap);
std::string render_trace_for_determinism(const t81::vm::State& state);
std::string sha3_512_text(std::string_view text);
std::string sha3_512_file(const fs::path& path);
std::optional<fs::path> find_repo_root(fs::path start);
fs::path discover_repo_root();
fs::path discover_build_dir();
fs::path discover_canonfs_root();
using t81::cli::ProcessCaptureResult;
using t81::cli::run_process_capture;
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
// struct Args defined in tools/cli/core/cli_args.hpp

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
          a.command == "man" || a.command == "feedback" || a.command == "c" ||
          a.command == "rust" || a.command == "python" || a.command == "llvm" ||
          a.command == "ai" || a.command == "artifact" || a.command == "mlir") {
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
                 a.command == "ai" || a.command == "artifact" || a.command == "project" ||
                 a.command == "env" || a.command == "internal" || a.command == "completion" ||
                 a.command == "man" || a.command == "feedback" || a.command == "canonize-tensor" ||
                 a.command == "canonize-file" || a.command == "memory-stats" ||
                 a.command == "tier" || a.command == "profile" || a.command == "c" ||
                 a.command == "rust" || a.command == "python" || a.command == "llvm" ||
                 a.command == "mlir" || a.command == "mcp" || a.command == "receipt") {
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
                 a.command == "ai" || a.command == "artifact" || a.command == "project" ||
                 a.command == "env" || a.command == "internal" || a.command == "completion" ||
                 a.command == "man" || a.command == "feedback" || a.command == "canonize-tensor" ||
                 a.command == "canonize-file" || a.command == "memory-stats" ||
                 a.command == "tier" || a.command == "c" || a.command == "rust" ||
                 a.command == "python" || a.command == "llvm" || a.command == "mlir" ||
                 a.command == "mcp" || a.command == "receipt") {
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
  std::string error_message;
  auto model = t81::cli::load_weights_model(path->string(), &error_message);
  if (!model) {
    error(error_message);
  }
  return model;
}

std::optional<std::string> extract_json_string_field(std::string_view text,
                                                     std::string_view field) {
  const std::string needle = "\"" + std::string(field) + "\": \"";
  const std::size_t start = text.find(needle);
  if (start == std::string_view::npos) {
    return std::nullopt;
  }
  const std::size_t value_start = start + needle.size();
  std::string value;
  bool escaped = false;
  for (std::size_t i = value_start; i < text.size(); ++i) {
    const char ch = text[i];
    if (escaped) {
      switch (ch) {
        case 'n':
          value.push_back('\n');
          break;
        case 'r':
          value.push_back('\r');
          break;
        case 't':
          value.push_back('\t');
          break;
        default:
          value.push_back(ch);
          break;
      }
      escaped = false;
      continue;
    }
    if (ch == '\\') {
      escaped = true;
      continue;
    }
    if (ch == '"') {
      return value;
    }
    value.push_back(ch);
  }
  return std::nullopt;
}

bool parse_canonical_hash_local(std::string_view prefixed_hash, t81::canonfs::CanonRef& ref,
                                std::string& error_message) {
  constexpr std::string_view prefix = "sha3-256:";
  if (!prefixed_hash.starts_with(prefix)) {
    error_message = "invalid canonical hash (expected sha3-256:...)";
    return false;
  }
  try {
    ref.hash.h =
        t81::hash::CanonHash81::from_string(std::string(prefixed_hash.substr(prefix.size())));
  } catch (const std::exception& e) {
    error_message = std::string("invalid canonical hash: ") + e.what();
    return false;
  }
  return true;
}

bool load_artifact_text_from_selector(const fs::path& canonfs_root, std::string_view selector,
                                      std::string& text, std::string& error_message) {
  if (selector.rfind("sha3-256:", 0) == 0) {
    t81::canonfs::CanonRef ref;
    if (!parse_canonical_hash_local(selector, ref, error_message)) {
      return false;
    }
    auto driver = t81::canonfs::make_persistent_driver(canonfs_root);
    auto read_res = driver->read_object_bytes(ref);
    if (!read_res) {
      error_message = "canonfs object not found or failed verification";
      return false;
    }
    text.clear();
    text.reserve(read_res->size());
    for (std::byte b : *read_res) {
      text.push_back(static_cast<char>(std::to_integer<unsigned char>(b)));
    }
    return true;
  }

  std::ifstream in(fs::absolute(fs::path(selector)), std::ios::binary);
  if (!in) {
    error_message = "could not read " + fs::absolute(fs::path(selector)).string();
    return false;
  }
  text.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  return true;
}

bool write_canonfs_raw_block_local(const fs::path& canonfs_root, std::string_view text,
                                   std::string& ref_text, std::string& error_message) {
  auto driver = t81::canonfs::make_persistent_driver(canonfs_root);
  const auto* begin = reinterpret_cast<const std::byte*>(text.data());
  auto write_res = driver->write_object(t81::canonfs::ObjectType::RawBlock,
                                        std::span<const std::byte>(begin, text.size()));
  if (!write_res) {
    error_message = "failed to write CanonFS object";
    return false;
  }
  ref_text = "sha3-256:" + write_res->hash.h.to_string();
  return true;
}

int run_artifact_command(const Args& args) {
  auto validate_supported_schema = [&](std::string_view subcommand_name,
                                       std::string_view schema_value) {
    if (!t81::cli::artifact_family::is_supported_schema(schema_value)) {
      error("artifact " + std::string(subcommand_name) + " " +
            t81::cli::artifact_family::supported_schema_error_suffix());
      return false;
    }
    return true;
  };

  auto write_and_store_payload = [&](const fs::path& canonfs_root_path,
                                     std::string_view summary_schema, std::string_view summary_text,
                                     std::string_view ref_key, std::string_view schema_value,
                                     std::string_view payload, std::string_view subcommand_name) {
    std::string validation_error;
    if (!t81::cli::artifact_family::validate_artifact(schema_value, payload, validation_error)) {
      error("artifact " + std::string(subcommand_name) + ": " + validation_error);
      return false;
    }
    std::string stored_ref;
    std::string store_error;
    if (!write_canonfs_raw_block_local(canonfs_root_path, payload, stored_ref, store_error)) {
      error("artifact " + std::string(subcommand_name) + ": " + store_error);
      return false;
    }
    std::cout << "{\n"
              << "  \"schema\": \"" << summary_schema << "\",\n"
              << "  \"result_summary\": \"" << summary_text << "\",\n"
              << "  \"schema_id\": \"" << schema_value << "\",\n"
              << "  \"validation_result\": \"pass\",\n"
              << "  \"" << ref_key << "\": \"" << stored_ref << "\",\n"
              << "  \"next_step_hint\": \"next (inspect): t81 artifact read-field " << stored_ref
              << " --schema " << schema_value << " --field "
              << t81::cli::artifact_family::next_inspect_field_for_schema(schema_value)
              << " --canonfs-root <path>\",\n"
              << "  \"status\": \"pass\"\n"
              << "}\n";
    return true;
  };

  auto parse_field_map = [&](std::string_view subcommand_name, std::string_view schema_value,
                             const std::vector<std::string>& assignments,
                             const auto& required_fields,
                             std::map<std::string, std::string>& field_map) {
    for (const std::string& assignment : assignments) {
      const std::size_t eq = assignment.find('=');
      if (eq == std::string::npos || eq == 0 || eq + 1 >= assignment.size()) {
        error("artifact " + std::string(subcommand_name) + ": fields must use key=value");
        return false;
      }
      const std::string key = assignment.substr(0, eq);
      const std::string value = assignment.substr(eq + 1);
      if (!t81::cli::artifact_family::is_supported_field(schema_value, key) || key == "schema") {
        error("artifact " + std::string(subcommand_name) + ": unknown field \"" + key + "\"");
        return false;
      }
      if (!field_map.emplace(key, value).second) {
        error("artifact " + std::string(subcommand_name) + ": duplicate field \"" + key + "\"");
        return false;
      }
    }
    for (std::string_view required_field : required_fields) {
      if (!field_map.contains(std::string(required_field))) {
        error("artifact " + std::string(subcommand_name) + ": missing required field \"" +
              std::string(required_field) + "\"");
        return false;
      }
    }
    return true;
  };

  if (!args.command_args.empty() &&
      (args.command_args[0] == "-h" || args.command_args[0] == "--help")) {
    return emit_help([&] { print_help_artifact(); });
  }
  if (args.command_args.empty()) {
    error("artifact requires a subcommand. Run 't81 help artifact'.");
    return 1;
  }

  const std::string subcommand = args.command_args[0];
  if (subcommand != "write-record" && subcommand != "write-store-record" &&
      subcommand != "store-bundle" && subcommand != "validate-record" &&
      subcommand != "store-record" && subcommand != "read-field") {
    error("Unknown artifact action: " + args.command_args[0] + ". Run 't81 help artifact'.");
    return 1;
  }

  std::string selector;
  std::string schema;
  std::string field;
  std::vector<std::string> field_assignments;
  fs::path canonfs_root = discover_canonfs_root();
  for (std::size_t i = 1; i < args.command_args.size(); ++i) {
    const std::string_view arg = args.command_args[i];
    if (arg == "--schema") {
      if (i + 1 >= args.command_args.size()) {
        error("artifact " + subcommand + ": missing value for --schema");
        return 1;
      }
      schema = args.command_args[++i];
    } else if (arg == "--field") {
      if (i + 1 >= args.command_args.size()) {
        error("artifact read-field: missing value for --field");
        return 1;
      }
      field = args.command_args[++i];
      if (subcommand == "write-record" || subcommand == "write-store-record" ||
          subcommand == "store-bundle") {
        field_assignments.push_back(field);
      }
    } else if (arg == "--file") {
      if (i + 1 >= args.command_args.size()) {
        error("artifact store-record: missing value for --file");
        return 1;
      }
      selector = args.command_args[++i];
    } else if (arg == "--canonfs-root") {
      if (i + 1 >= args.command_args.size()) {
        error("artifact " + subcommand + ": missing value for --canonfs-root");
        return 1;
      }
      canonfs_root = fs::absolute(fs::path(args.command_args[++i]));
    } else if (!arg.empty() && arg[0] == '-') {
      error("artifact " + subcommand + ": unknown option: " + std::string(arg));
      return 1;
    } else if (selector.empty()) {
      selector = std::string(arg);
    } else {
      error("artifact " + subcommand + " accepts exactly one file or CanonFS ref");
      return 1;
    }
  }

  if (subcommand != "write-record" && subcommand != "write-store-record" &&
      subcommand != "store-bundle" && selector.empty()) {
    error("artifact " + subcommand + " requires <file|sha3-256:hash>");
    return 1;
  }
  if (schema.empty()) {
    error("artifact " + subcommand + " requires --schema <schema-id>");
    return 1;
  }
  if (subcommand == "read-field" && field.empty()) {
    error("artifact read-field requires --field <name>");
    return 1;
  }
  if (subcommand == "read-field" && !t81::cli::artifact_family::is_supported_field(schema, field)) {
    error("artifact read-field supports only fixed downstream artifact fields");
    return 1;
  }
  if (subcommand == "write-record" && !args.output) {
    error("artifact write-record requires --out <file>");
    return 1;
  }
  if (subcommand == "store-record" && selector.empty()) {
    error("artifact store-record requires --file <path>");
    return 1;
  }

  if (subcommand == "write-record" || subcommand == "write-store-record") {
    if (!t81::cli::artifact_family::is_record_schema(schema)) {
      error("artifact " + subcommand + " supports only schema " +
            std::string(t81::cli::artifact_family::assess_record_schema()) + " or " +
            std::string(t81::cli::artifact_family::route_record_schema()) + " or " +
            std::string(t81::cli::artifact_family::classify_record_schema()));
      return 1;
    }
    std::map<std::string, std::string> field_map;
    if (!parse_field_map(subcommand, schema, field_assignments,
                         t81::cli::artifact_family::required_fields_for_schema(schema),
                         field_map)) {
      return 1;
    }
    const std::string payload = t81::cli::artifact_family::render_artifact(schema, field_map);
    if (subcommand == "write-store-record") {
      if (!write_and_store_payload(canonfs_root, "t81.artifact.record-write-store.v1",
                                   "validated downstream record written and stored in CanonFS",
                                   "record_ref", schema, payload, "write-store-record")) {
        return 1;
      }
      return 0;
    }
    std::ofstream out(*args.output, std::ios::binary | std::ios::trunc);
    if (!out) {
      error("artifact write-record: could not write " + args.output->string());
      return 1;
    }
    out << payload;
    if (!out.good()) {
      error("artifact write-record: could not write " + args.output->string());
      return 1;
    }
    std::cout << "{\n"
              << "  \"schema\": \"t81.artifact.record-write.v1\",\n"
              << "  \"ok\": true,\n"
              << "  \"record_schema\": \"" << schema << "\",\n"
              << "  \"out\": \"" << args.output->string() << "\",\n"
              << "  \"status\": \"pass\"\n"
              << "}\n";
    return 0;
  }

  if (subcommand == "store-bundle") {
    if (!t81::cli::artifact_family::is_bundle_schema(schema)) {
      error("artifact store-bundle supports only schema " +
            std::string(t81::cli::artifact_family::assess_bundle_schema()) + ", " +
            std::string(t81::cli::artifact_family::route_bundle_schema()) + ", or " +
            std::string(t81::cli::artifact_family::classify_bundle_schema()));
      return 1;
    }
    std::map<std::string, std::string> field_map;
    if (!parse_field_map(subcommand, schema, field_assignments,
                         t81::cli::artifact_family::required_fields_for_schema(schema),
                         field_map)) {
      return 1;
    }
    const std::string payload = t81::cli::artifact_family::render_artifact(schema, field_map);
    if (!write_and_store_payload(canonfs_root, "t81.artifact.bundle-store.v1",
                                 "validated downstream bundle stored in CanonFS", "bundle_ref",
                                 schema, payload, "store-bundle")) {
      return 1;
    }
    return 0;
  }

  std::string artifact_text;
  std::string load_error;
  if (!load_artifact_text_from_selector(canonfs_root, selector, artifact_text, load_error)) {
    error("artifact " + subcommand + ": " + load_error);
    return 1;
  }

  std::string validation_error;
  if (!t81::cli::artifact_family::validate_artifact(schema, artifact_text, validation_error)) {
    error("artifact " + subcommand + ": " + validation_error);
    return 1;
  }

  if (subcommand == "store-record") {
    if (!validate_supported_schema("store-record", schema)) {
      return 1;
    }
    if (!write_and_store_payload(canonfs_root, "t81.artifact.record-store.v1",
                                 "validated downstream record stored in CanonFS", "record_ref",
                                 schema, artifact_text, "store-record")) {
      return 1;
    }
    return 0;
  }

  if (subcommand == "read-field") {
    const auto value = extract_json_string_field(artifact_text, field);
    if (!value) {
      error("artifact read-field: field \"" + field +
            "\" not present in validated downstream record");
      return 1;
    }
    std::cout << *value << "\n";
    return 0;
  }

  std::cout << "{\n"
            << "  \"schema\": \"t81.artifact.record-validation.v1\",\n"
            << "  \"ok\": true,\n"
            << "  \"record_schema\": \"" << schema << "\",\n"
            << "  \"selector_kind\": \""
            << (selector.rfind("sha3-256:", 0) == 0 ? "canonfs_ref" : "file") << "\",\n"
            << "  \"status\": \"pass\"\n"
            << "}\n";
  return 0;
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

// Weights and model CLI commands extracted to tools/cli/core/cmd_weights_model.{hpp,cpp}.

// Policy, Axion, CanonFS, determinism, tensor, VM, profile, tier, TISC, and IR CLI commands
// extracted to tools/cli/core/cmd_*.{hpp,cpp}.

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

  std::vector<std::string> argv = {"python3",         script_path->string(),  "--t81-bin",
                                   exe_path.string(), "--fixtures-dir",       fixtures_arg,
                                   "--workdir",       workdir.string(),       "--hash-out",
                                   hash_out.string(), "--expected-hash-file", expected_arg};

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
    case Opcode::ADD:
      return "ADD";
    case Opcode::SUB:
      return "SUB";
    case Opcode::MUL:
      return "MUL";
    case Opcode::DIV:
      return "DIV";
    case Opcode::MOD:
      return "MOD";
    case Opcode::FADD:
      return "FADD";
    case Opcode::FSUB:
      return "FSUB";
    case Opcode::FMUL:
      return "FMUL";
    case Opcode::FDIV:
      return "FDIV";
    case Opcode::FRACADD:
      return "FRACADD";
    case Opcode::FRACSUB:
      return "FRACSUB";
    case Opcode::FRACMUL:
      return "FRACMUL";
    case Opcode::FRACDIV:
      return "FRACDIV";
    case Opcode::NEG:
      return "NEG";
    case Opcode::CMP:
      return "CMP";
    case Opcode::MOV:
      return "MOV";
    case Opcode::LOADI:
      return "LOADI";
    case Opcode::LOAD:
      return "LOAD";
    case Opcode::STORE:
      return "STORE";
    case Opcode::PUSH:
      return "PUSH";
    case Opcode::POP:
      return "POP";
    case Opcode::JMP:
      return "JMP";
    case Opcode::JZ:
      return "JZ";
    case Opcode::JNZ:
      return "JNZ";
    case Opcode::JN:
      return "JN";
    case Opcode::JP:
      return "JP";
    case Opcode::CALL:
      return "CALL";
    case Opcode::RET:
      return "RET";
    case Opcode::I2F:
      return "I2F";
    case Opcode::F2I:
      return "F2I";
    case Opcode::I2FRAC:
      return "I2FRAC";
    case Opcode::FRAC2I:
      return "FRAC2I";
    case Opcode::MAKE_OPTION_SOME:
      return "MAKE_OPTION_SOME";
    case Opcode::MAKE_OPTION_NONE:
      return "MAKE_OPTION_NONE";
    case Opcode::MAKE_RESULT_OK:
      return "MAKE_RESULT_OK";
    case Opcode::MAKE_RESULT_ERR:
      return "MAKE_RESULT_ERR";
    case Opcode::OPTION_IS_SOME:
      return "OPTION_IS_SOME";
    case Opcode::OPTION_UNWRAP:
      return "OPTION_UNWRAP";
    case Opcode::RESULT_IS_OK:
      return "RESULT_IS_OK";
    case Opcode::RESULT_UNWRAP_OK:
      return "RESULT_UNWRAP_OK";
    case Opcode::RESULT_UNWRAP_ERR:
      return "RESULT_UNWRAP_ERR";
    case Opcode::PRINT:
      return "PRINT";
    case Opcode::STRLEN:
      return "STRLEN";
    case Opcode::STREMPTY:
      return "STREMPTY";
    case Opcode::VECLEN:
      return "VECLEN";
    case Opcode::VECEMPTY:
      return "VECEMPTY";
    case Opcode::VECFIRST:
      return "VECFIRST";
    case Opcode::VECLAST:
      return "VECLAST";
    case Opcode::VECPUSH:
      return "VECPUSH";
    case Opcode::VECPOP:
      return "VECPOP";
    case Opcode::STRCONCAT:
      return "STRCONCAT";
    case Opcode::STRSTARTSWITH:
      return "STRSTARTSWITH";
    case Opcode::STRENDSWITH:
      return "STRENDSWITH";
    case Opcode::STRCONTAINS:
      return "STRCONTAINS";
    case Opcode::STRINDEXOF:
      return "STRINDEXOF";
    case Opcode::STRREPLACE:
      return "STRREPLACE";
    case Opcode::STRVECNEW:
      return "STRVECNEW";
    case Opcode::STRVECPUSH:
      return "STRVECPUSH";
    case Opcode::STRSPLIT:
      return "STRSPLIT";
    case Opcode::STRJOIN:
      return "STRJOIN";
    case Opcode::WEIGHTS_LOAD:
      return "WEIGHTS_LOAD";
    case Opcode::TTRANSPOSE:
      return "TTRANSPOSE";
    case Opcode::TGET:
      return "TGET";
    case Opcode::TNEW:
      return "TNEW";
    case Opcode::TSET:
      return "TSET";
    case Opcode::TSHAPE:
      return "TSHAPE";
    case Opcode::BITAND:
      return "BITAND";
    case Opcode::BITOR:
      return "BITOR";
    case Opcode::BITXOR:
      return "BITXOR";
    case Opcode::BITNOT:
      return "BITNOT";
    case Opcode::BITSHL:
      return "BITSHL";
    case Opcode::BITSHR:
      return "BITSHR";
    case Opcode::BITUSHR:
      return "BITUSHR";
    case Opcode::MAKE_COMPLEX:
      return "MAKE_COMPLEX";
    case Opcode::NOP:
      return "NOP";
    case Opcode::HALT:
      return "HALT";
    case Opcode::TRAP:
      return "TRAP";
    case Opcode::LABEL:
      return "LABEL";
    default:
      return "UNKNOWN";
  }
}

const char* ir_primitive_name(t81::tisc::ir::PrimitiveKind kind) {
  using PrimitiveKind = t81::tisc::ir::PrimitiveKind;
  switch (kind) {
    case PrimitiveKind::Integer:
      return "Int";
    case PrimitiveKind::Float:
      return "Float";
    case PrimitiveKind::Fraction:
      return "Frac";
    case PrimitiveKind::Boolean:
      return "Bool";
    default:
      return "Unknown";
  }
}

const char* ir_relation_name(t81::tisc::ir::ComparisonRelation relation) {
  using Relation = t81::tisc::ir::ComparisonRelation;
  switch (relation) {
    case Relation::Less:
      return "Less";
    case Relation::LessEqual:
      return "LessEqual";
    case Relation::Greater:
      return "Greater";
    case Relation::GreaterEqual:
      return "GreaterEqual";
    case Relation::Equal:
      return "Equal";
    case Relation::NotEqual:
      return "NotEqual";
    default:
      return "None";
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
    case Trap::ActivationFault:
      return "The VM could not activate a required component. Ensure dependencies are satisfied.";
    case Trap::FFINotInitialized:
      return "The VM FFI dispatcher was not initialized.";
    case Trap::FFIRegistrationError:
      return "An error occurred registering an FFI library or function.";
    case Trap::FFILoadError:
      return "An error occurred loading an FFI library.";
    case Trap::FFIPolicyDenied:
      return "An FFI action was denied by policy.";
    case Trap::FFITimeout:
      return "An FFI action timed out.";
    case Trap::FFIMemoryExhausted:
      return "An FFI action exhausted available memory.";
    case Trap::DecodeFault:
      return "The VM could not decode the TISC artifact. Rebuild the program and validate it with "
             "'t81 tisc validate'.";
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
      return "The program hit an opcode or runtime feature that is not implemented in this VM "
             "build.";
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
  std::vector<std::uint8_t> raw((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
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

std::optional<fs::path> discover_exe_path_internal() {
  std::error_code ec;
#ifdef _WIN32
  char buf[MAX_PATH];
  if (GetModuleFileNameA(NULL, buf, MAX_PATH)) {
    return fs::path(buf);
  }
#elif defined(__APPLE__)
  char buf[1024];
  uint32_t size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) == 0) {
    return fs::canonical(fs::path(buf), ec);
  }
#else
  char buf[1024];
  ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (len != -1) {
    buf[len] = '\0';
    return fs::path(buf);
  }
#endif
  return std::nullopt;
}

fs::path discover_build_dir() {
  std::error_code ec;
  const fs::path cwd = fs::current_path();
  const fs::path repo_root = discover_repo_root();

  // 1. Check if CWD is a build directory (but not the repo root — guards against
  //    stale in-source CMake artifacts left behind by accidental root-level configure).
  if (cwd != repo_root && fs::exists(cwd / "CTestTestfile.cmake", ec)) {
    return cwd;
  }
  ec.clear();

  // 2. Check executable's directory or its parent
  if (auto exe_path = discover_exe_path_internal()) {
    fs::path exe_dir = exe_path->parent_path();
    if (fs::exists(exe_dir / "CTestTestfile.cmake", ec)) {
      return exe_dir;
    }
    ec.clear();
    if (fs::exists(exe_dir.parent_path() / "CTestTestfile.cmake", ec)) {
      return exe_dir.parent_path();
    }
    ec.clear();
  }

  // 3. Fallback to repo root / build
  const fs::path repo_build_dir = repo_root / "build";
  if (fs::exists(repo_build_dir / "CTestTestfile.cmake", ec)) {
    return repo_build_dir;
  }
  ec.clear();

  return repo_build_dir;
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
      {"cmake", "cmake", false, std::nullopt},     {"ctest", "ctest", false, std::nullopt},
      {"python3", "python3", false, std::nullopt}, {"clang++", "clang++", false, std::nullopt},
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
              << "  \"ok\": "
              << ((probes[0].available && probes[1].available && probes[2].available &&
                   any_compiler)
                      ? "true"
                      : "false")
              << ",\n"
              << "  \"tools\": [\n";
    for (std::size_t i = 0; i < probes.size(); ++i) {
      const auto& probe = probes[i];
      std::cout << "    {\"name\":\"" << json_escape(probe.name)
                << "\",\"available\":" << (probe.available ? "true" : "false") << ",\"version\":";
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
  std::cout << "build metadata:        " << (build_metadata_present ? "present" : "missing")
            << "\n";
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
      abs_build_dir / "CMakeCache.txt",      abs_build_dir / "CMakeFiles",
      abs_build_dir / "CTestTestfile.cmake", abs_build_dir / "Testing",
      abs_build_dir / "build.ninja",         abs_build_dir / ".ninja_deps",
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
    const bool objects_ok =
        !root_exists || !fs::exists(objects_dir) || fs::is_directory(objects_dir);
    push_check("canonfs_objects_dir", objects_ok,
               fs::exists(objects_dir) ? (objects_ok ? "objects directory readable"
                                                     : "objects path exists but is not a directory")
                                       : "objects directory not present yet",
               "Ensure CanonFS object storage is intact under <root>/objects.");
    const fs::path snapshots_dir = canonfs_root / "snapshots";
    const bool snapshots_ok =
        !root_exists || !fs::exists(snapshots_dir) || fs::is_directory(snapshots_dir);
    push_check("canonfs_snapshots_dir", snapshots_ok,
               fs::exists(snapshots_dir)
                   ? (snapshots_ok ? "snapshots directory readable"
                                   : "snapshots path exists but is not a directory")
                   : "snapshots directory not present yet",
               "Ensure CanonFS snapshot storage is intact under <root>/snapshots.");
  };

  auto run_vm_checks = [&]() {
    const bool build_metadata_present = fs::exists(build_dir / "CTestTestfile.cmake");
    push_check("build_dir_present", build_metadata_present,
               build_metadata_present
                   ? ("CTest metadata found in " + build_dir.string())
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
      std::cout << "    {\"id\":\"" << json_escape(check.id)
                << "\",\"ok\":" << (check.ok ? "true" : "false") << ",\"detail\":\""
                << json_escape(check.detail) << "\",\"remediation\":\""
                << json_escape(check.remediation) << "\"}";
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
  fs::path build_dir = discover_build_dir();
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
        (lowered.find("unknown") != std::string::npos ||
         lowered.find("unrecognized") != std::string::npos ||
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
  auto parse_summary = [list_only](const std::string& text) -> TestSummary {
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
      summary.passed = list_only ? 0 : summary.total;
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
  int not_run = 0;
  if (used_junit && fs::exists(junit_file.path)) {
    std::ifstream junit_in(junit_file.path, std::ios::binary);
    const std::string xml((std::istreambuf_iterator<char>(junit_in)),
                          std::istreambuf_iterator<char>());
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
    int junit_errors = 0;
    if (std::regex_search(xml, m, std::regex("errors=\"([0-9]+)\"")) && m.size() == 2) {
      junit_errors = std::atoi(m[1].str().c_str());
    }
    if (summary.total >= 0 && summary.failed >= 0) {
      summary.failed += junit_errors;
      summary.passed = std::max(0, summary.total - summary.failed - std::max(0, summary.skipped));
    }
    if (std::regex_search(xml, m, std::regex("time=\"([0-9]+(?:\\.[0-9]+)?)\"")) && m.size() == 2) {
      summary.duration_sec = std::atof(m[1].str().c_str());
    }
  }
  if (list_only && summary.total >= 0) {
    not_run = summary.total;
  }
  const bool infrastructure_failure =
      rc != 0 && (summary.total < 0 || ((summary.failed <= 0) &&
                                        (summary.passed >= summary.total || summary.passed > 0)));
  if (infrastructure_failure) {
    if (summary.total < 0) {
      summary.total = 0;
    }
    not_run = std::max(not_run, summary.total);
    summary.passed = 0;
    summary.failed = std::max(summary.failed, 0);
    summary.skipped = std::max(summary.skipped, 0);
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
    std::cout << "    \"not_run\": " << not_run << ",\n";
    std::cout << "    \"duration_sec\": " << std::fixed << std::setprecision(3)
              << summary.duration_sec << "\n";
    std::cout << std::defaultfloat;
    std::cout << "  },\n";
    std::cout << "  \"stdout_bytes\": " << stdout_bytes << ",\n";
    std::cout << "  \"stderr_bytes\": " << stderr_bytes << ",\n";
    std::cout << "  \"summary_source\": \"" << (used_junit ? "junit_xml" : "ctest_text") << "\",\n";
    std::cout << "  \"infrastructure_failure\": " << (infrastructure_failure ? "true" : "false")
              << "\n";
    std::cout << "}\n";
    return rc == 0 ? 0 : 2;
  }

  std::cout << stdout_text;
  std::cerr << stderr_text;
  if (!g_flags.quiet) {
    std::ostringstream oss;
    oss << "test summary: total=" << summary.total << " passed=" << summary.passed
        << " failed=" << summary.failed << " skipped=" << summary.skipped << " not_run=" << not_run;
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

bool parse_package_field(const std::string& text, const std::string& field,
                         std::string& value_out) {
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

int run_completion_command(const Args& args) {
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
    print_help_completion();
    return args.command_args.empty() ? 1 : 0;
  }
  const std::string shell = args.command_args[0];
  if (shell == "bash") {
    std::cout << t81::cli::build_bash_completion();
    return 0;
  }
  if (shell == "zsh") {
    std::cout << t81::cli::build_zsh_completion();
    return 0;
  }
  if (shell == "fish") {
    std::cout << t81::cli::build_fish_completion();
    return 0;
  }
  error("completion: unsupported shell '" + shell + "'. Use bash|zsh|fish.");
  return 1;
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
  const std::string man_text = t81::cli::build_manpage_text();
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
  if (args.command_args.empty() || args.command_args[0] == "-h" ||
      args.command_args[0] == "--help") {
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
  fs::path canonfs_root = discover_canonfs_root();

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
  } else if (!fs::exists(model_path)) {
    std::string resolution_error;
    auto resolved = t81::cli::resolve_repo_model_path(positional[0], {".gguf"}, &resolution_error);
    if (!resolved) {
      error("llama-run: " + resolution_error);
      return 1;
    }
    model_path = *resolved;
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
  if (topic == "bench") return "t81 internal benchmark";
  return std::nullopt;
}

#ifdef _WIN32
static bool collect_windows_utf8_argv(std::vector<std::string>& storage, std::vector<char*>& ptrs,
                                      std::string& error_message) {
  int wargc = 0;
  LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
  if (wargv == nullptr || wargc <= 0) {
    if (wargv != nullptr) {
      LocalFree(wargv);
    }
    error_message = "CommandLineToArgvW failed";
    return false;
  }

  storage.clear();
  ptrs.clear();
  storage.reserve(static_cast<std::size_t>(wargc));

  for (int i = 0; i < wargc; ++i) {
    if (wargv[i] == nullptr || wargv[i][0] == L'\0') {
      storage.emplace_back();
      continue;
    }
    const int needed = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wargv[i], -1, nullptr, 0,
                                           nullptr, nullptr);
    if (needed <= 0) {
      LocalFree(wargv);
      error_message = "Failed to convert Windows command-line argument to UTF-8";
      return false;
    }
    std::string utf8(static_cast<std::size_t>(needed - 1), '\0');
    const int written = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wargv[i], -1,
                                            utf8.data(), needed, nullptr, nullptr);
    if (written <= 0) {
      LocalFree(wargv);
      error_message = "Failed to encode Windows command-line argument as UTF-8";
      return false;
    }
    storage.push_back(std::move(utf8));
  }

  LocalFree(wargv);
  ptrs.reserve(storage.size() + 1);
  for (auto& arg : storage) {
    ptrs.push_back(arg.data());
  }
  ptrs.push_back(nullptr);
  return true;
}
#endif

// ──────────────────────────────────────────────────────────────
// Main
// ──────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
#ifdef _WIN32
  std::vector<std::string> win_utf8_args;
  std::vector<char*> win_utf8_argv;
  std::string win_argv_error;
  if (!collect_windows_utf8_argv(win_utf8_args, win_utf8_argv, win_argv_error)) {
    std::cerr << "Error: " << win_argv_error << "\n";
    return 1;
  }
  argc = static_cast<int>(win_utf8_args.size());
  argv = win_utf8_argv.data();
#endif
  g_json_error_mode = false;
  g_json_error_emitted = false;
  for (int i = 1; i < argc; ++i) {
    if (std::string_view(argv[i]) == "--json") {
      g_json_error_mode = true;
      break;
    }
  }
  try {
    std::string entered_command;
    if (argc > 1) {
      entered_command = argv[1];
    }
    auto args = parse_args(argc, argv);

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
          error("Help topic '" + args.command_args[0] + "' has been removed. Use '" + *removed +
                "'.");
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
    if (args.weights_model && (args.command == "compile" || args.command == "run" ||
                               args.command == "debug" || args.command == "repl")) {
      weights_model_ptr = load_weights_model_optional(args.weights_model);
      if (!weights_model_ptr) return 1;
    }

    if (args.command == "compile") {
      if (args.policy) {
        error("compile does not support --policy yet; pass policy at runtime with 't81 code run "
              "--policy'.");
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
        fs::path tisc_path;
        std::optional<TempTiscFile> temp;

        bool project_mode = (entered_command == "project");
        bool skip_compile = false;

        if (project_mode && args.input == "main.t81") {
          fs::path local_tisc = "main.tisc";
          if (fs::exists(local_tisc)) {
            auto t81_time = fs::last_write_time(args.input);
            auto tisc_time = fs::last_write_time(local_tisc);
            if (tisc_time >= t81_time) {
              tisc_path = local_tisc;
              skip_compile = true;
            }
          }
        }

        if (!skip_compile) {
          temp.emplace(args.input.stem().string());
          int rc = t81::cli::compile(args.input, temp->path, {}, {}, weights_model_ptr, false);
          if (rc != 0) return rc;
          tisc_path = temp->path;
          std::cerr << "Compilation successful → " << tisc_path.string() << "\n";
        }

        return t81::cli::run_tisc(tisc_path, args.policy, args.trace, args.trace_output,
                                  weights_model_ptr);
      } else if (ext == ".tisc") {
        return t81::cli::run_tisc(args.input, args.policy, args.trace, args.trace_output,
                                  weights_model_ptr);
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
        return t81::cli::debug_tisc(temp.path, args.policy, weights_model_ptr);
      } else if (ext == ".tisc") {
        return t81::cli::debug_tisc(args.input, args.policy, weights_model_ptr);
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

    } else if (args.command == "mcp") {
      if (!args.command_args.empty() && args.command_args[0] == "help") {
        return emit_help([&] { print_help_mcp(); });
      }
      return t81::cli::run_mcp_serve(args.command_args, [](const std::string& croot) -> int {
        Args fsck_args;
        fsck_args.command = "canonfs";
        fsck_args.command_args = {"fsck", "--verify-hashes", "--json", "--canonfs-root", croot};
        return run_canonfs_command(fsck_args);
      });

    } else if (args.command == "receipt") {
      if (args.command_args.empty() || args.command_args[0] != "export") {
        std::cerr << "Usage: t81 receipt export --receipt <hex_ref> --out <dir> "
                     "[--ledger <dir>] [--json]\n";
        return 1;
      }
      std::string receipt_hex;
      fs::path ledger_dir = ".t81_receipt_ledger";
      fs::path out_dir = args.output.value_or(fs::path{});
      bool json_out = false;
      for (size_t i = 1; i < args.command_args.size(); ++i) {
        const std::string& tok = args.command_args[i];
        if (tok == "--receipt" && i + 1 < args.command_args.size()) {
          receipt_hex = args.command_args[++i];
        } else if (tok == "--ledger" && i + 1 < args.command_args.size()) {
          ledger_dir = args.command_args[++i];
        } else if (tok == "--out" && i + 1 < args.command_args.size()) {
          out_dir = args.command_args[++i];
        } else if (tok == "--json") {
          json_out = true;
        } else {
          std::cerr << "t81 receipt export: unknown argument '" << tok << "'\n";
          return 1;
        }
      }
      if (receipt_hex.size() != 64 || out_dir.empty()) {
        std::cerr << "t81 receipt export requires 64-hex --receipt and --out <dir>\n";
        return 1;
      }
      t81::canonfs::CanonRef ref{};
      for (size_t i = 0; i < 32; ++i) {
        auto nib = [](char c) -> int {
          if (c >= '0' && c <= '9') return c - '0';
          if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
          if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
          return -1;
        };
        int hi = nib(receipt_hex[2 * i]);
        int lo = nib(receipt_hex[2 * i + 1]);
        if (hi < 0 || lo < 0) {
          std::cerr << "t81 receipt export: invalid hex in --receipt\n";
          return 1;
        }
        ref.hash.h.bytes[i] = static_cast<uint8_t>((hi << 4) | lo);
      }
      auto drv = t81::canonfs::make_persistent_driver(ledger_dir);
      auto obj = drv->read_object_bytes(ref);
      if (!obj.has_value()) {
        std::cerr << "t81 receipt export: receipt not found in CanonFS ledger\n";
        return 1;
      }
      std::string raw(reinterpret_cast<const char*>(obj->data()), obj->size());
      const std::string raw_sha =
          "sha3-512:" + t81::crypto::sha3_512_hex(std::span<const uint8_t>(
                            reinterpret_cast<const uint8_t*>(raw.data()), raw.size()));
      auto extract_str_field = [&](const std::string& key) -> std::string {
        const std::string pat = "\"" + key + "\":\"";
        auto p = raw.find(pat);
        if (p == std::string::npos) return {};
        p += pat.size();
        std::string out;
        for (size_t i = p; i < raw.size(); ++i) {
          if (raw[i] == '\\' && i + 1 < raw.size()) {
            char nx = raw[++i];
            if (nx == 'n')
              out.push_back('\n');
            else if (nx == 'r')
              out.push_back('\r');
            else if (nx == 't')
              out.push_back('\t');
            else
              out.push_back(nx);
          } else if (raw[i] == '"') {
            break;
          } else {
            out.push_back(raw[i]);
          }
        }
        return out;
      };
      const std::string sig_b64 = extract_str_field("ed25519_signature_b64");
      const std::string pub_pem = extract_str_field("public_key_pem");
      std::string payload_json;
      {
        const std::string pat = "\"payload\":";
        auto p = raw.find(pat);
        auto end_p = raw.find(",\"ed25519_signature_b64\":");
        if (p != std::string::npos && end_p != std::string::npos && end_p > p + pat.size()) {
          payload_json = raw.substr(p + pat.size(), end_p - (p + pat.size()));
        }
      }
      bool ed25519_ok = false;
      if (!pub_pem.empty() && !sig_b64.empty() && !payload_json.empty()) {
        auto pub = t81::crypto::import_public_key_pem(pub_pem);
        if (pub.has_value()) {
          ed25519_ok = t81::crypto::verify_rfc00d4_payload_b64(*pub, payload_json, sig_b64);
        }
      }
      if (!ed25519_ok) {
        std::cerr << "t81 receipt export: fail-closed Ed25519 verification failed\n";
        return 1;
      }
      std::error_code ec;
      fs::create_directories(out_dir, ec);
      auto atomic_write = [&](const fs::path& dst, const std::string& content) -> bool {
        fs::path tmp = dst;
        tmp += ".tmp";
        {
          std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
          if (!ofs) return false;
          ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
          ofs.flush();
          if (!ofs) return false;
        }
        std::error_code rec;
        fs::rename(tmp, dst, rec);
        if (rec) {
          fs::remove(tmp, rec);
          return false;
        }
        return true;
      };
      const std::string bundle_json =
          "{\"schema\":\"t81.forensic-receipt-bundle.v1\",\"receipt_canonfs_ref\":\"" +
          receipt_hex + "\",\"raw_sha3_512\":\"" + raw_sha +
          "\",\"ed25519_verified\":true,\"receipt\":" + raw + "}\n";
      const std::string bundle_sha =
          "sha3-512:" +
          t81::crypto::sha3_512_hex(std::span<const uint8_t>(
              reinterpret_cast<const uint8_t*>(bundle_json.data()), bundle_json.size()));
      if (!atomic_write(out_dir / "receipt_raw.json", raw) ||
          !atomic_write(out_dir / "bundle.json", bundle_json) ||
          !atomic_write(out_dir / "manifest.sha3-512",
                        bundle_sha + "  bundle.json\n" + raw_sha + "  receipt_raw.json\n")) {
        std::cerr << "t81 receipt export: atomic write failed\n";
        return 1;
      }
      if (json_out) {
        std::cout << "{\"ok\":true,\"receipt_canonfs_ref\":\"" << receipt_hex
                  << "\",\"ed25519_verified\":true,\"bundle_sha3_512\":\"" << bundle_sha
                  << "\",\"out_dir\":\"" << out_dir.string() << "\"}\n";
      } else {
        std::cout << "Exported verified RFC-00D4 forensic bundle to " << out_dir.string() << " ("
                  << bundle_sha << ")\n";
      }
      return 0;

    } else if (args.command == "artifact") {
      return run_artifact_command(args);

    } else if (args.command == "determinism") {
      return run_determinism_command(argv[0], args);

    } else if (args.command == "tensor") {
      return run_tensor_command(argv[0], args);

    } else if (args.command == "ai") {
      std::vector<std::string_view> ai_args;
      ai_args.reserve(args.command_args.size() + (args.output ? 2U : 0U) + (args.policy ? 2U : 0U));
      for (const auto& token : args.command_args) {
        ai_args.emplace_back(token);
      }
      std::string ai_output_flag;
      std::string ai_output_value;
      std::string ai_policy_flag;
      std::string ai_policy_value;
      if (args.output) {
        const std::string subcommand = args.command_args.empty() ? "" : args.command_args.front();
        ai_output_flag = (subcommand == "quantize") ? "--output" : "--out";
        ai_output_value = args.output->string();
        ai_args.emplace_back(ai_output_flag);
        ai_args.emplace_back(ai_output_value);
      }
      if (args.policy) {
        ai_policy_flag = "--policy";
        ai_policy_value = args.policy->string();
        ai_args.emplace_back(ai_policy_flag);
        ai_args.emplace_back(ai_policy_value);
      }
      return t81::cli::ai::run("t81 ai", ai_args);

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

    } else if (args.command == "model") {
      return run_model(args);

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
          if (la.input.empty())
            la.input = tok;
          else
            la.output = tok;
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
          if (ca.input.empty())
            ca.input = tok;
          else
            ca.output = tok;
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
          if (ra.input.empty())
            ra.input = tok;
          else
            ra.output = tok;
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
          if (pa.input.empty())
            pa.input = tok;
          else
            pa.output = tok;
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
          if (ma.input.empty())
            ma.input = tok;
          else
            ma.output = tok;
        } else {
          error("mlir: unknown option '" + tok + "'. Run 't81 help mlir'.");
          return 1;
        }
      }
      return t81::cli::run_mlir(ma);

      // ── TUI frontends ─────────────────────────────────────────────────────
    } else if (args.command == "studio" || args.command == "agent" || args.command == "ui") {
#if defined(T81_HAS_TUI)
      {
        std::error_code ec;
        fs::path cli_path = argc > 0 ? fs::path(argv[0]) : fs::path("t81");
        if (!cli_path.is_absolute()) {
          const fs::path abs = fs::absolute(cli_path, ec);
          if (!ec) cli_path = abs;
        }
        t81::tui::set_cli_program_path(cli_path.string());
      }
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
        bool cancelled = false;
        auto screen = ScreenInteractive::Fullscreen();
        auto menu_c = Menu(&choices, &selected);
        auto root = Renderer(menu_c, [&]() -> Element {
          return vbox({
                     text("  T81 Foundation  —  Choose Interface") | bold | color(Color::Cyan),
                     separator(),
                     menu_c->Render(),
                     separator(),
                     text("  Arrow keys, Enter to select, Escape to cancel.") |
                         color(Color::GrayDark),
                 }) |
                 border | size(WIDTH, EQUAL, 52) | center;
        });
        root = CatchEvent(root, [&](Event e) -> bool {
          if (e == Event::Escape) {
            cancelled = true;
            screen.ExitLoopClosure()();
            return true;
          }
          if (e == Event::Return) {
            screen.ExitLoopClosure()();
            return true;
          }
          return false;
        });
        screen.Loop(root);
        if (cancelled) return 0;
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
