#include "t81/cli/driver.hpp"
#include "debugger.hpp"
#include "internal/tooling/logging.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/config.hpp"
#include "t81/crypto/sha3.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/isa/binary_io.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/tracing/canonhash.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

inline int trap_exit_code(t81::vm::Trap trap) {
  using T = t81::vm::Trap;
  switch (trap) {
    case T::None:
      return 0;
    case T::DecodeFault:
      return 14;
    case T::TypeFault:
      return 16;
    case T::BoundsFault:
      return 12;
    case T::StackFault:
      return 17;
    case T::DivisionFault:
      return 10;
    case T::SecurityFault:
      return 13;
    case T::TierFault:
      return 19;
    case T::ShapeFault:
      return 18;
    case T::TrapInstruction:
      return 15;
    default:
      return 1;
  }
}

namespace {
std::vector<std::string> split_lines(std::string_view content);

void print_semantic_diagnostics(const t81::frontend::SemanticAnalyzer& analyzer,
                                std::string_view primary_source, const std::string* source) {
  std::vector<std::string> lines;
  if (source) {
    lines = split_lines(*source);
  }
  for (const auto& diag : analyzer.diagnostics()) {
    const std::string file = diag.file.empty() ? "<source>" : diag.file;
    std::cerr << file << ':' << diag.line << ':' << diag.column << ": error: " << diag.message
              << '\n';

    bool print_context =
        source && (file == primary_source || file == "<source>" || primary_source.empty());
    if (print_context) {
      if (diag.line > 0 && diag.line <= static_cast<int>(lines.size())) {
        const std::string& context = lines[diag.line - 1];
        std::cerr << "    " << context << '\n';
        int indent = std::max(0, diag.column - 1);
        if (indent > static_cast<int>(context.size())) {
          indent = static_cast<int>(context.size());
        }
        std::cerr << "    " << std::string(indent, ' ') << "^\n";
      }
    }
  }
}

void print_parse_diagnostics(const t81::frontend::Parser& parser, std::string_view primary_source,
                             const std::string* source) {
  std::vector<std::string> lines;
  if (source) {
    lines = split_lines(*source);
  }
  for (const auto& diag : parser.diagnostics()) {
    std::cerr << diag.file << ':' << diag.line << ':' << diag.column << ": error: " << diag.message
              << '\n';
    const bool print_context =
        source && (diag.file == primary_source || diag.file == "<source>" || primary_source.empty());
    if (print_context && diag.line > 0 && diag.line <= static_cast<int>(lines.size())) {
      const std::string& context = lines[diag.line - 1];
      std::cerr << "    " << context << '\n';
      int indent = std::max(0, diag.column - 1);
      if (indent > static_cast<int>(context.size())) {
        indent = static_cast<int>(context.size());
      }
      std::cerr << "    " << std::string(indent, ' ') << "^\n";
    }
  }
}

std::string structural_kind_name(t81::tisc::StructuralKind kind) {
  switch (kind) {
    case t81::tisc::StructuralKind::TypeAlias:
      return "Alias";
    case t81::tisc::StructuralKind::Record:
      return "Record";
    case t81::tisc::StructuralKind::Enum:
      return "Enum";
  }
  return "Unknown";
}

std::string format_structural_alias(const t81::tisc::TypeAliasMetadata& alias) {
  std::string module = alias.module_path.empty() ? "<source>" : alias.module_path;
  std::ostringstream oss;
  oss << alias.name << " [" << structural_kind_name(alias.kind) << "]"
      << " schema=" << alias.schema_version << " module=" << module;
  if (!alias.fields.empty()) {
    oss << " fields=";
    for (size_t i = 0; i < alias.fields.size(); ++i) {
      if (i) oss << ',';
      oss << alias.fields[i].name;
    }
  }
  if (!alias.variants.empty()) {
    oss << " variants=";
    for (size_t i = 0; i < alias.variants.size(); ++i) {
      if (i) oss << ',';
      oss << alias.variants[i].name;
    }
  }
  return oss.str();
}

std::string to_lower(std::string_view text) {
  std::string result;
  result.reserve(text.size());
  for (char c : text) {
    result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }
  return result;
}

std::string trim_copy(std::string_view text) {
  size_t start = 0;
  while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
    ++start;
  }
  size_t end = text.size();
  while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return std::string(text.substr(start, end - start));
}

std::string escape_json_text(std::string_view text) {
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

std::vector<std::string> split_lines(std::string_view content) {
  std::string text(content);
  std::vector<std::string> lines;
  std::istringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    lines.push_back(line);
  }
  return lines;
}

std::string summarize_snippet(const std::string& snippet) {
  std::string summary = snippet;
  auto newline = summary.find('\n');
  if (newline != std::string::npos) {
    summary = summary.substr(0, newline);
  }
  if (summary.size() > 64) {
    summary = summary.substr(0, 61) + "...";
  }
  return summary;
}

std::string format_trace_entry(const t81::vm::TraceEntry& entry) {
  std::ostringstream oss;
  oss << "PC=" << entry.pc << ' ' << t81::tisc::opcode_name(entry.opcode);
  if (entry.trap) {
    oss << " trap=" << t81::vm::to_string(*entry.trap);
  }
  return oss.str();
}

void print_trace_summary(const t81::vm::State& state) {
  const auto& trace = state.trace;
  if (trace.empty()) {
    info("Trace is empty");
    return;
  }
  info("Last trace entries:");
  size_t limit = std::min<std::size_t>(trace.size(), 16);
  for (size_t i = 0; i < limit; ++i) {
    std::string entry = format_trace_entry(trace[i]);
    info("  " + std::to_string(i + 1) + ": " + entry);
  }
  if (trace.size() > limit) {
    info("  ... " + std::to_string(trace.size() - limit) + " more entries");
  }
}

void print_bindings_summary(const t81::vm::State& state) {
  if (state.symbols.empty()) {
    info("No symbols recorded from the last run");
    return;
  }
  info("Symbols from last run:");
  size_t limit = std::min<std::size_t>(state.symbols.size(), 16);
  for (size_t i = 0; i < limit; ++i) {
    info("  " + std::to_string(i + 1) + ": " + state.symbols[i]);
  }
  if (state.symbols.size() > limit) {
    info("  ... " + std::to_string(state.symbols.size() - limit) + " more symbols");
  }
}

bool load_weights_model_from_path(const fs::path& path,
                                  std::shared_ptr<t81::weights::ModelFile>& model,
                                  std::optional<fs::path>& model_path, std::string& error) {
  if (!fs::exists(path)) {
    error = "weights model file not found";
    return false;
  }
  std::string ext = to_lower(path.extension().string());
  try {
    t81::weights::ModelFile loaded;
    if (ext == ".gguf") {
      loaded = t81::weights::load_gguf(path);
    } else if (ext == ".safetensors" || ext == ".safetensor") {
      loaded = t81::weights::load_safetensors(path);
    } else if (ext == ".t81w") {
      loaded = t81::weights::load_t81w(path);
    } else {
      error = "unsupported weights extension '" + ext + "'";
      return false;
    }
    model = std::make_shared<t81::weights::ModelFile>(std::move(loaded));
    model_path = path;
    return true;
  } catch (const std::exception& e) {
    error = e.what();
    return false;
  }
}

bool is_valid_package_name(const std::string& name) {
  if (name.empty()) return false;
  for (char c : name) {
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') {
      return false;
    }
  }
  return true;
}

std::string sha3_512_hex_text(std::span<const std::byte> bytes) {
  std::vector<std::uint8_t> raw;
  raw.reserve(bytes.size());
  for (std::byte b : bytes) {
    raw.push_back(static_cast<std::uint8_t>(b));
  }
  return t81::crypto::sha3_512_hex(raw);
}

std::string sha3_512_hex_text(std::string_view text) {
  std::vector<std::uint8_t> raw;
  raw.reserve(text.size());
  for (char c : text) {
    raw.push_back(static_cast<std::uint8_t>(c));
  }
  return t81::crypto::sha3_512_hex(raw);
}

std::string read_file_binary(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Could not open file: " + path.string());
  }
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::vector<std::byte> read_file_bytes(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Could not open file: " + path.string());
  }
  std::vector<std::byte> out;
  char ch = 0;
  while (in.get(ch)) {
    out.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return out;
}

std::string normalize_trace_content(std::string_view content) {
  std::istringstream in{std::string(content)};
  std::ostringstream out;
  std::string line;
  while (std::getline(in, line)) {
    const std::string trimmed = trim_copy(line);
    if (trimmed.rfind("PC=", 0) == 0) {
      out << trimmed << '\n';
    }
  }
  return out.str();
}

struct CanonFsObjectInfo {
  std::size_t size_bytes = 0;
  std::string sha3_512;
};

bool parse_canonical_hash(std::string_view prefixed_hash, t81::canonfs::CanonRef& ref,
                          std::string& error_message) {
  std::string text = trim_copy(prefixed_hash);
  constexpr std::string_view prefix = "sha3-256:";
  if (text.rfind(prefix, 0) != 0) {
    error_message = "canonical hash must use sha3-256:<hash> form";
    return false;
  }
  try {
    ref.hash.h = t81::hash::CanonHash81::from_string(text.substr(prefix.size()));
  } catch (const std::exception& e) {
    error_message = e.what();
    return false;
  }
  return true;
}

bool load_canonfs_object(const fs::path& canonfs_root, std::string_view prefixed_hash,
                         std::vector<std::byte>& bytes, CanonFsObjectInfo& info,
                         std::string& error_message) {
  t81::canonfs::CanonRef ref;
  if (!parse_canonical_hash(prefixed_hash, ref, error_message)) {
    return false;
  }
  auto driver = t81::canonfs::make_persistent_driver(canonfs_root);
  auto read_res = driver->read_object_bytes(ref);
  if (!read_res) {
    error_message = "object not found or failed verification";
    return false;
  }
  bytes = std::move(read_res.value());
  info.size_bytes = bytes.size();
  info.sha3_512 = sha3_512_hex_text(bytes);
  return true;
}

