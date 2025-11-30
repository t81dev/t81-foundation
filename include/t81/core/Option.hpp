#pragma once

#include "t81/core/T81Maybe.hpp"

namespace t81 {

// Lightweight alias for the T81 optional type used by the T81Lang surface syntax.
// Option<T> maps directly to T81Maybe<T> while keeping the familiar name for
// language-level constructs and tests.
template <typename T>
using Option = T81Maybe<T>;

} // namespace t81

