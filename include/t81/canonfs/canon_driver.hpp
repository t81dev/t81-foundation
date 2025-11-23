#pragma once

#include <memory>
#include <span>
#include <vector>
#include <t81/support/expected.hpp>
#include "t81/canonfs/canon_types.hpp"

namespace t81::canonfs {
enum class Error {
  None = 0,
  NotFound,
  InvalidObject,
  CapabilityError,
  ParityFailure,
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
};

// Utility factory for the in-memory driver implementation.
std::unique_ptr<Driver> make_in_memory_driver();
}  // namespace t81::canonfs