struct SnapshotManifestEntry {
  std::string relative_path;
  std::string content_hash;
};

struct CanonFsListEntry {
  std::string hash;
  std::uintmax_t size_bytes = 0;
};

std::vector<SnapshotManifestEntry> collect_snapshot_manifest(const fs::path& root) {
  std::vector<SnapshotManifestEntry> entries;
  if (!fs::exists(root)) {
    return entries;
  }
  for (auto it = fs::recursive_directory_iterator(root); it != fs::recursive_directory_iterator();
       ++it) {
    const auto& path = it->path();
    const fs::path rel = fs::relative(path, root);
    if (!rel.empty() && *rel.begin() == "snapshots") {
      if (it->is_directory()) {
        it.disable_recursion_pending();
      }
      continue;
    }
    if (!it->is_regular_file()) {
      continue;
    }
    SnapshotManifestEntry entry;
    entry.relative_path = rel.generic_string();
    if (*rel.begin() == "objects" && path.extension() == ".blk") {
      entry.content_hash = "sha3-256:" + path.stem().string();
    } else {
      auto bytes = read_file_bytes(path);
      entry.content_hash =
          t81::hash::hash_bytes(std::span<const std::byte>(bytes.data(), bytes.size())).to_string();
    }
    entries.push_back(std::move(entry));
  }
  std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.relative_path < rhs.relative_path;
  });
  return entries;
}

std::string render_snapshot_manifest(const std::vector<SnapshotManifestEntry>& entries) {
  std::ostringstream manifest;
  for (const auto& entry : entries) {
    manifest << entry.relative_path << '\t' << entry.content_hash << '\n';
  }
  return manifest.str();
}

bool copy_directory_contents(const fs::path& from, const fs::path& to, std::string& error_message) {
  std::error_code ec;
  fs::create_directories(to, ec);
  if (ec) {
    error_message = "failed to create directory: " + to.string();
    return false;
  }
  for (auto it = fs::recursive_directory_iterator(from); it != fs::recursive_directory_iterator();
       ++it) {
    const auto& src = it->path();
    const fs::path rel = fs::relative(src, from);
    if (!rel.empty() && (*rel.begin() == "snapshots" || *rel.begin() == "objects")) {
      if (it->is_directory()) {
        it.disable_recursion_pending();
      }
      continue;
    }
    const fs::path dst = to / rel;
    if (it->is_directory()) {
      fs::create_directories(dst, ec);
      if (ec) {
        error_message = "failed to create directory: " + dst.string();
        return false;
      }
      continue;
    }
    if (!it->is_regular_file()) {
      continue;
    }
    fs::create_directories(dst.parent_path(), ec);
    if (ec) {
      error_message = "failed to create directory: " + dst.parent_path().string();
      return false;
    }
    fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
    if (ec) {
      error_message = "failed to copy file: " + src.string();
      return false;
    }
  }
  return true;
}

bool clear_directory_except_snapshots(const fs::path& root, std::string& error_message) {
  if (!fs::exists(root)) {
    return true;
  }
  std::error_code ec;
  for (const auto& entry : fs::directory_iterator(root)) {
    if (entry.path().filename() == "snapshots" || entry.path().filename() == "objects") {
      continue;
    }
    fs::remove_all(entry.path(), ec);
    if (ec) {
      error_message = "failed to remove path: " + entry.path().string();
      return false;
    }
  }
  return true;
}

std::vector<CanonFsListEntry> collect_canonfs_objects(const fs::path& canonfs_root) {
  std::vector<CanonFsListEntry> entries;
  const fs::path objects_dir = canonfs_root / "objects";
  if (!fs::exists(objects_dir)) {
    return entries;
  }
  for (const auto& entry : fs::directory_iterator(objects_dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    if (entry.path().extension() != ".blk") {
      continue;
    }
    CanonFsListEntry item;
    item.hash = "sha3-256:" + entry.path().stem().string();
    std::error_code ec;
    item.size_bytes = entry.file_size(ec);
    if (ec) {
      item.size_bytes = 0;
    }
    entries.push_back(std::move(item));
  }
  std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.hash < rhs.hash;
  });
  return entries;
}

}  // namespace

std::string sanitize_symbol(std::string_view input) {
  std::string out;
  out.reserve(input.size());
  for (char c : input) {
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
      out.push_back(c);
    } else {
      out.push_back('_');
    }
  }
  if (out.empty()) out = "_";
  return out;
}

std::string escape_metadata_string(std::string_view input) {
  std::string out;
  out.reserve(input.size());
  for (char c : input) {
    if (c == '\\' || c == '"') {
      out.push_back('\\');
    }
    out.push_back(c);
  }
  return out;
}

std::string format_loop_metadata(
    const std::vector<t81::frontend::SemanticAnalyzer::LoopMetadata>& loops) {
  if (loops.empty()) return {};
  std::ostringstream oss;
  oss << "(policy (tier 1)";
  for (const auto& meta : loops) {
    const std::string file = meta.source_file.empty() ? "<source>" : meta.source_file;
    oss << " (loop" << " (id " << meta.id << ")" << " (file " << sanitize_symbol(file) << ")"
        << " (line " << meta.keyword.line << ")" << " (column " << meta.keyword.column << ")"
        << " (annotated " << (meta.annotated() ? "true" : "false") << ")" << " (depth "
        << meta.depth << ")";
    oss << " (guard " << (meta.guard_present ? "true" : "false") << ")";
    oss << " (bound ";
    using t81::frontend::LoopStmt;
    if (meta.bound_kind == LoopStmt::BoundKind::Infinite) {
      oss << "infinite";
    } else if (meta.bound_kind == LoopStmt::BoundKind::Static && meta.bound_value) {
      oss << *meta.bound_value;
    } else if (meta.bound_kind == LoopStmt::BoundKind::Guarded) {
      oss << "guarded";
    } else {
      oss << "unknown";
    }
    oss << ")";
    oss << ")";
  }
  oss << ")";
  return oss.str();
}

std::string pattern_kind_name(t81::frontend::MatchPattern::Kind kind) {
  switch (kind) {
    case t81::frontend::MatchPattern::Kind::Identifier:
      return "Identifier";
    case t81::frontend::MatchPattern::Kind::Tuple:
      return "Tuple";
    case t81::frontend::MatchPattern::Kind::Record:
      return "Record";
    case t81::frontend::MatchPattern::Kind::Variant:
      return "Variant";
    case t81::frontend::MatchPattern::Kind::None:
      return "None";
  }
  return "UnknownPattern";
}

std::string match_kind_name(t81::frontend::SemanticAnalyzer::MatchMetadata::Kind kind) {
  using MatchKind = t81::frontend::SemanticAnalyzer::MatchMetadata::Kind;
  switch (kind) {
    case MatchKind::Option:
      return "Option";
    case MatchKind::Result:
      return "Result";
    case MatchKind::Enum:
      return "Enum";
    default:
      return "Unknown";
  }
}

std::string format_match_metadata(const t81::frontend::SemanticAnalyzer& analyzer) {
  const auto& matches = analyzer.match_metadata();
  if (matches.empty()) return {};
  std::ostringstream oss;
  oss << "(match-metadata";
  for (const auto& meta : matches) {
    oss << " (match";
    oss << " (scrutinee " << match_kind_name(meta.kind) << ")";
    oss << " (type " << sanitize_symbol(analyzer.type_name(meta.result_type)) << ")";
    oss << " (guards " << (meta.guard_present ? "true" : "false") << ")";
    if (!meta.arms.empty()) {
      oss << " (arms";
      for (const auto& arm : meta.arms) {
        oss << " (arm";
        oss << " (variant " << sanitize_symbol(arm.variant) << ")";
        if (arm.variant_id >= 0) {
          oss << " (variant-id " << arm.variant_id << ")";
        }
        oss << " (pattern " << pattern_kind_name(arm.pattern_kind) << ")";
        oss << " (guard " << (arm.has_guard ? "true" : "false") << ")";
        if (arm.payload_type.kind != t81::frontend::Type::Kind::Unknown) {
          oss << " (payload " << sanitize_symbol(analyzer.type_name(arm.payload_type)) << ")";
        }
        if (arm.arm_type.kind != t81::frontend::Type::Kind::Unknown) {
          oss << " (type " << sanitize_symbol(analyzer.type_name(arm.arm_type)) << ")";
        }
        if (arm.has_guard && !arm.guard_expression.empty()) {
          oss << " (guard-expr \"" << escape_metadata_string(arm.guard_expression) << "\")";
        }
        oss << ")";
      }
      oss << ")";
    }
    oss << ")";
  }
  oss << ")";
  return oss.str();
}

std::vector<t81::tisc::EnumMetadata> collect_enum_metadata(
    const t81::frontend::SemanticAnalyzer& analyzer) {
  std::vector<t81::tisc::EnumMetadata> enums;
  const auto& definitions = analyzer.enum_definitions();
  std::vector<std::pair<std::string, const t81::frontend::EnumInfo*>> ordered_definitions;
  ordered_definitions.reserve(definitions.size());
  for (const auto& [name, info] : definitions) {
    if (info.id >= 0) {
      ordered_definitions.emplace_back(name, &info);
    }
  }
  std::sort(ordered_definitions.begin(), ordered_definitions.end(),
            [](const auto& lhs, const auto& rhs) {
              if (lhs.second->id != rhs.second->id) {
                return lhs.second->id < rhs.second->id;
              }
              return lhs.first < rhs.first;
            });

  enums.reserve(ordered_definitions.size());
  for (const auto& [name, info_ptr] : ordered_definitions) {
    const auto& info = *info_ptr;
    t81::tisc::EnumMetadata entry;
    entry.enum_id = info.id;
    entry.name = name;
    entry.variants.reserve(info.variant_order.size());
    for (const auto& variant_name : info.variant_order) {
      auto variant_it = info.variants.find(variant_name);
      if (variant_it == info.variants.end()) {
        continue;
      }
      t81::tisc::EnumVariantMetadata variant_meta;
      variant_meta.name = variant_name;
      variant_meta.variant_id = variant_it->second.id;
      if (variant_it->second.payload.has_value()) {
        variant_meta.payload = analyzer.type_name(*variant_it->second.payload);
      }
      entry.variants.push_back(std::move(variant_meta));
    }
    enums.push_back(std::move(entry));
  }
  return enums;
}

