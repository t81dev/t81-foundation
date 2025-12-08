#include "t81/core/T81Time.hpp"
#include "t81/core/T81Symbol.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

using namespace t81;

int main() {
    std::cout << "Running T81Time tests...\n";

    // Basic construction via now()
    T81Time t1 = T81Time::now();
    T81Time t2 = T81Time::now();

    // Time difference must be non-negative (steady_clock is monotonic)
    auto d = t2.since(t1);
    assert(d.count() >= 0);

    // micros_since should match since()
    [[maybe_unused]] auto micros = t2.micros_since(t1);
    assert(micros == static_cast<std::uint64_t>(d.count()));

    // Event id plumbing
    T81Symbol ev1 = T81Symbol::intern("TEST_EVENT");
    T81Symbol ev2 = T81Symbol::intern("TEST_EVENT2");

    [[maybe_unused]] T81Time e1 = T81Time::now(ev1);
    [[maybe_unused]] T81Time e2 = T81Time::now(ev2);

    assert(e1.event_id().to_string() == ev1.to_string());
    assert(e2.event_id().to_string() == ev2.to_string());

    // Ensure time moves forward across a sleep
    auto before = T81Time::now(T81Symbol::intern("BEFORE"));
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto after  = T81Time::now(T81Symbol::intern("AFTER"));

    auto delta = after.since(before);
    assert(delta.count() > 0);

    // Reflection must be callable and not crash
    auto refl = before.reflect();
    (void)refl; // silence unused warning

    std::cout << "All T81Time tests PASSED!\n";
    return 0;
}
