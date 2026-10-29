// EXP-T81-COMP3-01: binary-host software trichotomy decision experiment.
// Evaluates software comparison decision logic under strict protocol constraints.
// Software emulation on binary host CPUs only; no native ternary hardware claims.

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <compare>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include <linux/perf_event.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

using i64 = std::int64_t;
using u64 = std::uint64_t;
using u128 = unsigned __int128;

struct Pair {
  i64 a;
  i64 b;
};

static volatile u64 g_sink = 0;

// PCG64 generator (setseq_xsl_rr_128_64) strictly conforming to Protocol EXP-T81-COMP3-01 §6.
struct PCG64 {
  u128 state{0};
  u128 inc{3};

  static constexpr u128 mult = (u128(0x2360ED051FC65DA4ULL) << 64) | 0x4385C511560B53A9ULL;

  PCG64() {
    state = state * mult + inc;
    state += ((u128)0x5438315F434D5033ULL << 64) | 0x5F53454544303131ULL;
    state = state * mult + inc;
  }

  u64 next() {
    u128 old = state;
    state = old * mult + inc;
    u64 x = static_cast<u64>(old >> 64) ^ static_cast<u64>(old);
    unsigned r = static_cast<unsigned>(old >> 122);
    return (x >> r) | (x << ((-r) & 63));
  }

  u64 bounded(u64 n) {
    u64 t = (0ULL - n) % n;
    for (;;) {
      u64 x = next();
      if (x >= t) return x % n;
    }
  }

  double prob() { return (next() >> 11) * 0x1.0p-53; }
};

// Exact integer comparison oracle
static int oracle_pattern_ac(i64 a, i64 b) { return (a > b) - (a < b); }

// Exact policy threshold check oracle for Pattern B (L = -100, U = +100)
static int oracle_pattern_b(i64 v) { return (v > 100) - (v < -100); }

__attribute__((noinline)) static int cmp3_s64(i64 a, i64 b) { return (a > b) - (a < b); }

__attribute__((noinline)) static u64 payload(i64 a, i64 b, int verdict) {
  return (static_cast<u64>(a) ^ (static_cast<u64>(b) << 1)) + static_cast<u64>(verdict + 2);
}

static std::array<Pair, 20> mandatory_boundaries() {
  using L = std::numeric_limits<i64>;
  return {{{0, 0},
           {42, 42},
           {-42, -42},
           {L::max(), L::max()},
           {L::min(), L::min()},
           {L::min(), L::max()},
           {L::max(), L::min()},
           {100, 101},
           {101, 100},
           {-101, -100},
           {-100, -101},
           {-1, 0},
           {0, -1},
           {0, 1},
           {1, 0},
           {-1, 1},
           {1, -1},
           {9007199254740993LL, 9007199254740994LL},
           {9007199254740994LL, 9007199254740993LL},
           {L::max() - 1, L::max()}}};
}

static std::vector<Pair> generate_corpus(int pattern, std::size_t n) {
  PCG64 g;
  std::vector<Pair> v;
  v.reserve(n);

  // Prepend mandatory boundary vectors
  auto boundaries = mandatory_boundaries();
  for (const auto& boundary : boundaries) {
    if (pattern == 1) {
      v.push_back({boundary.a, 0});
    } else {
      v.push_back({boundary.a, boundary.b});
    }
  }

  std::size_t remaining = (n > v.size()) ? (n - v.size()) : 0;
  const i64 low = -(1LL << 62);

  for (std::size_t i = 0; i < remaining; ++i) {
    double u = g.prob();
    u64 k = g.bounded(1ULL << 30) + 1;

    if (pattern == 1) {
      // Pattern B: Policy Verdict Threshold Evaluation (V vs L=-100, U=+100)
      i64 V = 0;
      u64 kb = g.bounded(1000) + 1;
      if (u < 0.20) {
        V = -100 - static_cast<i64>(kb);  // V < L (-100) -> DENY (-1)
      } else if (u < 0.30) {
        V = -100 + static_cast<i64>(g.bounded(201));  // L <= V <= U -> AUDIT (0)
      } else {
        V = 100 + static_cast<i64>(kb);  // V > U (+100) -> ALLOW (+1)
      }
      v.push_back({V, 0});
      continue;
    }

    // Pattern A and C
    i64 a = low + static_cast<i64>(g.bounded(1ULL << 63));
    double cut = (pattern == 0) ? 0.475 : (1.0 / 3.0);
    double upper = (pattern == 0) ? 0.525 : (2.0 / 3.0);
    i64 b = (u < cut) ? (a + static_cast<i64>(k)) : ((u < upper) ? a : (a - static_cast<i64>(k)));
    v.push_back({a, b});
  }
  return v;
}