std::vector<t81::tisc::FunctionMetadata> collect_function_metadata(
    const std::vector<std::unique_ptr<t81::frontend::Stmt>>& stmts) {
  std::vector<t81::tisc::FunctionMetadata> result;
  for (const auto& stmt : stmts) {
    if (auto* func = dynamic_cast<const t81::frontend::FunctionStmt*>(stmt.get())) {
      if (func->is_axion_verify) {
        t81::tisc::FunctionMetadata meta;
        meta.name = std::string(func->name.lexeme);
        meta.is_axion_verify = true;
        result.push_back(std::move(meta));
      }
    }
  }
  return result;
}

namespace t81::cli {

std::optional<t81::tisc::Program> build_program_from_source(
    const std::string& source, const std::string& diag_name,
    const std::shared_ptr<t81::weights::ModelFile>& weights_model) {
  verbose("Lexing...");
  t81::frontend::Lexer lexer(source);
  auto tokens = lexer.all_tokens();

  bool lexer_error = false;
  for (const auto& t : tokens) {
    if (t.type == t81::frontend::TokenType::Illegal) {
      lexer_error = true;
      std::cerr << diag_name << ':' << t.line << ':' << t.column << ": illegal token `" << t.lexeme
                << "`\n";
    }
  }
  if (lexer_error) {
    error("Lexing failed");
    return std::nullopt;
  }

  verbose("Parsing...");
  t81::frontend::Lexer parser_lexer(source);
  t81::frontend::Parser parser(parser_lexer, diag_name);
  auto stmts = parser.parse();
  if (parser.had_error()) {
    print_parse_diagnostics(parser, diag_name, &source);
    error("Parse errors encountered");
    return std::nullopt;
  }

  verbose("Semantic analysis...");
  t81::frontend::SemanticAnalyzer semantic_analyzer(stmts, diag_name);
  semantic_analyzer.analyze();
  if (semantic_analyzer.had_error()) {
    print_semantic_diagnostics(semantic_analyzer, diag_name, &source);
    error("Semantic errors encountered");
    return std::nullopt;
  }

  verbose("Generating IR...");
  t81::frontend::IRGenerator ir_gen;
  ir_gen.attach_semantic_analyzer(&semantic_analyzer);
  auto ir = ir_gen.generate(stmts);

  verbose("Emitting TISC bytecode...");
  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir);

  auto loop_policy = format_loop_metadata(semantic_analyzer.loop_metadata());
  if (!loop_policy.empty()) {
    program.axion_policy_text = loop_policy;
    verbose("Axion loop metadata emitted");
  }

  auto match_policy = format_match_metadata(semantic_analyzer);
  if (!match_policy.empty()) {
    program.match_metadata_text = match_policy;
    verbose("Match metadata emitted");
    verbose(match_policy);
  }

  auto enum_metadata = collect_enum_metadata(semantic_analyzer);
  if (!enum_metadata.empty()) {
    program.enum_metadata = std::move(enum_metadata);
    verbose("Enum metadata emitted");
  }

  auto func_metadata = collect_function_metadata(stmts);
  if (!func_metadata.empty()) {
    program.function_metadata = std::move(func_metadata);
    verbose("Function metadata emitted");
  }

  if (weights_model) {
    program.weights_model = weights_model;
  }

  if (!program.type_aliases.empty()) {
    std::ostringstream oss;
    oss << "Structural metadata (" << program.type_aliases.size() << " entries):";
    for (const auto& alias : program.type_aliases) {
      oss << "\n  " << format_structural_alias(alias);
    }
    verbose(oss.str());
  }

  return program;
}

int compile(const fs::path& input, const fs::path& output, const std::string& source_override,
            const std::string& source_name,
            std::shared_ptr<t81::weights::ModelFile> weights_model) {
  verbose("Compiling " + input.string() + " → " + output.string());

  std::string diag_name = source_name.empty() ? input.string() : source_name;
  std::string source;
  if (source_override.empty()) {
    if (!fs::exists(input)) {
      error("Input file not found: " + input.string());
      return 1;
    }

    source = [](const fs::path& p) {
      std::ifstream f(p, std::ios::binary);
      if (!f) throw std::runtime_error("Failed to open source file");
      std::ostringstream ss;
      ss << f.rdbuf();
      return ss.str();
    }(input);
  } else {
    source = source_override;
  }

  auto program = build_program_from_source(source, diag_name, weights_model);
  if (!program) return 1;

  verbose("Writing " + output.string());
  t81::tisc::save_program(*program, output.string());

  info("Compilation successful → " + output.string());
  verbose(std::to_string(program->insns.size()) + " instructions emitted");
  return 0;
}

namespace {

void print_repl_help() {
  info("REPL commands:");
  info("  :quit / :exit       Exit the interactive session");
  info("  :help               Show this message again");
  info("  :history            Dump the pending buffer and recent runs");
  info("  :reset              Clear the current buffer");
  info("  :load <path>        Load a file into the buffer");
  info("  :save <path>        Persist the buffer to disk");
  info("  :run                Force execution without an empty line");
  info("  :model [path]       Show/replace the attached weights model");
  info("  :verbose / :quiet   Toggle logging verbosity");
  info("  :bindings / :symbols List recorded symbols from the last run");
  info("  :trace / :trill     Dump the last VM trace");
  info("Submit an empty line to compile and execute the buffered snippet.");
}

void print_repl_history(const std::vector<std::string>& buffer_lines,
                        const std::vector<std::string>& history_snippets) {
  if (buffer_lines.empty()) {
    info("REPL buffer is empty");
  } else {
    info("REPL buffer:");
    for (size_t i = 0; i < buffer_lines.size(); ++i) {
      const std::string& line = buffer_lines[i];
      const std::string display = line.empty() ? "<empty>" : line;
      info("  " + std::to_string(i + 1) + ": " + display);
    }
  }

  if (history_snippets.empty()) {
    info("No previous executions");
    return;
  }
  info("Previous executions:");
  size_t limit = std::min<std::size_t>(history_snippets.size(), 5);
  for (size_t i = 0; i < limit; ++i) {
    info("  " + std::to_string(i + 1) + ": " + summarize_snippet(history_snippets[i]));
  }
  if (history_snippets.size() > limit) {
    info("  ... " + std::to_string(history_snippets.size() - limit) + " more entries");
  }
}

}  // namespace

