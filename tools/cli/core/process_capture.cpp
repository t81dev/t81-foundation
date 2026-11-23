#include "tools/cli/core/process_capture.hpp"
#include "tools/cli/core/process_capture_detail.hpp"

#include <array>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <aclapi.h>
#include <cwctype>
#include <map>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;
namespace {
#ifdef _WIN32
[[noreturn]] void system_failure(const char* operation) {
  throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), operation);
}
struct UniqueHandle {
  HANDLE value = nullptr;
  explicit UniqueHandle(HANDLE handle = nullptr) : value(handle) {}
  ~UniqueHandle() { reset(); }
  UniqueHandle(const UniqueHandle&) = delete;
  UniqueHandle& operator=(const UniqueHandle&) = delete;
  void reset() noexcept {
    if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value);
    value = nullptr;
  }
};
#else
[[noreturn]] void system_failure(const char* operation) {
  throw std::system_error(errno, std::generic_category(), operation);
}
#endif
}  // namespace

namespace t81::cli::detail {
CaptureFile CaptureFile::create_exclusive(const fs::path& path) {
#ifdef _WIN32
  UniqueHandle token;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token.value))
    system_failure("OpenProcessToken");
  DWORD bytes = 0;
  if (GetTokenInformation(token.value, TokenUser, nullptr, 0, &bytes) ||
      GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytes == 0)
    system_failure("GetTokenInformation size");
  std::vector<BYTE> token_info(bytes);
  if (!GetTokenInformation(token.value, TokenUser, token_info.data(), bytes, &bytes))
    system_failure("GetTokenInformation");
  const auto* user = reinterpret_cast<const TOKEN_USER*>(token_info.data());
  const DWORD acl_size =
      static_cast<DWORD>(sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
      GetLengthSid(user->User.Sid);
  std::vector<BYTE> acl_storage(acl_size);
  auto* acl = reinterpret_cast<PACL>(acl_storage.data());
  SECURITY_DESCRIPTOR descriptor{};
  if (!InitializeAcl(acl, acl_size, ACL_REVISION) ||
      !AddAccessAllowedAce(acl, ACL_REVISION, FILE_ALL_ACCESS, user->User.Sid) ||
      !InitializeSecurityDescriptor(&descriptor, SECURITY_DESCRIPTOR_REVISION) ||
      !SetSecurityDescriptorDacl(&descriptor, TRUE, acl, FALSE) ||
      !SetSecurityDescriptorControl(&descriptor, SE_DACL_PROTECTED, SE_DACL_PROTECTED))
    system_failure("private capture DACL");
  SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), &descriptor, FALSE};
  HANDLE handle = CreateFileW(
      path.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, &security, CREATE_NEW,
      FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
  if (handle == INVALID_HANDLE_VALUE) system_failure("exclusive capture creation");
  CaptureFile file(handle);
  PACL actual_acl = nullptr;
  PSECURITY_DESCRIPTOR actual_descriptor = nullptr;
  const DWORD security_error =
      GetSecurityInfo(handle, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr,
                      &actual_acl, nullptr, &actual_descriptor);
  if (security_error != ERROR_SUCCESS)
    throw std::system_error(static_cast<int>(security_error), std::system_category(),
                            "verify capture DACL");
  std::unique_ptr<void, decltype(&LocalFree)> descriptor_owner(actual_descriptor, &LocalFree);
  SECURITY_DESCRIPTOR_CONTROL control = 0;
  DWORD revision = 0;
  ACL_SIZE_INFORMATION info{};
  void* raw_ace = nullptr;
  if (!GetSecurityDescriptorControl(actual_descriptor, &control, &revision) ||
      !(control & SE_DACL_PROTECTED) || !actual_acl ||
      !GetAclInformation(actual_acl, &info, sizeof(info), AclSizeInformation) ||
      info.AceCount != 1 || !GetAce(actual_acl, 0, &raw_ace))
    throw std::runtime_error("capture DACL is not private");
  auto* ace = static_cast<ACCESS_ALLOWED_ACE*>(raw_ace);
  if (ace->Header.AceType != ACCESS_ALLOWED_ACE_TYPE || ace->Mask != FILE_ALL_ACCESS ||
      !EqualSid(&ace->SidStart, user->User.Sid))
    throw std::runtime_error("capture DACL is not restricted to the current user");
  return file;
#else
  const int fd = ::open(path.c_str(), O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);
  if (fd < 0) system_failure("exclusive capture creation");
  CaptureFile file(fd);
  // Keep only the opened object. No later read or cleanup resolves this pathname.
  if (::unlink(path.c_str()) != 0) system_failure("unlink owned capture");
  if (::fchmod(fd, 0600) != 0) system_failure("private capture permissions");
  struct stat info {};
  if (::fstat(fd, &info) != 0) system_failure("verify capture permissions");
  if ((info.st_mode & 0777) != 0600 || info.st_uid != ::geteuid())
    throw std::runtime_error("capture permissions are not private to the current user");
  if (fd <= STDERR_FILENO) {
    const int promoted = ::fcntl(fd, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
    if (promoted < 0) system_failure("promote capture descriptor");
    file = CaptureFile(promoted);
  }
  return file;
#endif
}

CaptureFile CaptureFile::create_temporary() {
  const fs::path directory = fs::temp_directory_path();
  std::random_device random;
  static std::atomic<unsigned long long> sequence{0};
  for (unsigned attempt = 0; attempt < 128; ++attempt) {
    const auto name = "t81-capture-" + std::to_string(random()) + "-" + std::to_string(random()) +
                      "-" + std::to_string(random()) + "-" + std::to_string(random()) + "-" +
                      std::to_string(sequence.fetch_add(1));
    try {
      return create_exclusive(directory / name);
    } catch (const std::system_error& error) {
#ifdef _WIN32
      if (error.code().value() != ERROR_FILE_EXISTS && error.code().value() != ERROR_ALREADY_EXISTS)
        throw;
#else
      if (error.code().value() != EEXIST) throw;
#endif
    }
  }
  throw std::runtime_error("capture filename collisions exhausted");
}

CaptureFile::CaptureFile(CaptureFile&& other) noexcept {
#ifdef _WIN32
  handle_ = std::exchange(other.handle_, nullptr);
#else
  handle_ = std::exchange(other.handle_, -1);
#endif
}
CaptureFile& CaptureFile::operator=(CaptureFile&& other) noexcept {
  if (this != &other) {
    close();
#ifdef _WIN32
    handle_ = std::exchange(other.handle_, nullptr);
#else
    handle_ = std::exchange(other.handle_, -1);
#endif
  }
  return *this;
}
CaptureFile::~CaptureFile() { close(); }
void CaptureFile::close() noexcept {
#ifdef _WIN32
  if (handle_) CloseHandle(std::exchange(handle_, nullptr));
#else
  if (handle_ >= 0) ::close(std::exchange(handle_, -1));
#endif
}
std::string CaptureFile::read_all() {
  std::string text;
  std::array<char, 65536> buffer{};
#ifdef _WIN32
  LARGE_INTEGER start{};
  if (!SetFilePointerEx(handle_, start, nullptr, FILE_BEGIN)) system_failure("rewind capture");
  for (;;) {
    DWORD count = 0;
    if (!ReadFile(handle_, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr))
      system_failure("read capture");
    if (count == 0) break;
    text.append(buffer.data(), count);
  }
#else
  if (::lseek(handle_, 0, SEEK_SET) < 0) system_failure("rewind capture");
  for (;;) {
    const auto count = ::read(handle_, buffer.data(), buffer.size());
    if (count < 0) {
      if (errno == EINTR) continue;
      system_failure("read capture");
    }
    if (count == 0) break;
    text.append(buffer.data(), static_cast<std::size_t>(count));
  }
#endif
  return text;
}
}  // namespace t81::cli::detail

