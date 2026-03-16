#pragma once

#include <string>

namespace t81::vm {
enum class Trap {
  None = 0,
  DecodeFault,
  TypeFault,
  BoundsFault,
  StackFault,
  DivisionFault,
  SecurityFault,
  TierFault,
  ShapeFault,
  TrapInstruction,
  Unimplemented,
  AssertionFailed,
  // RFC-0000 §3: Axion ethics / capability fault types (normative).
  EthicsViolation,   // Raised when a Θ-overlay (Θ₁–Θ₉) is violated; triggers AXHALT.
  CapabilityDenied,  // Raised when an operation is attempted without a valid CapabilityGrant.
  // RFC-0034 §5.17.6: TACT post-execute activation policy — Deny verdict.
  ActivationFault,   // TACT: activation-ceiling policy returned Deny; thread was already quarantined.
};

inline std::string to_string(Trap trap) {
  switch (trap) {
    case Trap::None:
      return "None";
    case Trap::DecodeFault:
      return "DecodeFault";
    case Trap::TypeFault:
      return "TypeFault";
    case Trap::BoundsFault:
      return "BoundsFault";
    case Trap::StackFault:
      return "StackFault";
    case Trap::DivisionFault:
      return "DivisionFault";
    case Trap::SecurityFault:
      return "SecurityFault";
    case Trap::TierFault:
      return "TierFault";
    case Trap::ShapeFault:
      return "ShapeFault";
    case Trap::TrapInstruction:
      return "TrapInstruction";
    case Trap::Unimplemented:
      return "Unimplemented";
    case Trap::AssertionFailed:
      return "AssertionFailed";
    case Trap::EthicsViolation:
      return "EthicsViolation";
    case Trap::CapabilityDenied:
      return "CapabilityDenied";
    case Trap::ActivationFault:
      return "ActivationFault";
  }
  return "UnknownTrap";
}
}  // namespace t81::vm
