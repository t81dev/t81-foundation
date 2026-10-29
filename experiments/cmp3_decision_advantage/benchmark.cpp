// EXP-T81-COMP3-01: software comparison and dispatch on binary hosts only.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include "corpus.hpp"
#include "dispatch.hpp"
#include "resource_budget.hpp"
#if defined(__linux__)
#include <linux/perf_event.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
struct Counters {
  int leader{-1}, branch{-1};
  static int open_counter(u64 config, int group) {
    perf_event_attr attr{};
    attr.type = PERF_TYPE_HARDWARE;
    attr.size = sizeof(attr);
    attr.config = config;
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
    attr.disabled = group < 0 ? 1 : 0;
    attr.pinned = group < 0 ? 1 : 0;
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    attr.read_format =
        PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    return static_cast<int>(syscall(__NR_perf_event_open, &attr, 0, -1, group, 0));
  }
  Counters() {
    leader = open_counter(PERF_COUNT_HW_INSTRUCTIONS, -1);
    if (leader >= 0) branch = open_counter(PERF_COUNT_HW_BRANCH_MISSES, leader);
  }
  ~Counters() {
    if (branch >= 0) close(branch);
    if (leader >= 0) close(leader);
  }
  bool available() const { return leader >= 0 && branch >= 0; }
  void start() {
    if (ioctl(leader, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) != 0 ||
        ioctl(leader, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) != 0)
      throw std::runtime_error("PMU_ENABLE_FAILURE");
  }
  std::pair<u64, u64> stop() {
    if (ioctl(leader, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP) != 0)
      throw std::runtime_error("PMU_DISABLE_FAILURE");
    struct {
      u64 count, enabled, running, instructions, misses;
    } values{};
    if (read(leader, &values, sizeof(values)) != static_cast<ssize_t>(sizeof(values)) ||
        values.count != 2)
      throw std::runtime_error("PMU_READ_FAILURE");
    if (values.running == 0 || values.enabled != values.running)
      throw std::runtime_error("PMU_NOT_SCHEDULED");
    return {values.instructions, values.misses};
  }
};
static int setup_cpu_affinity() {
  cpu_set_t allowed;
  CPU_ZERO(&allowed);
  if (sched_getaffinity(0, sizeof(allowed), &allowed) != 0)
    throw std::runtime_error("AFFINITY_UNAVAILABLE");
  int selected = -1;
  for (int i = 0; i < CPU_SETSIZE; ++i)
    if (CPU_ISSET(i, &allowed)) {
      selected = i;
      break;
    }
  if (selected < 0) throw std::runtime_error("AFFINITY_UNAVAILABLE");
  cpu_set_t one;
  CPU_ZERO(&one);
  CPU_SET(selected, &one);
  if (sched_setaffinity(0, sizeof(one), &one) != 0)
    throw std::runtime_error("AFFINITY_UNAVAILABLE");
  std::fprintf(stderr, "CPU_CORE=%d\n", selected);
  return selected;
}
struct FrequencyControls {
  std::vector<std::pair<std::string, std::string>> saved;
  bool set(const std::string& path, const std::string& value) {
    std::ifstream input(path);
    std::string old;
    if (!(input >> old)) return false;
    if (old == value) return true;
    if (access(path.c_str(), W_OK) != 0) return false;
    saved.emplace_back(path, old);
    std::ofstream output(path);
    output << value;
    output.close();
    return !output.fail();
  }
  explicit FrequencyControls(int core) {
    const auto governor =
        "/sys/devices/system/cpu/cpu" + std::to_string(core) + "/cpufreq/scaling_governor";
    const bool governor_ok = set(governor, "performance");
    bool boost_ok = set("/sys/devices/system/cpu/intel_pstate/no_turbo", "1");
    if (!boost_ok) boost_ok = set("/sys/devices/system/cpu/cpufreq/boost", "0");
    std::fprintf(stderr, "%s\n",
                 governor_ok && boost_ok ? "GOVERNOR_PERFORMANCE_BOOST_DISABLED"
                                         : "GOVERNOR_PINNING_SKIPPED_UNPRIVILEGED");
  }
  ~FrequencyControls() {
    for (auto it = saved.rbegin(); it != saved.rend(); ++it) {
      std::ofstream output(it->first);
      output << it->second;
      output.close();
      if (output.fail()) std::fprintf(stderr, "CONTROL_RESTORE_FAILED %s\n", it->first.c_str());
    }
  }
};
#else
struct Counters {
  bool available() const { return false; }
};
#endif

