#include <cstdint>

// Volatile operands keep signed multiply/divide/remainder in the linked
// executable under Release optimization. A syntax-only probe is insufficient.
int main() {
  volatile std::int64_t input = -9223372036854775807LL;
  volatile std::int64_t divisor = -10;
  __int128 a = input;
  __int128 d = divisor;
  __int128 n = a * 10 - 7;
  __int128 q = n / d;
  __int128 r = n % d;
  return q == 9223372036854775807LL && r == -7 ? 0 : 1;
}
