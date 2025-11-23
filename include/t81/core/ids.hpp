#pragma once

#include <string>
#include "t81/core/base81.hpp"

namespace t81::core {
// Canonical identifiers (hashes, addresses). See spec/t81-data-types.md.
struct CanonicalId {
  Base81String value;
};

inline CanonicalId make_id(const Base81String& value) { return CanonicalId{value}; }
}  // namespace t81::core

