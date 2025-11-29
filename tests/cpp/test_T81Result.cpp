#include "t81/core/T81Result.hpp"
#include "t81/core/T81Int.hpp"
#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81String.hpp"
#include <cassert>

using namespace t81;
using namespace t81::core;

int main() {
    // Running T81Result tests...

    // Success case
    T81Result<T81Int<27>> success(T81Int<27>(42));
    assert(success.is_ok());
    assert(!success.is_err());
    assert(success.unwrap().to_int64() == 42);

    // Failure case
    T81Symbol error_code = T81Symbol::intern("TEST_ERROR");
    T81Result<T81Int<27>> failure = T81Result<T81Int<27>>::failure(
        error_code, T81String("Test error message")
    );
    assert(!failure.is_ok());
    assert(failure.is_err());
    assert(failure.error().code == error_code);

    // Unwrap or default
    T81Int<27> val1 = success.unwrap_or(T81Int<27>(999));
    assert(val1.to_int64() == 42);

    T81Int<27> val2 = failure.unwrap_or(T81Int<27>(999));
    assert(val2.to_int64() == 999);

    // Map
    auto doubled = success.map([](const T81Int<27>& x) { return x * T81Int<27>(2); });
    assert(doubled.is_ok());
    assert(doubled.unwrap().to_int64() == 84);

    // And then
    auto chained = success.and_then([](const T81Int<27>& x) {
        if (x.to_int64() > 0) {
            return T81Result<T81Int<27>>(x * T81Int<27>(2));
        }
        return T81Result<T81Int<27>>::failure(
            T81Symbol::intern("NEGATIVE"), T81String("Negative value")
        );
    });
    assert(chained.is_ok());
    assert(chained.unwrap().to_int64() == 84);

    // All T81Result tests PASSED!
    return 0;
}