static volatile u64 g_sink = 0;
static const std::array<Dispatch, 5> modes = {candidate, baseline1, baseline2, baseline3,
                                              baseline4};
static const std::array<Dispatch, 5> policy_modes = {
    candidate_policy, baseline1_policy, baseline2_policy, baseline3_policy, baseline4_policy};
static const auto& functions(int p) { return p == 1 ? policy_modes : modes; }
static u64 oracle_payload(Pair x, int p) {
  const int c = p == 1 ? oracle_pattern_b(x.a) : oracle_pattern_ac(x.a, x.b);
  return (static_cast<u64>(x.a) ^ (static_cast<u64>(x.b) << 1)) + static_cast<u64>(c + 2);
}
static const Classifier probes[] = {probe_candidate, probe_baseline1, probe_baseline2,
                                    probe_baseline3, probe_baseline4};
static int test_fault = 0;
static void verify(Pair x, int p) {
  const int expected = p == 1 ? oracle_pattern_b(x.a) : oracle_pattern_ac(x.a, x.b);
  for (std::size_t mode = 0; mode < 5; ++mode) {
    const int got = probes[mode](x.a, x.b, p == 1);
    if (test_fault == 1 || got != expected) throw std::runtime_error("CORRECTNESS_FAILURE");
    if (test_fault == 2 || functions(p)[mode](x.a, x.b) != oracle_payload(x, p))
      throw std::runtime_error("PAYLOAD_PARITY");
  }
}
static u64 run(const std::vector<Pair>& data, Dispatch f) {
  u64 sum = 0;
  std::size_t i = 0;
  for (const auto& x : data) {
    sum += f(x.a, x.b);
    if (++i % 100000 == 0) check_resource_budget();
  }
  g_sink = sum;
  return sum;
}
int main(int argc, char** argv) try {
  bool selftest = false, smoke = false, dump = false, indices = false, pmu_probe = false;
  check_resource_budget();
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--selftest")
      selftest = true;
    else if (arg == "--dump-corpus")
      dump = true;
    else if (arg == "--dump-bootstrap-indices")
      indices = true;
    else if (arg == "--probe-pmu")
      pmu_probe = true;
    else if (arg == "--test-fault-correctness")
      test_fault = 1;
    else if (arg == "--test-fault-payload")
      test_fault = 2;
    else if (arg == "--smoke")
      smoke = true;
    else
      throw std::runtime_error("INVALID_ARGUMENT");
  }
  if (test_fault != 0 && !selftest) throw std::runtime_error("TEST_FAULT_REQUIRES_SELFTEST");
  if (indices) {
    PCG64 index_rng;
    for (int i = 0; i < 100000; ++i) std::putchar(static_cast<int>(index_rng.bounded(10)));
    return 0;
  }
  PCG64 g;
  for (u64 expected :
       {0x854CAFBD0A540778ULL, 0x3D174C3776108E7DULL, 0xA8398F10D0F2976CULL, 0x722D0D72D6D3EA65ULL})
    if (g.next() != expected) throw std::runtime_error("PCG_VECTOR_MISMATCH");
  for (int p = 0; p < 3; ++p) {
    for (auto x : mandatory_boundaries()) verify(x, p);
    for (i64 a : {std::numeric_limits<i64>::min(), i64{-101}, i64{-100}, i64{-99}, i64{99},
                  i64{100}, i64{101}, std::numeric_limits<i64>::max()})
      verify({a, 0}, 1);
    const auto data = generate_corpus(p, 10000);
    const auto again = generate_corpus(p, 10000);
    for (std::size_t i = 0; i < data.size(); ++i) {
      if (data[i].a != again[i].a || data[i].b != again[i].b)
        throw std::runtime_error("CORPUS_NONDETERMINISTIC");
      verify(data[i], p);
    }
    for (auto f : functions(p)) (void)run(data, f);
  }
  std::fprintf(stderr, "PASS PCG=4 BOUNDARY=300 POLICY=120 GENERATED=150000 "
                       "DETERMINISM=30000\n");
  if (dump) {
    for (int p = 0; p < 3; ++p)
      for (const auto& x : generate_corpus(p, 10000))
        std::printf("%d,%lld,%lld\n", p, static_cast<long long>(x.a), static_cast<long long>(x.b));
    return 0;
  }
  if (selftest) return 0;
  if (!pmu_probe) throw std::runtime_error("MEASUREMENT_NOT_AUTHORIZED_RUNNER_INCOMPLETE");
  Counters perf;
  if (!perf.available()) throw std::runtime_error("PMU_UNAVAILABLE");
