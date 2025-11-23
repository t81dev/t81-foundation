// src/tensor/stats.cpp (softmax: numerically stable)
template<class T>
Tensor<T> softmax(const Tensor<T>& x, int axis) {
  auto m = reduce_max(x, {axis}, /*keepdim=*/true);
  auto z = exp(x - m);                 // broadcasting already in place
  auto s = reduce_sum(z, {axis}, true);
  return z / s;
}