int repl(const std::shared_ptr<t81::weights::ModelFile>& weights_model,
         const std::optional<fs::path>& policy_path, std::istream& input) {
  info("Entering T81 interactive REPL. Type ':quit' or ':exit' to leave; submit an empty line to "
       "run.");
  std::string buffer;
  std::vector<std::string> buffer_lines;
  std::vector<std::string> executed_snippets;
  std::unique_ptr<t81::vm::IVirtualMachine> last_vm;
  std::shared_ptr<t81::weights::ModelFile> active_model = weights_model;
  std::optional<fs::path> attached_model_path;

  std::string policy_content;
  if (policy_path) {
    std::ifstream ifs(*policy_path);
    if (ifs) {
      policy_content.assign((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
      info("Loaded Axion policy from " + policy_path->string());
    } else {
      error("Could not open policy file: " + policy_path->string());
    }
  }

  auto ensure_newline = [&]() {
    if (!buffer.empty() && buffer.back() != '\n') {
      buffer.push_back('\n');
    }
  };

  auto append_line = [&](const std::string& line) {
    ensure_newline();
    buffer += line;
    buffer.push_back('\n');
    buffer_lines.push_back(line);
  };

  auto clear_buffer = [&]() {
    buffer.clear();
    buffer_lines.clear();
  };

  constexpr size_t kHistoryLimit = 64;

  auto execute_buffer = [&]() -> bool {
    if (buffer.empty()) {
      info("Nothing to run (buffer is empty)");
      return true;
    }
    auto program = build_program_from_source(buffer, "<repl>", active_model);
    if (!program) {
      clear_buffer();
      return false;
    }
    if (!policy_content.empty()) {
      program->axion_policy_text = policy_content;
    }
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(*program);
    auto result = vm->run_to_halt();
    last_vm = std::move(vm);
    executed_snippets.push_back(buffer);
    if (executed_snippets.size() > kHistoryLimit) {
      executed_snippets.erase(executed_snippets.begin());
    }
    for (const auto& line : last_vm->state().printed_output) {
      std::cout << line << "\n";
    }

    if (!result) {
      error("Execution trapped: " + t81::vm::to_string(result.error()));
      clear_buffer();
      return false;
    }

    info("Execution completed");
    clear_buffer();
    return true;
  };

  auto set_buffer_from_string = [&](std::string content) {
    buffer = std::move(content);
    buffer_lines = split_lines(buffer);
  };

  while (true) {
    std::cout << (buffer.empty() ? "t81> " : ".... ") << std::flush;
    std::string line;
    if (!std::getline(input, line)) {
      info("Exiting REPL");
      return 0;
    }

    std::string trimmed = trim_copy(line);
    if (!trimmed.empty() && trimmed.front() == ':') {
      std::istringstream command(trimmed);
      std::string token;
      command >> token;
      std::string args;
      std::getline(command, args);
      args = trim_copy(args);

      if (token == ":quit" || token == ":exit") {
        break;
      }
      if (token == ":help") {
        print_repl_help();
        continue;
      }
      if (token == ":history") {
        print_repl_history(buffer_lines, executed_snippets);
        continue;
      }
      if (token == ":reset") {
        if (buffer.empty()) {
          info("REPL buffer already empty");
        } else {
          clear_buffer();
          info("REPL buffer cleared");
        }
        continue;
      }
      if (token == ":load") {
        if (args.empty()) {
          error("Missing path for :load");
          continue;
        }
        fs::path path(args);
        std::ifstream in(path);
        if (!in) {
          error("Failed to open file: " + path.string());
          continue;
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        set_buffer_from_string(ss.str());
        info("Loaded snippet from " + path.string());
        continue;
      }
      if (token == ":save") {
        if (args.empty()) {
          error("Missing path for :save");
          continue;
        }
        fs::path path(args);
        std::ofstream out(path, std::ios::binary);
        if (!out) {
          error("Failed to write file: " + path.string());
          continue;
        }
        out << buffer;
        info("Buffer saved to " + path.string());
        continue;
      }
      if (token == ":run") {
        execute_buffer();
        continue;
      }
      if (token == ":model") {
        if (args.empty()) {
          if (attached_model_path) {
            info("Attached weights model: " + attached_model_path->string());
          } else if (active_model) {
            info("Attached weights model (path unknown)");
          } else {
            info("No weights model attached");
          }
          continue;
        }
        if (args == "none") {
          active_model.reset();
          attached_model_path.reset();
          info("Weights model cleared");
          continue;
        }
        std::string error_msg;
        fs::path path(args);
        if (!load_weights_model_from_path(path, active_model, attached_model_path, error_msg)) {
          error("Failed to load model: " + error_msg);
        } else {
          info("Loaded weights model from " + path.string());
        }
        continue;
      }
      if (token == ":verbose") {
        g_flags.verbose = true;
        g_flags.quiet = false;
        info("Verbose logging enabled");
        continue;
      }
      if (token == ":quiet") {
        g_flags.quiet = true;
        g_flags.verbose = false;
        info("Quiet mode enabled");
        continue;
      }
      if (token == ":bindings" || token == ":symbols") {
        if (last_vm) {
          print_bindings_summary(last_vm->state());
        } else {
          info("No execution recorded yet");
        }
        continue;
      }
      if (token == ":trace" || token == ":trill") {
        if (last_vm) {
          print_trace_summary(last_vm->state());
        } else {
          info("No trace available yet");
        }
        continue;
      }
      info("Unknown command: " + token);
      continue;
    }

    if (line.empty()) {
      if (buffer.empty()) {
        continue;
      }
      execute_buffer();
      continue;
    }

    append_line(line);
  }

  info("Exiting REPL");
  return 0;
}

int run_tisc(const fs::path& path, const std::optional<fs::path>& policy_path, bool trace_enabled,
             const std::optional<fs::path>& trace_output_path) {
  verbose("Loading TISC program: " + path.string());

  auto program = t81::tisc::load_program(path.string());
  verbose("Program loaded (" + std::to_string(program.insns.size()) + " insns)");

  if (policy_path) {
    std::ifstream ifs(*policy_path);
    if (ifs) {
      std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
      program.axion_policy_text = content;
      verbose("Axion policy loaded from " + policy_path->string());
    } else {
      error("Could not open policy file: " + policy_path->string());
    }
  }

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);

  verbose("Executing...");
  auto result = vm->run_to_halt();

  for (const auto& line : vm->state().printed_output) {
    std::cout << line << "\n";
  }

  if (!result) {
    error("Execution trapped: " + t81::vm::to_string(result.error()));
    return trap_exit_code(result.error());
  }

  if (trace_enabled) {
    std::ostream* trace_stream = &std::cout;
    std::ofstream trace_file;
    if (trace_output_path) {
      trace_file.open(*trace_output_path, std::ios::binary | std::ios::trunc);
      if (!trace_file) {
        error("Could not open trace output path: " + trace_output_path->string());
        return 1;
      }
      trace_stream = &trace_file;
    }
    for (const auto& entry : vm->state().trace) {
      *trace_stream << format_trace_entry(entry) << "\n";
    }
    if (trace_output_path) {
      info("Trace written to " + trace_output_path->string());
    }
  } else {
    if (!vm->state().axion_log.empty()) {
      info("Program terminated normally (" + std::to_string(vm->state().axion_log.size()) +
           " axion event(s))");
    } else {
      info("Program terminated normally");
    }
  }
  return 0;
}

int disasm_tisc(const fs::path& path) {
  verbose("Loading TISC program for disassembly: " + path.string());

  auto program = t81::tisc::load_program(path.string());
  std::cout << "; t81 disasm " << path.string() << "\n";
  std::cout << "; instructions=" << program.insns.size() << " floats=" << program.float_pool.size()
            << " fractions=" << program.fraction_pool.size()
            << " symbols=" << program.symbol_pool.size()
            << " tensors=" << program.tensor_pool.size() << " shapes=" << program.shape_pool.size()
            << "\n";

  if (!program.type_aliases.empty()) {
    std::cout << "; type_aliases:\n";
    for (const auto& alias : program.type_aliases) {
      std::cout << ";   " << format_structural_alias(alias) << "\n";
    }
  }
  if (!program.enum_metadata.empty()) {
    std::cout << "; enums:\n";
    for (const auto& meta : program.enum_metadata) {
      std::cout << ";   " << meta.name << " (id=" << meta.enum_id << ")\n";
    }
  }

  for (size_t pc = 0; pc < program.insns.size(); ++pc) {
    const auto& insn = program.insns[pc];
    std::cout << std::setw(4) << std::setfill('0') << pc << std::setfill(' ') << ": "
              << t81::tisc::opcode_name(insn.opcode) << " a=" << insn.a << " b=" << insn.b
              << " c=" << insn.c << " lit=" << static_cast<int>(insn.literal_kind) << "\n";
  }
  return 0;
}

int debug_tisc(const fs::path& path, const std::optional<fs::path>& policy_path) {
  verbose("Loading TISC program for debugging: " + path.string());

  auto program = t81::tisc::load_program(path.string());

  if (policy_path) {
    verbose("Loading Axion policy for debugging: " + policy_path->string());
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

  Debugger dbg(std::move(vm), program);
  dbg.run();

  return 0;
}

int check_syntax(const fs::path& path) {
  verbose("Syntax-checking " + path.string());

  std::string source = [](const fs::path& p) {
    std::ifstream f(p);
    if (!f) throw std::runtime_error("Cannot open file");
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
  }(path);

  auto program = build_program_from_source(source, path.string());
  if (!program) {
    return 1;
  }

  info("No syntax or semantic errors");
  return 0;
}

int canonfs_put_file(const fs::path& input, const fs::path& canonfs_root) {
  if (!fs::exists(input)) {
    error("Could not open input file: " + input.string());
    return 1;
  }
  auto bytes = read_file_bytes(input);
  auto driver = t81::canonfs::make_persistent_driver(canonfs_root);
  auto write_res = driver->write_object(t81::canonfs::ObjectType::RawBlock, bytes);
  if (!write_res) {
    error("canonfs put-file: failed to write CanonFS object");
    return 1;
  }
  std::cout << "sha3-256:" << write_res->hash.h.to_string() << "\n";
  return 0;
}

int canonfs_list(const fs::path& canonfs_root, bool as_json) {
  const auto entries = collect_canonfs_objects(canonfs_root);
  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.canonfs-list.v1\",\n"
              << "  \"canonfs_root\": \"" << escape_json_text(canonfs_root.string()) << "\",\n"
              << "  \"count\": " << entries.size() << ",\n"
              << "  \"objects\": [\n";
    for (std::size_t i = 0; i < entries.size(); ++i) {
      const auto& entry = entries[i];
      std::cout << "    {\"hash\": \"" << escape_json_text(entry.hash)
                << "\", \"size_bytes\": " << entry.size_bytes << "}";
      if (i + 1 < entries.size()) {
        std::cout << ",";
      }
      std::cout << "\n";
    }
    std::cout << "  ]\n}\n";
  } else {
    if (entries.empty()) {
      const bool store_exists = fs::exists(canonfs_root);
      if (!store_exists) {
        std::cerr << "CanonFS store not found: " << canonfs_root.string()
                  << " (use --canonfs-root or set T81_CANONFS_ROOT)\n";
        return 1;
      }
      std::cout << "CanonFS object store is empty: " << canonfs_root.string() << "\n";
      return 0;
    }
    for (const auto& entry : entries) {
      std::cout << entry.hash << "  " << entry.size_bytes << " bytes\n";
    }
  }
  return 0;
}

int canonfs_get(const std::string& canonical_hash, const std::optional<fs::path>& output_path,
                const fs::path& canonfs_root, bool as_json) {
  std::vector<std::byte> bytes;
  CanonFsObjectInfo info;
  std::string error_message;
  if (!load_canonfs_object(canonfs_root, canonical_hash, bytes, info, error_message)) {
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.canonfs-get.v1\",\n"
                << "  \"ok\": false,\n"
                << "  \"hash\": \"" << escape_json_text(canonical_hash) << "\",\n"
                << "  \"error\": \"" << escape_json_text(error_message) << "\"\n"
                << "}\n";
    } else {
      error("canonfs get: " + error_message);
    }
    return 1;
  }
  if (output_path) {
    std::ofstream out(*output_path, std::ios::binary | std::ios::trunc);
    if (!out) {
      error("canonfs get: could not open output file: " + output_path->string());
      return 1;
    }
    for (std::byte b : bytes) {
      out.put(static_cast<char>(std::to_integer<unsigned char>(b)));
    }
    if (!out.good()) {
      error("canonfs get: failed writing output file: " + output_path->string());
      return 1;
    }
  } else {
    for (std::byte b : bytes) {
      std::cout.put(static_cast<char>(std::to_integer<unsigned char>(b)));
    }
  }
  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.canonfs-get.v1\",\n"
              << "  \"ok\": true,\n"
              << "  \"hash\": \"" << escape_json_text(canonical_hash) << "\",\n"
              << "  \"size_bytes\": " << info.size_bytes << ",\n"
              << "  \"output\": ";
    if (output_path) {
      std::cout << "\"" << escape_json_text(output_path->string()) << "\"\n";
    } else {
      std::cout << "null\n";
    }
    std::cout << "}\n";
  }
  return 0;
}

