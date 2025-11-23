#include <cassert>
#include <t81/bigint.hpp>
#include <t81/bigint/divmod.hpp>
#include <t81/bigint/gcd.hpp>
#include <vector>
#include <utility>

using t81::T243BigInt;
using t81::DivModResult;
using t81::gcd;

static void test_divmod_basic_cases() {
    struct Case {
        long long a;
        long long b;
    };

    const std::vector<Case> cases = {
        {  0,  1},
        {  0, -1},
        {  5,  2},
        {  5, -2},
        { -5,  2},
        { -5, -2},
        { 13,  5},
        { 13, -5},
        { -13, 5},
        { -13,-5},
        { 42,  7},
        { 42, -7},
        { -42, 7},
        { -42,-7},
        { 1,  2},
        { -1, 2},
        { 1, -2},
        { -1,-2},
    };

    for (auto c : cases) {
        T243BigInt A(c.a);
        T243BigInt B(c.b);

        DivModResult dm = t81::divmod(A, B);

        // Invariant: a = b*q + r
        T243BigInt lhs = B * dm.q + dm.r;
        assert(lhs == A && "divmod invariant a = b*q + r failed");

        // Remainder constraints: 0 <= r < |b|
        assert(!dm.r.is_negative() && "divmod remainder must be non-negative");

        T243BigInt absB = B.is_negative() ? B.abs() : B;
        assert(dm.r < absB && "divmod remainder must satisfy r < |b|");
    }
}

static void test_gcd_basic_cases() {
    auto make = [](long long x) { return T243BigInt(x); };

    struct Case {
        long long a;
        long long b;
        long long g;
    };

    const std::vector<Case> cases = {
        { 0, 0, 0},
        { 0, 5, 5},
        { 5, 0, 5},
        { 48, 18, 6},
        { -48, 18, 6},
        { 48, -18, 6},
        { -48, -18, 6},
        { 7, 13, 1},
        { -7, 13, 1},
        { 7, -13, 1},
        { -7, -13, 1},
    };

    for (auto c : cases) {
        T243BigInt A = make(c.a);
        T243BigInt B = make(c.b);
        T243BigInt G = gcd(A, B);

        // gcd non-negative
        assert(!G.is_negative());

        // gcd correct for known pairs
        assert(G == make(c.g) && "gcd basic case failed");

        // Divides a and b: a % G == 0, b % G == 0
        if (!G.is_zero()) {
            DivModResult da = t81::divmod(A, G);
            DivModResult db = t81::divmod(B, G);
            assert(da.r.is_zero());
            assert(db.r.is_zero());
        }
    }
}

static void test_base81_roundtrip() {
    using t81::T243BigInt;
    std::vector<std::string> cases = {
        "0",
        "1",
        "80",
        "1.80.5",
        "-2.0.1",
        "3.0.0.0.4"
    };
    for (const auto& s : cases) {
        T243BigInt a = T243BigInt::from_base81_string(s);
        auto t = a.to_base81_string();
        // The to_base81_string may normalize leading zeros; reparse and compare values.
        T243BigInt b = T243BigInt::from_base81_string(t);
        assert(a == b);
    }
}

int main() {
    // existing tests...
    // test_existing_roundtrip();

    test_divmod_basic_cases();
    test_gcd_basic_cases();
    test_base81_roundtrip();

    return 0;
}
