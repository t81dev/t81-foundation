#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

static bool is_hex_hash(const std::string& value) {
  if (value.size() != 64) return false;
  for (char c : value) {
    if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
  }
  return true;
}

int main(int argc, char* argv[]) {
  auto expect = [](bool cond, const char* msg) -> bool {
    if (!cond) {
      std::cerr << "cli_repro_hash_test failure: " << msg << "\n";
      return false;
    }
    return true;
  };

  if (argc < 2) {
    std::cerr << "cli_repro_hash_test expects argv[1]=path to t81 binary\n";
    return 1;
  }

  const fs::path t81_bin = fs::path(argv[1]);
  if (!fs::exists(t81_bin)) {
    std::cerr << "Missing t81 binary at: " << t81_bin << "\n";
    return 1;
  }

  const fs::path out_path = fs::temp_directory_path() / "t81-cli-repro-hash.out";
  std::string cmd =
      "\"" + t81_bin.string() + "\" internal repro-hash tests/fixtures/t81lang_determinism > \"" +
      out_path.string() + "\"";
#if defined(_WIN32)
  cmd = "\"" + cmd + "\"";
#endif
  const int rc = std::system(cmd.c_str());
  if (rc != 0) {
    std::cerr << "t81 internal repro-hash returned non-zero: " << rc << "\n";
    return 1;
  }

  std::ifstream in(out_path);
  if (!expect(static_cast<bool>(in), "expected repro hash output file")) return 1;
  std::string line;
  std::string last_non_empty;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (!line.empty()) {
      last_non_empty = line;
    }
  }
  if (!is_hex_hash(last_non_empty)) {
    std::cerr << "unexpected repro hash output: " << last_non_empty << "\n";
    return 1;
  }

  std::error_code ec;
  fs::remove(out_path, ec);
  std::cout << "cli repro hash test passed!\n";
  return 0;
}
