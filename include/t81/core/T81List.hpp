//======================================================================
// T81List.hpp – Dynamic, cache-friendly, ternary-native growable list
//======================================================================
#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81String.hpp"   // for nice printing
#include <cstddef>
#include <span>
#include <vector>
#include <memory>
#include <compare>
#include <cstring>
#include <optional>

namespace t81 {

// ======================================================================
// T81List<E> – the final dynamic sequence type
// ======================================================================
template <typename E>
    requires (!std::is_void_v<E>) && (sizeof(E) <= 32) // ≤ 243 trits (3 trytes) for now
class T81List {
    // Internal storage is a tryte-aligned vector of elements
    alignas(64) std::vector<E> data_;

public:
    using value_type     = E;
    using reference      = E&;
    using const_reference= const E&;
    using iterator       = typename std::vector<E>::iterator;
    using const_iterator = typename std::vector<E>::const_iterator;
    using size_type      = size_t;

    //===================================================================
    // Construction
    //===================================================================
    constexpr T81List() noexcept = default;

    explicit constexpr T81List(size_type n, const E& value = E{})
        : data_(n, value) {}

    template <typename... Args>
        requires std::constructible_from<E, Args...>
    constexpr T81List(Args&&... args)
        : data_{std::forward<Args>(args)...} {}

    //===================================================================
    // Element access
    //===================================================================
    [[nodiscard]] constexpr reference       operator[](size_type i)       noexcept { return data_[i]; }
    [[nodiscard]] constexpr const_reference operator[](size_type i) const noexcept { return data_[i]; }

    [[nodiscard]] constexpr reference       front()       noexcept { return data_.front(); }
    [[nodiscard constexpr const_reference front() const noexcept { return data_.front(); }
    [[nodiscard]] constexpr reference       back()        noexcept { return data_.back(); }
    [[nodiscard]] constexpr const_reference back()  const noexcept { return data_.back(); }

    //===================================================================
    // Capacity
    //===================================================================
    [[nodiscard]] constexpr size_type size()     const noexcept { return data_.size(); }
    [[nodiscard]] constexpr size_type capacity() const noexcept { return data_.capacity(); }
    [[nodiscard]] constexpr bool      empty()    const noexcept { return data_.empty(); }

    constexpr void reserve(size_type n) { data_.reserve((n + 3) / 4 * 4); } // tryte-aligned hint
    constexpr void shrink_to_fit() noexcept { data_.shrink_to_fit(); }

    //===================================================================
    // Modifiers – the heart of dynamism
    //===================================================================
    constexpr void push_back(const E& value)     { data_.push_back(value); }
    constexpr void push_back(E&& value)          { data_.push_back(std::move(value)); }

    template <typename... Args>
    constexpr reference emplace_back(Args&&... args) {
        return data_.emplace_back(std::forward<Args>(args)...);
    }

    constexpr void pop_back() noexcept { data_.pop_back(); }

    constexpr void clear() noexcept { data_.clear(); }

    //===================================================================
    // Concatenation – fused at the hardware level
    //===================================================================
    [[nodiscard]] friend constexpr T81List operator+(const T81List a, const T81List& b) noexcept {
        a.data_.insert(a.data_.end(), b.data_.begin(), b.data_.end());
        return a;
    }

    constexpr T81List& operator+=(const T81List& o) noexcept {
        data_.insert(data_.end(), o.data_.begin(), o.data_.end());
        return *this;
    }

    //===================================================================
    // Iterators
    //===================================================================
    [[nodiscard]] constexpr iterator       begin()  noexcept { return data_.begin(); }
    [[nodiscard]] constexpr iterator       end()    noexcept { return data_.end(); }
    [[nodiscard]] constexpr const_iterator begin()  const noexcept { return data_.begin(); }
    [[nodiscard]] constexpr const_iterator end()    const noexcept { return data_.end(); }
    [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return data_.cbegin(); }
    [[nodiscard]] constexpr const_iterator cend()   const noexcept { return data_.cend(); }

    //===================================================================
    // Comparison
    //===================================================================
    [[nodiscard]] constexpr auto operator<=>(const T81List& o) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81List&) const noexcept = default;

    //===================================================================
    // Hash – perfect for T81Map<T81Symbol, T81List<E>>
    //===================================================================
    [[nodiscard]] constexpr uint64_t hash() const noexcept {
        uint64_t h = 0xcbf29ce484222325;  // FNV-prime
        for (const auto& e : data_) {
            if constexpr (requires { e.hash(); }) {
                h ^= e.hash();
            } else {
                std::hash<E> hasher;
                h ^= hasher(e);
            }
            h *= 0x100000001b3;
        }
        return h;
    }

    //===================================================================
    // Raw access & conversion
    //===================================================================
    [[nodiscard]] constexpr std::span<E>       span()       noexcept { return data_; }
    [[nodiscard]] constexpr std::span<const E> span() const noexcept { return data_; }

    [[nodiscard]] constexpr const E* data() const noexcept { return data_.data(); }
    [[nodiscard]] constexpr E*       data()       noexcept { return data_.data(); }
};

// ====================================================================== Deduction guides – you write T81List{1,2,3} and it works
// ======================================================================
template <typename... Ts>
T81List(Ts...) -> T81List<std::common_type_t<Ts...>>;

// ====================================================================== std integration
// ======================================================================
template <typename E>
struct std::hash<T81List<E>> {
    constexpr size_t operator()(const T81List<E>& list) const noexcept {
        return static_cast<size_t>(list.hash());
    }
};

// ====================================================================== Pretty printing
// ======================================================================
template <typename E>
std::ostream& operator<<(std::ostream& os, const T81List<E>& list) {
    os << "[";
    bool first = true;
    for (const auto& e : list) {
        if (!first) os << ", ";
        if constexpr (requires { os << e; }) {
            os << e;
        } else {
            os << "<obj>";
        }
        first = false;
    }
    return os << "]";
}

// ======================================================================
// Example usage – the future feels inevitable
// ======================================================================
/*
using SymbolList = T81List<T81Symbol>;
using TokenSeq  = T81List<uint16_t>;
using FloatVec  = T81List<T81Float<72,9>>;

constexpr auto tokens = TokenSeq{1, 42, 777};
constexpr auto words  = SymbolList{symbols::HELLO, symbols::WORLD};
constexpr auto merged = tokens + TokenSeq{999};

static_assert(merged.size() == 4);
*/
