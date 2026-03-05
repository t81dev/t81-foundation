#pragma once

#include <variant>
#include <optional>
#include <string>

namespace t81 {

template <typename T, typename E>
class expected {
public:
  expected(const T& value) : has_(true), storage_(value) {}
  expected(T&& value) : has_(true), storage_(std::move(value)) {}

  template <typename Err = E>
  expected(Err&& error) : has_(false), storage_(std::forward<Err>(error)) {}

  bool has_value() const noexcept { return has_; }
  explicit operator bool() const noexcept { return has_; }

  const T& value() const {
    if (!has_) std::abort();
    return std::get<T>(storage_);
  }

  const E& error() const {
    if (has_) std::abort();
    return std::get<E>(storage_);
  }

private:
  bool has_;
  std::variant<T, E> storage_;
};

template <typename E>
class unexpected {
public:
  unexpected(E&& error) : error_(std::forward<E>(error)) {}
  const E& error() const { return error_; }

private:
  E error_;
};

template <typename E>
unexpected<std::decay_t<E>> make_unexpected(E&& error) {
  return unexpected<std::decay_t<E>>(std::forward<E>(error));
}

} // namespace t81
