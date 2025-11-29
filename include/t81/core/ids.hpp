/**
 * @file ids.hpp
 * @brief Defines the CanonicalId struct for representing canonical identifiers.
 *
 * This file provides the `CanonicalId` struct, which is used for representing
 * canonical identifiers such as hashes and addresses. The identifier is stored
 * as a `Base81String`, ensuring a consistent, human-readable textual format
 * across the T81 platform.
 */
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