int canonfs_stat(const std::string& canonical_hash, const fs::path& canonfs_root, bool as_json) {
  std::vector<std::byte> bytes;
  CanonFsObjectInfo info;
  std::string error_message;
  if (!load_canonfs_object(canonfs_root, canonical_hash, bytes, info, error_message)) {
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.canonfs-stat.v1\",\n"
                << "  \"ok\": false,\n"
                << "  \"hash\": \"" << escape_json_text(canonical_hash) << "\",\n"
                << "  \"canonfs_root\": \"" << escape_json_text(canonfs_root.string()) << "\",\n"
                << "  \"error\": \"" << escape_json_text(error_message) << "\"\n"
                << "}\n";
    } else {
      error("canonfs stat: " + error_message);
    }
    return 1;
  }
  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.canonfs-stat.v1\",\n"
              << "  \"ok\": true,\n"
              << "  \"hash\": \"" << escape_json_text(canonical_hash) << "\",\n"
              << "  \"canonfs_root\": \"" << escape_json_text(canonfs_root.string()) << "\",\n"
              << "  \"size_bytes\": " << info.size_bytes << ",\n"
              << "  \"sha3_512\": \"" << info.sha3_512 << "\"\n"
              << "}\n";
  } else {
    std::cout << "Hash:       " << canonical_hash << "\n";
    std::cout << "Root:       " << canonfs_root << "\n";
    std::cout << "Size:       " << info.size_bytes << " bytes\n";
    std::cout << "SHA3-512:   " << info.sha3_512 << "\n";
  }
  return 0;
}

int canonfs_verify(const std::string& canonical_hash, const fs::path& canonfs_root, bool as_json) {
  std::vector<std::byte> bytes;
  CanonFsObjectInfo info;
  std::string error_message;
  const bool ok = load_canonfs_object(canonfs_root, canonical_hash, bytes, info, error_message);
  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.canonfs-verify.v1\",\n"
              << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
              << "  \"hash\": \"" << escape_json_text(canonical_hash) << "\",\n"
              << "  \"canonfs_root\": \"" << escape_json_text(canonfs_root.string()) << "\"";
    if (ok) {
      std::cout << ",\n  \"size_bytes\": " << info.size_bytes << ",\n"
                << "  \"sha3_512\": \"" << info.sha3_512 << "\"\n";
    } else {
      std::cout << ",\n  \"error\": \"" << escape_json_text(error_message) << "\"\n";
    }
    std::cout << "}\n";
  } else if (ok) {
    std::cout << "verified: " << canonical_hash << "\n";
  } else {
    error("canonfs verify: " + error_message);
  }
  return ok ? 0 : 1;
}

std::optional<std::string> canonfs_capture_snapshot_hash(const fs::path& canonfs_root,
                                                         std::string* error_message) {
  std::error_code ec;
  fs::create_directories(canonfs_root, ec);
  if (ec) {
    if (error_message) *error_message = "could not create root: " + canonfs_root.string();
    return std::nullopt;
  }
  const auto entries = collect_snapshot_manifest(canonfs_root);
  const std::string manifest = render_snapshot_manifest(entries);
  const std::string snapshot_hash =
      t81::hash::hash_bytes(std::span<const std::byte>(
                                reinterpret_cast<const std::byte*>(manifest.data()),
                                manifest.size()))
          .to_string();

  const fs::path snapshots_root = canonfs_root / "snapshots";
  const fs::path snapshot_dir = snapshots_root / snapshot_hash;
  fs::create_directories(snapshots_root, ec);
  if (ec) {
    if (error_message) *error_message = "could not create snapshots directory";
    return std::nullopt;
  }
  std::string copy_error;
  if (!fs::exists(snapshot_dir) && !copy_directory_contents(canonfs_root, snapshot_dir, copy_error)) {
    if (error_message) *error_message = copy_error;
    return std::nullopt;
  }
  {
    std::ofstream out(snapshot_dir / "MANIFEST.txt", std::ios::binary | std::ios::trunc);
    if (!out) {
      if (error_message) *error_message = "failed to write manifest";
      return std::nullopt;
    }
    out << manifest;
  }
  return snapshot_hash;
}

int canonfs_snapshot(const fs::path& canonfs_root, bool as_json) {
  std::string snapshot_error;
  const auto snapshot_hash = canonfs_capture_snapshot_hash(canonfs_root, &snapshot_error);
  if (!snapshot_hash) {
    error("canonfs snapshot: " + snapshot_error);
    return 1;
  }
  const auto entries = collect_snapshot_manifest(canonfs_root);
  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.canonfs-snapshot.v1\",\n"
              << "  \"ok\": true,\n"
              << "  \"hash\": \"" << *snapshot_hash << "\",\n"
              << "  \"canonfs_root\": \"" << escape_json_text(canonfs_root.string()) << "\",\n"
              << "  \"entries\": " << entries.size() << "\n"
              << "}\n";
  } else {
    std::cout << "CanonFS snapshot captured.\nHash: " << *snapshot_hash << "\n";
  }
  return 0;
}

int canonfs_snapshot_diff(const std::string& lhs_snapshot_hash, const std::string& rhs_snapshot_hash,
                          const fs::path& canonfs_root, bool as_json) {
  const fs::path lhs_manifest = canonfs_root / "snapshots" / lhs_snapshot_hash / "MANIFEST.txt";
  const fs::path rhs_manifest = canonfs_root / "snapshots" / rhs_snapshot_hash / "MANIFEST.txt";
  if (!fs::exists(lhs_manifest)) {
    error("canonfs snapshot-diff: snapshot not found: " + lhs_snapshot_hash);
    return 1;
  }
  if (!fs::exists(rhs_manifest)) {
    error("canonfs snapshot-diff: snapshot not found: " + rhs_snapshot_hash);
    return 1;
  }
  const std::string lhs_text = read_file_binary(lhs_manifest);
  const std::string rhs_text = read_file_binary(rhs_manifest);
  const auto lhs_lines = split_lines(lhs_text);
  const auto rhs_lines = split_lines(rhs_text);
  std::set<std::string> lhs_set(lhs_lines.begin(), lhs_lines.end());
  std::set<std::string> rhs_set(rhs_lines.begin(), rhs_lines.end());
  std::vector<std::string> only_lhs;
  std::vector<std::string> only_rhs;
  std::set_difference(lhs_set.begin(), lhs_set.end(), rhs_set.begin(), rhs_set.end(),
                      std::back_inserter(only_lhs));
  std::set_difference(rhs_set.begin(), rhs_set.end(), lhs_set.begin(), lhs_set.end(),
                      std::back_inserter(only_rhs));
  const bool identical = only_lhs.empty() && only_rhs.empty();
  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.canonfs-snapshot-diff.v1\",\n"
              << "  \"ok\": " << (identical ? "true" : "false") << ",\n"
              << "  \"lhs\": \"" << escape_json_text(lhs_snapshot_hash) << "\",\n"
              << "  \"rhs\": \"" << escape_json_text(rhs_snapshot_hash) << "\",\n"
              << "  \"only_lhs\": [";
    for (std::size_t i = 0; i < only_lhs.size(); ++i) {
      if (i) std::cout << ", ";
      std::cout << "\"" << escape_json_text(only_lhs[i]) << "\"";
    }
    std::cout << "],\n  \"only_rhs\": [";
    for (std::size_t i = 0; i < only_rhs.size(); ++i) {
      if (i) std::cout << ", ";
      std::cout << "\"" << escape_json_text(only_rhs[i]) << "\"";
    }
    std::cout << "]\n}\n";
  } else if (identical) {
    std::cout << "Snapshots are identical.\n";
  } else {
    for (const auto& line : only_lhs) {
      std::cout << "- " << line << "\n";
    }
    for (const auto& line : only_rhs) {
      std::cout << "+ " << line << "\n";
    }
  }
  return identical ? 0 : 1;
}

int canonfs_rollback(const std::string& snapshot_hash, const fs::path& canonfs_root, bool as_json) {
  const fs::path snapshot_dir = canonfs_root / "snapshots" / snapshot_hash;
  if (!fs::exists(snapshot_dir)) {
    error("canonfs rollback: snapshot not found: " + snapshot_hash);
    return 1;
  }
  std::string error_message;
  if (!clear_directory_except_snapshots(canonfs_root, error_message)) {
    error("canonfs rollback: " + error_message);
    return 1;
  }
  for (auto it = fs::recursive_directory_iterator(snapshot_dir); it != fs::recursive_directory_iterator();
       ++it) {
    const auto& src = it->path();
    if (src.filename() == "MANIFEST.txt") {
      continue;
    }
    const fs::path rel = fs::relative(src, snapshot_dir);
    const fs::path dst = canonfs_root / rel;
    std::error_code ec;
    if (it->is_directory()) {
      fs::create_directories(dst, ec);
    } else if (it->is_regular_file()) {
      fs::create_directories(dst.parent_path(), ec);
      if (!ec) {
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
      }
    }
    if (ec) {
      error("canonfs rollback: failed copying " + src.string());
      return 1;
    }
  }
  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.canonfs-rollback.v1\",\n"
              << "  \"ok\": true,\n"
              << "  \"hash\": \"" << snapshot_hash << "\",\n"
              << "  \"canonfs_root\": \"" << escape_json_text(canonfs_root.string()) << "\"\n"
              << "}\n";
  } else {
    std::cout << "Rollback complete. Active snapshot: " << snapshot_hash << "\n";
  }
  return 0;
}

