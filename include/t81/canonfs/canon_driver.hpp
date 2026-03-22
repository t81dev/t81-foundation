#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <t81/support/expected.hpp>
#include <vector>
#include "t81/canonfs/canon_types.hpp"

// Forward-declare IBlockDevice before opening t81::canonfs so that the
// make_block_backed_driver factory parameter type resolves correctly.
namespace t81::ternaryos::dev { class IBlockDevice; }

namespace t81::canonfs {
enum class Error {
  None = 0,
  NotFound,
  InvalidObject,
  CapabilityError,
  ParityFailure,
  DecodeError,
};

template <typename T>
using Result = std::expected<T, Error>;

class Driver {
public:
  virtual ~Driver() = default;
  virtual Result<CanonRef> write_object(ObjectType type, std::span<const std::byte> bytes) = 0;
  virtual Result<std::vector<std::byte>> read_object_bytes(const CanonRef& ref) = 0;
  virtual Result<void> publish_capability(const CapabilityGrant& grant) = 0;
  virtual Result<void> revoke_capability(const CanonRef& ref) = 0;
  virtual Result<void> parity_repair_subtree(const CanonRef& ref) = 0;

  // Optional Axion hook: set a verdict callback; returns false if rejected.
  virtual void set_axion_hook(std::function<AxionVerdict(OpKind, const CanonRef&)> hook) = 0;
};

// ── Driver factories ──────────────────────────────────────────────────────────

/// Volatile in-memory store; lost on process exit.
std::unique_ptr<Driver> make_in_memory_driver();

/// Filesystem-rooted persistent store; survives process restart.
std::unique_ptr<Driver> make_persistent_driver(std::filesystem::path root);

/// Block-device-backed persistent store (virtio-blk, hosted image, etc.).
/// The driver takes ownership of `dev`.  If `rebuild_on_open` is true,
/// the on-device index is scanned and rebuilt on construction (safe after
/// a clean shutdown that called flush(); required after an unclean shutdown).
/// Returns nullptr if `dev` is null.
std::unique_ptr<Driver> make_block_backed_driver(
    std::unique_ptr<t81::ternaryos::dev::IBlockDevice> dev,
    bool rebuild_on_open = true);

}  // namespace t81::canonfs
