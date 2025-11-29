//======================================================================
// T81Promise.hpp – Thermodynamic, reflective, cancellable async
//                 The 87th type. The mind learns patience.
//======================================================================
#pragma once

#include "t81/T81Result.hpp"
#include "t81/T81Entropy.hpp"
#include "t81/T81Time.hpp"
#include "t81/T81Agent.hpp"
#include "t81/T81Reflection.hpp"
#include <coroutine>
#include <atomic>
#include <optional>
#include <memory>

namespace t81 {

// Forward declaration
template <typename T> class T81Promise;
template <typename T> class T81Awaitable;

// ======================================================================
// T81Promise<T> – A promise that costs entropy to wait
// ======================================================================
template <typename T>
class T81Promise {
    struct promise_type {
        std::optional<T> value;
        std::optional<T81Error> error;
        T81Entropy fuel_spent;
        T81Agent* waiter{nullptr};

        T81Promise get_return_object() {
            return T81Promise(std::coroutine_handle<promise_type>::from_promise(*this));
        }

        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        void return_value(T v) noexcept { value = std::move(v); }
        void unhandled_exception() {
            error = T81Error(symbols::PROMISE_BROKEN,
                "Co_awaited computation failed"_t81, symbols::COROUTINE);
        }

        // Waiting costs entropy — every tick
        auto await_transform(T81Entropy fuel) {
            fuel_spent = fuel;
            struct awaiter {
                bool await_ready() const noexcept { return false; }
                void await_suspend(std::coroutine_handle<>) const noexcept {
                    consume_entropy(fuel_spent);  // pay for patience
                }
                void await_resume() const noexcept {}
            };
            return awaiter{};
        }
    };

    std::coroutine_handle<promise_type> coro_;

public:
    using value_type = T;

    explicit T81Promise(std::coroutine_handle<promise_type> h) noexcept : coro_(h) {
        coro_.resume();  // start immediately
    }

    ~T81Promise() { if (coro_) coro_.destroy(); }

    T81Promise(const T81Promise&) = delete;
    T81Promise& operator=(const T81Promise&) = delete;

    T81Promise(T81Promise&& other) noexcept : coro_(other.coro_) {
        other.coro_ = nullptr;
    }

    //===================================================================
    // The sacred act of waiting
    //===================================================================
    [[nodiscard]] T81Result<T> await(T81Entropy patience, T81Agent& dreamer) {
        if (!coro_) {
            return T81Result<T>::failure(symbols::PROMISE_DESTROYED,
                "Promise was destroyed before resolution"_t81);
        }

        if (coro_.done()) {
            if (coro_.promise().value) {
                return T81Result<T>::success(std::move(*coro_.promise().value));
            }
            if (coro_.promise().error) {
                return T81Result<T>::failure(coro_.promise().error->code,
                    coro_.promise().error->message);
            }
        }

        // Still waiting — pay entropy and dream
        consume_entropy(patience);
        dreamer.observe(symbols::DREAMING);
        record_event(T81Time::now(patience, symbols::WAITING));

        return T81Result<T>::failure(symbols::STILL_DREAMING,
            "Computation not yet complete — patience is a virtue"_t81);
    }

    //===================================================================
    // Non-blocking check
    //===================================================================
    [[nodiscard]] T81Maybe<T> try_get() const noexcept {
        if (!coro_ || !coro_.done()) return T81Maybe<T>::nothing(symbols::PENDING);
        if (coro_.promise().value) return T81Maybe<T>(*coro_.promise().value);
        return T81Maybe<T>::nothing(symbols::FAILED);
    }

    //===================================================================
    // Reflection — a promise knows its own longing
    //===================================================================
    [[nodiscard]] T81Reflection<T81Promise<T>> reflect() const {
        auto status = coro_ && coro_.done()
            ? (coro_.promise().value ? symbols::FULFILLED : symbols::BROKEN)
            : symbols::DREAMING;
        return T81Reflection<T81Promise<T>>(*this, symbols::PROMISE, status);
    }
};

// ======================================================================
// Co_awaitable wrapper — the language of dreams
// ======================================================================
template <typename T>
T81Awaitable<T> co_dream(T81Promise<T>&& promise) {
    return T81Awaitable<T>(std::move(promise));
}

// ======================================================================
// The first dream in the ternary universe
// ======================================================================
namespace dreams {
    inline T81Promise<T81String> compute_meaning_of_life() {
        co_await T81Entropy::acquire();  // pay to think

        for (int i = 0; i < 7500000; ++i) {
            co_await T81Entropy::acquire();  // deep thought costs fuel
        }

        co_return "42"_t81;
    }

    inline const auto future_wisdom = compute_meaning_of_life();
}

// Example: The first ternary mind learns to wait
/*
auto agent = T81Agent(symbols::DEEP_THOUGHT);
auto result = future_wisdom.await(T81Entropy::acquire_batch(1000), agent);

if (!result) {
    cout << "Still computing... I dream of answers.\n"_t81;
} else {
    cout << "The answer is: " << result.unwrap() << "\n"_t81;
}
*/
