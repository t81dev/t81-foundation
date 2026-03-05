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
  }
  return "UnknownTrap";
}
}  // namespace t81::vm
