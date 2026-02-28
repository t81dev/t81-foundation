/**
 * @file T81Promise.hpp
 * @brief Defines the T81Promise class for thermodynamic, reflective asynchrony.
 */
#pragma once

#include <atomic>
#include <coroutine>
#include <memory>
#include <optional>
#include "t81/types/T81Agent.hpp"
#include "t81/types/T81Entropy.hpp"
#include "t81/types/T81Reflection.hpp"
#include "t81/types/T81Result.hpp"
#include "t81/types/T81String.hpp"
#include "t81/types/T81Time.hpp"

namespace t81 {

template <typename T>
class T81Promise {
public:
  enum class State { PENDING, FULFILLED, BROKEN, CANCELLED };

  struct promise_type {
    std::optional<T> value;
    std::optional<T81Error> error;
    std::atomic<State> state{State::PENDING};
    T81Entropy fuel_spent = acquire_entropy(T81Symbol::intern("PROMISE_INIT"));

    T81Promise get_return_object() {
      return T81Promise(std::coroutine_handle<promise_type>::from_promise(*this));
    }

    // Eager execution: start immediately
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }

    void return_value(T v) noexcept {
      value = std::move(v);
      state.store(State::FULFILLED);
    }

    void unhandled_exception() {
      error.emplace(symbols::PROMISE_BROKEN, T81String("CO_AWAITED COMPUTATION FAILED"),
                    symbols::COROUTINE);
      state.store(State::BROKEN);
    }

    auto await_transform(T81Entropy fuel) {
      struct awaiter {
        T81Entropy fuel_;
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
          h.promise().fuel_spent = std::move(fuel_);
        }
        void await_resume() const noexcept {}
      };
      return awaiter{std::move(fuel)};
    }
  };

  using handle_type = std::coroutine_handle<promise_type>;
  handle_type coro_;

public:
  using value_type = T;

  explicit T81Promise(handle_type h) noexcept : coro_(h) {
    // Coroutine starts eagerly via initial_suspend returning suspend_never
  }

  ~T81Promise() {
    if (coro_) coro_.destroy();
  }

  T81Promise(const T81Promise&) = delete;
  T81Promise& operator=(const T81Promise&) = delete;

  T81Promise(T81Promise&& other) noexcept : coro_(other.coro_) { other.coro_ = nullptr; }

  [[nodiscard]] T81Result<T> await(T81Entropy patience, T81Agent& dreamer) {
    if (!coro_) {
      return T81Result<T>::failure(symbols::PROMISE_DESTROYED,
                                   T81String("PROMISE WAS DESTROYED BEFORE RESOLUTION"));
    }

    auto current_state = coro_.promise().state.load();
    if (current_state == State::FULFILLED) {
      return T81Result<T>::success(std::move(*coro_.promise().value));
    }
    if (current_state == State::BROKEN) {
      return T81Result<T>::failure(coro_.promise().error->code, coro_.promise().error->message);
    }
    if (current_state == State::CANCELLED) {
      return T81Result<T>::failure(T81Symbol::intern("CANCELLED"),
                                   T81String("PROMISE WAS CANCELLED"));
    }

    (void)patience.consume();
    dreamer.observe(symbols::DREAMING);

    return T81Result<T>::failure(symbols::STILL_DREAMING,
                                 T81String("COMPUTATION NOT YET COMPLETE"));
  }

  void cancel() {
    if (coro_ && coro_.promise().state.load() == State::PENDING) {
      coro_.promise().state.store(State::CANCELLED);
    }
  }

  [[nodiscard]] State state() const noexcept {
    return coro_ ? coro_.promise().state.load() : State::BROKEN;
  }

  [[nodiscard]] T81Maybe<T> try_get() const noexcept {
    if (!coro_ || coro_.promise().state.load() != State::FULFILLED) {
      return T81Maybe<T>::nothing();
    }
    return T81Maybe<T>(*coro_.promise().value);
  }

  [[nodiscard]] T81Reflection<T81Promise<T>> reflect() const {
    T81Symbol status_sym;
    switch (state()) {
      case State::PENDING:
        status_sym = symbols::PENDING;
        break;
      case State::FULFILLED:
        status_sym = symbols::FULFILLED;
        break;
      case State::BROKEN:
        status_sym = symbols::BROKEN;
        break;
      case State::CANCELLED:
        status_sym = T81Symbol::intern("CANCELLED");
        break;
    }
    return T81Reflection<T81Promise<T>>(*this, symbols::PROMISE, status_sym);
  }
  [[nodiscard]] std::string serialize_canonical() const { return "Promise()"; }
};

}  // namespace t81
