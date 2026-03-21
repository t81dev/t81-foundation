#include "t81/types/T81Int.hpp"
#include "t81/types/detail/dmath.hpp"
#include "t81/types/T81Float.hpp"

namespace t81::v1 {
  template <std::size_t M, std::size_t E> class T81Float;
}

namespace t81::core::math::t81_soft_math {

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_exp(const t81::v1::T81Float<M, E>& x) { return t81::core::detail::exp(x); }
template t81::v1::T81Float<27, 9> t81_exp(const t81::v1::T81Float<27, 9>& x);
template t81::v1::T81Float<72, 9> t81_exp(const t81::v1::T81Float<72, 9>& x);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_log(const t81::v1::T81Float<M, E>& x) { return t81::core::detail::log(x); }
template t81::v1::T81Float<27, 9> t81_log(const t81::v1::T81Float<27, 9>& x);
template t81::v1::T81Float<72, 9> t81_log(const t81::v1::T81Float<72, 9>& x);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_sin(const t81::v1::T81Float<M, E>& x) { return t81::core::detail::sin(x); }
template t81::v1::T81Float<27, 9> t81_sin(const t81::v1::T81Float<27, 9>& x);
template t81::v1::T81Float<72, 9> t81_sin(const t81::v1::T81Float<72, 9>& x);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_cos(const t81::v1::T81Float<M, E>& x) { return t81::core::detail::cos(x); }
template t81::v1::T81Float<27, 9> t81_cos(const t81::v1::T81Float<27, 9>& x);
template t81::v1::T81Float<72, 9> t81_cos(const t81::v1::T81Float<72, 9>& x);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_sqrt(const t81::v1::T81Float<M, E>& x) { return t81::core::detail::sqrt(x); }
template t81::v1::T81Float<27, 9> t81_sqrt(const t81::v1::T81Float<27, 9>& x);
template t81::v1::T81Float<72, 9> t81_sqrt(const t81::v1::T81Float<72, 9>& x);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_div(const t81::v1::T81Float<M, E>& a, const t81::v1::T81Float<M, E>& b) { return t81::core::detail::div(a, b); }
template t81::v1::T81Float<27, 9> t81_div(const t81::v1::T81Float<27, 9>& a, const t81::v1::T81Float<27, 9>& b);
template t81::v1::T81Float<72, 9> t81_div(const t81::v1::T81Float<72, 9>& a, const t81::v1::T81Float<72, 9>& b);
template t81::v1::T81Float<18, 9> t81_div(const t81::v1::T81Float<18, 9>& a, const t81::v1::T81Float<18, 9>& b);

template <std::size_t M, std::size_t E>
t81::v1::T81Float<M, E> t81_pow(const t81::v1::T81Float<M, E>& a, const t81::v1::T81Float<M, E>& b) { return t81::core::detail::pow(a, b); }
template t81::v1::T81Float<27, 9> t81_pow(const t81::v1::T81Float<27, 9>& a, const t81::v1::T81Float<27, 9>& b);
template t81::v1::T81Float<72, 9> t81_pow(const t81::v1::T81Float<72, 9>& a, const t81::v1::T81Float<72, 9>& b);
template t81::v1::T81Float<18, 9> t81_pow(const t81::v1::T81Float<18, 9>& a, const t81::v1::T81Float<18, 9>& b);

}  // namespace t81::core::math::t81_soft_math