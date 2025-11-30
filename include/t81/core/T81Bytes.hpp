/**
 * @file T81Bytes.hpp
 * @brief Lightweight byte buffer for canonical, deterministic byte handling.
 *
 * This header provides the `T81Bytes` class, a small utility wrapper around
 * a contiguous sequence of bytes. It is intentionally simple:
 *
 *   • Owns a `std::vector<std::uint8_t>` internally.
 *   • Supports construction from strings, arrays, and raw byte ranges.
 *   • Provides `slice(offset, length)` to extract subranges.
 *   • Provides equality comparison and conversion back to std::string.
 *   • Integrates with a `"..."_b` user-defined literal for ASCII bytes.
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
    using value_type     = std::uint8_t;
    using container_type = std::vector<value_type>;
    using size_type      = container_type::size_type;
    using iterator       = container_type::iterator;
    using const_iterator = container_type::const_iterator;

private:
    container_type data_{};

public:
    // ================================================================
    // Construction
    // ================================================================
    T81Bytes() = default;

    explicit T81Bytes(container_type data) noexcept
        : data_(std::move(data)) {}

    T81Bytes(std::initializer_list<value_type> init)
        : data_(init) {}

    // Construct from raw pointer + size
    T81Bytes(const value_type* data, size_type size) {
        if (data && size > 0) {
            data_.assign(data, data + size);
        }
    }

    // Construct from fixed-size uint8_t[N] array
    template <std::size_t N>
    T81Bytes(const value_type (&arr)[N]) {
        data_.assign(arr, arr + N);
    }

    // Construct from C-string (ASCII)
    T81Bytes(const char* s) {
        if (!s) return;
        const unsigned char* p = reinterpret_cast<const unsigned char*>(s);
        while (*p != 0) {
            data_.push_back(static_cast<value_type>(*p));
            ++p;
        }
    }

    // Construct from string-like (factory, not ctor, to avoid ambiguity)
    static T81Bytes from_string(std::string_view sv) {
        container_type buf;
        buf.reserve(sv.size());
        for (char c : sv) {
            buf.push_back(static_cast<value_type>(
                static_cast<unsigned char>(c)
            ));
        }
        return T81Bytes(std::move(buf));
    }

    // ================================================================
    // Basic accessors
    // ================================================================
    [[nodiscard]] size_type size() const noexcept { return data_.size(); }
    [[nodiscard]] bool      empty() const noexcept { return data_.empty(); }

    [[nodiscard]] value_type*       data() noexcept { return data_.data(); }
    [[nodiscard]] const value_type* data() const noexcept { return data_.data(); }

    [[nodiscard]] iterator       begin() noexcept { return data_.begin(); }
    [[nodiscard]] iterator       end() noexcept { return data_.end(); }
    [[nodiscard]] const_iterator begin() const noexcept { return data_.begin(); }
    [[nodiscard]] const_iterator end() const noexcept { return data_.end(); }
    [[nodiscard]] const_iterator cbegin() const noexcept { return data_.cbegin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return data_.cend(); }

    value_type& operator[](size_type idx) noexcept { return data_[idx]; }
    const value_type& operator[](size_type idx) const noexcept { return data_[idx]; }

    void push_back(value_type b) { data_.push_back(b); }

    T81Bytes& append(const T81Bytes& other) {
        data_.insert(data_.end(), other.data_.begin(), other.data_.end());
        return *this;
    }

    // ================================================================
    // Slice – safe subrange extraction
    // ================================================================
    //
    // Returns a new T81Bytes containing at most `length` bytes starting
    // at `offset`. If offset >= size, returns an empty T81Bytes.
    [[nodiscard]] T81Bytes slice(size_type offset, size_type length) const {
        if (offset >= data_.size()) {
            return T81Bytes{};
        }
        const size_type end_index =
            std::min<size_type>(data_.size(), offset + length);
        container_type sub;
        sub.reserve(end_index - offset);
        sub.insert(
            sub.end(),
            data_.begin() + static_cast<std::ptrdiff_t>(offset),
            data_.begin() + static_cast<std::ptrdiff_t>(end_index)
        );
        return T81Bytes(std::move(sub));
    }

    // ================================================================
    // Conversion back to std::string (ASCII-oriented)
    // ================================================================
    [[nodiscard]] std::string to_string() const {
        return std::string(data_.begin(), data_.end());
    }

    // ================================================================
    // Comparison
    // ================================================================
    friend bool operator==(const T81Bytes& a, const T81Bytes& b) noexcept {
        return a.data_ == b.data_;
    }

    friend bool operator!=(const T81Bytes& a, const T81Bytes& b) noexcept {
        return !(a == b);
    }

    // ================================================================
    // Concatenation
    // ================================================================
    friend T81Bytes operator+(const T81Bytes& a, const T81Bytes& b) {
        T81Bytes out;
        out.data_.reserve(a.size() + b.size());
        out.data_.insert(out.data_.end(), a.data_.begin(), a.data_.end());
        out.data_.insert(out.data_.end(), b.data_.begin(), b.data_.end());
        return out;
    }
};

// A small, canonical genesis phrase as bytes.
//
// We explicitly pass a std::string_view to avoid overload ambiguity.
inline const T81Bytes GENESIS_BLOCK =
    T81Bytes::from_string(std::string_view{"In the beginning was the trit."});

} // namespace t81

// ======================================================================
// User-defined literal: "hello"_b → t81::T81Bytes
// ======================================================================
//
// Defined in the global namespace so that `"..."_b` works without an
// extra using directive.
inline t81::T81Bytes operator"" _b(const char* str, std::size_t len) {
    t81::T81Bytes::container_type buf;
    buf.reserve(len);
    for (std::size_t i = 0; i < len; ++i) {
        buf.push_back(static_cast<t81::T81Bytes::value_type>(
            static_cast<unsigned char>(str[i])
        ));
    }
    return t81::T81Bytes(std::move(buf));
}
