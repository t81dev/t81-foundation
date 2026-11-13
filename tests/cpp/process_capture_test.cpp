#include "tools/cli/core/process_capture.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {

fs::path current_executable_path(const char* argv0) {
#if defined(_WIN32)
  std::wstring buf(1024, L'\0');
  DWORD len = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
  if (len > 0 && len < buf.size()) {
    return fs::path(buf.substr(0, len));
  }
#elif defined(__APPLE__)
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  if (size > 0) {
    std::string buf(size, '\0');
    if (_NSGetExecutablePath(buf.data(), &size) == 0) {
      return fs::canonical(fs::path(buf.c_str()));
    }
  }
#else
  std::error_code ec;
  fs::path self = fs::read_symlink("/proc/self/exe", ec);
  if (!ec) {
    return self;
  }
#endif
  return fs::absolute(fs::path(argv0));
}

struct ScopedTempDir {
  fs::path path;
  explicit ScopedTempDir(const fs::path& p) : path(p) {
    std::error_code ec;
    fs::create_directories(path, ec);
  }
  ~ScopedTempDir() {
    std::error_code ec;
    fs::remove_all(path, ec);
  }
};

}  // namespace

int main(int argc, char* argv[]) {
  if (argc >= 3 && std::string(argv[1]) == "--child-mode") {
    const int exit_code = std::atoi(argv[2]);
    for (int i = 3; i < argc; ++i) {
      std::cout << "ARG[" << (i - 3) << "]:" << argv[i] << "\n";
    }
    std::error_code ec;
    std::cout << "CWD:" << fs::current_path(ec).filename().string() << "\n";
    if (const char* env_val = std::getenv("T81_CAPTURE_TEST_ENV")) {
      std::cout << "ENV:" << env_val << "\n";
    }
    std::cerr << "STDERR_OK\n";
    return exit_code;
  }

  const fs::path self_exe = current_executable_path(argv[0]);
  const fs::path base_tmp = fs::temp_directory_path() / "t81 process capture space dir";
  ScopedTempDir dir_guard(base_tmp);
  const fs::path cwd_dir = base_tmp / "sub cwd";
  std::error_code ec;
  fs::create_directories(cwd_dir, ec);
  if (ec) {
    std::cerr << "Failed to create cwd_dir: " << ec.message() << "\n";
    return 1;
  }

  const fs::path copied_exe =
      base_tmp / ("child helper with spaces" + self_exe.extension().string());
  fs::copy_file(self_exe, copied_exe, fs::copy_options::overwrite_existing, ec);
  if (ec) {
    std::cerr << "Failed to copy test executable: " << ec.message() << "\n";
    return 1;
  }

  const std::vector<std::string> child_args = {
      copied_exe.string(),      "--child-mode",          "42", "hello world",
      "quoted \"inner\" value", "trailing\\backslash\\", "",
  };
  const auto res =
      t81::cli::run_process_capture(child_args, cwd_dir, {{"T81_CAPTURE_TEST_ENV", "override-ok"}});

  if (res.exit_code != 42) {
    std::cerr << "Expected exit_code 42, got " << res.exit_code << "\n";
    return 1;
  }
  const std::string expected_stdout = "ARG[0]:hello world\n"
                                      "ARG[1]:quoted \"inner\" value\n"
                                      "ARG[2]:trailing\\backslash\\\n"
                                      "ARG[3]:\n"
                                      "CWD:sub cwd\n"
                                      "ENV:override-ok\n";
  // Normalize CRLF on Windows if text mode ever differs; binary mode preserves LF from child.
  std::string norm_stdout;
  norm_stdout.reserve(res.stdout_text.size());
  for (char c : res.stdout_text) {
    if (c != '\r') {
      norm_stdout.push_back(c);
    }
  }
  if (norm_stdout != expected_stdout) {
    std::cerr << "Unexpected stdout:\n"
              << norm_stdout << "\nExpected:\n"
              << expected_stdout << "\n";
    return 1;
  }
  if (res.stderr_text.find("STDERR_OK") == std::string::npos) {
    std::cerr << "Unexpected stderr: " << res.stderr_text << "\n";
    return 1;
  }

  const auto zero_res = t81::cli::run_process_capture({copied_exe.string(), "--child-mode", "0"});
  if (zero_res.exit_code != 0) {
    std::cerr << "Expected exit_code 0, got " << zero_res.exit_code << "\n";
    return 1;
  }

  return 0;
}