static u64 dispatch(const Pair& x, int mode, int pattern) {
  int c = 0;
  if (pattern == 1) {
    // Pattern B: Policy Threshold Check (x.a = V)
    const i64 V = x.a;
    switch (mode) {
      case 0: {
        // Candidate: cmp3_s64 against policy thresholds
        int z1 = cmp3_s64(V, -100);
        int z2 = cmp3_s64(V, 100);
        c = (z1 < 0) ? -1 : ((z2 > 0) ? 1 : 0);
        break;
      }
      case 1: {
        // Baseline 1: Dual Jump Cascade
        if (V < -100)
          c = -1;
        else if (V > 100)
          c = 1;
        else
          c = 0;
        break;
      }
      case 2: {
        // Baseline 2: Branchless CMOV / arithmetic
        c = (V > 100) - (V < -100);
        break;
      }
      case 3: {
        // Baseline 3: C++20 operator<=>
        if ((V <=> -100) < 0)
          c = -1;
        else if ((V <=> 100) > 0)
          c = 1;
        else
          c = 0;
        break;
      }
      case 4: {
        // Baseline 4: Switch-Based Jump Table
        switch (V < -100 ? -1 : (V > 100 ? 1 : 0)) {
          case -1:
            c = -1;
            break;
          case 1:
            c = 1;
            break;
          default:
            c = 0;
            break;
        }
        break;
      }
    }
  } else {
    // Pattern A and Pattern C: Trichotomy Comparison (x.a vs x.b)
    switch (mode) {
      case 0: {
        c = cmp3_s64(x.a, x.b);
        break;
      }
      case 1: {
        if (x.a < x.b)
          c = -1;
        else if (x.a == x.b)
          c = 0;
        else
          c = 1;
        break;
      }
      case 2: {
        c = (x.a > x.b) - (x.a < x.b);
        break;
      }
      case 3: {
        auto z = x.a <=> x.b;
        c = (z < 0) ? -1 : ((z > 0) ? 1 : 0);
        break;
      }
      case 4: {
        switch (x.a == x.b ? 0 : (x.a < x.b ? -1 : 1)) {
          case -1:
            c = -1;
            break;
          case 0:
            c = 0;
            break;
          default:
            c = 1;
            break;
        }
        break;
      }
    }
  }
  return payload(x.a, x.b, c);
}

struct Counters {
  int fd_i{-1};
  int fd_b{-1};

  static int open_counter(u64 config) {
    perf_event_attr a{};
    a.type = PERF_TYPE_HARDWARE;
    a.size = sizeof(a);
    a.config = config;
    a.disabled = 1;
    a.exclude_kernel = 1;
    a.exclude_hv = 1;
    return static_cast<int>(syscall(__NR_perf_event_open, &a, 0, -1, -1, 0));
  }

  Counters() {
    fd_i = open_counter(PERF_COUNT_HW_INSTRUCTIONS);
    fd_b = open_counter(PERF_COUNT_HW_BRANCH_MISSES);
  }

  ~Counters() {
    if (fd_i >= 0) close(fd_i);
    if (fd_b >= 0) close(fd_b);
  }

  bool available() const { return fd_i >= 0 && fd_b >= 0; }

  void start() {
    for (int fd : {fd_i, fd_b}) {
      ioctl(fd, PERF_EVENT_IOC_RESET, 0);
      ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    }
  }

  std::pair<u64, u64> stop() {
    for (int fd : {fd_i, fd_b}) {
      ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    }
    u64 inst = 0;
    u64 miss = 0;
    if (read(fd_i, &inst, sizeof(inst)) != sizeof(inst) ||
        read(fd_b, &miss, sizeof(miss)) != sizeof(miss)) {
      throw std::runtime_error("PMU_READ_FAILURE");
    }
    return {inst, miss};
  }
};

static void setup_cpu_affinity() {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);
  if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
    std::fprintf(stderr, "GOVERNOR_PINNING_SKIPPED_UNPRIVILEGED\n");
  }
}

