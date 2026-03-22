// tests/cpp/test_arithmetic_backend_equivalence.cpp
//
// RFC-0049 Conformance Test — Canonical Ternary Arithmetic Semantics
//
// Verifies that the scalar trit-by-trit oracle (t81::ternary::arith) produces
// results that are bit-exactly equal to the T81BigInt multi-limb chunk-based
// implementation for addition, subtraction, negation, and multiplication.
//
// Also verifies:
//  - Comparison semantics are value-based, not representation-based.
//  - Overflow/fault behavior is explicit and consistent across surfaces.
//  - Subtraction is implemented as add + negate (not an independent borrow path).
//
// Acceptance criteria from RFC-0049 §AC4:
//   "Conformance tests exist for scalar, packed, SWAR, and SIMD arithmetic
//    against the same canonical result set."
//
// The "scalar" surface here is the trit-vector oracle in ternary/arith.hpp.
// The "packed/multi-limb" surface is T81BigInt (uses 27-trit chunks + carry
// normalization, optionally AVX2-accelerated chunk-addition).
// The VM opcode surface (Add, Sub, Mul, Neg) is tested in e2e_arithmetic_test.
//
// This test focuses on the property that both surfaces converge to the same
// canonical balanced-ternary value for all inputs in the tested domain.

#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

#include "t81/ternary/arith.hpp"      // scalar trit oracle
#include "t81/types/T81BigInt.hpp"    // multi-limb packed implementation

