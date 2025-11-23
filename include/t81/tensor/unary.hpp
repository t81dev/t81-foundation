#pragma once
#include <cmath>
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"

namespace t81::ops {

// Map a unary functor over all elements (shape preserved).
template <typename Fn>
inline T729Tensor unary_map(const T729Tensor& x, Fn fn) {
  std::vector<float> out(x.size());
  const auto& d = x.data();
  for (std::size_t i = 0; i < d.size(); ++i) out[i] = fn(d[i]);
  return T729Tensor(x.shape(), std::move(out));
}

inline T729Tensor relu(const T729Tensor& x) {
  return unary_map(x, [](float v){ return v < 0.0f ? 0.0f : v; });
}

inline T729Tensor tanh(const T729Tensor& x) {
  return unary_map(x, [](float v){ return std::tanh(v); });
}

inline T729Tensor exp(const T729Tensor& x) {
  return unary_map(x, [](float v){ return std::exp(v); });
}

inline T729Tensor log(const T729Tensor& x) {
  return unary_map(x, [](float v){
    if (v <= 0.0f) throw std::domain_error("unary log: non-positive input");
    return std::log(v);
  });
}

} // namespace t81::ops
