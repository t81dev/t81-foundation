#include "t81/cli/driver.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/isa/binary_io.hpp"
#include "t81/isa/encoding.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
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

static void test_fallback_and_explicit_module_path_metadata() {
  const fs::path temp_root = fs::temp_directory_path() / "t81-enum-module-path-regression";
  const fs::path nested_dir = temp_root / "sub";
  std::error_code ec;
  fs::remove_all(temp_root, ec);
  fs::create_directories(nested_dir, ec);
  T81_TEST_CHECK(!ec);

  const fs::path fallback_src = nested_dir / "fallback.t81";
  const fs::path fallback_out = temp_root / "fallback.tisc";
  {
    std::ofstream out(fallback_src, std::ios::binary | std::ios::trunc);
    T81_TEST_CHECK(static_cast<bool>(out));
    out << "enum Color { Red; Blue; };\n"
        << "fn main() -> i32 { return 0; }\n";
  }

  const fs::path explicit_out = temp_root / "explicit.tisc";

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

  fs::remove_all(temp_root, ec);
}

int main() {
  test_enum_metadata_is_deterministic_and_canonical();
  test_fallback_and_explicit_module_path_metadata();
  std::cout << "e2e enum metadata determinism test passed!\n";
  return 0;
}
