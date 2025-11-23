#pragma once

#include <istream>
#include <ostream>

#include "t81/tensor.hpp"

namespace t81::io {
// Parse text tensor representation (spec-driven placeholder).
T729Tensor load_tensor_txt(std::istream& in);
// Serialize tensor back to text.
void save_tensor_txt(std::ostream& out, const T729Tensor& t);
}  // namespace t81::io

