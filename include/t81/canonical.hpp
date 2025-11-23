#pragma once

#include <string_view>

// Canonical invariants for the T81 platform. See spec/v1.1.0-canonical.md.
namespace t81 {
inline constexpr std::string_view kVersion = "v1.1.0-canonical";
inline constexpr bool kDeterministic = true;  // All execution must be deterministic per spec.
inline constexpr bool kTernaryNative = true;  // Core arithmetic defined in balanced ternary.
}  // namespace t81

