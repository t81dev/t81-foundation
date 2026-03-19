#pragma once

#include "t81/experimental/packed_trit_vector.hpp"

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
