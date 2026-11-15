#include "t81/cli/driver.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

namespace fs = std::filesystem;

namespace {

fs::path make_temp_path(const std::string& prefix, const std::string& extension) {
  static std::mt19937_64 rng{std::random_device{}()};
  [[maybe_unused]] std::uniform_int_distribution<uint64_t> dist;
  fs::path path =
      fs::temp_directory_path() / (prefix + "-" + std::to_string(dist(rng)) + extension);
  return path;
}

void write_source(const fs::path& path, std::string_view contents) {
  std::ofstream out(path, std::ios::binary);
  out << contents;
  out.flush();
}

}  // namespace

int main() {
  auto expect = [](bool cond, const char* msg) -> bool {
    if (!cond) {
      std::cerr << "cli_pipeline_test failure: " << msg << "\n";
      return false;
    }
    return true;
  };

  const std::string minimal_program = R"(
        fn main() -> i32 {
            return 0;
        }
    )";

  [[maybe_unused]] auto success_src = make_temp_path("t81-success", ".t81");
  write_source(success_src, minimal_program);
  [[maybe_unused]] auto success_tisc = success_src;
  success_tisc.replace_extension(".tisc");

  [[maybe_unused]] int compile_rc = t81::cli::compile(success_src, success_tisc);
  if (!expect(compile_rc == 0, "compile(success) returned non-zero")) return 1;
  if (!expect(fs::exists(success_tisc), "compiled .tisc output missing")) return 1;

  [[maybe_unused]] int run_rc = t81::cli::run_tisc(success_tisc);
  if (!expect(run_rc == 0, "run_tisc(success) returned non-zero")) return 1;

  fs::remove(success_src);
  fs::remove(success_tisc);

  const std::string bad_program = R"(
        fn main() -> i32 {
            let bad: i2 = 1.5;
            return 0;
        }
    )";

  [[maybe_unused]] auto fail_src = make_temp_path("t81-fail", ".t81");
  write_source(fail_src, bad_program);
  [[maybe_unused]] auto fail_tisc = fail_src;
  fail_tisc.replace_extension(".tisc");

  [[maybe_unused]] std::ostringstream captured;
  auto* old = std::cerr.rdbuf(captured.rdbuf());
  [[maybe_unused]] int bad_rc = t81::cli::compile(fail_src, fail_tisc);
  std::cerr.rdbuf(old);

  if (!expect(bad_rc != 0, "compile(fail) unexpectedly succeeded")) return 1;
  [[maybe_unused]] std::string output = captured.str();
  if (!expect(output.find("Cannot assign initializer") != std::string::npos,
              "expected diagnostic text missing"))
    return 1;

  [[maybe_unused]] const std::string expected_diag_path = fail_src.generic_string();
  [[maybe_unused]] size_t path_pos = output.find(expected_diag_path);
  if (!expect(path_pos != std::string::npos, "diagnostic missing source path")) return 1;
  [[maybe_unused]] size_t first_colon = output.find(':', path_pos + expected_diag_path.size());
  if (!expect(first_colon != std::string::npos, "diagnostic missing line separator")) return 1;
  if (!expect(output.find(":", first_colon + 1) != std::string::npos,
              "diagnostic missing column separator"))
    return 1;
  if (!expect(output.find("error:", first_colon + 1) != std::string::npos,
              "diagnostic missing error marker"))
    return 1;

  fs::remove(fail_src);
  if (fs::exists(fail_tisc)) {
    fs::remove(fail_tisc);
  }
  return 0;
}