int determinism_hash_file(const fs::path& input, bool as_json) {
  auto bytes = read_file_bytes(input);
  const std::string hash = sha3_512_hex_text(bytes);
  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.determinism-hash.v1\",\n"
              << "  \"path\": \"" << escape_json_text(input.string()) << "\",\n"
              << "  \"sha3_512\": \"" << hash << "\"\n"
              << "}\n";
  } else {
    std::cout << hash << "  " << input.string() << "\n";
  }
  return 0;
}

int determinism_hash_trace(const fs::path& input, bool as_json) {
  const std::string normalized = normalize_trace_content(read_file_binary(input));
  const std::string hash = sha3_512_hex_text(normalized);
  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.determinism-trace-hash.v1\",\n"
              << "  \"path\": \"" << escape_json_text(input.string()) << "\",\n"
              << "  \"normalized_entries\": "
              << std::count(normalized.begin(), normalized.end(), '\n') << ",\n"
              << "  \"sha3_512\": \"" << hash << "\"\n"
              << "}\n";
  } else {
    std::cout << hash << "  " << input.string() << "\n";
  }
  return 0;
}

int determinism_diff_files(const fs::path& lhs, const fs::path& rhs, bool as_json) {
  const std::string lhs_bytes = read_file_binary(lhs);
  const std::string rhs_bytes = read_file_binary(rhs);
  const bool identical = lhs_bytes == rhs_bytes;
  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.determinism-diff.v1\",\n"
              << "  \"ok\": " << (identical ? "true" : "false") << ",\n"
              << "  \"lhs\": \"" << escape_json_text(lhs.string()) << "\",\n"
              << "  \"rhs\": \"" << escape_json_text(rhs.string()) << "\",\n"
              << "  \"lhs_sha3_512\": \"" << sha3_512_hex_text(lhs_bytes) << "\",\n"
              << "  \"rhs_sha3_512\": \"" << sha3_512_hex_text(rhs_bytes) << "\"\n"
              << "}\n";
  } else if (identical) {
    std::cout << "files are identical\n";
  } else {
    std::cout << "files differ\n";
    std::cout << "lhs sha3-512: " << sha3_512_hex_text(lhs_bytes) << "\n";
    std::cout << "rhs sha3-512: " << sha3_512_hex_text(rhs_bytes) << "\n";
  }
  return identical ? 0 : 1;
}

int determinism_diff_trace_files(const fs::path& lhs, const fs::path& rhs, bool as_json) {
  const std::string lhs_normalized = normalize_trace_content(read_file_binary(lhs));
  const std::string rhs_normalized = normalize_trace_content(read_file_binary(rhs));
  const bool identical = lhs_normalized == rhs_normalized;
  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.determinism-trace-diff.v1\",\n"
              << "  \"ok\": " << (identical ? "true" : "false") << ",\n"
              << "  \"lhs\": \"" << escape_json_text(lhs.string()) << "\",\n"
              << "  \"rhs\": \"" << escape_json_text(rhs.string()) << "\",\n"
              << "  \"lhs_sha3_512\": \"" << sha3_512_hex_text(lhs_normalized) << "\",\n"
              << "  \"rhs_sha3_512\": \"" << sha3_512_hex_text(rhs_normalized) << "\"\n"
              << "}\n";
  } else if (identical) {
    std::cout << "trace-normalized-identical\n";
  } else {
    std::cout << "trace-normalized-different\n";
  }
  return identical ? 0 : 1;
}

int init_project(const std::string& name) {
  if (name.empty()) {
    error("Project name cannot be empty");
    return 1;
  }
  if (!is_valid_package_name(name)) {
    error("Project name must contain only alphanumeric characters, underscores, and hyphens.");
    return 1;
  }

  fs::path project_dir(name);
  if (fs::exists(project_dir)) {
    error("Directory already exists: " + name);
    return 1;
  }

  try {
    fs::create_directories(project_dir);

    // Create main.t81
    std::ofstream main_file(project_dir / "main.t81");
    main_file << "// T81 Foundation Project: " << name << "\n"
              << "// Created by t81 init\n\n"
              << "let x = 81;\n"
              << "let y = 100;\n"
              << "let sum = x + y;\n"
              << "sum;\n";
    main_file.close();

    // Create README.md
    std::ofstream readme_file(project_dir / "README.md");
    readme_file << "# " << name << "\n\n"
                << "A ternary-native project built on the T81 Foundation stack.\n\n"
                << "## How to run\n\n"
                << "```bash\n"
                << "t81 run main.t81\n"
                << "```\n";
    readme_file.close();

    info("Project initialized in " + name);
    return 0;
  } catch (const std::exception& e) {
    error("Failed to initialize project: " + std::string(e.what()));
    return 1;
  }
}

#define COLOR_RESET "\033[0m"
#define COLOR_BOLD "\033[1m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_CYAN "\033[36m"

int run_trace_show(const TraceArgs& args) {
  if (args.args.empty()) {
    error("trace show requires a trace file");
    return 1;
  }
  std::ifstream ifs(args.args[0]);
  if (!ifs) {
    error("Could not open trace file: " + args.args[0]);
    return 1;
  }
  std::string line;
  while (std::getline(ifs, line)) {
    if (!args.no_color &&
        (line.find("fault") != std::string::npos || line.find("trap") != std::string::npos)) {
      std::cout << COLOR_RED << line << COLOR_RESET << "\n";
    } else if (!args.no_color && (line.find("allow") != std::string::npos ||
                                  line.find("satisfied") != std::string::npos)) {
      std::cout << COLOR_GREEN << line << COLOR_RESET << "\n";
    } else if (!args.no_color && line.find("PC=") != std::string::npos) {
      std::cout << COLOR_CYAN << line << COLOR_RESET << "\n";
    } else {
      std::cout << line << "\n";
    }
  }
  return 0;
}

int run_trace_diff(const TraceArgs& args) {
  if (args.args.size() < 2) {
    error("trace diff requires two trace files");
    return 1;
  }
  std::ifstream f1(args.args[0]), f2(args.args[1]);
  if (!f1 || !f2) {
    error("Could not open trace files");
    return 1;
  }
  std::string l1, l2;
  int line_num = 1;
  bool found_diff = false;
  while (std::getline(f1, l1) && std::getline(f2, l2)) {
    if (l1 != l2) {
      if (args.no_color) {
        std::cout << "Difference at line " << line_num << ":\n";
        std::cout << "- " << l1 << "\n";
        std::cout << "+ " << l2 << "\n";
      } else {
        std::cout << COLOR_BOLD << "Difference at line " << line_num << ":" << COLOR_RESET << "\n";
        std::cout << COLOR_RED << "- " << l1 << COLOR_RESET << "\n";
        std::cout << COLOR_GREEN << "+ " << l2 << COLOR_RESET << "\n";
      }
      found_diff = true;
    }
    line_num++;
  }
  while (std::getline(f1, l1)) {
    if (args.no_color) {
      std::cout << "Difference at line " << line_num << ":\n";
      std::cout << "- " << l1 << "\n";
      std::cout << "+ <missing>\n";
    } else {
      std::cout << COLOR_BOLD << "Difference at line " << line_num << ":" << COLOR_RESET << "\n";
      std::cout << COLOR_RED << "- " << l1 << COLOR_RESET << "\n";
      std::cout << COLOR_GREEN << "+ <missing>" << COLOR_RESET << "\n";
    }
    found_diff = true;
    ++line_num;
  }
  while (std::getline(f2, l2)) {
    if (args.no_color) {
      std::cout << "Difference at line " << line_num << ":\n";
      std::cout << "- <missing>\n";
      std::cout << "+ " << l2 << "\n";
    } else {
      std::cout << COLOR_BOLD << "Difference at line " << line_num << ":" << COLOR_RESET << "\n";
      std::cout << COLOR_RED << "- <missing>" << COLOR_RESET << "\n";
      std::cout << COLOR_GREEN << "+ " << l2 << COLOR_RESET << "\n";
    }
    found_diff = true;
    ++line_num;
  }
  if (!found_diff) info("Traces are identical");
  return found_diff ? 1 : 0;
}

