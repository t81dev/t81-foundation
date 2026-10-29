#include "t81/canonfs/canon_driver.hpp"

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <istream>
#include <iterator>
#include <limits>
#include <list>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <sys/mman.h>
#include <unistd.h>
#else
#include <io.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "t81/canonfs/storage_mapping.hpp"
#include "t81/determinism/canon_hash81.hpp"
#include "t81/tracing/canonhash.hpp"

namespace std {
template <>
struct hash<t81::canonfs::CanonHash> {
  size_t operator()(const t81::canonfs::CanonHash& h) const noexcept {
    size_t seed = 0;
    for (uint8_t b : h.h.bytes) {
      seed ^= t81::CanonHash<uint8_t>{}(b) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};
}  // namespace std

namespace t81::canonfs {
namespace {
bool read_verify_enabled() {
  const char* raw = std::getenv("T81_CANONFS_READ_VERIFY");
  if (raw == nullptr) {
    return true;
  }
  std::string_view v(raw);
  return !(v == "0" || v == "false" || v == "FALSE" || v == "off" || v == "OFF");
}

std::filesystem::path objects_dir(const std::filesystem::path& root) { return root / "objects"; }

std::filesystem::path capabilities_dir(const std::filesystem::path& root) { return root / "caps"; }

std::filesystem::path parity_dir(const std::filesystem::path& root) { return root / "parity"; }

std::filesystem::path object_path(const std::filesystem::path& root, const CanonHash& hash,
                                  bool portable) {
  return objects_dir(root) /
         ((portable ? portable_storage_key(hash) : hash.h.to_string()) + ".blk");
}

std::filesystem::path capability_path(const std::filesystem::path& root, const CanonHash& hash,
                                      bool portable) {
  return capabilities_dir(root) /
         ((portable ? portable_storage_key(hash) : hash.h.to_string()) + ".cap");
}

std::optional<uint16_t> read_capability(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) return std::nullopt;
  std::ifstream in(path);
  if (!in) return std::nullopt;
  uint16_t value = 0;
  in >> value;
  if (!in) return std::nullopt;
  return value;
}

bool write_capability(const std::filesystem::path& path, uint16_t perms) {
  std::ofstream out(path, std::ios::trunc);
  if (!out) return false;
  out << perms;
  return static_cast<bool>(out);
}

bool write_all_fd(int fd, const std::byte* data, std::size_t size) {
  const std::byte* cursor = data;
  std::size_t remaining = size;
  while (remaining > 0) {
#ifdef _WIN32
    int to_write = remaining > INT_MAX ? INT_MAX : static_cast<int>(remaining);
    int wrote = ::_write(fd, cursor, to_write);
#else
    const ssize_t wrote = ::write(fd, cursor, remaining);
#endif
    if (wrote == 0) return false;
    if (wrote < 0) {
      if (errno == EINTR) {
        continue;
      }
      return false;
    }
    cursor += static_cast<std::size_t>(wrote);
    remaining -= static_cast<std::size_t>(wrote);
  }
  return true;
}

// The v2 namespace is separate from the frozen legacy directories. No fallback
// or migration is performed. All operations require one serialized root owner.
constexpr std::string_view kLayoutMarker = "t81.canonfs.storage-map.v2\n";
const char* kMarkerName = "canonfs-layout";

bool plain_path(const std::filesystem::path& path) {
  std::error_code ec;
  auto status = std::filesystem::symlink_status(path, ec);
  if (ec && ec != std::errc::no_such_file_or_directory) return false;
  if (std::filesystem::is_symlink(status)) return false;
#ifdef _WIN32
  const auto attributes = GetFileAttributesW(path.c_str());
  if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
    return false;
#endif
  return true;
}

std::optional<std::string> read_plain(const std::filesystem::path& path) {
  if (!plain_path(path) || !std::filesystem::is_regular_file(path)) return std::nullopt;
  std::ifstream input(path, std::ios::binary);
  if (!input) return std::nullopt;
  std::string data{std::istreambuf_iterator<char>(input), {}};
  if (input.bad()) return std::nullopt;
  return data;
}

bool exclusive_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
#ifdef _WIN32
  int fd = ::_wopen(path.c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE);
#else
  int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
#endif
  if (fd < 0) return false;
  const bool written = write_all_fd(fd, bytes.data(), bytes.size());
#ifdef _WIN32
  const bool closed = ::_close(fd) == 0;
#else
  const bool closed = ::close(fd) == 0;
#endif
  // Preserve unsuccessful/stale staging bytes for explicit recovery, not reuse.
  return written && closed;
}

bool publish_file(const std::filesystem::path& stage, const std::filesystem::path& target,
                  bool replace) {
#ifdef _WIN32
  return MoveFileExW(stage.c_str(), target.c_str(), replace ? MOVEFILE_REPLACE_EXISTING : 0) != 0;
#else
  if (replace) return ::rename(stage.c_str(), target.c_str()) == 0;
  // link() publishes without overwriting an existing object; rename() alone
  // would replace it. Staging and objects belong to the same local filesystem.
  if (::link(stage.c_str(), target.c_str()) != 0) return false;
  return ::unlink(stage.c_str()) == 0;
#endif
}

bool valid_v2(const std::filesystem::path& root, bool inspect_names) {
  if (!plain_path(root) || !std::filesystem::is_directory(root)) return false;
  const auto marker = read_plain(root / kMarkerName);
  if (!marker || *marker != kLayoutMarker) return false;
  for (const auto& entry : std::filesystem::directory_iterator(root)) {
    const auto name = entry.path().filename();
    if (name != kMarkerName && name != "v2") return false;
  }
  const auto data = root / "v2";
  if (!plain_path(data) || !std::filesystem::is_directory(data)) return false;
  for (const auto& entry : std::filesystem::directory_iterator(data)) {
    const auto name = entry.path().filename();
    if (name != "objects" && name != "caps" && name != "parity" && name != "staging") return false;
  }
  for (const std::string dir : {"objects", "caps", "parity", "staging"}) {
    const auto parent = data / dir;
    if (!plain_path(parent) || !std::filesystem::is_directory(parent)) return false;
    if (!inspect_names) continue;
    for (const auto& entry : std::filesystem::directory_iterator(parent)) {
      if (!plain_path(entry.path()) || !entry.is_regular_file()) return false;
      const auto name = entry.path().filename().string();
      const std::string suffix = dir == "objects" ? ".blk" : dir == "caps" ? ".cap" : ".partial";
      if (dir == "parity" || name.size() != 64 + suffix.size() || name.substr(64) != suffix ||
          !parse_portable_storage_key(name.substr(0, 64)))
        return false;
    }
  }
  return true;
}

void prepare_v2(const std::filesystem::path& root) {
  if (!plain_path(root))
    throw std::runtime_error("CanonFS v2 root must not be a link/reparse point");
  if (std::filesystem::exists(root) && !std::filesystem::is_empty(root)) {
    if (!valid_v2(root, true))
      throw std::runtime_error("CanonFS v2 invalid, mixed or legacy layout");
    return;
  }
  std::filesystem::create_directories(root);
  // Exclusive marker creation discriminates this format even after an
  // interrupted initialization. An incomplete layout must be reviewed, not
  // silently treated as a legacy/bootstrap store.
  if (!exclusive_file(root / kMarkerName,
                      std::as_bytes(std::span(kLayoutMarker.data(), kLayoutMarker.size()))))
    throw std::runtime_error("CanonFS v2 marker creation failed");
  for (const std::string dir : {"objects", "caps", "parity", "staging"})
    std::filesystem::create_directories(root / "v2" / dir);
}

std::optional<uint16_t> read_v2_capability(const std::filesystem::path& path) {
  auto value = read_plain(path);
  if (!value || value->empty() || value->back() != '\n') return std::nullopt;
  value->pop_back();
  unsigned number = 0;
  const auto result = std::from_chars(value->data(), value->data() + value->size(), number);
  if (result.ec != std::errc{} || result.ptr != value->data() + value->size() ||
      number > std::numeric_limits<uint16_t>::max() || *value != std::to_string(number))
    return std::nullopt;
  return static_cast<uint16_t>(number);
}

class PersistentDriver final : public Driver {
public:
  explicit PersistentDriver(std::filesystem::path root, bool portable = false)
      : store_root_(std::move(root)),
        root_(portable ? store_root_ / "v2" : store_root_),
        portable_(portable),
        objects_dir_(objects_dir(root_)),
        capabilities_dir_(capabilities_dir(root_)),
        parity_dir_(parity_dir(root_)) {
    if (portable_) {
      prepare_v2(store_root_);
    } else if (std::filesystem::symlink_status(store_root_ / kMarkerName).type() !=
               std::filesystem::file_type::not_found) {
      throw std::runtime_error("CanonFS legacy factory refuses a versioned store");
    }
    std::error_code ec;
    std::filesystem::create_directories(objects_dir_, ec);
    std::filesystem::create_directories(capabilities_dir_, ec);
    std::filesystem::create_directories(parity_dir_, ec);
    if (ec) {
      throw std::runtime_error("CanonFS persistent driver mkdir failed: " + ec.message());
    }
    has_capabilities_ = !std::filesystem::is_empty(capabilities_dir_);
  }

