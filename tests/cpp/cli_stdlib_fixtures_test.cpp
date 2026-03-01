#include "test_fixture_util.hpp"

#include "t81/cli/driver.hpp"
#include "t81/isa/binary_io.hpp"
#include "t81/vm/traps.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace t81::test;

namespace {

struct Module {
  std::string id;
  std::string fixture_dir;
  bool needs_weights;
};

// One entry per standard library module.  Add new modules here as they are promoted.
// needs_weights=true modules receive a fixture weights model (mat_a, mat_b 2x2 tensors).
const std::vector<Module> STDLIB_MODULES = {
    {"core", "t81lang_std_core", false},
    {"math", "t81lang_std_math", false},
    {"bytes", "t81lang_std_bytes", false},
    {"collections", "t81lang_std_collections", false},
    {"polynomial", "t81lang_std_polynomial", false},
    {"runtime", "t81lang_std_runtime", false},
    {"symbol", "t81lang_std_symbol", false},
    {"symbolic", "t81lang_std_symbolic", false},
    {"tensor", "t81lang_std_tensor", true},
    {"text", "t81lang_std_text", false},
};

std::shared_ptr<t81::weights::ModelFile> make_fixture_weights_model() {
  auto model = std::make_shared<t81::weights::ModelFile>();

  t81::weights::NativeTensor mat_a;
  mat_a.shape = {2, 2};
  mat_a.trits = 4;
  mat_a.data = {40};  // [1, 0, -1, 1]
  model->native["mat_a"] = mat_a;

  t81::weights::NativeTensor mat_b;
  mat_b.shape = {2, 2};
  mat_b.trits = 4;
  mat_b.data = {67};  // [1, 1, 0, -1]
  model->native["mat_b"] = mat_b;

  return model;
}

bool run_module(const Module& mod) {
  const fs::path root =
      fs::path(__FILE__).parent_path().parent_path() / "fixtures" / mod.fixture_dir;

  if (!fs::exists(root)) {
    std::cerr << "[" << mod.id << "] Missing fixture root: " << root << "\n";
    return false;
  }

  std::vector<fs::path> fixtures;
  for (const auto& entry : fs::directory_iterator(root)) {
    if (entry.is_regular_file() && entry.path().extension() == ".t81")
      fixtures.push_back(entry.path());
  }
  std::sort(fixtures.begin(), fixtures.end());

  if (fixtures.empty()) {
    std::cerr << "[" << mod.id << "] No fixtures found under: " << root << "\n";
    return false;
  }

  auto weights_model = mod.needs_weights ? make_fixture_weights_model() : nullptr;

  for (const auto& fixture : fixtures) {
    const fs::path expected_path = fixture.parent_path() / (fixture.stem().string() + ".out");
    if (!fs::exists(expected_path)) {
      std::cerr << "[" << mod.id << "] Missing golden output for: " << fixture.filename() << "\n";
      return false;
    }

    const fs::path tisc_path = fs::temp_directory_path() /
                               ("t81-cli-std-" + mod.id + "-" + fixture.stem().string() + ".tisc");

    const int rc = mod.needs_weights ? t81::cli::compile(fixture, tisc_path, {}, {}, weights_model)
                                     : t81::cli::compile(fixture, tisc_path);

    if (rc != 0) {
      std::cerr << "[" << mod.id << "] compile failed for " << fixture.filename() << " (code " << rc
                << ")\n";
      return false;
    }

    auto program = t81::tisc::load_program(tisc_path.string());
    if (mod.needs_weights) program.weights_model = weights_model;

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    const auto run = vm->run_to_halt();

    if (!run.has_value()) {
      std::cerr << "[" << mod.id << "] VM failed for " << fixture.filename()
                << " (trap=" << t81::vm::to_string(run.error()) << ")";
      if (mod.needs_weights && !program.symbol_pool.empty()) {
        std::cerr << " symbols:";
        for (const auto& s : program.symbol_pool) std::cerr << " [" << s << "]";
      }
      std::cerr << "\n";
      return false;
    }

    const std::string actual = normalize_text(join_lines(vm->state().printed_output));
    const std::string expected = normalize_text(read_text(expected_path));
    if (actual != expected) {
      std::cerr << "[" << mod.id << "] Output mismatch for " << fixture.filename() << "\n";
      std::cerr << "Expected:\n" << expected << "\n";
      std::cerr << "Actual:\n" << actual << "\n";
      return false;
    }

    std::error_code ec;
    fs::remove(tisc_path, ec);
  }

  std::cout << "cli std." << mod.id << " fixture test passed!\n";
  return true;
}

}  // namespace

int main() {
  for (const auto& mod : STDLIB_MODULES) {
    if (!run_module(mod)) return 1;
  }
  return 0;
}
