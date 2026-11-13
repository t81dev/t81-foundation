#include "t81/cli/driver.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/isa/binary_io.hpp"
#include "t81/isa/encoding.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "test_runtime_check.hpp"

using namespace t81;
namespace fs = std::filesystem;

static fs::path fixture_root() {
  return fs::path(__FILE__).parent_path().parent_path() / "fixtures" / "t81lang_determinism";
}

static std::string read_text(const fs::path& path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("Failed to open fixture file: " + path.string());
  }
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

static bool same_enum_metadata(const std::vector<tisc::EnumMetadata>& lhs,
                               const std::vector<tisc::EnumMetadata>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (lhs[i].enum_id != rhs[i].enum_id || lhs[i].name != rhs[i].name ||
        lhs[i].variants.size() != rhs[i].variants.size()) {
      return false;
    }
    for (std::size_t j = 0; j < lhs[i].variants.size(); ++j) {
      if (lhs[i].variants[j].name != rhs[i].variants[j].name ||
          lhs[i].variants[j].variant_id != rhs[i].variants[j].variant_id ||
          lhs[i].variants[j].payload != rhs[i].variants[j].payload) {
        return false;
      }
    }
  }
  return true;
}

static bool sorted_by_enum_id_then_name(const std::vector<tisc::EnumMetadata>& entries) {
  return std::is_sorted(entries.begin(), entries.end(),
                        [](const tisc::EnumMetadata& lhs, const tisc::EnumMetadata& rhs) {
                          if (lhs.enum_id != rhs.enum_id) {
                            return lhs.enum_id < rhs.enum_id;
                          }
                          return lhs.name < rhs.name;
                        });
}

static void test_enum_metadata_is_deterministic_and_canonical() {
  const fs::path source_path = fixture_root() / "13_match_enum_payload_guard_print.t81";
  const std::string source = read_text(source_path);

  const auto program_a = cli::build_program_from_source(source, source_path.string());
  const auto program_b = cli::build_program_from_source(source, source_path.string());
  T81_TEST_CHECK(program_a.has_value());
  T81_TEST_CHECK(program_b.has_value());

  T81_TEST_CHECK(!program_a->enum_metadata.empty());
  T81_TEST_CHECK(!program_b->enum_metadata.empty());
  T81_TEST_CHECK(same_enum_metadata(program_a->enum_metadata, program_b->enum_metadata));
  T81_TEST_CHECK(sorted_by_enum_id_then_name(program_a->enum_metadata));

  const auto bytecode_a = tisc::encode(*program_a);
  const auto bytecode_b = tisc::encode(*program_b);
  T81_TEST_CHECK(bytecode_a == bytecode_b);
}

struct OwnedTempDir {
  fs::path path;
  bool owned = false;

  OwnedTempDir() = default;
  OwnedTempDir(const OwnedTempDir&) = delete;
  OwnedTempDir& operator=(const OwnedTempDir&) = delete;
  OwnedTempDir(OwnedTempDir&& other) noexcept : path(std::move(other.path)), owned(other.owned) {
    other.owned = false;
    other.path.clear();
  }
  OwnedTempDir& operator=(OwnedTempDir&& other) noexcept {
    if (this != &other) {
      cleanup();
      path = std::move(other.path);
      owned = other.owned;
      other.owned = false;
      other.path.clear();
    }
    return *this;
  }
  ~OwnedTempDir() { cleanup(); }

  void cleanup() noexcept {
    if (owned && !path.empty()) {
      std::error_code ec;
      fs::remove_all(path, ec);
      owned = false;
    }
  }
};

static bool try_create_exclusive_dir(const fs::path& candidate, OwnedTempDir& out,
                                     std::error_code& ec) {
  out.cleanup();
  out.path.clear();
  ec.clear();
  if (fs::create_directory(candidate, ec) && !ec) {
    out.path = candidate;
    out.owned = true;
    return true;
  }
  return false;
}

static OwnedTempDir create_exclusive_temp_dir(const std::string& prefix) {
  const fs::path base = fs::temp_directory_path();
  std::random_device rd;
  std::mt19937_64 rng(
      (static_cast<std::uint64_t>(rd()) << 32) ^
      static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()));

  for (int attempt = 0; attempt < 128; ++attempt) {
    std::ostringstream name;
    name << prefix << "-" << std::hex << std::setw(16) << std::setfill('0') << rng();
    OwnedTempDir dir;
    std::error_code ec;
    if (try_create_exclusive_dir(base / name.str(), dir, ec)) {
      return dir;
    }
    if (ec && ec != std::errc::file_exists) {
      std::error_code exists_ec;
      if (!fs::exists(base / name.str(), exists_ec)) {
        throw std::runtime_error("Failed to create exclusive temp directory: " + ec.message());
      }
    }
  }
  throw std::runtime_error("Exhausted attempts creating exclusive temp directory");
}