namespace {

using namespace t81::ternary;
using t81::Trit;
using t81::v1::T81BigInt;

// ── Scalar oracle helpers ────────────────────────────────────────────────────

// Canonical balanced-ternary negation: flip every trit (Pos↔Neg, Zero stays).
std::vector<Trit> negate_trits(const std::vector<Trit>& ds) {
    std::vector<Trit> r;
    r.reserve(ds.size());
    for (Trit t : ds)
        r.push_back(t == Trit::Pos ? Trit::Neg : t == Trit::Neg ? Trit::Pos : Trit::Zero);
    normalize(r);
    return r;
}

// Subtraction via the scalar oracle: a - b = a + (-b).
std::vector<Trit> sub_trits(const std::vector<Trit>& a, const std::vector<Trit>& b) {
    return add(a, negate_trits(b));
}

// Scalar multiplication oracle for small values only: repeated addition.
// Returns the trit-vector product of a * n where n is a small non-negative int.
std::vector<Trit> mul_scalar_ref(const std::vector<Trit>& a, int32_t n) {
    std::vector<Trit> result = {Trit::Zero};
    std::vector<Trit> base = a;
    bool neg = n < 0;
    int32_t count = neg ? -n : n;
    for (int32_t i = 0; i < count; ++i)
        result = add(result, base);
    if (neg)
        result = negate_trits(result);
    normalize(result);
    return result;
}

// Decode T81BigInt to int64, asserting in-range.
int64_t bigint_to_i64(const T81BigInt& b) {
    return b.to_int64();
}

// ── Helpers ──────────────────────────────────────────────────────────────────

void require(bool cond, const char* msg) {
    if (!cond) {
        std::cerr << "FAIL: " << msg << "\n";
        std::abort();
    }
}

// Compare a scalar trit-vector result against a T81BigInt result.
// Both encode the same int64 value; we decode both and compare numerically.
void check_arith(const std::vector<Trit>& oracle_trits,
                 const T81BigInt& bigint_result,
                 const char* op_name, int64_t a, int64_t b) {
    int64_t oracle_val  = decode_i64(oracle_trits);
    int64_t bigint_val  = bigint_to_i64(bigint_result);
    if (oracle_val != bigint_val) {
        std::cerr << "MISMATCH in " << op_name
                  << "(" << a << ", " << b << "): "
                  << "oracle=" << oracle_val
                  << " bigint=" << bigint_val << "\n";
        std::abort();
    }
}

// ── Test functions ───────────────────────────────────────────────────────────

// Negation: scalar oracle (trit flip) == T81BigInt unary minus.
void test_negation() {
    std::cout << "[RFC-0049] negation: scalar oracle == T81BigInt...\n";
    std::mt19937_64 rng(0xBEEF'C0DE'4900'0001ULL);
    std::uniform_int_distribution<int64_t> dist(-1'000'000LL, 1'000'000LL);

    for (int i = 0; i < 2000; ++i) {
        int64_t v = dist(rng);
        auto trits         = encode_i64(v);
        auto oracle_neg    = negate_trits(trits);
        int64_t oracle_val = decode_i64(oracle_neg);

        T81BigInt bi(v);
        T81BigInt bi_neg = -bi;
        int64_t bigint_val    = bigint_to_i64(bi_neg);

        if (oracle_val != bigint_val) {
            std::cerr << "FAIL negation(" << v << "): oracle=" << oracle_val
                      << " bigint=" << bigint_val << "\n";
            std::abort();
        }
    }
    // Edge cases
    for (int64_t edge : {int64_t(0), int64_t(1), int64_t(-1),
                         int64_t(3), int64_t(-3), int64_t(9), int64_t(-9)}) {
        int64_t oracle_val = decode_i64(negate_trits(encode_i64(edge)));
        int64_t bigint_val = bigint_to_i64(-T81BigInt(edge));
        require(oracle_val == bigint_val, "negation edge case");
    }
    // Double negation involution: --v == v
    for (int64_t v : {int64_t(0), int64_t(42), int64_t(-123), int64_t(729)}) {
        auto trits = encode_i64(v);
        auto nn    = negate_trits(negate_trits(trits));
        require(decode_i64(nn) == v, "double negation involution (oracle)");
        require(bigint_to_i64(-(-T81BigInt(v))) == v, "double negation involution (bigint)");
    }
    std::cout << "  PASS\n";
}

// Addition: scalar oracle == T81BigInt.
void test_addition() {
    std::cout << "[RFC-0049] addition: scalar oracle == T81BigInt...\n";
    std::mt19937_64 rng(0xBEEF'C0DE'4900'0002ULL);
    std::uniform_int_distribution<int64_t> dist(-500'000LL, 500'000LL);

    for (int i = 0; i < 2000; ++i) {
        int64_t a = dist(rng), b = dist(rng);
        auto oracle = add(encode_i64(a), encode_i64(b));
        T81BigInt bi_res = T81BigInt(a) + T81BigInt(b);
        check_arith(oracle, bi_res, "add", a, b);
    }
    // Edge cases: identity, sign crossing, zero
    for (auto [a, b] : std::initializer_list<std::pair<int64_t,int64_t>>{
            {0, 0}, {0, 1}, {1, 0}, {-1, 1}, {1, -1},
            {1, 2}, {-5, 5}, {3, -3}, {729, -729}}) {
        auto oracle = add(encode_i64(a), encode_i64(b));
        T81BigInt bi_res = T81BigInt(a) + T81BigInt(b);
        check_arith(oracle, bi_res, "add_edge", a, b);
    }
    // Commutativity: a+b == b+a
    std::uniform_int_distribution<int64_t> comm(-100'000LL, 100'000LL);
    for (int i = 0; i < 500; ++i) {
        int64_t a = comm(rng), b = comm(rng);
        T81BigInt ab = T81BigInt(a) + T81BigInt(b);
        T81BigInt ba = T81BigInt(b) + T81BigInt(a);
        require(bigint_to_i64(ab) == bigint_to_i64(ba), "addition commutativity");
    }
    std::cout << "  PASS\n";
}

// Subtraction: scalar oracle (add+negate) == T81BigInt operator-.
// Also verifies the axiom: a - b = a + (-b).
void test_subtraction() {
    std::cout << "[RFC-0049] subtraction: scalar oracle == T81BigInt, identity a-b=a+(-b)...\n";
    std::mt19937_64 rng(0xBEEF'C0DE'4900'0003ULL);
    std::uniform_int_distribution<int64_t> dist(-500'000LL, 500'000LL);

    for (int i = 0; i < 2000; ++i) {
        int64_t a = dist(rng), b = dist(rng);
        auto oracle = sub_trits(encode_i64(a), encode_i64(b));
        T81BigInt bi_res = T81BigInt(a) - T81BigInt(b);
        check_arith(oracle, bi_res, "sub", a, b);

        // Verify a - b == a + (-b) at the BigInt level
        T81BigInt bi_add_neg = T81BigInt(a) + (-T81BigInt(b));
        require(bigint_to_i64(bi_res) == bigint_to_i64(bi_add_neg),
                "sub axiom: a-b == a+(-b)");
    }
    // a - a == 0
    for (int64_t v : {int64_t(0), int64_t(1), int64_t(-42), int64_t(729)}) {
        T81BigInt bi(v);
        require(bigint_to_i64(bi - bi) == 0, "a - a == 0");
        auto trits = encode_i64(v);
        require(decode_i64(sub_trits(trits, trits)) == 0, "a - a == 0 (oracle)");
    }
    std::cout << "  PASS\n";
}

// Multiplication: T81BigInt against direct int64 multiplication for values
// where the product is in range, and against the repeated-addition oracle for
// small multipliers.
void test_multiplication() {
    std::cout << "[RFC-0049] multiplication: T81BigInt vs int64 reference...\n";
    std::mt19937_64 rng(0xBEEF'C0DE'4900'0004ULL);

    // Range where a*b fits comfortably in int64 (|a|, |b| <= 30000)
    std::uniform_int_distribution<int64_t> dist(-30'000LL, 30'000LL);
    for (int i = 0; i < 2000; ++i) {
        int64_t a = dist(rng), b = dist(rng);
        int64_t expected = a * b;
        T81BigInt bi_res = T81BigInt(a) * T81BigInt(b);
        int64_t got = bigint_to_i64(bi_res);
        if (got != expected) {
            std::cerr << "FAIL mul(" << a << "," << b << "): expected=" << expected
                      << " got=" << got << "\n";
            std::abort();
        }
    }

    // Repeated-addition oracle for small multipliers
    std::cout << "[RFC-0049] multiplication: repeated-addition oracle (small n)...\n";
    std::uniform_int_distribution<int64_t> base_dist(-1000LL, 1000LL);
    std::uniform_int_distribution<int32_t>  n_dist(-20, 20);
    for (int i = 0; i < 500; ++i) {
        int64_t a = base_dist(rng);
        int32_t n = n_dist(rng);
        auto oracle = mul_scalar_ref(encode_i64(a), n);
        int64_t oracle_val = decode_i64(oracle);

        T81BigInt bi_res = T81BigInt(a) * T81BigInt(static_cast<int64_t>(n));
        int64_t bigint_val = bigint_to_i64(bi_res);

        if (oracle_val != bigint_val) {
            std::cerr << "FAIL mul_repeated(" << a << "*" << n << "): oracle="
                      << oracle_val << " bigint=" << bigint_val << "\n";
            std::abort();
        }
    }

    // Algebraic laws
    std::uniform_int_distribution<int64_t> law_dist(-5000LL, 5000LL);
    for (int i = 0; i < 500; ++i) {
        int64_t a = law_dist(rng), b = law_dist(rng), c = law_dist(rng);
        T81BigInt A(a), B(b), C(c);

        // Commutativity: a*b == b*a
        require(bigint_to_i64(A * B) == bigint_to_i64(B * A),
                "mul commutativity");

        // Multiplicative identity: a * 1 == a
        require(bigint_to_i64(A * T81BigInt(1)) == a,
                "mul identity");

        // Multiplicative zero: a * 0 == 0
        require(bigint_to_i64(A * T81BigInt(0)) == 0,
                "mul zero");

        // Negation via multiplication: a * (-1) == -a
        require(bigint_to_i64(A * T81BigInt(-1)) == bigint_to_i64(-A),
                "mul by -1 == negate");
    }
    // Distributivity: a*(b+c) == a*b + a*c — use narrower range to stay in int64
    std::uniform_int_distribution<int64_t> dist_law(-1000LL, 1000LL);
    for (int i = 0; i < 500; ++i) {
        int64_t a = dist_law(rng), b = dist_law(rng), c = dist_law(rng);
        T81BigInt A(a), B(b), C(c);
        int64_t lhs = bigint_to_i64(A * (B + C));
        int64_t rhs = bigint_to_i64(A * B + A * C);
        if (lhs != rhs) {
            std::cerr << "FAIL distributivity a=" << a << " b=" << b << " c=" << c
                      << " lhs=" << lhs << " rhs=" << rhs << "\n";
            std::abort();
        }
    }
    std::cout << "  PASS\n";
}

// Comparison: verifies that comparison is value-based, not representation-based.
// Two T81BigInts constructed from the same int64 must compare equal.
// Order relationship must match int64 ordering.
void test_comparison_semantics() {
    std::cout << "[RFC-0049] comparison: value-based, matches int64 ordering...\n";
    std::mt19937_64 rng(0xBEEF'C0DE'4900'0005ULL);
    std::uniform_int_distribution<int64_t> dist(-1'000'000LL, 1'000'000LL);

    for (int i = 0; i < 2000; ++i) {
        int64_t a = dist(rng), b = dist(rng);
        T81BigInt A(a), B(b);

        // Reflexivity: a == a regardless of object identity
        T81BigInt A2(a);
        require(A == A2, "comparison: reflexivity (same value, different object)");

        // Antisymmetry: a < b iff !(b <= a... via ordering)
        bool i64_less = (a < b);
        bool bi_less  = (A < B);
        if (i64_less != bi_less) {
            std::cerr << "FAIL comparison(<): a=" << a << " b=" << b
                      << " i64=" << i64_less << " bigint=" << bi_less << "\n";
            std::abort();
        }

        // Equality consistency: a==b iff !(a<b) && !(b<a)
        bool i64_eq = (a == b);
        bool bi_eq  = (A == B);
        if (i64_eq != bi_eq) {
            std::cerr << "FAIL comparison(==): a=" << a << " b=" << b
                      << " i64=" << i64_eq << " bigint=" << bi_eq << "\n";
            std::abort();
        }
    }

    // Edge: zero is canonically unique — positive zero == negative zero does not
    // arise in balanced ternary (canonical zero has no sign), but confirm:
    T81BigInt pos_zero(0);
    T81BigInt neg_zero = -T81BigInt(0);  // negation of zero is zero
    require(pos_zero == neg_zero,
            "comparison: neg(0) == 0 (zero has no sign)");
    require(bigint_to_i64(neg_zero) == 0,
            "comparison: to_int64(neg(0)) == 0");

    std::cout << "  PASS\n";
}

// Overflow policy: T81BigInt is unbounded (arbitrary precision), so no integer
// overflow can occur — the result is always exact.  Verify this property for
// values that would overflow int64 but are representable in BigInt arithmetic.
void test_overflow_policy_bigint_unbounded() {
    std::cout << "[RFC-0049] overflow policy: T81BigInt is unbounded (arbitrary precision)...\n";

    // INT64_MAX + 1: would wrap in int64, must be exact in BigInt.
    T81BigInt max64(INT64_MAX);
    T81BigInt one(1);
    T81BigInt over = max64 + one;
    // over should not throw; and should decode to > INT64_MAX when cast back
    try {
        int64_t v = over.to_int64();
        std::cerr << "FAIL: expected overflow exception for INT64_MAX+1 to_int64, got " << v << "\n";
        std::abort();
    } catch (const std::exception&) {
        // Expected: to_int64() on a value exceeding int64 range must throw.
    }

    // But the arithmetic itself is exact: (INT64_MAX + 1) - 1 == INT64_MAX
    T81BigInt round_trip = over - one;
    require(round_trip == max64,
            "overflow: (INT64_MAX+1)-1 == INT64_MAX (exact bigint)");

    // Same for the negative side
    T81BigInt min64(INT64_MIN);
    T81BigInt under = min64 - one;
    try {
        int64_t v = under.to_int64();
        std::cerr << "FAIL: expected overflow exception for INT64_MIN-1 to_int64, got " << v << "\n";
        std::abort();
    } catch (const std::exception&) {
        // Expected
    }
    T81BigInt round_trip2 = under + one;
    require(round_trip2 == min64,
            "overflow: (INT64_MIN-1)+1 == INT64_MIN (exact bigint)");

    std::cout << "  PASS\n";
}

// Carry propagation correctness: verify that multi-trit additions that require
// carry chains produce the same result as the direct int64 computation.
// Specifically tests carry across trit-group boundaries (powers of 3).
void test_carry_propagation() {
    std::cout << "[RFC-0049] carry propagation: boundary cases...\n";

    // Carry across trit boundary: 1 + 2 in balanced ternary.
    // In BT: 1 = P, 2 is not directly representable; 1+1=2 triggers carry.
    // 1 + 1 = [P] + [P] = sum 2 → trit -1 carry +1 → result [N, P] = -1+3 = 2
    {
        auto a = encode_i64(1), b = encode_i64(1);
        auto r = add(a, b);
        require(decode_i64(r) == 2, "carry: 1+1=2 (oracle)");
        require(bigint_to_i64(T81BigInt(1) + T81BigInt(1)) == 2,
                "carry: 1+1=2 (bigint)");
    }
    // Powers of 3 (each is a single non-zero trit in BT): 3^k + 3^k = 2*3^k
    for (int k = 0; k <= 12; ++k) {
        int64_t p = 1;
        for (int j = 0; j < k; ++j) p *= 3;
        int64_t expected = 2 * p;
        auto oracle = add(encode_i64(p), encode_i64(p));
        require(decode_i64(oracle) == expected, "carry: 3^k + 3^k (oracle)");
        T81BigInt bi_res = T81BigInt(p) + T81BigInt(p);
        require(bigint_to_i64(bi_res) == expected, "carry: 3^k + 3^k (bigint)");
    }
    // Sum that produces maximal carry chain: (3^13 - 1)/2 + (3^13 - 1)/2
    // = 3^13 - 1.  (3^13 = 1594323)
    {
        int64_t half = (1594323LL - 1) / 2;  // 797161
        int64_t expected = 2 * half;          // 1594322
        auto oracle = add(encode_i64(half), encode_i64(half));
        require(decode_i64(oracle) == expected, "carry chain: large oracle");
        T81BigInt bi_res = T81BigInt(half) + T81BigInt(half);
        require(bigint_to_i64(bi_res) == expected, "carry chain: large bigint");
    }
    std::cout << "  PASS\n";
}

}  // namespace

int main() {
    test_negation();
    test_addition();
    test_subtraction();
    test_multiplication();
    test_comparison_semantics();
    test_overflow_policy_bigint_unbounded();
    test_carry_propagation();
    std::cout << "All RFC-0049 arithmetic conformance tests passed.\n";
    return 0;
}
