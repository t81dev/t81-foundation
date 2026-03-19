#pragma once

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

  template <typename Err = E, std::enable_if_t<!std::is_same_v<T, std::decay_t<Err>> &&
                                                   !std::is_same_v<expected, std::decay_t<Err>> &&
                                                   !std::is_same_v<unexpect_t, std::decay_t<Err>> &&
                                                   !std::is_same_v<unexpected<E>, std::decay_t<Err>>,
                                               int> = 0>
  expected(Err&& error) : has_(false), storage_(std::in_place_index<1>, std::forward<Err>(error)) {}

  template <typename... Args>
  expected(unexpect_t, Args&&... args)
      : has_(false), storage_(std::in_place_index<1>, std::forward<Args>(args)...) {}

  expected(const unexpected<E>& unexp)
      : has_(false), storage_(std::in_place_index<1>, unexp.error()) {}
  expected(unexpected<E>&& unexp)
      : has_(false), storage_(std::in_place_index<1>, std::move(unexp).error()) {}

  [[nodiscard]] bool has_value() const noexcept { return has_; }
  [[nodiscard]] explicit operator bool() const noexcept { return has_; }

  T& value() { return std::get<0>(storage_); }
  const T& value() const { return std::get<0>(storage_); }
  T* operator->() { return &std::get<0>(storage_); }
  const T* operator->() const { return &std::get<0>(storage_); }
  T& operator*() { return value(); }
  const T& operator*() const { return value(); }
  E& error() { return std::get<1>(storage_); }
  const E& error() const { return std::get<1>(storage_); }

  template <typename F>
  auto and_then(F&& f) & {
    if (has_value()) return std::invoke(std::forward<F>(f), value());
    return std::remove_cvref_t<std::invoke_result_t<F, T&>>(error());
  }

  template <typename F>
  auto and_then(F&& f) const& {
    if (has_value()) return std::invoke(std::forward<F>(f), value());
    return std::remove_cvref_t<std::invoke_result_t<F, const T&>>(error());
  }

  template <typename F>
  auto transform(F&& f) & {
    using U = std::remove_cv_t<std::invoke_result_t<F, T&>>;
    if (has_value()) return expected<U, E>(std::invoke(std::forward<F>(f), value()));
    return expected<U, E>(error());
  }

  template <typename F>
  auto transform(F&& f) const& {
    using U = std::remove_cv_t<std::invoke_result_t<F, const T&>>;
    if (has_value()) return expected<U, E>(std::invoke(std::forward<F>(f), value()));
    return expected<U, E>(error());
  }

  template <typename F>
  auto or_else(F&& f) & {
    if (has_value()) return *this;
    return std::invoke(std::forward<F>(f), error());
  }

  template <typename F>
  auto or_else(F&& f) const& {
    if (has_value()) return *this;
    return std::invoke(std::forward<F>(f), error());
  }

  template <typename F>
  auto transform_error(F&& f) & {
    using G = std::remove_cv_t<std::invoke_result_t<F, E&>>;
    if (has_value()) return expected<T, G>(value());
    return expected<T, G>(std::invoke(std::forward<F>(f), error()));
  }

  template <typename F>
  auto transform_error(F&& f) const& {
    using G = std::remove_cv_t<std::invoke_result_t<F, const E&>>;
    if (has_value()) return expected<T, G>(value());
    return expected<T, G>(std::invoke(std::forward<F>(f), error()));
  }

private:
  bool has_;
  std::variant<T, E> storage_;
};

// Partial specialization for void success.
template <typename E>
class expected<void, E> {
public:
  expected() : has_(true) {}
  expected(const E& error) : has_(false), error_(error) {}
  expected(E&& error) : has_(false), error_(std::move(error)) {}

  template <typename... Args>
  expected(unexpect_t, Args&&... args) : has_(false), error_(std::forward<Args>(args)...) {}

  expected(const unexpected<E>& unexp) : has_(false), error_(unexp.error()) {}
  expected(unexpected<E>&& unexp) : has_(false), error_(std::move(unexp).error()) {}

  [[nodiscard]] bool has_value() const noexcept { return has_; }
  [[nodiscard]] explicit operator bool() const noexcept { return has_; }

  void value() const noexcept {}
  E& error() { return error_; }
  const E& error() const { return error_; }

  template <typename F>
  auto and_then(F&& f) & {
    if (has_value()) return std::invoke(std::forward<F>(f));
    return std::remove_cvref_t<std::invoke_result_t<F>>(error());
  }

  template <typename F>
  auto and_then(F&& f) const& {
    if (has_value()) return std::invoke(std::forward<F>(f));
    return std::remove_cvref_t<std::invoke_result_t<F>>(error());
  }

  template <typename F>
  auto transform(F&& f) & {
    using U = std::remove_cv_t<std::invoke_result_t<F>>;
    if (has_value()) return expected<U, E>(std::invoke(std::forward<F>(f)));
    return expected<U, E>(error());
  }

  template <typename F>
  auto transform(F&& f) const& {
    using U = std::remove_cv_t<std::invoke_result_t<F>>;
    if (has_value()) return expected<U, E>(std::invoke(std::forward<F>(f)));
    return expected<U, E>(error());
  }

  template <typename F>
  auto or_else(F&& f) & {
    if (has_value()) return *this;
    return std::invoke(std::forward<F>(f), error());
  }

  template <typename F>
  auto or_else(F&& f) const& {
    if (has_value()) return *this;
    return std::invoke(std::forward<F>(f), error());
  }

  template <typename F>
  auto transform_error(F&& f) & {
    using G = std::remove_cv_t<std::invoke_result_t<F, E&>>;
    if (has_value()) return expected<void, G>();
    return expected<void, G>(std::invoke(std::forward<F>(f), error()));
  }

  template <typename F>
  auto transform_error(F&& f) const& {
    using G = std::remove_cv_t<std::invoke_result_t<F, const E&>>;
    if (has_value()) return expected<void, G>();
    return expected<void, G>(std::invoke(std::forward<F>(f), error()));
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
