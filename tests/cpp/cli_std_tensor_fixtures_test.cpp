#include "t81/cli/driver.hpp"
#include "t81/tisc/binary_io.hpp"
#include "t81/vm/traps.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

fs::path fixture_root() {
  return fs::path(__FILE__).parent_path().parent_path() / "fixtures" / "t81lang_std_tensor";
}

std::string read_text(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open file: " + path.string());
  }
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::string normalize_text(std::string text) {
  text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
  while (!text.empty() && (text.back() == '\n' || text.back() == ' ' || text.back() == '\t')) {
    text.pop_back();
  }
  return text;
}

std::string join_lines(const std::vector<std::string>& lines) {
  std::string out;
  for (size_t i = 0; i < lines.size(); ++i) {
    out += lines[i];
    if (i + 1 < lines.size()) {
      out.push_back('\n');
    }
  }
  return out;
}

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

}  // namespace

int main() {
  const fs::path root = fixture_root();
  if (!fs::exists(root)) {
    std::cerr << "Missing fixture root: " << root << "\n";
    return 1;
  }

  std::vector<fs::path> fixtures;
  for (const auto& entry : fs::directory_iterator(root)) {
    if (entry.is_regular_file() && entry.path().extension() == ".t81") {
      fixtures.push_back(entry.path());
    }
  }
  std::sort(fixtures.begin(), fixtures.end());

  if (fixtures.empty()) {
    std::cerr << "No fixtures found under: " << root << "\n";
    return 1;
  }

  auto weights_model = make_fixture_weights_model();

  for (const auto& fixture : fixtures) {
    const fs::path expected_path = fixture.parent_path() / (fixture.stem().string() + ".out");
    if (!fs::exists(expected_path)) {
      std::cerr << "Missing golden output for fixture: " << fixture.filename() << "\n";
      return 1;
    }

    const fs::path tisc_path =
        fs::temp_directory_path() / ("t81-cli-std-tensor-" + fixture.stem().string() + ".tisc");

    const int rc = t81::cli::compile(fixture, tisc_path, {}, {}, weights_model);
    if (rc != 0) {
      std::cerr << "t81::cli::compile failed for fixture " << fixture.filename() << " with code "
                << rc << "\n";
      return 1;
    }

    auto program = t81::tisc::load_program(tisc_path.string());
    program.weights_model = weights_model;
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    const auto run = vm->run_to_halt();
    if (!run.has_value()) {
      std::cerr << "VM execution failed for fixture: " << fixture.filename()
                << " (trap=" << t81::vm::to_string(run.error()) << ")\n";
      if (!program.symbol_pool.empty()) {
        std::cerr << "Program symbols:";
        for (const auto& symbol : program.symbol_pool) {
          std::cerr << " [" << symbol << "]";
        }
        std::cerr << "\n";
      }
      return 1;
    }

    const std::string actual = normalize_text(join_lines(vm->state().printed_output));
    const std::string expected = normalize_text(read_text(expected_path));
    if (actual != expected) {
      std::cerr << "Output mismatch for fixture: " << fixture.filename() << "\n";
      std::cerr << "Expected:\n" << expected << "\n";
      std::cerr << "Actual:\n" << actual << "\n";
      return 1;
    }

    std::error_code ec;
    fs::remove(tisc_path, ec);
  }

  std::cout << "cli std.tensor fixture test passed!\n";
  return 0;
}
