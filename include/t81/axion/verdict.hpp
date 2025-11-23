#pragma once

#include <string>

namespace t81::axion {
enum class VerdictKind { Allow, Deny, Defer };

struct Verdict {
  VerdictKind kind{VerdictKind::Defer};
  std::string reason;
};
}  // namespace t81::axion

