/**
 * @file T81List.hpp
 * @brief T81List — dynamic ternary-native list container.
 *
 * T81List<E> is a thin, future-proof wrapper over std::vector<E> with:
 *   • Size and element constraints suitable for ternary-native storage.
 *   • Clear, minimal API (push_back, iterators, concatenation, hashing).
 *   • Clean comparison and pretty-printing semantics.
 *
 * Notes:
 *   • Currently backed by std::vector<E>; the API is shaped so the storage
 *     can later be migrated to a true tryte-aligned ternary buffer without
 *     breaking user code.
 */

#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81String.hpp"

#include <cstddef>
#include <cstdint>
#include <compare>
#include <concepts>
#include <functional>
#include <iterator>
#include <ostream>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace t81 {

// ======================================================================
// T81List<E> – dynamic sequence with ternary-friendly constraints
// ======================================================================
template <typename E>
    // relaxed upper bound to support larger payloads like T81Entropy
    requires (!std::is_void_v<E> && (sizeof(E) <= 64))
class T81List {
public:
    using value_type      = E;
    using reference       = E&;
    using const_reference = const E&;
    using iterator        = typename std::vector<E>::iterator;
    using const_iterator  = typename std::vector<E>::const_iterator;
    using size_type       = std::size_t;

private:
    // Internal storage; alignment chosen to be friendly to cache/tryte lines.
    alignas(64) std::vector<E> data_;

public:
    //===================================================================
    // Construction
    //===================================================================

    T81List() = default;

    explicit T81List(size_type n, const E& value = E{})
        : data_(n, value) {}

    T81List(std::initializer_list<E> init)
        : data_(init) {}

    T81List(const T81List&)            = default;
    T81List(T81List&&) noexcept        = default;
    T81List& operator=(const T81List&) = default;
    T81List& operator=(T81List&&) noexcept = default;

    //===================================================================
    // Element access
    //===================================================================

    [[nodiscard]] reference operator[](size_type i) noexcept {
        return data_[i];
    }

    [[nodiscard]] const_reference operator[](size_type i) const noexcept {
        return data_[i];
    }

    [[nodiscard]] reference front() noexcept {
        return data_.front();
    }

    [[nodiscard]] const_reference front() const noexcept {
        return data_.front();
    }

    [[nodiscard]] reference back() noexcept {
        return data_.back();
    }

    [[nodiscard]] const_reference back() const noexcept {
        return data_.back();
    }

    //===================================================================
    // Capacity
    //===================================================================

    [[nodiscard]] size_type size() const noexcept {
        return data_.size();
    }

    [[nodiscard]] size_type capacity() const noexcept {
        return data_.capacity();
    }

    [[nodiscard]] bool empty() const noexcept {
        return data_.empty();
    }

    // Reserve with a coarse "tryte-aligned" hint (multiple of 4).
    void reserve(size_type n) {
        const size_type rounded = ((n + 3) / 4) * 4;
        data_.reserve(rounded);
    }

    void shrink_to_fit() {
        data_.shrink_to_fit();
    }

    //===================================================================
    // Modifiers
    //===================================================================

    void push_back(const E& value) {
        data_.push_back(value);
    }

    void push_back(E&& value) {
        data_.push_back(std::move(value));
    }

    template <typename... Args>
        requires std::constructible_from<E, Args...>
    reference emplace_back(Args&&... args) {
        return data_.emplace_back(std::forward<Args>(args)...);
    }

    void pop_back() {
        data_.pop_back();
    }

    void clear() noexcept {
        data_.clear();
    }

    //===================================================================
    // Concatenation
    //
    //  • For copyable E: + and +=(const&) use copies.
    //  • For move-only E (e.g., T81Entropy): use +=(T81List&&) with moves.
    //===================================================================

    // Copying concatenation only if E is copy-constructible.
    friend T81List
    operator+(T81List lhs, const T81List& rhs)
        requires std::is_copy_constructible_v<E>
    {
        lhs.data_.insert(lhs.data_.end(), rhs.data_.begin(), rhs.data_.end());
        return lhs;
    }

    // Copying += only if E is copy-constructible.
    T81List&
    operator+=(const T81List& o)
        requires std::is_copy_constructible_v<E>
    {
        data_.insert(data_.end(), o.data_.begin(), o.data_.end());
        return *this;
    }

    // Move-based += for move-only or move-preferred types.
    T81List&
    operator+=(T81List&& o) noexcept(std::is_nothrow_move_constructible_v<E>) {
        data_.insert(
            data_.end(),
            std::make_move_iterator(o.data_.begin()),
            std::make_move_iterator(o.data_.end())
        );
        o.clear();
        return *this;
    }

    //===================================================================
    // Iterators
    //===================================================================

    [[nodiscard]] iterator begin() noexcept {
        return data_.begin();
    }

    [[nodiscard]] iterator end() noexcept {
        return data_.end();
    }

    [[nodiscard]] const_iterator begin() const noexcept {
        return data_.begin();
    }

    [[nodiscard]] const_iterator end() const noexcept {
        return data_.end();
    }

    [[nodiscard]] const_iterator cbegin() const noexcept {
        return data_.cbegin();
    }

    [[nodiscard]] const_iterator cend() const noexcept {
        return data_.cend();
    }

    //===================================================================
    // Comparison
    //===================================================================

    [[nodiscard]] auto operator<=>(const T81List& o) const noexcept = default;
    [[nodiscard]] bool operator==(const T81List& o) const noexcept  = default;

    //===================================================================
    // Hash – suitable for T81Map<T81Symbol, T81List<E>>
    //===================================================================

    [[nodiscard]] std::uint64_t hash() const {
        std::uint64_t h = 0xcbf29ce484222325ull; // FNV offset basis

        for (const auto& e : data_) {
            std::uint64_t part = 0;

            if constexpr (requires(const E& x) { x.hash(); }) {
                part = static_cast<std::uint64_t>(e.hash());
            } else {
                std::hash<E> hasher;
                part = static_cast<std::uint64_t>(hasher(e));
            }

            h ^= part;
            h *= 0x100000001b3ull; // FNV prime
        }

        return h;
    }

    //===================================================================
    // Raw access
    //===================================================================

    [[nodiscard]] std::span<E> span() noexcept {
        return std::span<E>(data_.data(), data_.size());
    }

    [[nodiscard]] std::span<const E> span() const noexcept {
        return std::span<const E>(data_.data(), data_.size());
    }

    [[nodiscard]] const E* data() const noexcept {
        return data_.data();
    }

    [[nodiscard]] E* data() noexcept {
        return data_.data();
    }
};

// ======================================================================
// Deduction guide – allows T81List{1,2,3} style construction
// ======================================================================

template <typename... Ts>
T81List(Ts...) -> T81List<std::common_type_t<Ts...>>;

} // namespace t81

// ======================================================================
// std integration
// ======================================================================

namespace std {
template <typename E>
struct hash<t81::T81List<E>> {
    size_t operator()(const t81::T81List<E>& list) const noexcept {
        return static_cast<size_t>(list.hash());
    }
};
} // namespace std

// ======================================================================
// Pretty printing
// ======================================================================

template <typename E>
std::ostream& operator<<(std::ostream& os, const t81::T81List<E>& list) {
    os << "[";
    bool first = true;
    for (const auto& e : list) {
        if (!first) {
            os << ", ";
        }
        if constexpr (requires { os << e; }) {
            os << e;
        } else {
            os << "<obj>";
        }
        first = false;
    }
    return os << "]";
}
