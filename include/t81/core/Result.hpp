#pragma once

#include "t81/core/T81Result.hpp"

namespace t81 {

// Alias to align the T81Lang surface type `Result[T, E]` with the core
// implementation `T81Result<T, E>`.
template <typename T, typename E>
using Result = T81Result<T, E>;

} // namespace t81