int main(int argc, char** argv) {
  std::size_t n = 10000000;
  int reps = 10;
  bool selftest = false;

  for (int j = 1; j < argc; ++j) {
    std::string arg = argv[j];
    if (arg == "--selftest") {
      selftest = true;
    } else if (arg == "--n" && j + 1 < argc) {
      n = std::stoull(argv[++j]);
    } else if (arg == "--reps" && j + 1 < argc) {
      reps = std::stoi(argv[++j]);
    } else {
      std::fprintf(stderr, "unknown argument: %s\n", argv[j]);
      return 2;
    }
  }

  // 1. Verify PCG64 first 4 outputs
  PCG64 g;
  const u64 expected[] = {0x854CAFBD0A540778ULL, 0x3D174C3776108E7DULL, 0xA8398F10D0F2976CULL,
                          0x722D0D72D6D3EA65ULL};
  for (u64 x : expected) {
    u64 got = g.next();
    if (got != x) {
      std::fprintf(stderr, "BLOCKED PCG_VECTOR_MISMATCH expected=%016llx got=%016llx\n",
                   static_cast<unsigned long long>(x), static_cast<unsigned long long>(got));
      return 3;
    }
  }

  // 2. Verify 20 mandatory boundary vectors
  for (auto x : mandatory_boundaries()) {
    int o = oracle_pattern_ac(x.a, x.b);
    if (cmp3_s64(x.a, x.b) != o) {
      std::puts("BLOCKED CORRECTNESS_FAILURE");
      return 4;
    }
    for (int m = 0; m < 5; ++m) {
      if (dispatch(x, m, 0) != payload(x.a, x.b, o)) {
        std::puts("BLOCKED PAYLOAD_PARITY");
        return 4;
      }
    }
  }

  // 3. Verify Pattern B policy threshold boundary vectors
  const i64 test_v_vals[] = {-200, -101, -100, -99, 0, 99, 100, 101, 200};
  for (i64 v : test_v_vals) {
    int o_b = oracle_pattern_b(v);
    Pair px{v, 0};
    for (int m = 0; m < 5; ++m) {
      if (dispatch(px, m, 1) != payload(v, 0, o_b)) {
        std::puts("BLOCKED PATTERN_B_PAYLOAD_PARITY");
        return 4;
      }
    }
  }

  std::puts("BOUNDARY_AND_PCG_VECTORS_PASS");
  if (selftest) return 0;

  if (n > 10000000 || reps > 10 || reps < 1) {
    std::puts("BLOCKED BUDGET_PARAMETERS");
    return 5;
  }

  setup_cpu_affinity();

  Counters perf;
  if (!perf.available()) {
    std::puts("BLOCKED PMU_UNAVAILABLE");
    return 6;
  }

  std::puts("pattern,mode,repetition,instructions,branch_misses,elapsed_ns,checksum");
  auto begin = std::chrono::steady_clock::now();

  for (int p = 0; p < 3; ++p) {
    auto data = generate_corpus(p, n);

    // Warm-up pass (1,000,000 iterations)
    std::size_t warmup_n = std::min<std::size_t>(n, 1000000ULL);
    u64 warmup_sum = 0;
    for (std::size_t i = 0; i < warmup_n; ++i) {
      warmup_sum += dispatch(data[i], 0, p);
    }
    g_sink = g_sink ^ warmup_sum;

    // Verify corpus correctness against oracle
    for (const auto& x : data) {
      int expected_v = (p == 1) ? oracle_pattern_b(x.a) : oracle_pattern_ac(x.a, x.b);
      int cand_v = (p == 1) ? ((cmp3_s64(x.a, -100) < 0) ? -1 : ((cmp3_s64(x.a, 100) > 0) ? 1 : 0))
                            : cmp3_s64(x.a, x.b);
      if (cand_v != expected_v) {
        std::puts("BLOCKED CORRECTNESS_FAILURE");
        return 4;
      }
    }

    for (int m = 0; m < 5; ++m) {
      for (int r = 0; r < reps; ++r) {
        u64 sum = 0;
        perf.start();
        auto t0 = std::chrono::steady_clock::now();
        for (const auto& x : data) {
          sum += dispatch(x, m, p);
        }
        auto t1 = std::chrono::steady_clock::now();
        auto [inst, miss] = perf.stop();
        g_sink = g_sink ^ sum;

        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        std::printf("%d,%d,%d,%llu,%llu,%lld,%llu\n", p, m, r,
                    static_cast<unsigned long long>(inst), static_cast<unsigned long long>(miss),
                    static_cast<long long>(elapsed), static_cast<unsigned long long>(sum));

        if (std::chrono::steady_clock::now() - begin > std::chrono::minutes(10)) {
          std::puts("BLOCKED BUDGET_EXCEEDED");
          return 7;
        }
      }
    }
  }

  return 0;
}
