#pragma once
#include <stdint.h>

// Experimental sealed-suite evaluator. No hosted VM or allocation interface.
extern "C" {
typedef void (*Cpa006Emit)(const char*);
enum Cpa006Status {
  Cpa006Halted = 0,
  Cpa006Malformed = 1,
  Cpa006Unsupported = 2,
  Cpa006ArtifactMismatch = 3,
  Cpa006Trap = 4,
  Cpa006OutsideProfile = 5,
  Cpa006InstructionLimit = 6,
};
int cpa006_evaluate(const uint8_t* bytes, unsigned length, Cpa006Emit emit,
                    unsigned instruction_limit = 64) noexcept;
void cpa006_run(Cpa006Emit emit) noexcept;
}