namespace t81::cli {
namespace {
#if defined(_WIN32)
bool utf8_to_wide(std::string_view input, std::wstring& out) {
  out.clear();
  if (input.empty()) {
    return true;
  }
  if (input.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
      input.find('\0') != std::string_view::npos)
    return false;
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
  std::unique_ptr<wchar_t, decltype(&FreeEnvironmentStringsW)> env_guard(raw_env,
                                                                         &FreeEnvironmentStringsW);
  for (const wchar_t* cur = raw_env; *cur != L'\0';) {
    const std::wstring entry(cur);
    cur += entry.size() + 1;
    const std::size_t eq = entry.find(L'=', 1);
    if (eq != std::wstring::npos) {
      env_map[entry.substr(0, eq)] = entry.substr(eq + 1);
    }
  }

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
#ifdef _WIN32
void inherit_duplicate(HANDLE source, UniqueHandle& destination) {
  if (!DuplicateHandle(GetCurrentProcess(), source, GetCurrentProcess(), &destination.value, 0,
                       TRUE, DUPLICATE_SAME_ACCESS))
    system_failure("DuplicateHandle");
}
struct AttributeListGuard {
  LPPROC_THREAD_ATTRIBUTE_LIST value = nullptr;
  ~AttributeListGuard() {
    if (value) DeleteProcThreadAttributeList(value);
  }
};
#endif
}  // namespace

ProcessCaptureResult run_process_capture(
    const std::vector<std::string>& argv, const std::optional<fs::path>& cwd,
    const std::vector<std::pair<std::string, std::string>>& env_overrides) {
  ProcessCaptureResult result;
  try {
    if (argv.empty() || argv[0].empty()) throw std::invalid_argument("missing executable");
    for (const auto& arg : argv)
      if (arg.find('\0') != std::string::npos) throw std::invalid_argument("NUL in argument");
    for (const auto& [key, value] : env_overrides)
      if (key.empty() || key.find_first_of("=\0", 0, 2) != std::string::npos ||
          value.find('\0') != std::string::npos)
        throw std::invalid_argument("invalid environment override");
    auto output = detail::CaptureFile::create_temporary();
    auto errors = detail::CaptureFile::create_temporary();
#ifdef _WIN32
    std::wstring cmdline;
    for (const auto& arg : argv) {
      std::wstring wide;
      if (!utf8_to_wide(arg, wide)) throw std::invalid_argument("invalid UTF-8 argument");
      if (!cmdline.empty()) cmdline.push_back(L' ');
      cmdline += quote_windows_arg(wide);
    }
    std::wstring cwd_w;
    if (cwd) cwd_w = cwd->wstring();
    std::vector<wchar_t> env_block;
    if (!env_overrides.empty() && !build_windows_environment(env_overrides, env_block))
      throw std::runtime_error("cannot build Unicode environment");

    UniqueHandle child_output, child_errors, child_input;
    inherit_duplicate(output.native_handle(), child_output);
    inherit_duplicate(errors.native_handle(), child_errors);
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    if (input && input != INVALID_HANDLE_VALUE) {
      inherit_duplicate(input, child_input);
    } else {
      SECURITY_ATTRIBUTES sa{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
      child_input.value = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
      if (child_input.value == INVALID_HANDLE_VALUE) system_failure("open null stdin");
    }
    std::array<HANDLE, 3> inherited{child_input.value, child_output.value, child_errors.value};
    SIZE_T bytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);
    if (bytes == 0) system_failure("attribute list size");
    std::vector<BYTE> storage(bytes);
    AttributeListGuard attributes;
    auto* list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
    if (!InitializeProcThreadAttributeList(list, 1, 0, &bytes))
      system_failure("initialize handle list");
    attributes.value = list;
    if (!UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited.data(),
                                   sizeof(inherited), nullptr, nullptr))
      system_failure("restrict inherited handles");
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = child_input.value;
    startup.StartupInfo.hStdOutput = child_output.value;
    startup.StartupInfo.hStdError = child_errors.value;
    startup.lpAttributeList = list;
    PROCESS_INFORMATION pi{};
    const BOOL created = CreateProcessW(
        nullptr, cmdline.data(), nullptr, nullptr, TRUE,
        EXTENDED_STARTUPINFO_PRESENT | (env_block.empty() ? 0 : CREATE_UNICODE_ENVIRONMENT),
        env_block.empty() ? nullptr : env_block.data(), cwd ? cwd_w.c_str() : nullptr,
        &startup.StartupInfo, &pi);
    const DWORD create_error = created ? ERROR_SUCCESS : GetLastError();
    child_output.reset();
    child_errors.reset();
    child_input.reset();
    if (!created)
      throw std::system_error(static_cast<int>(create_error), std::system_category(),
                              "CreateProcessW");
    UniqueHandle process(pi.hProcess), thread(pi.hThread);
    if (WaitForSingleObject(process.value, INFINITE) != WAIT_OBJECT_0)
      system_failure("wait for child");
    DWORD exit_code = 0;
    if (!GetExitCodeProcess(process.value, &exit_code)) system_failure("child exit status");
    result.exit_code = static_cast<int>(exit_code);
#else
    std::vector<std::string> storage = argv;
    std::vector<char*> pointers;
    for (auto& arg : storage) pointers.push_back(arg.data());
    pointers.push_back(nullptr);
    const pid_t pid = ::fork();
    if (pid < 0) system_failure("fork");
    if (pid == 0) {
      if (::dup2(output.native_handle(), STDOUT_FILENO) < 0 ||
          ::dup2(errors.native_handle(), STDERR_FILENO) < 0)
        _exit(127);
      ::close(output.native_handle());
      ::close(errors.native_handle());
      if (cwd && ::chdir(cwd->c_str()) != 0) _exit(127);
      for (const auto& [key, value] : env_overrides)
        if (::setenv(key.c_str(), value.c_str(), 1) != 0) _exit(127);
      ::execvp(pointers[0], pointers.data());
      _exit(127);
    }
    int status = 0;
    while (::waitpid(pid, &status, 0) < 0) {
      if (errno != EINTR) system_failure("waitpid");
    }
    result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status)
                                         : (WIFSIGNALED(status) ? 128 + WTERMSIG(status) : status);
#endif
    result.stdout_text = output.read_all();
    result.stderr_text = errors.read_all();
  } catch (const std::exception& error) {
    result.exit_code = -1;
    result.error_message = error.what();
  }
  return result;
}
}  // namespace t81::cli
