#include "t81/core/T81Prob.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace t81::core;

int main() {
    std::cout << "Running T81Prob tests...\n";

    // Special values
    T81Prob zero = T81Prob::zero();
    T81Prob one = T81Prob::one();
    T81Prob minus_inf = T81Prob::minus_infinity();
    T81Prob plus_inf = T81Prob::plus_infinity();

    assert(zero.log_odds().to_int64() == 0);
    assert(minus_inf.log_odds().to_int64() < 0);
    assert(plus_inf.log_odds().to_int64() > 0);

    // Construction from probabilities
    T81Prob p05 = T81Prob::from_prob(0.5);
    T81Prob p01 = T81Prob::from_prob(0.1);
    T81Prob p09 = T81Prob::from_prob(0.9);

    // Arithmetic (log-odds addition)
    T81Prob sum = p05 + p05;
    T81Prob diff = p09 - p01;

    // Negation
    T81Prob neg = -p05;
    assert(neg.log_odds().to_int64() == -p05.log_odds().to_int64());

    // Comparison
    assert(p09 > p01);
    assert(p01 < p09);
    assert(p05 == p05);

    std::cout << "All T81Prob tests PASSED!\n";
    return 0;
}

