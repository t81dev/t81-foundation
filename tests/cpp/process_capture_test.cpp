#include "tools/cli/core/process_capture.hpp"
#include "tools/cli/core/process_capture_detail.hpp"

#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <fcntl.h>
#include <io.h>
#include <windows.h>

#include <aclapi.h>
#elif defined(__APPLE__)
#include <fcntl.h>
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;
using t81::cli::detail::CaptureFile;

// These checks deliberately remain active in Release / -DNDEBUG builds.
#define REQUIRE(condition)                                  \
  do {                                                      \
    if (!(condition)) throw std::runtime_error(#condition); \
  } while (false)

namespace {
std::string utf8(std::u8string_view text) { return {text.begin(), text.end()}; }
std::string path_utf8(const fs::path& path) { return utf8(path.u8string()); }
fs::path from_utf8(const std::string& text) {
  return fs::path(std::u8string(text.begin(), text.end()));
}
std::string contents(const fs::path& path) {
  std::ifstream input(path, std::ios::binary);
  REQUIRE(input.good());
  return {(std::istreambuf_iterator<char>(input)), {}};
}
void put(const fs::path& path, const std::string& value) {
  std::ofstream output(path, std::ios::binary);
  REQUIRE(output.good());
  output << value;
  REQUIRE(output.good());
}
struct TempDirectory {
  fs::path path;
  TempDirectory() {
    std::random_device random;
    for (unsigned i = 0; i < 128; ++i) {
      auto candidate = fs::temp_directory_path() / ("t81-capture-test-" + std::to_string(random()) +
                                                    "-" + std::to_string(random()));
      std::error_code error;
      if (fs::create_directory(candidate, error)) {
        path = candidate;
#ifndef _WIN32
        fs::permissions(path, fs::perms::owner_all, fs::perm_options::replace);
#endif
        return;
      }
      if (error && error != std::errc::file_exists) throw std::system_error(error);
    }
    throw std::runtime_error("test directory collision limit");
  }
  ~TempDirectory() {
    std::error_code error;
    fs::remove_all(path, error);
  }
};
fs::path current_executable([[maybe_unused]] const std::string& argv0) {
#ifdef _WIN32
  std::wstring buffer(1024, L'\0');
  for (;;) {
    const DWORD count =
        GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    REQUIRE(count > 0);
    if (count < buffer.size()) return fs::path(buffer.substr(0, count));
    REQUIRE(buffer.size() < 65536);
    buffer.resize(buffer.size() * 2);
  }
#elif defined(__APPLE__)
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  std::string buffer(size, '\0');
  REQUIRE(_NSGetExecutablePath(buffer.data(), &size) == 0);
  return fs::canonical(fs::path(buffer.c_str()));
#else
  std::error_code error;
  auto path = fs::read_symlink("/proc/self/exe", error);
  if (!error) return path;
  return fs::absolute(from_utf8(argv0));
#endif
}
void reject_existing(const fs::path& path) {
  bool rejected = false;
  try {
    auto capture = CaptureFile::create_exclusive(path);
  } catch (const std::system_error&) {
    rejected = true;
  }
  REQUIRE(rejected);
}
void private_permissions(CaptureFile& file) {
#ifdef _WIN32
  DWORD flags = 0;
  REQUIRE(GetHandleInformation(file.native_handle(), &flags));
  REQUIRE((flags & HANDLE_FLAG_INHERIT) == 0);
  PACL acl = nullptr;
  PSECURITY_DESCRIPTOR descriptor = nullptr;
  REQUIRE(GetSecurityInfo(file.native_handle(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr,
                          nullptr, &acl, nullptr, &descriptor) == ERROR_SUCCESS);
  SECURITY_DESCRIPTOR_CONTROL control = 0;
  DWORD revision = 0;
  ACL_SIZE_INFORMATION info{};
  void* raw_ace = nullptr;
  bool private_acl = GetSecurityDescriptorControl(descriptor, &control, &revision) &&
                     (control & SE_DACL_PROTECTED) && acl &&
                     GetAclInformation(acl, &info, sizeof(info), AclSizeInformation) &&
                     info.AceCount == 1 && GetAce(acl, 0, &raw_ace);
  HANDLE token = nullptr;
  DWORD bytes = 0;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) private_acl = false;
  if (token) GetTokenInformation(token, TokenUser, nullptr, 0, &bytes);
  std::vector<BYTE> token_info(bytes);
  if (token && bytes && GetTokenInformation(token, TokenUser, token_info.data(), bytes, &bytes) &&
      private_acl) {
    auto* ace = static_cast<ACCESS_ALLOWED_ACE*>(raw_ace);
    private_acl =
        ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE && ace->Mask == FILE_ALL_ACCESS &&
        EqualSid(&ace->SidStart, reinterpret_cast<TOKEN_USER*>(token_info.data())->User.Sid);
  } else
    private_acl = false;
  if (token) CloseHandle(token);
  LocalFree(descriptor);
  REQUIRE(private_acl);
#else
  struct stat info {};
  REQUIRE(::fstat(file.native_handle(), &info) == 0);
  REQUIRE((info.st_mode & 0777) == 0600);
  REQUIRE((::fcntl(file.native_handle(), F_GETFD) & FD_CLOEXEC) != 0);
#endif
}
void creation_security(const fs::path& base) {
  const auto victim = base / "existing.txt";
  put(victim, "PRESERVE_EXISTING");
  reject_existing(victim);
  REQUIRE(contents(victim) == "PRESERVE_EXISTING");

  const auto link = base / "capture-link";
  std::error_code link_error;
  fs::create_symlink(victim, link, link_error);
  if (!link_error) {
    reject_existing(link);
    REQUIRE(fs::is_symlink(link));
    REQUIRE(contents(victim) == "PRESERVE_EXISTING");
    std::cout << "symlink/reparse rejection: executed\n";
  } else {
#ifdef _WIN32
    REQUIRE(link_error.value() == ERROR_PRIVILEGE_NOT_HELD ||
            link_error.value() == ERROR_NOT_SUPPORTED ||
            link_error.value() == ERROR_ACCESS_DENIED ||
            link_error.value() == ERROR_INVALID_FUNCTION);
    std::cout << "SKIP reparse-point creation: " << link_error.message() << " ("
              << link_error.value() << ")\n";
#else
    throw std::system_error(link_error);
#endif
  }

  const auto owned_path = base / "owned-capture";
  {
    auto capture = CaptureFile::create_exclusive(owned_path);
    private_permissions(capture);
    const std::string value = "owned-object-content";
#ifdef _WIN32
    DWORD count = 0;
    REQUIRE(WriteFile(capture.native_handle(), value.data(), static_cast<DWORD>(value.size()),
                      &count, nullptr));
    REQUIRE(count == value.size());
    // No other open can truncate the retained, delete-on-close object.
    reject_existing(owned_path);
#else
    REQUIRE(!fs::exists(owned_path));
    REQUIRE(::write(capture.native_handle(), value.data(), value.size()) ==
            static_cast<ssize_t>(value.size()));
    put(owned_path, "REPLACEMENT_PATH_MUST_SURVIVE");
#endif
    REQUIRE(capture.read_all() == value);
    REQUIRE(capture.read_all() == value);  // Rewind/read the SAME owned object.
  }
#ifdef _WIN32
  REQUIRE(!fs::exists(owned_path));
#else
  REQUIRE(contents(owned_path) == "REPLACEMENT_PATH_MUST_SURVIVE");
#endif

  // Second creation fails: the first object is cleaned up without touching the
  // pre-existing second path. Both objects use the production ownership primitive.
  const auto partial = base / "partial-first";
  bool partial_failed = false;
  try {
    auto first = CaptureFile::create_exclusive(partial);
    auto second = CaptureFile::create_exclusive(victim);
  } catch (const std::system_error&) {
    partial_failed = true;
  }
  REQUIRE(partial_failed);
  REQUIRE(!fs::exists(partial));
  REQUIRE(contents(victim) == "PRESERVE_EXISTING");
  reject_existing(base / "missing-parent" / "cannot-create");
  std::cout << "exclusive creation, private permissions, retained-object read and partial cleanup: "
               "passed\n";
}
int run(const std::vector<std::string>& args) {
  if (args.size() >= 3 && args[1] == "--child-mode") {
    for (std::size_t i = 3; i < args.size(); ++i)
      std::cout << "ARG[" << i - 3 << "]:" << args[i].size() << ":" << args[i] << "\n";
    std::cout << "CWD:" << path_utf8(fs::current_path().filename()) << "\n";
    if (const char* value = std::getenv("T81_CAPTURE_TEST_ENV"))
      std::cout << "ENV:" << value << "\n";
    std::cerr << "STDERR_OK\n";
    return std::stoi(args[2]);
  }
  constexpr std::size_t large_size = 2 * 1024 * 1024;
  if (args.size() == 2 && args[1] == "--large-output") {
    std::cerr.tie(nullptr);
    std::cout.tie(nullptr);
    std::thread writer([] { std::cerr << std::string(large_size, 'Y'); });
    std::cout << std::string(large_size, 'X');
    writer.join();
    return 37;
  }

  TempDirectory directory;
  creation_security(directory.path);
  const auto cwd = directory.path / from_utf8(utf8(u8"sub cwd \u96ea"));
  fs::create_directory(cwd);
  const auto self = current_executable(args.at(0));
  auto name = from_utf8(utf8(u8"child helper \u96ea with spaces"));
  name += self.extension();
  const auto child = directory.path / name;
  fs::copy_file(self, child);
  const std::vector<std::string> values{"",
                                        "hello world",
                                        "quoted \"inner\" value",
                                        R"(one\\\"two)",
                                        R"(space trailing\\)",
                                        R"(\"quoted\")",
                                        utf8(u8"Unicode: \u03c0 \u96ea \U0001f9ea"),
                                        "& | < > ^ %T81_CAPTURE_TEST_ENV% !VAR! ; $(literal)",
                                        "& echo INJECTED > capture-injection-marker.txt"};
  std::vector<std::string> command{path_utf8(child), "--child-mode", "42"};
  command.insert(command.end(), values.begin(), values.end());
  const auto captured =
      t81::cli::run_process_capture(command, cwd, {{"T81_CAPTURE_TEST_ENV", "override-ok"}});
  REQUIRE(captured.error_message.empty());
  REQUIRE(captured.exit_code == 42);
  std::string expected;
  for (std::size_t i = 0; i < values.size(); ++i)
    expected += "ARG[" + std::to_string(i) + "]:" + std::to_string(values[i].size()) + ":" +
                values[i] + "\n";
  expected += "CWD:" + path_utf8(cwd.filename()) + "\nENV:override-ok\n";
  REQUIRE(captured.stdout_text == expected);
  REQUIRE(captured.stderr_text == "STDERR_OK\n");
  REQUIRE(!fs::exists(cwd / "capture-injection-marker.txt"));
  const auto zero = t81::cli::run_process_capture({path_utf8(child), "--child-mode", "0"});
  REQUIRE(zero.exit_code == 0 && zero.error_message.empty());
  const auto large = t81::cli::run_process_capture({path_utf8(child), "--large-output"});
  REQUIRE(large.exit_code == 37 && large.error_message.empty());
  REQUIRE(large.stdout_text == std::string(large_size, 'X'));
  REQUIRE(large.stderr_text == std::string(large_size, 'Y'));
  const auto missing =
      t81::cli::run_process_capture({path_utf8(directory.path / "missing-executable")});
  REQUIRE(missing.exit_code != 0);
  REQUIRE(t81::cli::run_process_capture({}).exit_code == -1);
#ifdef _WIN32
  const auto invalid = t81::cli::run_process_capture({path_utf8(child), std::string(1, '\xff')});
  REQUIRE(invalid.exit_code == -1 && !invalid.error_message.empty());
  std::cout << "Unicode observed via wmain UTF-16 arguments; ";
#endif
  std::cout << "argument fidelity, shell literals, cwd/env, zero/nonzero exits and concurrent 2 "
               "MiB streams: passed\n";
  return 0;
}
}  // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
  try {
    REQUIRE(_setmode(_fileno(stdout), _O_BINARY) != -1);
    REQUIRE(_setmode(_fileno(stderr), _O_BINARY) != -1);
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i) {
      const int size = static_cast<int>(std::wcslen(argv[i]));
      if (!size) {
        args.emplace_back();
        continue;
      }
      const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argv[i], size, nullptr,
                                            0, nullptr, nullptr);
      REQUIRE(count > 0);
      std::string value(static_cast<std::size_t>(count), '\0');
      REQUIRE(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argv[i], size, value.data(), count,
                                  nullptr, nullptr) == count);
      args.push_back(std::move(value));
    }
    return run(args);
  } catch (const std::exception& error) {
    std::cerr << "capture test failed: " << error.what() << "\n";
    return 1;
  }
}
#else
int main(int argc, char* argv[]) {
  try {
    return run(std::vector<std::string>(argv, argv + argc));
  } catch (const std::exception& error) {
    std::cerr << "capture test failed: " << error.what() << "\n";
    return 1;
  }
}
#endif
