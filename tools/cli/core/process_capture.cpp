#include "tools/cli/core/process_capture.hpp"

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string_view>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cwctype>
#include <map>
#else
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace t81::cli {
namespace {

struct ScopedCaptureFile {
  fs::path path;

  explicit ScopedCaptureFile(const std::string& hint, const std::string& ext) {
    static std::atomic<unsigned long long> counter{0};
    const unsigned long long seq = counter.fetch_add(1, std::memory_order_relaxed);
#if defined(_WIN32)
    const unsigned long long pid = static_cast<unsigned long long>(GetCurrentProcessId());
#else
    const unsigned long long pid = static_cast<unsigned long long>(::getpid());
#endif
    std::error_code ec;
    const fs::path base = fs::temp_directory_path(ec);
    const fs::path dir = ec ? fs::current_path(ec) : base;
    path = dir / ("t81-" + hint + "-" + std::to_string(pid) + "-" + std::to_string(seq) + ext);
    std::ofstream touch(path, std::ios::binary | std::ios::trunc);
  }

  ~ScopedCaptureFile() {
    std::error_code ec;
    fs::remove(path, ec);
  }

  ScopedCaptureFile(const ScopedCaptureFile&) = delete;
  ScopedCaptureFile& operator=(const ScopedCaptureFile&) = delete;
};

#if defined(_WIN32)
bool utf8_to_wide(std::string_view input, std::wstring& out) {
  out.clear();
  if (input.empty()) {
    return true;
  }
  const int needed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(),
                                         static_cast<int>(input.size()), nullptr, 0);
  if (needed <= 0) {
    return false;
  }
  out.resize(static_cast<std::size_t>(needed));
  const int written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(),
                                          static_cast<int>(input.size()), out.data(), needed);
  return written == needed;
}

std::wstring quote_windows_arg(std::wstring_view arg) {
  const bool needs_quotes =
      arg.empty() || arg.find_first_of(L" \t\n\v\"") != std::wstring_view::npos;
  if (!needs_quotes) {
    return std::wstring(arg);
  }
  std::wstring out;
  out.push_back(L'"');
  std::size_t backslashes = 0;
  for (const wchar_t ch : arg) {
    if (ch == L'\\') {
      ++backslashes;
    } else if (ch == L'"') {
      out.append(backslashes * 2 + 1, L'\\');
      out.push_back(L'"');
      backslashes = 0;
    } else {
      out.append(backslashes, L'\\');
      out.push_back(ch);
      backslashes = 0;
    }
  }
  out.append(backslashes * 2, L'\\');
  out.push_back(L'"');
  return out;
}

struct CaseInsensitiveWideLess {
  bool operator()(const std::wstring& a, const std::wstring& b) const {
    const std::size_t n = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t i = 0; i < n; ++i) {
      const wchar_t ca = static_cast<wchar_t>(std::towlower(a[i]));
      const wchar_t cb = static_cast<wchar_t>(std::towlower(b[i]));
      if (ca != cb) {
        return ca < cb;
      }
    }
    return a.size() < b.size();
  }
};

bool build_windows_environment(const std::vector<std::pair<std::string, std::string>>& overrides,
                               std::vector<wchar_t>& block) {
  block.clear();
  std::map<std::wstring, std::wstring, CaseInsensitiveWideLess> env_map;
  LPWCH raw_env = GetEnvironmentStringsW();
  if (raw_env == nullptr) {
    return false;
  }
  for (const wchar_t* cur = raw_env; *cur != L'\0';) {
    const std::wstring entry(cur);
    cur += entry.size() + 1;
    const std::size_t eq = entry.find(L'=', 1);
    if (eq != std::wstring::npos) {
      env_map[entry.substr(0, eq)] = entry.substr(eq + 1);
    }
  }
  FreeEnvironmentStringsW(raw_env);

  for (const auto& [k, v] : overrides) {
    std::wstring wk;
    std::wstring wv;
    if (!utf8_to_wide(k, wk) || !utf8_to_wide(v, wv)) {
      return false;
    }
    env_map[wk] = wv;
  }

  for (const auto& [k, v] : env_map) {
    block.insert(block.end(), k.begin(), k.end());
    block.push_back(L'=');
    block.insert(block.end(), v.begin(), v.end());
    block.push_back(L'\0');
  }
  block.push_back(L'\0');
  return true;
}
#endif

}  // namespace

