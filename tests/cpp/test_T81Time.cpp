#include "t81/core/T81Time.hpp"
#include "t81/core/T81Entropy.hpp"
#include "t81/core/T81Symbol.hpp"
#include <cassert>
#include <iostream>
#include <utility>

using namespace t81;

int main() {
    std::cout << "Running T81Time tests...\n";

    // Genesis time
    T81Time genesis = T81Time::genesis();
    assert(genesis.is_genesis());
    assert(genesis.tick().to_int64() == 0);

    // Current time
    T81Symbol event = T81Symbol::intern("TEST_EVENT");
    T81Time now1 = T81Time::now(acquire_entropy(), event);
    assert(!now1.is_genesis());
    assert(now1.tick().to_int64() > 0);
    assert(now1.event() == event);

    // Time ordering
    T81Time now2 = T81Time::now(acquire_entropy(), T81Symbol::intern("TEST_EVENT2"));
    assert(now2 > now1);
    assert(now1 < now2);
    assert(now1 != now2);

    // Narrate
    T81String narrative = now1.narrate();
    assert(!narrative.empty());

    std::cout << "All T81Time tests PASSED!\n";
    return 0;
}

