/**
 * @file T81Bytes.hpp
 * @brief Lightweight byte buffer for canonical, deterministic byte handling.
 */
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace t81 {

class T81Bytes {
public:
  using value_type = std::uint8_t;
  using container_type = std::vector<value_type>;
  using size_type = container_type::size_type;
  using iterator = container_type::iterator;
  using const_iterator = container_type::const_iterator;

private:
  container_type data_{};

public:
  T81Bytes() = default;

  explicit T81Bytes(size_type size) : data_(size) {}

  explicit T81Bytes(container_type data) noexcept : data_(std::move(data)) {}

  T81Bytes(std::initializer_list<value_type> init) : data_(init) {}

  T81Bytes(const value_type* data, size_type size) {
    if (data && size > 0) {
      data_.assign(data, data + size);
    }
  }

  template <std::size_t N>
  T81Bytes(const value_type (&arr)[N]) {
    data_.assign(arr, arr + N);
  }

  T81Bytes(const char* s) {
    if (!s) return;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s);
    while (*p != 0) {
      data_.push_back(static_cast<value_type>(*p));
      ++p;
    }
  }

  static T81Bytes from_string(std::string_view sv) {
    container_type buf;
    buf.reserve(sv.size());
    for (char c : sv) {
      buf.push_back(static_cast<value_type>(static_cast<unsigned char>(c)));
    }
    return T81Bytes(std::move(buf));
  }

  [[nodiscard]] size_type size() const noexcept { return data_.size(); }
  [[nodiscard]] bool empty() const noexcept { return data_.empty(); }

  [[nodiscard]] value_type* data() noexcept { return data_.data(); }
  [[nodiscard]] const value_type* data() const noexcept { return data_.data(); }

  [[nodiscard]] iterator begin() noexcept { return data_.begin(); }
  [[nodiscard]] iterator end() noexcept { return data_.end(); }
  [[nodiscard]] const_iterator begin() const noexcept { return data_.begin(); }
  [[nodiscard]] const_iterator end() const noexcept { return data_.end(); }

  value_type& operator[](size_type idx) noexcept { return data_[idx]; }
  const value_type& operator[](size_type idx) const noexcept { return data_[idx]; }

  void push_back(value_type b) { data_.push_back(b); }

  T81Bytes& operator+=(const T81Bytes& other) {
    data_.insert(data_.end(), other.data_.begin(), other.data_.end());
    return *this;
  }

  [[nodiscard]] T81Bytes slice(size_type offset, size_type length) const {
    if (offset >= data_.size()) return T81Bytes{};
    const size_type end_index = std::min<size_type>(data_.size(), offset + length);
    return T81Bytes(container_type(data_.begin() + offset, data_.begin() + end_index));
  }

  [[nodiscard]] std::string to_string() const { return std::string(data_.begin(), data_.end()); }

  [[nodiscard]] std::string to_utf8() const { return to_string(); }

  friend bool operator==(const T81Bytes& a, const T81Bytes& b) noexcept {
    return a.data_ == b.data_;
  }

  friend T81Bytes operator+(const T81Bytes& a, const T81Bytes& b) {
    T81Bytes out = a;
    out += b;
    return out;
  }
};

}  // namespace t81

inline t81::T81Bytes operator""_b(const char* str, std::size_t len) {
  return t81::T81Bytes(reinterpret_cast<const std::uint8_t*>(str), len);
}
