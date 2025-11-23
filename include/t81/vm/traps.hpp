#pragma once

namespace t81::vm {
enum class Trap {
  None = 0,
  InvalidMemory,
  IllegalInstruction,
  DivideByZero,
  BoundsFault,
  SecurityFault,
};
}  // namespace t81::vm
