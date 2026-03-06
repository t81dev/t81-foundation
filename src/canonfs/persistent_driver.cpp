#include "t81/canonfs/canon_driver.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <istream>
#include <iterator>
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
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "t81/tracing/canonhash.hpp"
#include "t81/determinism/canon_hash81.hpp"

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

std::filesystem::path object_path(const std::filesystem::path& root, const CanonHash& hash) {
  return objects_dir(root) / (hash.h.to_string() + ".blk");
}

std::filesystem::path capability_path(const std::filesystem::path& root, const CanonHash& hash) {
  return capabilities_dir(root) / (hash.h.to_string() + ".cap");
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
    const ssize_t wrote = ::write(fd, cursor, remaining);
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

class PersistentDriver final : public Driver {
public:
  explicit PersistentDriver(std::filesystem::path root)
      : root_(std::move(root)),
        objects_dir_(objects_dir(root_)),
        capabilities_dir_(capabilities_dir(root_)),
        parity_dir_(parity_dir(root_)) {
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

    auto target = object_path(root_, ref.hash);
    const int fd = ::open(target.string().c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0) {
      if (errno == EEXIST) {
        return ref;
      }
      return Result<CanonRef>(t81::unexpect, Error::DecodeError);
    }

    const bool ok = write_all_fd(fd, bytes.data(), bytes.size());
    ::close(fd);
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

    auto target = object_path(root_, ref.hash);
    int fd = open(target.string().c_str(), O_RDONLY);
    if (fd < 0) return Result<std::vector<std::byte>>(t81::unexpect, Error::NotFound);

    struct stat st;
    if (fstat(fd, &st) < 0) {
      close(fd);
      return Result<std::vector<std::byte>>(t81::unexpect, Error::DecodeError);
    }
    size_t size = static_cast<size_t>(st.st_size);

    std::vector<std::byte> result;
    if (size > 0) {
      void* addr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
      if (addr == MAP_FAILED) {
        close(fd);
        return Result<std::vector<std::byte>>(t81::unexpect, Error::DecodeError);
      }
      result.resize(size);
      std::memcpy(result.data(), addr, size);
      munmap(addr, size);
    }
    close(fd);

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
    auto target = capability_path(root_, grant.target.hash);
    if (!write_capability(target, grant.perms)) {
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
    auto target = capability_path(root_, ref.hash);
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
    auto target = object_path(root_, ref.hash);
    if (!std::filesystem::exists(target)) {
      return Result<void>(t81::unexpect, Error::NotFound);
    }
    return {};
  }

private:
  bool axion_allow(OpKind kind, const CanonRef& ref) const {
    if (!hook_) return true;
    AxionVerdict v = hook_(kind, ref);
    return v.allow;
  }

  bool has_capability(const CanonHash& hash, uint16_t required) const {
    if (required == 0) return true;
    if (!has_capabilities_) return true;

    uint16_t perms_val = 0;
    auto it = capability_cache_.find(hash);
    if (it != capability_cache_.end()) {
      perms_val = it->second;
    } else {
      auto perms = read_capability(capability_path(root_, hash));
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

  std::filesystem::path root_;
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

}  // namespace t81::canonfs
