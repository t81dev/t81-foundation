/**
 * @file T81Qutrit.hpp
 * @brief Defines the T81Qutrit type for representing a ternary quantum state.
 *
 * This file provides the T81Qutrit type, which is an alias for a 2-trit
 * balanced ternary integer (T81Int<2>). It serves as the fundamental unit for
 * representing a qutrit, a 3-state quantum system, and is intended for direct
 * mapping to future ternary quantum hardware.
 */
#pragma once

#include "t81/core/T81Int.hpp"

namespace t81 {

// ======================================================================
// T81Qutrit — Direct mapping to ternary quantum hardware
// 2 logical trits → 1 qutrit (3-state quantum system)
// This is the gateway drug to post-binary quantum AI
// ======================================================================
using T81Qutrit = T81Int<2>;

// The three basis states — exact, native, eternal
namespace qutrit {
    inline const T81Qutrit ZERO  = T81Qutrit(0);   // |0⟩   →  00 in balanced ternary
    inline const T81Qutrit ONE   = T81Qutrit(1);   // |1⟩   → +0+
    inline const T81Qutrit TWO   = T81Qutrit(-1);  // |2⟩   → -- (balanced -1)
}

// Future ternary quantum instructions will operate directly on this type
static_assert(sizeof(T81Qutrit) == 1);  // fits in one byte, 2 trits used

} // namespace t81::core