#if defined(__linux__)
  if (pmu_probe) {
    perf.start();
    (void)perf.stop();
    std::fprintf(stderr, "PMU_INFRASTRUCTURE_PROBE_PASS_NO_WORKLOAD\n");
    return 0;
  }
  throw std::runtime_error("MEASUREMENT_NOT_AUTHORIZED_RUNNER_INCOMPLETE");
  const int core = setup_cpu_affinity();
  // Log actual controls; restricted access selects the prescribed fallback.
  FrequencyControls controls(core);

  if (smoke)
    std::fprintf(stderr, "SMOKE_ONLY_NOT_PROTOCOL_MEASUREMENTS\n");
  else
    std::puts("pattern,mode,repetition,instructions,branch_misses,elapsed_ns,"
              "checksum");
  for (int p = 0; p < 3; ++p) {
    check_resource_budget();
    const auto data = generate_corpus(p, smoke ? 10000 : 10000000);
    u64 expected = 0;
    for (const auto& x : data) {
      verify(x, p);
      expected += oracle_payload(x, p);
    }
    const auto& funcs = functions(p);
    for (auto f : funcs) {
      for (int cycle = 0; cycle < 10; ++cycle) {
        u64 warm = 0;
        for (std::size_t i = 0; i < 1000000; ++i) {
          const auto& x = data[i % data.size()];
          warm += f(x.a, x.b);
        }
        g_sink = warm;
        check_resource_budget();
      }
    }
    for (int r = 0; r < (smoke ? 1 : 10); ++r) {
      // Matched repetition blocks; rotate order to distribute order effects.
      for (std::size_t j = 0; j < 5; ++j) {
        const std::size_t m = (j + static_cast<std::size_t>(r)) % 5;
        std::fprintf(stderr, "RUN_ORDER pattern=%d repetition=%d mode=%zu\n", p, r, m);
        perf.start();
        const auto t0 = std::chrono::steady_clock::now();
        const auto sum = run(data, funcs[m]);
        const auto t1 = std::chrono::steady_clock::now();
        const auto counts = perf.stop();
        if (sum != expected) throw std::runtime_error("PAYLOAD_PARITY");
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        check_resource_budget();
        std::fprintf(smoke ? stderr : stdout, "%d,%zu,%d,%llu,%llu,%lld,%llu\n", p, m, r,
                     static_cast<unsigned long long>(counts.first),
                     static_cast<unsigned long long>(counts.second),
                     static_cast<long long>(elapsed), static_cast<unsigned long long>(sum));
        check_resource_budget();
      }
    }
  }
#else
  (void)smoke;
  (void)pmu_probe;
#endif
  return 0;
} catch (const std::exception& e) {
  std::fprintf(stderr, "BLOCKED — %s\n", e.what());
  return 6;
}
