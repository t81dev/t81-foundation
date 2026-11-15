#pragma once
#include <stdint.h>

// Experimental fixed-artifact evaluator. No hosted VM or allocation interface.
extern "C" {
typedef void (*Cpa004Emit)(const char*);
enum Cpa004Status {
  Cpa004Halted = 0,
  Cpa004Malformed = 1,
  Cpa004Unsupported = 2,
  Cpa004ArtifactMismatch = 3,
  Cpa004Trap = 4,
  Cpa004OutsideProfile = 5,
  Cpa004InstructionLimit = 6,
};
int cpa004_evaluate(const uint8_t* bytes, unsigned length, Cpa004Emit emit,
                    unsigned instruction_limit = 64) noexcept;
void cpa004_run(Cpa004Emit emit) noexcept;
}
