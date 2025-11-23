// include/t81/tensor/unary.hpp
#pragma once
#include "t81/tensor/shape.hpp"
#include "t81/tensor/tensor.hpp"

namespace t81::tensor {
template<class T> Tensor<T> exp(const Tensor<T>& x);
template<class T> Tensor<T> log(const Tensor<T>& x);
template<class T> Tensor<T> relu(const Tensor<T>& x);
} // namespace
