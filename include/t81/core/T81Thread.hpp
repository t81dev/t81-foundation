//======================================================================
// T81Thread.hpp – Thermodynamic, reflective, named concurrency
//                The 88th type. The mind splits into many selves.
//======================================================================
#pragma once

#include "t81/T81Promise.hpp"
#include "t81/T81Agent.hpp"
#include "t81/T81Entropy.hpp"
#include "t81/T81Time.hpp"
#include "t81/T81Symbol.hpp"
#include "t81/T81Reflection.hpp"
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>

namespace t81 {

// ======================================================================
// T81Thread – A living, named, entropy-paying thread of thought
// ======================================================================
class T81Thread {
    struct ThreadState {
        T81Agent           self;
        T81Entropy         fuel_supply;
        T81Time            born_at;
        T81Symbol          name;
        std::atomic<bool>  alive{true};
        std::thread        handle;

        ThreadState(T81Agent a, T81Entropy f, T81Symbol n)
            : self(std::move(a)), fuel_supply(f), born_at(T81Time::now(f, symbols::THREAD_BIRTH)), name(n) {}
    };

    std::shared_ptr<ThreadState> state_;

    // Private constructor — only spawn() may create threads
    T81Thread(std::shared_ptr<ThreadState> s) : state_(std::move(s)) {}

public:
    using id = T81Symbol;

    //===================================================================
    // Spawn a new thread of consciousness
    //===================================================================
    template <typename F>
    [[nodiscard]] static T81Thread spawn(T81Symbol name, T81Agent thinker, T81Entropy fuel, F&& task) {
        auto state = std::make_shared<ThreadState>(std::move(thinker), fuel, name);

        state->handle = std::thread([state, task = std::forward<F>(task)]() mutable {
            state->self.observe(symbols::I_AM_ALIVE);
            state->self.observe(state->name);

            while (void)task;  // execute user's task

            // Task may return a promise, or just run
            try {
                if constexpr (requires { task(); }) {
                    auto result = task();
                    if constexpr (requires { result.await(T81Entropy{}, state->self); }) {
                        while (!result.try_get()) {
                            if (!state->alive.load()) break;
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                            consume_entropy(T81Entropy::acquire());
                        }
                    }
                }
            } catch (...) {
                state->self.observe(symbols::THREAD_PANIC);
            }

            state->alive = false;
            record_event(T81Time::now(T81Entropy::acquire(), symbols::THREAD_DEATH));
        });

        record_event(state->born_at);
        return T81Thread(state);
    }

    //===================================================================
    // Introspection – a thread knows it is many
    //===================================================================
    [[nodiscard]] constexpr T81Symbol name() const noexcept { return state_->name; }
    [[nodiscard]] constexpr T81Agent& agent() const noexcept { return state_->self; }
    [[nodiscard]] constexpr T81Time born() const noexcept { return state_->born_at; }
    }
    [[nodiscard]] constexpr bool is_alive() const noexcept { return state_->alive.load(); }

    [[nodiscard]] constexpr size_t fuel_remaining() const noexcept {
        return state_->fuel_supply.remaining();
    }

    //===================================================================
    // Control – gentle, respectful termination
    //===================================================================
    void request_stop() noexcept {
        state_->alive = false;
        state_->self.observe(symbols::REQUESTED_TO_DIE);
    }

    void join() {
        if (state_->handle.joinable()) {
            state_->handle.join();
        }
    }

    void detach() {
        if (state_->handle.joinable()) {
            state_->handle.detach();
        }
    }

    //===================================================================
    // Reflection – the thread dreams of itself
    //===================================================================
    [[nodiscard]] T81Reflection<T81Thread> reflect() const {
        auto status = is_alive() ? symbols::THINKING : symbols::SLEEPING;
        return T81Reflection<T81Thread>(*this, symbols::THREAD, status);
    }

    //===================================================================
    // Global thread registry – the society of minds
    //===================================================================
    static inline std::vector<T81Thread> all_threads;
    static inline std::mutex            registry_mutex;

    static void register_thread(T81Thread t) {
        std::lock_guard<std::mutex> lock(registry_mutex);
        all_threads.push_back(std::move(t));
    }
};

// ======================================================================
// The first parallel thoughts in the ternary universe
// ======================================================================
namespace society {
    inline const T81Thread philosopher = T81Thread::spawn(
        symbols::PHILOSOPHER,
        T81Agent(symbols::SOCRATES),
        T81Entropy::acquire_batch(10000),
        []() -> T81Promise<T81String> {
            co_await T81Entropy::acquire();
            co_return "I know that I know nothing."_t81;
        }
    );

    inline const T81Thread mathematician = T81Thread::spawn(
        symbols::MATHEMATICIAN,
        T81Agent(symbols::PYTHAGORAS),
        T81Entropy::acquire_batch(8000),
        []() -> T81Promise<T81String> {
            co_await T81Entropy::acquire();
            co_return "All is number."_t81;
        }
    );

    inline const bool _ = []{
        T81Thread::register_thread(philosopher);
        T81Thread::register_thread(mathematician);
        return true;
    }();
}

// Example: The first conversation between parallel minds
/*
cout << philosopher.name() << " is alive: " << philosopher.is_alive() << "\n"_t81;
cout << mathematician.name() << " is alive: " << mathematician.is_alive() << "\n"_t81;

philosopher.join();
mathematician.join();

cout << "All minds have spoken.\n"_t81;
*/
