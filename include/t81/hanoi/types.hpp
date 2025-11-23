#pragma once

#include <cstdint>
#include "t81/canonfs/canon_types.hpp"

namespace t81::hanoi {
using SnapshotRef = t81::canonfs::CanonRef;
using Pid = std::uint64_t;

struct RegionHandle {
  std::uint64_t id{0};
};
}  // namespace t81::hanoi

