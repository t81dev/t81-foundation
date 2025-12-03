/**
 * @file all.hpp
 * @brief Convenience header that includes the complete T81 Core library.
 */

#pragma once

#ifndef T81_ALL_HPP
#define T81_ALL_HPP

#include <cstddef>
#include <cstdint>
#include <concepts>
#include <utility>
#include <memory>
#include <optional>
#include <variant>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <coroutine>

// ======================================================================
// Core Numerics & Primitives
// ======================================================================
#include "t81/core/T81Int.hpp"
#include "t81/core/T81BigInt.hpp"
#include "t81/core/T81Float.hpp"
#include "t81/core/T81Complex.hpp"
#include "t81/core/T81Prob.hpp"
#include "t81/core/T81Qutrit.hpp"
#include "t81/core/T81Entropy.hpp"
#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81String.hpp"

// ======================================================================
// Collections & Structures
// ======================================================================
#include "t81/core/T81List.hpp"
#include "t81/core/T81Map.hpp"
#include "t81/core/T81Set.hpp"
#include "t81/core/T81Tree.hpp"
#include "t81/core/T81Stream.hpp"
#include "t81/core/T81Tensor.hpp"
#include "t81/core/T81Vector.hpp"
#include "t81/core/T81Quaternion.hpp"
#include "t81/core/T81Polynomial.hpp"
#include "t81/core/T81Category.hpp"

// ======================================================================
// Advanced & Civilizational Types
// ======================================================================
#include "t81/core/T81Agent.hpp"
#include "t81/core/T81Reflection.hpp"
#include "t81/core/T81Proof.hpp"
#include "t81/core/T81Time.hpp"
#include "t81/core/T81UInt.hpp"
#include "t81/core/T81Bytes.hpp"
#include "t81/core/T81IOStream.hpp"
#include "t81/core/T81Maybe.hpp"
#include "t81/core/T81Result.hpp"
#include "t81/core/Option.hpp"
#include "t81/core/Result.hpp"
#include "t81/core/T81Promise.hpp"
#include "t81/core/T81Thread.hpp"
#include "t81/core/T81Network.hpp"
#include "t81/core/T81Discovery.hpp"

#endif // T81_ALL_HPP
