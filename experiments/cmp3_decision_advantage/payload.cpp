#include "dispatch.hpp"
// Identical payload work, with the verdict encoded in the observable result.
NOINLINE u64 target_less(i64 a, i64 b) {
  return (static_cast<u64>(a) ^ (static_cast<u64>(b) << 1)) + 1;
}
NOINLINE u64 target_equal(i64 a, i64 b) {
  return (static_cast<u64>(a) ^ (static_cast<u64>(b) << 1)) + 2;
}
NOINLINE u64 target_greater(i64 a, i64 b) {
  return (static_cast<u64>(a) ^ (static_cast<u64>(b) << 1)) + 3;
}
