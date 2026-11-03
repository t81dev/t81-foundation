#pragma once
// Prepared B1 primitives. B0 has no call path to these functions. No global
// initializers, performance campaigns, PMU APIs, or alternate experiment imports.
#include <chrono>
#include <atomic>
#include <functional>

struct Base01Interval { int64_t nanoseconds; uint64_t operations; };

// Caller owns preallocated input/output and verifies every output after timing.
// Kernel is a separately compiled, non-LTO batch function; no per-item barrier.
template<class Kernel>
Base01Interval base01_interval(Kernel&& kernel, uint64_t operations) {
  using Clock=std::chrono::steady_clock;
  static_assert(Clock::is_steady);
  std::atomic_signal_fence(std::memory_order_seq_cst);
  auto start=Clock::now();
  kernel();
  auto finish=Clock::now();
  std::atomic_signal_fence(std::memory_order_seq_cst);
  return {std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count(),operations};
}