  void set_axion_hook(std::function<AxionVerdict(OpKind, const CanonRef&)> hook) override {
    hook_ = std::move(hook);
  }

  Result<CanonRef> write_object(ObjectType, std::span<const std::byte> bytes) override {
    auto hashed = t81::hash::hash_bytes(bytes);
    CanonRef ref{CanonHash{hashed}};
    if (!axion_allow(OpKind::Write, ref)) {
      return Result<CanonRef>(t81::unexpect, Error::CapabilityError);
    }
    if (!has_capability(ref.hash, CANON_PERM_WRITE)) {
      return Result<CanonRef>(t81::unexpect, Error::CapabilityError);
    }

    auto target = object_path(root_, ref.hash, portable_);
    if (portable_) return write_portable(ref, bytes, target);
#ifdef _WIN32
    const int fd = ::_open(target.string().c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
                           _S_IREAD | _S_IWRITE);
#else
    const int fd = ::open(target.string().c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
#endif
    if (fd < 0) {
      if (errno == EEXIST) {
        return ref;
      }
      return Result<CanonRef>(t81::unexpect, Error::DecodeError);
    }

    const bool ok = write_all_fd(fd, bytes.data(), bytes.size());
#ifdef _WIN32
    ::_close(fd);
#else
    ::close(fd);
#endif
    if (!ok) {
      std::error_code ec;
      std::filesystem::remove(target, ec);
      return Result<CanonRef>(t81::unexpect, Error::DecodeError);
    }

    // Write-through cache for hot re-reads.
    if (object_cache_.size() >= kMaxCacheSize) {
      auto last = lru_list_.back();
      object_cache_.erase(last);
      lru_list_.pop_back();
    }
    std::vector<std::byte> cached(bytes.begin(), bytes.end());
    lru_list_.push_front(ref.hash);
    object_cache_[ref.hash] = {std::move(cached), lru_list_.begin()};

    return ref;
  }

  Result<std::vector<std::byte>> read_object_bytes(const CanonRef& ref) override {
    if (!axion_allow(OpKind::Read, ref)) {
      return Result<std::vector<std::byte>>(t81::unexpect, Error::CapabilityError);
    }
    if (!has_capability(ref.hash, CANON_PERM_READ)) {
      return Result<std::vector<std::byte>>(t81::unexpect, Error::CapabilityError);
    }

    if (portable_) {
      if (!valid_v2(store_root_, false))
        return Result<std::vector<std::byte>>(t81::unexpect, Error::DecodeError);
      const auto path = object_path(root_, ref.hash, true);
      const auto contents = read_plain(path);
      if (!contents)
        return Result<std::vector<std::byte>>(
            t81::unexpect, std::filesystem::exists(path) ? Error::DecodeError : Error::NotFound);
      const auto raw = std::as_bytes(std::span(contents->data(), contents->size()));
      if (t81::hash::hash_bytes(raw) != ref.hash.h)
        return Result<std::vector<std::byte>>(t81::unexpect, Error::DecodeError);
      return std::vector<std::byte>(raw.begin(), raw.end());
    }

    auto it = object_cache_.find(ref.hash);
    if (it != object_cache_.end()) {
      if (read_verify_enabled()) {
        auto computed = t81::hash::hash_bytes(
            std::span<const std::byte>(it->second.data.data(), it->second.data.size()));
        if (computed != ref.hash.h) {
          return Result<std::vector<std::byte>>(t81::unexpect, Error::DecodeError);
        }
      }
      // Move to front of LRU
      lru_list_.erase(it->second.lru_it);
      lru_list_.push_front(ref.hash);
      it->second.lru_it = lru_list_.begin();
      return it->second.data;
    }

    auto target = object_path(root_, ref.hash, portable_);
#ifdef _WIN32
    int fd = ::_open(target.string().c_str(), _O_RDONLY | _O_BINARY);
#else
    int fd = ::open(target.string().c_str(), O_RDONLY);
#endif
    if (fd < 0) return Result<std::vector<std::byte>>(t81::unexpect, Error::NotFound);

#ifdef _WIN32
    struct _stat64i32 st;
    if (::_fstat64i32(fd, &st) < 0) {
      ::_close(fd);
      return Result<std::vector<std::byte>>(t81::unexpect, Error::DecodeError);
    }
#else
    struct stat st;
    if (::fstat(fd, &st) < 0) {
      ::close(fd);
      return Result<std::vector<std::byte>>(t81::unexpect, Error::DecodeError);
    }
#endif
    size_t size = static_cast<size_t>(st.st_size);

    std::vector<std::byte> result;
    if (size > 0) {
      result.resize(size);
#ifdef _WIN32
      std::size_t remaining = size;
      std::byte* cursor = result.data();
      while (remaining > 0) {
        int to_read = remaining > INT_MAX ? INT_MAX : static_cast<int>(remaining);
        int bytes_read = ::_read(fd, cursor, to_read);
        if (bytes_read < 0) {
          if (errno == EINTR) continue;
          ::_close(fd);
          return Result<std::vector<std::byte>>(t81::unexpect, Error::DecodeError);
        }
        if (bytes_read == 0) break;  // EOF
        cursor += bytes_read;
        remaining -= bytes_read;
      }
#else
      void* addr = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
      if (addr == MAP_FAILED) {
        ::close(fd);
        return Result<std::vector<std::byte>>(t81::unexpect, Error::DecodeError);
      }
      std::memcpy(result.data(), addr, size);
      ::munmap(addr, size);
#endif
    }
#ifdef _WIN32
    ::_close(fd);
#else
    ::close(fd);
#endif

    if (read_verify_enabled()) {
      auto computed =
          t81::hash::hash_bytes(std::span<const std::byte>(result.data(), result.size()));
      if (computed != ref.hash.h) {
        return Result<std::vector<std::byte>>(t81::unexpect, Error::DecodeError);
      }
    }

    // Update LRU cache
    if (object_cache_.size() >= kMaxCacheSize) {
      auto last = lru_list_.back();
      object_cache_.erase(last);
      lru_list_.pop_back();
    }
    lru_list_.push_front(ref.hash);
    object_cache_[ref.hash] = {result, lru_list_.begin()};

    return result;
  }

  Result<void> publish_capability(const CapabilityGrant& grant) override {
    if (!axion_allow(OpKind::Publish, grant.target)) {
      return Result<void>(t81::unexpect, Error::CapabilityError);
    }
    auto target = capability_path(root_, grant.target.hash, portable_);
    if (portable_) {
      if (!valid_v2(store_root_, false) || !plain_path(target))
        return Result<void>(t81::unexpect, Error::DecodeError);
      const auto text = std::to_string(grant.perms) + "\n";
      const auto stage = root_ / "staging" / (portable_storage_key(grant.target.hash) + ".partial");
      if (!exclusive_file(stage, std::as_bytes(std::span(text.data(), text.size()))) ||
          !publish_file(stage, target, true))
        return Result<void>(t81::unexpect, Error::DecodeError);
    } else if (!write_capability(target, grant.perms)) {
      return Result<void>(t81::unexpect, Error::DecodeError);
    }
    has_capabilities_ = true;
    capability_cache_[grant.target.hash] = grant.perms;
    return {};
  }

  Result<void> revoke_capability(const CanonRef& ref) override {
    if (!axion_allow(OpKind::Revoke, ref)) {
      return Result<void>(t81::unexpect, Error::CapabilityError);
    }
    auto target = capability_path(root_, ref.hash, portable_);
    if (portable_ && (!valid_v2(store_root_, false) || !plain_path(target)))
      return Result<void>(t81::unexpect, Error::DecodeError);
    std::error_code ec;
    std::filesystem::remove(target, ec);
    if (ec) return Result<void>(t81::unexpect, Error::CapabilityError);
    has_capabilities_ = !std::filesystem::is_empty(capabilities_dir_);
    capability_cache_.erase(ref.hash);
    return {};
  }

  Result<void> parity_repair_subtree(const CanonRef& ref) override {
    if (!axion_allow(OpKind::Repair, ref)) {
      return Result<void>(t81::unexpect, Error::CapabilityError);
    }
    auto target = object_path(root_, ref.hash, portable_);
    if (portable_ && (!valid_v2(store_root_, false) || !plain_path(target)))
      return Result<void>(t81::unexpect, Error::DecodeError);
    if (!std::filesystem::exists(target)) {
      return Result<void>(t81::unexpect, Error::NotFound);
    }
    return {};
  }

private:
  Result<CanonRef> write_portable(const CanonRef& ref, std::span<const std::byte> bytes,
                                  const std::filesystem::path& target) {
    if (!valid_v2(store_root_, false) || !plain_path(target))
      return Result<CanonRef>(t81::unexpect, Error::DecodeError);
    if (std::filesystem::exists(target)) {
      // EEXIST is success only for exactly the same verified bytes. Never
      // repair/overwrite a corrupted, truncated or collision-aliasing object.
      const auto old = read_plain(target);
      if (!old || std::as_bytes(std::span(old->data(), old->size())).size() != bytes.size() ||
          !std::equal(bytes.begin(), bytes.end(),
                      std::as_bytes(std::span(old->data(), old->size())).begin()) ||
          t81::hash::hash_string(*old) != ref.hash.h)
        return Result<CanonRef>(t81::unexpect, Error::DecodeError);
      return ref;
    }
    const auto stage = root_ / "staging" / (portable_storage_key(ref.hash) + ".partial");
    if (!exclusive_file(stage, bytes) || !publish_file(stage, target, false))
      return Result<CanonRef>(t81::unexpect, Error::DecodeError);
    return ref;
  }

  bool axion_allow(OpKind kind, const CanonRef& ref) const {
    if (!hook_) return true;
    try {
      AxionVerdict v = hook_(kind, ref);
      return v.allow;
    } catch (...) {
      return false;
    }
  }

  bool has_capability(const CanonHash& hash, uint16_t required) const {
    if (portable_ && !valid_v2(store_root_, false)) return false;
    if (required == 0) return true;
    if (!has_capabilities_) return true;

    if (portable_) {
      auto perms = read_v2_capability(capability_path(root_, hash, true));
      return perms && ((*perms & required) == required);
    }
    uint16_t perms_val = 0;
    auto it = capability_cache_.find(hash);
    if (it != capability_cache_.end()) {
      perms_val = it->second;
    } else {
      auto perms = read_capability(capability_path(root_, hash, portable_));
      if (!perms.has_value()) return false;
      perms_val = perms.value();
      capability_cache_[hash] = perms_val;
    }
    return (perms_val & required) != 0;
  }

  struct CacheEntry {
    std::vector<std::byte> data;
    std::list<CanonHash>::iterator lru_it;
  };

  std::filesystem::path store_root_;
  std::filesystem::path root_;
  bool portable_;
  std::filesystem::path objects_dir_;
  std::filesystem::path capabilities_dir_;
  std::filesystem::path parity_dir_;
  bool has_capabilities_{false};
  static constexpr size_t kMaxCacheSize = 2048;  // Increased from 1024
  mutable std::unordered_map<CanonHash, uint16_t> capability_cache_{};
  mutable std::unordered_map<CanonHash, CacheEntry> object_cache_{};
  mutable std::list<CanonHash> lru_list_{};
  std::function<AxionVerdict(OpKind, const CanonRef&)> hook_{};
};

}  // namespace

std::unique_ptr<Driver> make_persistent_driver(std::filesystem::path root) {
  return std::make_unique<PersistentDriver>(std::move(root));
}

std::unique_ptr<Driver> make_portable_persistent_driver(std::filesystem::path root) {
  return std::make_unique<PersistentDriver>(std::move(root), true);
}

}  // namespace t81::canonfs

#ifdef _WIN32
#pragma warning(pop)
#endif
