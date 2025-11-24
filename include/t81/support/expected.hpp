#pragma once

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
}
#else
namespace t81 {

// Minimal expected implementation for C++20 environments without <expected>.
// This is intentionally simple and deterministic; it should be replaced with
// the standard library implementation when available. // TODO: align with
// std::expected semantics per C++23 when toolchains upgrade.
template <typename T, typename E>
class expected {
 public:
  expected(const T& value) : has_(true), storage_(value) {}
  expected(T&& value) : has_(true), storage_(std::move(value)) {}
  expected(const E& error) : has_(false), storage_(error) {}
  expected(E&& error) : has_(false), storage_(std::move(error)) {}

  [[nodiscard]] bool has_value() const noexcept { return has_; }
  [[nodiscard]] explicit operator bool() const noexcept { return has_; }

  T& value() { return std::get<T>(storage_); }
  const T& value() const { return std::get<T>(storage_); }
  T* operator->() { return &std::get<T>(storage_); }
  const T* operator->() const { return &std::get<T>(storage_); }
  E& error() { return std::get<E>(storage_); }
  const E& error() const { return std::get<E>(storage_); }

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

  [[nodiscard]] bool has_value() const noexcept { return has_; }
  [[nodiscard]] explicit operator bool() const noexcept { return has_; }

  void value() const noexcept {}
  E& error() { return error_; }
  const E& error() const { return error_; }

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
}
#endif
