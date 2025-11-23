// include/t81/bigint/gcd.hpp
#pragma once
#include "t81/bigint/core.hpp"
namespace t81::bigint {
BigInt gcd(BigInt a, BigInt b);             // binary GCD (Stein)
BigInt gcd_lehmer(BigInt a, BigInt b);      // optional accelerated path
}
