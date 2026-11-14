#pragma once

#include <filesystem>
#include <string>

namespace t81::cli::detail {

// Internal ownership primitive shared by the launcher and its security tests.
// The native handle is borrowed; callers must neither close it nor transfer it.
// Failed exclusive creation never removes or truncates an existing path.
class CaptureFile {
public:
  static CaptureFile create_exclusive(const std::filesystem::path& path);
  static CaptureFile create_temporary();
  CaptureFile(CaptureFile&& other) noexcept;
  CaptureFile& operator=(CaptureFile&& other) noexcept;
  ~CaptureFile();
  CaptureFile(const CaptureFile&) = delete;
  CaptureFile& operator=(const CaptureFile&) = delete;
  std::string read_all();

#ifdef _WIN32
  void* native_handle() const noexcept { return handle_; }
#else
  int native_handle() const noexcept { return handle_; }
#endif

private:
#ifdef _WIN32
  explicit CaptureFile(void* handle) noexcept : handle_(handle) {}
  void* handle_ = nullptr;
#else
  explicit CaptureFile(int handle) noexcept : handle_(handle) {}
  int handle_ = -1;
#endif
  void close() noexcept;
};

}  // namespace t81::cli::detail
