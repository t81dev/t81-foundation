#include "t81/core/T81Maybe.hpp"
#include "t81/core/T81Int.hpp"
#include "t81/core/T81Symbol.hpp"
#include <cassert>

using namespace t81;

int main() {
    // Running T81Maybe tests...

    // Default construction (nothing)
    T81Maybe<T81Int<27>> nothing;
    assert(!nothing.has_value());
    assert(nothing.is_nothing());
    assert(!static_cast<bool>(nothing));

    // Value construction
    T81Maybe<T81Int<27>> something(T81Int<27>(42));
    assert(something.has_value());
    assert(!something.is_nothing());
    assert(static_cast<bool>(something));
    assert(something.value().to_int64() == 42);

    // Explicit nothing
    T81Maybe<T81Int<27>> explicit_nothing(nullptr);
    assert(!explicit_nothing.has_value());

    // Nothing with reason
    T81Symbol reason = T81Symbol::intern("test_reason");
    T81Maybe<T81Int<27>> nothing_with_reason = T81Maybe<T81Int<27>>::nothing(reason);
    assert(!nothing_with_reason.has_value());

    // Value or default
    T81Int<27> val1 = something.value_or(T81Int<27>(999));
    assert(val1.to_int64() == 42);

    T81Int<27> val2 = nothing.value_or(T81Int<27>(999));
    assert(val2.to_int64() == 999);

    // Map
    auto doubled = something.map([](const T81Int<27>& x) { return x * T81Int<27>(2); });
    assert(doubled.has_value());
    assert(doubled.value().to_int64() == 84);

    auto mapped_nothing = nothing.map([](const T81Int<27>& x) { return x * T81Int<27>(2); });
    assert(!mapped_nothing.has_value());

    // All T81Maybe tests PASSED!
    return 0;
}

