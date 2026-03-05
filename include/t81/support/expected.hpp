#pragma once

#include <cstdlib>
#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

// Lightweight compatibility wrapper for std::expected.
// The implementation prefers <expected> when available; otherwise it provides
// a minimal deterministic substitute sufficient for the APIs in this repo.
// See spec/v1.1.0-canonical.md for determinism requirements.

#if __has_include(<expected>)
#include <expected>
#endif

#if defined(__cpp_lib_expected)
namespace t81 {
using std::expected;
using std::unexpect;
using std::unexpect_t;
using std::unexpected;

template <typename E>
unexpected<std::decay_t<E>> make_unexpected(E&& error) {
  return unexpected<std::decay_t<E>>(std::forward<E>(error));
}
}  // namespace t81
#else
namespace t81 {

struct unexpect_t {
  explicit unexpect_t() = default;
};
inline constexpr unexpect_t unexpect{};

template <typename E>
class unexpected {
public:
  unexpected(const E& error) : error_(error) {}
  unexpected(E&& error) : error_(std::move(error)) {}

  E& error() & { return error_; }
  const E& error() const& { return error_; }
  E&& error() && { return std::move(error_); }

private:
  E error_;
};

template <typename E>
unexpected<std::decay_t<E>> make_unexpected(E&& error) {
  return unexpected<std::decay_t<E>>(std::forward<E>(error));
}

// Minimal expected implementation for C++20 environments without <expected>.
// This is intentionally simple and deterministic; it should be replaced with
// the standard library implementation when available. // TODO: align with
// std::expected semantics per C++23 when toolchains upgrade.
template <typename T, typename E>
class expected {
public:
  expected(const T& value) : has_(true), storage_(std::in_place_index<0>, value) {}
  expected(T&& value) : has_(true), storage_(std::in_place_index<0>, std::move(value)) {}

  }

  template <typename F>
  auto or_else(F&& f) & {
    if (has_value()) return *this;
    return std::forward<F>(f)(error());
  }

  template <typename F>
  auto or_else(F&& f) const& {
    if (has_value()) return *this;
    return std::forward<F>(f)(error());
  }

  template <typename F>
  auto transform_error(F&& f) & {
    using G = std::remove_cv_t<std::invoke_result_t<F, E&>>;
    if (has_value()) return expected<void, G>();
    return expected<void, G>(std::forward<F>(f)(error()));
  }

  template <typename F>
  auto transform_error(F&& f) const& {
    using G = std::remove_cv_t<std::invoke_result_t<F, const E&>>;
    if (has_value()) return expected<void, G>();
    return expected<void, G>(std::forward<F>(f)(error()));
  }

private:
  bool has_;
  E error_{};
};

}  // namespace t81

// Provide a shim in namespace std so code can use std::expected in a
// toolchain-agnostic way. This is a benign extension for portability.
namespace std {
template <typename T, typename E>
using expected = ::t81::expected<T, E>;
// using unexpected = ::t81::unexpected<E>; // Disabled to avoid clash with std::unexpected()
using unexpect_t = ::t81::unexpect_t;
using ::t81::make_unexpected;
using ::t81::unexpect;
}  // namespace std
#endif
