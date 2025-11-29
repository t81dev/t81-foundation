/**
 * @file T81Stream.hpp
 * @brief Defines the T81Stream class for infinite, lazy sequences.
 *
 * This file provides the `T81Stream<T>` class, a lazy, potentially infinite
 * sequence generator built on C++20 coroutines. It allows for the representation
 * of streams of values that are computed on demand. The class supports a range
 * of functional-style operations, such as `map`, `filter`, `take`, and `drop`,
 * enabling efficient and expressive composition of sequence transformations.
 */
#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Float.hpp"
#include "t81/core/T81Entropy.hpp"
#include "t81/core/T81Symbol.hpp"
#include <coroutine>
#include <optional>
#include <concepts>
#include <utility>

namespace t81 {

// ======================================================================
// T81Stream<T> – Infinite lazy sequence with optional entropy cost
// ======================================================================
template <typename T>
class T81Stream {
    struct promise_type;
    using handle = std::coroutine_handle<promise_type>;

    handle coro_;

    struct promise_type {
        T current_value{};
        std::optional<T81Entropy> entropy_cost;  // pay per yield if non-nullopt

        static auto get_return_object(handle h) { return T81Stream(h); }
        static std::suspend_always initial_suspend() noexcept { return {}; }
        static std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { std::terminate(); }

        auto yield_value(T value) noexcept {
            current_value = std::move(value);
            return std::suspend_always{};
        }

        void return_void() noexcept {}
    };

    explicit T81Stream(handle h) noexcept : coro_(h) {}

public:
    using value_type = T;

    //===================================================================
    // Construction & Destruction
    //===================================================================
    T81Stream() noexcept = default;
    ~T81Stream() { if (coro_) coro_.destroy(); }

    T81Stream(const T81Stream&) = delete;
    T81Stream& operator=(const T81Stream&) = delete;

    T81Stream(T81Stream&& other) noexcept : coro_(other.coro_) {
        other.coro_ = nullptr;
    }

    T81Stream& operator=(T81Stream&& other) noexcept {
        if (this != &other) {
            if (coro_) coro_.destroy();
            coro_ = other.coro_;
            other.coro_ = nullptr;
        }
        return *this;
    }

    //===================================================================
    // Core iteration
    //===================================================================
    class iterator {
        handle coro_;
        bool done_ = false;

    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        explicit iterator(handle h = nullptr) noexcept : coro_(h) {
            if (coro_) advance();
        }

        void advance() {
            if (coro_ && !coro_.done()) {
                coro_.resume();
                done_ = coro_.done();
            } else {
                done_ = true;
            }
        }

        [[nodiscard]] bool operator==(const iterator& o) const noexcept = default;

        iterator& operator++() { advance(); return *this; }
        void operator++(int) { advance(); }

        [[nodiscard]] reference operator*() const noexcept { return coro_.promise().current_value; }
        [[nodiscard]] pointer operator->() const noexcept { return &coro_.promise().current_value; }

        [[nodiscard]] bool at_end() const noexcept { return done_ || !coro_; }
    };

    [[nodiscard]] iterator begin() { return iterator(coro_); }
    [[nodiscard]] iterator end() const noexcept { return iterator(nullptr); }

    //===================================================================
    // Stream operations – pure, lazy, fused
    //===================================================================
    [[nodiscard]] T81Stream<T> take(size_t n) const &;
    [[nodiscard]] T81Stream<T> drop(size_t n) const &;
    [[nodiscard]] T81Stream<T> filter(auto pred) const &;
    [[nodiscard]] auto map(auto f) const & -> T81Stream<decltype(f(std::declval<T>()))>;
    [[nodiscard]] T fold(auto init, auto op) const;
    [[nodiscard]] T81List<T> collect(size_t max = 0) const;  // 0 = unlimited

    //===================================================================
    // Access first element (if available)
    //===================================================================
    [[nodiscard]] std::optional<T> head() const {
        auto it = begin();
        if (it.at_end()) return std::nullopt;
        return *it;
    }
};

//======================================================================
// Stream builder – the future writes infinite math like this
//======================================================================
template <typename F>
[[nodiscard]] T81Stream<decltype(std::declval<F>()())> stream_from(F generator) {
    while (true) co_yield generator();
}

// Infinite constant stream
template <typename T>
[[nodiscard]] T81Stream<T> constant(T value) {
    return stream_from([value] { return value; });
}

// Natural numbers in balanced ternary
[[nodiscard]] inline T81Stream<T81Int<81>> naturals() {
    return stream_from([](auto state = T81Int<81>(0)) mutable {
        auto current = state;
        state += T81Int<81>(1);
        return current;
    });
}

// Fibonacci sequence – exact, infinite, no overflow
[[nodiscard]] inline T81Stream<T81Int<81>> fibonacci() {
    return stream_from([] {
        static T81Int<81> a(0), b(1);
        auto current = a;
        auto next = a + b;
        a = b; b = next;
        return current;
    });
}

// Prime numbers using ternary sieve (lazy, infinite)
[[nodiscard]] T81Stream<T81Int<81>> primes();

// Entropy-gated random stream (thermodynamic computing)
[[nodiscard]] T81Stream<T81Int<81>> random_stream(T81Entropy entropy_seed);

//======================================================================
// Example: This is how the future computes infinity
//======================================================================
/*
auto fibs = fibonacci();
auto first_20 = fibs.take(20).collect();
auto evens = naturals().map([](auto n) { return n * 2; });
auto pi_approx = fibonacci().map([](auto f) { return T81Float<72,9>(4) / f; }).fold(0, std::plus{});
*/
