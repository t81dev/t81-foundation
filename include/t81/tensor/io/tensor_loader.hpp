#pragma once
#include <istream>
#include <ostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include "t81/tensor.hpp"

namespace t81::io {

// Text tensor format:
//
// Line 1: RANK D1 D2 ... DR
// Line 2: v0 v1 v2 ... v{D1*...*DR-1}   (row-major)
//
// Example (2x3):
//   2 2 3
//   1 2 3 4 5 6

// Parse from an input stream.
T729Tensor load_tensor_txt(std::istream& in);

// Parse from a file path.
inline T729Tensor load_tensor_txt_file(const std::string& path) {
  std::ifstream f(path);
  if (!f) throw std::runtime_error("load_tensor_txt_file: cannot open: " + path);
  return load_tensor_txt(f);
}

// Write to an output stream.
void save_tensor_txt(std::ostream& out, const T729Tensor& t);

// Write to a file path (truncates).
inline void save_tensor_txt_file(const std::string& path, const T729Tensor& t) {
  std::ofstream f(path);
  if (!f) throw std::runtime_error("save_tensor_txt_file: cannot open: " + path);
  save_tensor_txt(f, t);
}

} // namespace t81::io
