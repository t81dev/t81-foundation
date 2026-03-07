#pragma once

#include "t81/types/T81Float.hpp"

namespace t81::core::math::t81_soft_math {

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_exp(const t81::v1::T81Float<M, E>& x);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_log(const t81::v1::T81Float<M, E>& x);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_sin(const t81::v1::T81Float<M, E>& x);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_cos(const t81::v1::T81Float<M, E>& x);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_sqrt(const t81::v1::T81Float<M, E>& x);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_div(const t81::v1::T81Float<M, E>& a, const t81::v1::T81Float<M, E>& b);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_pow(const t81::v1::T81Float<M, E>& a, const t81::v1::T81Float<M, E>& b);

}  // namespace t81::core::math::t81_soft_math