int run_trace_replay(const TraceArgs& args) {
  bool as_json = false;
  std::vector<std::string> positional;
  positional.reserve(args.args.size());
  for (const auto& token : args.args) {
    if (token == "--json") {
      as_json = true;
      continue;
    }
    if (!token.empty() && token[0] == '-') {
      error("trace replay: unknown option '" + token + "'");
      return 1;
    }
    positional.push_back(token);
  }

  if (positional.size() < 2) {
    error("trace replay requires a .tisc file and a canonical trace file");
    return 1;
  }

  const auto json_escape_local = [](std::string_view text) {
    std::string out;
    out.reserve(text.size());
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
  };

  fs::path tisc_path = positional[0];
  fs::path trace_path = positional[1];
  auto program = t81::tisc::load_program(tisc_path.string());
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  vm->run_to_halt();
  const auto& current_trace = vm->state().trace;
  std::ifstream ifs(trace_path);
  if (!ifs) {
    if (as_json) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.trace-replay.v1\",\n"
                << "  \"ok\": false,\n"
                << "  \"kind\": \"open_error\",\n"
                << "  \"actual_entries\": " << current_trace.size() << ",\n"
                << "  \"expected_entries\": 0,\n"
                << "  \"compared_entries\": 0,\n"
                << "  \"mismatch_index\": null,\n"
                << "  \"expected\": null,\n"
                << "  \"actual\": null,\n"
                << "  \"error\": \"could not open trace file\"\n"
                << "}\n";
      return 1;
    }
    error("trace replay: could not open trace file: " + trace_path.string());
    return 1;
  }
  std::vector<std::string> saved_trace;
  std::string line;
  while (std::getline(ifs, line)) saved_trace.push_back(line);

  const auto print_json = [&](bool ok, std::string_view kind, std::size_t mismatch_index,
                              const std::string* expected, const std::string* actual) {
    std::cout << "{\n";
    std::cout << "  \"schema\": \"t81.trace-replay.v1\",\n";
    std::cout << "  \"ok\": " << (ok ? "true" : "false") << ",\n";
    std::cout << "  \"kind\": \"" << kind << "\",\n";
    std::cout << "  \"actual_entries\": " << current_trace.size() << ",\n";
    std::cout << "  \"expected_entries\": " << saved_trace.size() << ",\n";
    if (ok) {
      std::cout << "  \"compared_entries\": " << current_trace.size() << ",\n";
      std::cout << "  \"mismatch_index\": null,\n";
      std::cout << "  \"expected\": null,\n";
      std::cout << "  \"actual\": null\n";
    } else {
      std::cout << "  \"compared_entries\": " << mismatch_index << ",\n";
      std::cout << "  \"mismatch_index\": " << mismatch_index << ",\n";
      if (expected) {
        std::cout << "  \"expected\": \"" << json_escape_local(*expected) << "\",\n";
      } else {
        std::cout << "  \"expected\": null,\n";
      }
      if (actual) {
        std::cout << "  \"actual\": \"" << json_escape_local(*actual) << "\"\n";
      } else {
        std::cout << "  \"actual\": null\n";
      }
    }
    std::cout << "}\n";
  };

  if (current_trace.size() != saved_trace.size()) {
    std::size_t mismatch_index = std::min(current_trace.size(), saved_trace.size());
    std::optional<std::string> expected_line;
    std::optional<std::string> actual_line;
    if (mismatch_index < saved_trace.size()) {
      expected_line = saved_trace[mismatch_index];
    }
    if (mismatch_index < current_trace.size()) {
      actual_line = format_trace_entry(current_trace[mismatch_index]);
    }
    if (as_json) {
      print_json(false, "size_mismatch", mismatch_index, expected_line ? &*expected_line : nullptr,
                 actual_line ? &*actual_line : nullptr);
      return 1;
    }
    error("Trace size mismatch at entry " + std::to_string(mismatch_index) +
          " (actual=" + std::to_string(current_trace.size()) +
          ", expected=" + std::to_string(saved_trace.size()) + ")");
    if (expected_line) std::cerr << "Expected[" << mismatch_index << "]: " << *expected_line << "\n";
    if (actual_line) std::cerr << "Actual[" << mismatch_index << "]:   " << *actual_line << "\n";
    return 1;
  }
  for (size_t i = 0; i < current_trace.size(); ++i) {
    std::string cur = format_trace_entry(current_trace[i]);
    if (cur != saved_trace[i]) {
      if (as_json) {
        print_json(false, "entry_mismatch", i, &saved_trace[i], &cur);
        return 1;
      }
      error("Trace mismatch at entry " + std::to_string(i));
      std::cerr << "Expected[" << i << "]: " << saved_trace[i] << "\n";
      std::cerr << "Actual[" << i << "]:   " << cur << "\n";
      if (i > 0) {
        std::cerr << "Previous expected[" << (i - 1) << "]: " << saved_trace[i - 1] << "\n";
        std::cerr << "Previous actual[" << (i - 1) << "]:   "
                  << format_trace_entry(current_trace[i - 1]) << "\n";
      }
      if (i + 1 < current_trace.size()) {
        std::cerr << "Next expected[" << (i + 1) << "]: " << saved_trace[i + 1] << "\n";
        std::cerr << "Next actual[" << (i + 1) << "]:   " << format_trace_entry(current_trace[i + 1])
                  << "\n";
      }
      return 1;
    }
  }
  if (as_json) {
    print_json(true, "identical", current_trace.size(), nullptr, nullptr);
    return 0;
  }
  info("Replay successful: traces are bit-identical");
  return 0;
}

namespace {
struct ParsedTraceEntry {
  std::string raw;
  std::optional<std::uint64_t> pc;
  std::string opcode;
  std::optional<std::string> trap;
};

std::string json_escape(std::string_view text) {
  std::string out;
  out.reserve(text.size());
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

std::string csv_quote(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 2);
  out.push_back('"');
  for (char c : text) {
    if (c == '"') out.push_back('"');
    out.push_back(c);
  }
  out.push_back('"');
  return out;
}

ParsedTraceEntry parse_trace_line(std::string_view line) {
  ParsedTraceEntry parsed;
  parsed.raw = std::string(line);
  std::string trimmed = trim_copy(line);
  if (trimmed.rfind("PC=", 0) != 0) {
    return parsed;
  }

  size_t pos = 3;
  while (pos < trimmed.size() && std::isdigit(static_cast<unsigned char>(trimmed[pos]))) {
    ++pos;
  }
  if (pos == 3) {
    return parsed;
  }

  std::uint64_t pc = 0;
  const std::string pc_token = trimmed.substr(3, pos - 3);
  const char* begin = pc_token.data();
  const char* end = begin + pc_token.size();
  auto conv = std::from_chars(begin, end, pc);
  if (conv.ec != std::errc{} || conv.ptr != end) {
    return parsed;
  }
  parsed.pc = pc;

  while (pos < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[pos]))) {
    ++pos;
  }
  if (pos < trimmed.size() && trimmed.compare(pos, 5, "trap=") != 0) {
    size_t opcode_end = pos;
    while (opcode_end < trimmed.size() &&
           !std::isspace(static_cast<unsigned char>(trimmed[opcode_end]))) {
      ++opcode_end;
    }
    parsed.opcode = trimmed.substr(pos, opcode_end - pos);
    pos = opcode_end;
  }

  size_t trap_pos = trimmed.find("trap=", pos);
  if (trap_pos != std::string::npos) {
    std::string trap = trim_copy(trimmed.substr(trap_pos + 5));
    if (!trap.empty()) {
      parsed.trap = trap;
    }
  }
  return parsed;
}

int run_trace_export(const TraceArgs& args) {
  if (args.args.empty()) {
    error("trace export requires an input trace file");
    return 1;
  }

  std::string format = "json";
  std::optional<std::string> out_path;
  std::vector<std::string> positional;
  positional.reserve(args.args.size());

  for (size_t i = 0; i < args.args.size(); ++i) {
    const std::string& token = args.args[i];
    if (token == "--format") {
      if (i + 1 >= args.args.size()) {
        error("--format requires a value (json|csv)");
        return 1;
      }
      format = to_lower(args.args[++i]);
      if (format != "json" && format != "csv") {
        error("unsupported trace export format: " + format);
        return 1;
      }
      continue;
    }
    if (token == "-o" || token == "--out" || token == "--output") {
      if (i + 1 >= args.args.size()) {
        error(token + " requires a file path");
        return 1;
      }
      out_path = args.args[++i];
      continue;
    }
    if (!token.empty() && token[0] == '-') {
      error("unknown trace export option: " + token);
      return 1;
    }
    positional.push_back(token);
  }

  if (positional.size() != 1) {
    error("trace export requires exactly one input trace file");
    return 1;
  }

  std::ifstream input(positional[0]);
  if (!input) {
    error("Could not open trace file: " + positional[0]);
    return 1;
  }

  std::vector<ParsedTraceEntry> entries;
  std::string line;
  while (std::getline(input, line)) {
    entries.push_back(parse_trace_line(line));
  }

  std::ostringstream rendered;
  if (format == "json") {
    rendered << "[\n";
    for (size_t i = 0; i < entries.size(); ++i) {
      const auto& entry = entries[i];
      rendered << "  {\"schema\":\"t81.trace-export-entry.v1\",\"index\":" << (i + 1)
               << ",\"pc\":";
      if (entry.pc.has_value()) {
        rendered << *entry.pc;
      } else {
        rendered << "null";
      }
      rendered << ",\"opcode\":\"" << json_escape(entry.opcode) << "\",\"trap\":";
      if (entry.trap.has_value()) {
        rendered << "\"" << json_escape(*entry.trap) << "\"";
      } else {
        rendered << "null";
      }
      rendered << ",\"raw\":\"" << json_escape(entry.raw) << "\"}";
      if (i + 1 != entries.size()) {
        rendered << ',';
      }
      rendered << '\n';
    }
    rendered << "]\n";
  } else {
    rendered << "index,pc,opcode,trap,raw\n";
    for (size_t i = 0; i < entries.size(); ++i) {
      const auto& entry = entries[i];
      rendered << (i + 1) << ',';
      if (entry.pc.has_value()) {
        rendered << *entry.pc;
      }
      rendered << ',' << csv_quote(entry.opcode) << ',';
      rendered << csv_quote(entry.trap.has_value() ? *entry.trap : std::string{}) << ',';
      rendered << csv_quote(entry.raw) << '\n';
    }
  }

  if (out_path.has_value()) {
    std::ofstream out(*out_path, std::ios::binary | std::ios::trunc);
    if (!out) {
      error("Could not open output file: " + *out_path);
      return 1;
    }
    out << rendered.str();
    out.close();
    if (!out) {
      error("Failed writing output file: " + *out_path);
      return 1;
    }
    info("Trace exported to " + *out_path);
    return 0;
  }

  std::cout << rendered.str();
  return 0;
}