static void test_exclusive_dir_preserves_preexisting_contents() {
  OwnedTempDir existing_dir = create_exclusive_temp_dir("t81-enum-preexisting");
  T81_TEST_CHECK(existing_dir.owned);

  const fs::path sentinel = existing_dir.path / "keep.txt";
  {
    std::ofstream out(sentinel, std::ios::binary | std::ios::trunc);
    T81_TEST_CHECK(static_cast<bool>(out));
    out << "preserve-me\n";
  }

  {
    OwnedTempDir collided;
    std::error_code ec;
    const bool created = try_create_exclusive_dir(existing_dir.path, collided, ec);
    T81_TEST_CHECK(!created);
    T81_TEST_CHECK(!collided.owned);
  }

  T81_TEST_CHECK(fs::exists(existing_dir.path));
  T81_TEST_CHECK(fs::exists(sentinel));
  T81_TEST_CHECK(read_text(sentinel) == "preserve-me\n");
}

static void test_fallback_and_explicit_module_path_metadata() {
  OwnedTempDir temp_root = create_exclusive_temp_dir("t81-enum-module-path");
  T81_TEST_CHECK(temp_root.owned);
  const fs::path nested_dir = temp_root.path / "sub";
  std::error_code ec;
  T81_TEST_CHECK(fs::create_directory(nested_dir, ec) && !ec);

  const fs::path fallback_src = nested_dir / "fallback.t81";
  const fs::path fallback_out = temp_root.path / "fallback.tisc";
  {
    std::ofstream out(fallback_src, std::ios::binary | std::ios::trunc);
    T81_TEST_CHECK(static_cast<bool>(out));
    out << "enum Color { Red; Blue; };\n"
        << "fn main() -> i32 { return 0; }\n";
  }

  const fs::path explicit_out = temp_root.path / "explicit.tisc";

  T81_TEST_CHECK(cli::compile(fallback_src, fallback_out, "", "", nullptr, false) == 0);
  T81_TEST_CHECK(cli::compile(fallback_src, explicit_out, "", "Core.Explicit", nullptr, false) ==
                 0);

  const tisc::Program fallback_prog = tisc::load_program(fallback_out.string());
  T81_TEST_CHECK(!fallback_prog.type_aliases.empty());
  const std::string& fallback_mod = fallback_prog.type_aliases.front().module_path;
  T81_TEST_CHECK(fallback_mod == fallback_src.generic_string());
  T81_TEST_CHECK(fallback_mod.find('/') != std::string::npos);
  T81_TEST_CHECK(fallback_mod.find('\\') == std::string::npos);

  const tisc::Program explicit_prog = tisc::load_program(explicit_out.string());
  T81_TEST_CHECK(!explicit_prog.type_aliases.empty());
  T81_TEST_CHECK(explicit_prog.type_aliases.front().module_path == "Core.Explicit");

  // Also verify AST-level explicit module(...) attribute preserves value over fallback
  // _source_name.
  const std::string ast_src = "enum Mode { Fast; Safe; };\nfn main() -> i32 { return 0; }\n";
  frontend::Lexer lexer(ast_src);
  frontend::Parser parser(lexer, fallback_src.generic_string());
  auto stmts = parser.parse();
  T81_TEST_CHECK(!parser.had_error());
  T81_TEST_CHECK(!stmts.empty());
  const auto* enum_decl = dynamic_cast<const frontend::EnumDecl*>(stmts.front().get());
  T81_TEST_CHECK(enum_decl != nullptr);
  std::vector<frontend::EnumDecl::Variant> moved_variants;
  for (const auto& v : enum_decl->variants) {
    moved_variants.push_back(frontend::EnumDecl::Variant{v.name, nullptr});
  }
  stmts[0] = std::make_unique<frontend::EnumDecl>(enum_decl->name, std::move(moved_variants),
                                                  enum_decl->schema_version,
                                                  std::optional<std::string>("Core.AttrModule"));

  frontend::SemanticAnalyzer analyzer(stmts, fallback_src.generic_string());
  analyzer.analyze();
  T81_TEST_CHECK(!analyzer.had_error());
  frontend::IRGenerator ir_gen;
  ir_gen.attach_semantic_analyzer(&analyzer);
  auto ir = ir_gen.generate(stmts);
  tisc::BinaryEmitter emitter;
  tisc::Program ast_prog = emitter.emit(ir);
  T81_TEST_CHECK(!ast_prog.type_aliases.empty());
  T81_TEST_CHECK(ast_prog.type_aliases.front().module_path == "Core.AttrModule");
}

int main() {
  test_enum_metadata_is_deterministic_and_canonical();
  test_exclusive_dir_preserves_preexisting_contents();
  test_fallback_and_explicit_module_path_metadata();
  std::cout << "e2e enum metadata determinism test passed!\n";
  return 0;
}
