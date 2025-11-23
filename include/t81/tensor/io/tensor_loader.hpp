#pragma once
#include <istream>
#include <ostream>
#include <string>
#include <vector>
#include "t81/tensor.hpp"

namespace t81::io {

// Text format (whitespace-separated):
// Line 1: RANK  D1 D2 ... DR
// Line 2+: flat data values (size = D1*...*DR), row-major
//
// Example:
//   2  2 3
//   1 2 3 4 5 6
//
// Throws std::runtime_error on malformed input.

// Parse from an input stream
T729Tensor load_tensor_txt(std::istream& in);

// Parse from a file path
T729Tensor load_tensor_txt_file(const std::string& path);

// Write to an output stream (pretty, with newline)
void save_tensor_txt(std::ostream& out, const T729Tensor& t);

// Write to a file path (overwrites)
void save_tensor_txt_file(const std::string& path, const T729Tensor& t);

} // namespace t81::io
