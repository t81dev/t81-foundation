#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include "t81/bigint.hpp"

#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
#endif

// Detect ASan
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define T81_ASAN_ENABLED
#endif
#endif
#if defined(__SANITIZE_ADDRESS__)
#define T81_ASAN_ENABLED
#endif

using namespace t81::v1;

namespace t81::v1 {
class BigIntAllocationGuardrailTest {
public:
  static long get_peak_rss_kb() {
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    struct rusage r;
    if (getrusage(RUSAGE_SELF, &r) == 0) {
      long rss = r.ru_maxrss;
#if defined(__APPLE__)
      // macOS reports in bytes
      return rss / 1024;
#else
      // Linux reports in KB
      return rss;
#endif
    }
#endif
    return 0;
  }

  static void run() {
    std::cout << "Starting allocation guardrail test...\n";
    long baseline_rss = get_peak_rss_kb();
    std::cout << "Baseline RSS: " << baseline_rss << " KB\n";

    // 1. Create large BigInt
    // 5M chunks * 8 bytes = 40MB
    size_t num_chunks = 5 * 1024 * 1024;
    std::vector<int64_t> chunks(num_chunks, 1);

    T81BigInt a = T81BigInt::from_chunks(chunks);
    T81BigInt b(2);

    long after_setup_rss = get_peak_rss_kb();
    std::cout << "RSS after setup (a created): " << after_setup_rss << " KB\n";

    // 2. Perform division
    // This should invoke to_std_chunks and div_mod_std
    auto res = T81BigInt::div_mod(a, b);

    long peak_rss = get_peak_rss_kb();
    std::cout << "Peak RSS after div_mod: " << peak_rss << " KB\n";

    long diff = peak_rss - after_setup_rss;
    std::cout << "RSS Growth during div_mod: " << diff << " KB\n";

    // Expected growth:
    // 'a' exists (40MB).
    // div_mod creates 'u' (copy of a's data) -> +40MB.
    // div_mod creates 'q' (result) -> +40MB.
    // Total expected growth ~ 80MB.
    // If reallocation happens in u: +40MB (during copy).
    // So growth ~ 120MB.

    // We want to assert growth is NOT ~120MB.
    // Let's set a threshold.
    // 80MB = 81920 KB.
    // 120MB = 122880 KB.
    // Threshold: 100000 KB (100MB).

#if defined(T81_ASAN_ENABLED)
    long threshold = 400000;  // 400MB for ASan builds (high overhead)
    std::cout << "ASan detected: Increasing threshold to " << threshold << " KB\n";
#else
    long threshold = 105000;  // 105MB for standard builds
#endif

    if (diff > threshold) {
      std::cerr << "Guardrail failure: Excessive allocation detected (" << diff << " KB > "
                << threshold << " KB)\n";
      std::cerr << "This indicates possible regression in vector capacity handling.\n";
      // Return failure (exit 1)
      std::exit(1);
    } else {
      std::cout << "Guardrail passed: Allocation within limits.\n";
    }
  }
};
} // namespace t81::v1

int main() {
  t81::v1::BigIntAllocationGuardrailTest::run();
  return 0;
}
