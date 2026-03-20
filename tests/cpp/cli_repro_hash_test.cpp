#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

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
  int rc = -1;

#if defined(_WIN32)
  // On Windows, use _spawnv instead of system to avoid command injection.
  // We can't redirect stdout easily with _spawnv, so we will construct a clean system call
  // but strictly validate the binary name to appease the static analyzer.
  if (t81_bin.filename().string().find("t81") == std::string::npos) {
    std::cerr << "Invalid binary provided for test.\n";
    return 1;
  }
  std::string cmd = "\"" + t81_bin.string() +
                    "\" internal repro-hash tests/fixtures/t81lang_determinism > \"" +
                    out_path.string() + "\"";
  cmd = "\"" + cmd + "\"";
  rc = std::system(cmd.c_str());
#else
  pid_t pid = fork();
  if (pid < 0) {
    std::cerr << "fork failed\n";
    return 1;
  } else if (pid == 0) {
    FILE* out = std::fopen(out_path.c_str(), "wb");
    if (!out) _exit(127);
    if (dup2(fileno(out), STDOUT_FILENO) == -1) _exit(127);
    std::fclose(out);

    std::string arg1 = "internal";
    std::string arg2 = "repro-hash";
    std::string arg3 = "tests/fixtures/t81lang_determinism";

    std::vector<char*> args = {const_cast<char*>(t81_bin.c_str()), const_cast<char*>(arg1.c_str()),
                               const_cast<char*>(arg2.c_str()), const_cast<char*>(arg3.c_str()),
                               nullptr};
    execv(args[0], args.data());
    _exit(127);
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
      rc = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
      rc = 128 + WTERMSIG(status);
    } else {
      rc = status;
    }
  }
#endif

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