ProcessCaptureResult run_process_capture(
    const std::vector<std::string>& argv, const std::optional<fs::path>& cwd,
    const std::vector<std::pair<std::string, std::string>>& env_overrides) {
  ProcessCaptureResult result;
  if (argv.empty()) {
    return result;
  }
#if defined(_WIN32)
  ScopedCaptureFile out_file("capture", ".out");
  ScopedCaptureFile err_file("capture", ".err");

  std::wstring cmdline;
  for (std::size_t i = 0; i < argv.size(); ++i) {
    std::wstring warg;
    if (!utf8_to_wide(argv[i], warg)) {
      return result;
    }
    if (i != 0) {
      cmdline.push_back(L' ');
    }
    cmdline += quote_windows_arg(warg);
  }

  std::wstring cwd_w;
  const wchar_t* cwd_ptr = nullptr;
  if (cwd.has_value()) {
    cwd_w = cwd->wstring();
    cwd_ptr = cwd_w.c_str();
  }

  std::vector<wchar_t> env_block;
  void* env_ptr = nullptr;
  DWORD create_flags = 0;
  if (!env_overrides.empty()) {
    if (!build_windows_environment(env_overrides, env_block)) {
      return result;
    }
    env_ptr = env_block.data();
    create_flags |= CREATE_UNICODE_ENVIRONMENT;
  }

  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;

  HANDLE hOut =
      CreateFileW(out_file.path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hOut == INVALID_HANDLE_VALUE) {
    return result;
  }
  HANDLE hErr =
      CreateFileW(err_file.path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hErr == INVALID_HANDLE_VALUE) {
    CloseHandle(hOut);
    return result;
  }

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdOutput = hOut;
  si.hStdError = hErr;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

  PROCESS_INFORMATION pi{};
  const BOOL ok = CreateProcessW(nullptr, cmdline.data(), nullptr, nullptr, TRUE, create_flags,
                                 env_ptr, cwd_ptr, &si, &pi);
  CloseHandle(hOut);
  CloseHandle(hErr);
  if (!ok) {
    return result;
  }

  const DWORD wait_rc = WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 1;
  const BOOL got_exit = GetExitCodeProcess(pi.hProcess, &exit_code);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  if (wait_rc == WAIT_FAILED || !got_exit) {
    return result;
  }
  result.exit_code = static_cast<int>(exit_code);
  std::ifstream out_stream(out_file.path, std::ios::binary);
  std::ifstream err_stream(err_file.path, std::ios::binary);
  result.stdout_text.assign((std::istreambuf_iterator<char>(out_stream)),
                            std::istreambuf_iterator<char>());
  result.stderr_text.assign((std::istreambuf_iterator<char>(err_stream)),
                            std::istreambuf_iterator<char>());
  return result;
#else
  ScopedCaptureFile out_file("capture", ".out");
  ScopedCaptureFile err_file("capture", ".err");

  std::vector<std::string> argv_storage = argv;
  std::vector<char*> argv_ptrs;
  argv_ptrs.reserve(argv_storage.size() + 1);
  for (auto& item : argv_storage) {
    argv_ptrs.push_back(item.data());
  }
  argv_ptrs.push_back(nullptr);

  pid_t pid = fork();
  if (pid == 0) {
    int out_fd = open(out_file.path.c_str(), O_WRONLY | O_TRUNC);
    int err_fd = open(err_file.path.c_str(), O_WRONLY | O_TRUNC);
    if (out_fd == -1 || err_fd == -1) {
      _exit(127);
    }
    if (dup2(out_fd, STDOUT_FILENO) == -1 || dup2(err_fd, STDERR_FILENO) == -1) {
      _exit(127);
    }
    close(out_fd);
    close(err_fd);
    if (cwd && chdir(cwd->c_str()) != 0) {
      _exit(127);
    }
    for (const auto& [key, value] : env_overrides) {
      setenv(key.c_str(), value.c_str(), 1);
    }
    execvp(argv_ptrs[0], argv_ptrs.data());
    _exit(127);
  }
  if (pid < 0) {
    return result;
  }

  int status = 0;
  if (waitpid(pid, &status, 0) == -1) {
    return result;
  }
  if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    result.exit_code = 128 + WTERMSIG(status);
  } else {
    result.exit_code = status;
  }
  std::ifstream out_stream(out_file.path, std::ios::binary);
  std::ifstream err_stream(err_file.path, std::ios::binary);
  result.stdout_text.assign((std::istreambuf_iterator<char>(out_stream)),
                            std::istreambuf_iterator<char>());
  result.stderr_text.assign((std::istreambuf_iterator<char>(err_stream)),
                            std::istreambuf_iterator<char>());
  return result;
#endif
}

}  // namespace t81::cli
