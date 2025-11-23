#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "t81/core/base81.hpp"

namespace t81::canonfs {
using CanonHash = t81::core::Base81String;

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
  std::string subject;  // Placeholder identity; TODO align with spec identity model.
};

struct CanonLink {
  std::string name;
  CanonRef ref;
};

struct CanonParityShard {
  CanonRef original;
  std::vector<std::byte> shard_data;
};
}  // namespace t81::canonfs

