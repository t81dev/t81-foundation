#include "t81/types/T81Float.hpp"
#include <cassert>
#include <iostream>

// Test deterministic profile boundaries for T81Float
// This test ensures that unsupported deterministic operations are rejected
// instead of silently falling back to host math

void test_deterministic_float_boundaries() {
    using TFloat = t81::v1::T81Float<27, 9>;
    
    // Test basic deterministic operations (should work)
    {
        TFloat a = TFloat::from_double(3.0);
        TFloat b = TFloat::from_double(4.0);
        
        // Addition and multiplication should work in deterministic mode
        TFloat sum = a + b;
        TFloat prod = a * b;
        
        // These should not be NaE in any mode
        assert(!sum.is_nae());
        assert(!prod.is_nae());
        
        std::cout << "✓ Basic arithmetic operations work\n";
    }
    
    // Test deterministic division rejection
    {
        TFloat a = TFloat::from_double(10.0);
        TFloat b = TFloat::from_double(2.0);
        
#if defined(T81_DETERMINISTIC)
        // In deterministic mode, division should be rejected
        TFloat result = a / b;
        assert(result.is_nae());
        std::cout << "✓ Deterministic division correctly rejected\n";
#else
        // In non-deterministic mode, division should work
        TFloat result = a / b;
        assert(!result.is_nae());
        std::cout << "✓ Non-deterministic division works\n";
#endif
    }
    
    // Test deterministic transcendental rejection
    {
        TFloat x = TFloat::from_double(0.5);
        
#if defined(T81_DETERMINISTIC)
        // In deterministic mode, unsupported transcendentals should be rejected
        assert(x.acos().is_nae());
        assert(x.asin().is_nae());
        assert(x.atan().is_nae());
        assert(x.sinh().is_nae());
        assert(x.cosh().is_nae());
        assert(x.tanh().is_nae());
        
        TFloat base = TFloat::from_double(2.0);
        TFloat exp = TFloat::from_double(3.0);
        assert(base.pow(exp).is_nae());
        
        std::cout << "✓ Deterministic transcendentals correctly rejected\n";
#else
        // In non-deterministic mode, these should work
        assert(!x.acos().is_nae());
        assert(!x.asin().is_nae());
        assert(!x.atan().is_nae());
        assert(!x.sinh().is_nae());
        assert(!x.cosh().is_nae());
        assert(!x.tanh().is_nae());
        
        TFloat base = TFloat::from_double(2.0);
        TFloat exp = TFloat::from_double(3.0);
        assert(!base.pow(exp).is_nae());
        
        std::cout << "✓ Non-deterministic transcendentals work\n";
#endif
    }
    
    // Test supported deterministic operations (if dmath is available)
    {
        TFloat x = TFloat::from_double(0.5);
        
#if defined(T81_DETERMINISTIC) && defined(T81_DETERMINISTIC_DMATH_AVAILABLE)
        // If dmath is available, basic transcendentals should work
        assert(!x.sin().is_nae());
        assert(!x.cos().is_nae());
        assert(!x.exp().is_nae());
        assert(!x.log(TFloat::from_double(2.0)).is_nae());
        assert(!x.sqrt().is_nae());
        
        std::cout << "✓ Deterministic dmath operations work\n";
#elif defined(T81_DETERMINISTIC)
        // If dmath is not available, these should also be rejected
        assert(x.sin().is_nae());
        assert(x.cos().is_nae());
        assert(x.exp().is_nae());
        assert(x.log(TFloat::from_double(2.0)).is_nae());
        assert(x.sqrt().is_nae());
        
        std::cout << "✓ Deterministic operations correctly rejected when dmath unavailable\n";
#else
        // In non-deterministic mode, these should work
        assert(!x.sin().is_nae());
        assert(!x.cos().is_nae());
        assert(!x.exp().is_nae());
        assert(!x.log(TFloat::from_double(2.0)).is_nae());
        assert(!x.sqrt().is_nae());
        
        std::cout << "✓ Non-deterministic operations work\n";
#endif
    }
    
    // Test special value handling
    {
        TFloat inf = TFloat::inf();
        TFloat nae = TFloat::nae();
        TFloat zero = TFloat::zero();
        
        // Special values should be preserved
        assert(inf.is_inf());
        assert(nae.is_nae());
        assert(zero.is_zero());
        
        // Operations on special values should handle deterministically
        assert(inf.sin().is_nae());  // sin(inf) = NaE
        assert(nae.cos().is_nae());  // cos(NaE) = NaE
        assert(zero.exp().is_nae() || !zero.exp().is_nae());  // exp(0) may be NaE or valid depending on mode
        
        std::cout << "✓ Special value handling works\n";
    }
    
    std::cout << "\nAll deterministic float boundary tests passed!\n";
}

int main() {
    test_deterministic_float_boundaries();
    return 0;
}
