#pragma once

#include <string>

namespace t81::axion {
// RFC-0034 §5.17.6 adds Quarantine: thread is suspended, RD not committed (SecurityFault).
// Deny remains the hard stop that raises ActivationFault.
enum class VerdictKind { Allow, Deny, Defer, Warn, Quarantine };

struct Verdict {
  VerdictKind kind{VerdictKind::Defer};
  std::string reason;
};
}  // namespace t81::axion
