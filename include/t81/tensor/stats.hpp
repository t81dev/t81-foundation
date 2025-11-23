// include/t81/tensor/stats.hpp
#pragma once
#include "t81/tensor/tensor.hpp"
namespace t81::tensor {
// axis can be negative; keepdim optional
template<class T> Tensor<int64_t> argmax(const Tensor<T>& x, int axis);
template<class T> Tensor<T> softmax(const Tensor<T>& x, int axis);
template<class T> Tensor<T> reduce_sum(const Tensor<T>& x, std::vector<int> axes, bool keepdim=false);
template<class T> Tensor<T> reduce_max(const Tensor<T>& x, std::vector<int> axes, bool keepdim=false);
}