int run_trace_summary(const TraceArgs& args) {
  bool as_json = false;
  std::vector<std::string> positional;
  positional.reserve(args.args.size());
  for (const auto& token : args.args) {
    if (token == "--json") {
      as_json = true;
      continue;
    }
    if (!token.empty() && token[0] == '-') {
      error("trace summary: unknown option '" + token + "'");
      return 1;
    }
    positional.push_back(token);
  }

  if (positional.size() != 1) {
    error("trace summary requires exactly one input trace file");
    return 1;
  }

  std::ifstream input(positional[0]);
  if (!input) {
    error("Could not open trace file: " + positional[0]);
    return 1;
  }

  std::size_t line_count = 0;
  std::size_t canonical_entries = 0;
  std::size_t trap_entries = 0;
  std::optional<std::uint64_t> first_pc;
  std::optional<std::uint64_t> last_pc;
  std::map<std::string, std::size_t> opcode_counts;
  std::string line;
  while (std::getline(input, line)) {
    ++line_count;
    ParsedTraceEntry entry = parse_trace_line(line);
    if (!entry.pc.has_value()) {
      continue;
    }
    ++canonical_entries;
    if (!first_pc) {
      first_pc = entry.pc;
    }
    last_pc = entry.pc;
    if (!entry.opcode.empty()) {
      ++opcode_counts[entry.opcode];
    }
    if (entry.trap.has_value()) {
      ++trap_entries;
    }
  }

  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.trace-summary.v1\",\n"
              << "  \"path\": \"" << json_escape(positional[0]) << "\",\n"
              << "  \"line_count\": " << line_count << ",\n"
              << "  \"canonical_entries\": " << canonical_entries << ",\n"
              << "  \"trap_entries\": " << trap_entries << ",\n"
              << "  \"first_pc\": ";
    if (first_pc) {
      std::cout << *first_pc << ",\n";
    } else {
      std::cout << "null,\n";
    }
    std::cout << "  \"last_pc\": ";
    if (last_pc) {
      std::cout << *last_pc << ",\n";
    } else {
      std::cout << "null,\n";
    }
    std::cout << "  \"opcodes\": [\n";
    std::size_t emitted = 0;
    for (const auto& [opcode, count] : opcode_counts) {
      std::cout << "    {\"opcode\":\"" << json_escape(opcode) << "\",\"count\":" << count << "}";
      if (++emitted < opcode_counts.size()) {
        std::cout << ",";
      }
      std::cout << "\n";
    }
    std::cout << "  ]\n}\n";
    return 0;
  }

  std::cout << "Trace:             " << positional[0] << "\n";
  std::cout << "Lines:             " << line_count << "\n";
  std::cout << "Canonical entries: " << canonical_entries << "\n";
  std::cout << "Trap entries:      " << trap_entries << "\n";
  std::cout << "First PC:          " << (first_pc ? std::to_string(*first_pc) : std::string("<none>"))
            << "\n";
  std::cout << "Last PC:           " << (last_pc ? std::to_string(*last_pc) : std::string("<none>"))
            << "\n";
  std::cout << "Opcodes:\n";
  for (const auto& [opcode, count] : opcode_counts) {
    std::cout << "  " << opcode << ": " << count << "\n";
  }
  return 0;
}

int run_trace_filter(const TraceArgs& args) {
  bool as_json = false;
  std::optional<std::string> opcode_filter;
  std::optional<std::string> trap_filter;
  std::optional<std::uint64_t> pc_start;
  std::optional<std::uint64_t> pc_end;
  std::vector<std::string> positional;

  for (std::size_t i = 0; i < args.args.size(); ++i) {
    const std::string& token = args.args[i];
    if (token == "--json") {
      as_json = true;
    } else if (token == "--opcode") {
      if (i + 1 >= args.args.size()) {
        error("trace filter: missing value for --opcode");
        return 1;
      }
      opcode_filter = args.args[++i];
    } else if (token == "--trap") {
      if (i + 1 >= args.args.size()) {
        error("trace filter: missing value for --trap");
        return 1;
      }
      trap_filter = args.args[++i];
    } else if (token == "--pc-start" || token == "--pc-end") {
      if (i + 1 >= args.args.size()) {
        error("trace filter: missing value for " + token);
        return 1;
      }
      try {
        auto value = static_cast<std::uint64_t>(std::stoull(args.args[++i]));
        if (token == "--pc-start") {
          pc_start = value;
        } else {
          pc_end = value;
        }
      } catch (...) {
        error("trace filter: invalid numeric value for " + token);
        return 1;
      }
    } else if (!token.empty() && token[0] == '-') {
      error("trace filter: unknown option '" + token + "'");
      return 1;
    } else {
      positional.push_back(token);
    }
  }

  if (positional.size() != 1) {
    error("trace filter requires exactly one input trace file");
    return 1;
  }

  std::ifstream input(positional[0]);
  if (!input) {
    error("Could not open trace file: " + positional[0]);
    return 1;
  }

  std::vector<ParsedTraceEntry> matches;
  std::string line;
  while (std::getline(input, line)) {
    ParsedTraceEntry entry = parse_trace_line(line);
    if (opcode_filter && entry.opcode != *opcode_filter) {
      continue;
    }
    if (trap_filter) {
      if (!entry.trap || *entry.trap != *trap_filter) {
        continue;
      }
    }
    if (pc_start && (!entry.pc || *entry.pc < *pc_start)) {
      continue;
    }
    if (pc_end && (!entry.pc || *entry.pc > *pc_end)) {
      continue;
    }
    matches.push_back(std::move(entry));
  }

  if (as_json) {
    std::cout << "{\n"
              << "  \"schema\": \"t81.trace-filter.v1\",\n"
              << "  \"path\": \"" << json_escape(positional[0]) << "\",\n"
              << "  \"count\": " << matches.size() << ",\n"
              << "  \"entries\": [\n";
    for (std::size_t i = 0; i < matches.size(); ++i) {
      const auto& entry = matches[i];
      std::cout << "    {\"raw\":\"" << json_escape(entry.raw) << "\",\"pc\":";
      if (entry.pc) {
        std::cout << *entry.pc;
      } else {
        std::cout << "null";
      }
      std::cout << ",\"opcode\":\"" << json_escape(entry.opcode) << "\",\"trap\":";
      if (entry.trap) {
        std::cout << "\"" << json_escape(*entry.trap) << "\"";
      } else {
        std::cout << "null";
      }
      std::cout << "}";
      if (i + 1 < matches.size()) {
        std::cout << ",";
      }
      std::cout << "\n";
    }
    std::cout << "  ]\n}\n";
    return 0;
  }

  for (const auto& entry : matches) {
    std::cout << entry.raw << "\n";
  }
  return 0;
}

int run_trace_canonicalize(const TraceArgs& args) {
  std::optional<std::string> out_path;
  std::vector<std::string> positional;
  positional.reserve(args.args.size());
  for (std::size_t i = 0; i < args.args.size(); ++i) {
    const std::string& token = args.args[i];
    if (token == "-o" || token == "--out" || token == "--output") {
      if (i + 1 >= args.args.size()) {
        error(token + " requires a file path");
        return 1;
      }
      out_path = args.args[++i];
      continue;
    }
    if (!token.empty() && token[0] == '-') {
      error("unknown trace canonicalize option: " + token);
      return 1;
    }
    positional.push_back(token);
  }

  if (positional.size() != 1) {
    error("trace canonicalize requires exactly one input trace file");
    return 1;
  }

  const std::string normalized = normalize_trace_content(read_file_binary(positional[0]));
  if (out_path) {
    std::ofstream out(*out_path, std::ios::binary | std::ios::trunc);
    if (!out) {
      error("Could not open output file: " + *out_path);
      return 1;
    }
    out << normalized;
    if (!out.good()) {
      error("Failed writing output file: " + *out_path);
      return 1;
    }
    info("Canonical trace written to " + *out_path);
    return 0;
  }

  std::cout << normalized;
  return 0;
}
}  // namespace

int run_trace(const TraceArgs& args) {
  if (args.subcommand.empty()) {
    error("trace requires a subcommand (show|diff|replay|summary|stats|filter|canonicalize|export). Run 't81 help trace'.");
    return 1;
  }
  if (args.subcommand == "show") return run_trace_show(args);
  if (args.subcommand == "diff") return run_trace_diff(args);
  if (args.subcommand == "replay") return run_trace_replay(args);
  if (args.subcommand == "summary" || args.subcommand == "stats") return run_trace_summary(args);
  if (args.subcommand == "filter") return run_trace_filter(args);
  if (args.subcommand == "canonicalize") return run_trace_canonicalize(args);
  if (args.subcommand == "export") return run_trace_export(args);
  error("Unknown trace subcommand: " + args.subcommand + ". Run 't81 help trace'.");
  return 1;
}

int init_package(const std::string& name) {
  if (name.empty()) {
    error("Package name cannot be empty");
    return 1;
  }
  if (!is_valid_package_name(name)) {
    error("Package name must contain only alphanumeric characters, underscores, and hyphens.");
    return 1;
  }

  try {
    std::ofstream pkg_file("package.t81");
    pkg_file << "(package\n"
             << "  (name \"" << name << "\")\n"
             << "  (version \"" << T81_VERSION_STR << "\")\n"
             << "  (description \"A ternary-native T81 package\")\n"
             << "  (dependencies\n"
             << "    (std \"" << T81_VERSION_STR << "\")\n"
             << "  )\n"
             << ")\n";
    pkg_file.close();

    info("Package manifest 'package.t81' initialized for " + name);
    return 0;
  } catch (const std::exception& e) {
    error("Failed to initialize package: " + std::string(e.what()));
    return 1;
  }
}

}  // namespace t81::cli
