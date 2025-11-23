#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "t81/core/base81.hpp"
#include "t81/hash/canonhash.hpp"

namespace t81::canonfs {
struct CanonHash {
  t81::hash::CanonHash81 h;

  bool operator<(const CanonHash& o) const noexcept { return h.bytes < o.h.bytes; }
  bool operator==(const CanonHash& o) const noexcept { return h.bytes == o.h.bytes; }
};

// Canonical object kinds per spec/canonfs-spec.md.
enum class ObjectType : std::uint8_t {
  Blob = 0,
  Directory = 1,
  Capability = 2,
  ParityShard = 3,
};

struct CanonRef {
  CanonHash hash;  // Content address (spec/canonfs-spec.md §2).
};

struct CapabilityGrant {
  CanonRef target;
  struct Subject {
    std::string id;
    std::string pubkey; // placeholder
  } subject;
  std::uint16_t perms{0};
};

struct CanonLink {
  std::string name;
  CanonRef ref;
};

struct CanonParityShard {
  CanonRef original;
  std::vector<std::byte> shard_data;
};

// Optional permission bits (example; extend as needed)
enum : uint16_t {
  CANON_PERM_READ   = 1u << 0,
  CANON_PERM_WRITE  = 1u << 1,
  CANON_PERM_APPEND = 1u << 2,
  CANON_PERM_ADMIN  = 1u << 15
};

// Minimal Axion verdict hook for CanonFS operations.
struct AxionVerdict {
  bool allow{true};
  std::string reason;
};

enum class OpKind {
  Read,
  Write,
  Publish,
  Revoke,
};
}  // namespace t81::canonfs
