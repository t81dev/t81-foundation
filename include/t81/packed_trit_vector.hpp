#pragma once

// Suppress the direct-include deprecation notice that
// t81/experimental/packed_trit_vector.hpp emits to callers that bypass this
// stable header.  This header IS the approved include path (RFC-0041).
#define T81_PACKED_TRIT_VECTOR_STABLE_INCLUDE
#include "t81/experimental/packed_trit_vector.hpp"
#undef T81_PACKED_TRIT_VECTOR_STABLE_INCLUDE

namespace t81 {

// RFC-0044 stable surface: the experimental implementation remains the
// underlying engine for now, but public consumers should bind to this header.
using PackedTritVector = experimental::PackedTritVector;
using ComputeTritVector = experimental::ComputeTritVector;

namespace packed {

using PackedTritVector = t81::PackedTritVector;
using ComputeTritVector = t81::ComputeTritVector;

}  // namespace packed

}  // namespace t81
