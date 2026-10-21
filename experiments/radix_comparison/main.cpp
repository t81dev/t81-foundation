/**
 * @file main.cpp
 * @brief Test runner and benchmark harness for Radix Comparison Experiment.
 *
 * Runs exhaustive correctness tests, carry-chain stress tests, property tests,
 * and empirical latency/storage/complexity benchmarks.
 *
 * Outputs human-readable tables and machine-readable JSON results.
 */

#include "radix_comparison.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

using namespace t81::experiments::radix;

struct BenchmarkResults {
  double binary_add_ns = 0.0;
  double canonical_add_ns = 0.0;
  double redundant_add_ns = 0.0;
  double radix9_add_ns = 0.0;
  double radix27_add_ns = 0.0;
  double radix81_add_ns = 0.0;

  double binary_conv_ns = 0.0;
  double canonical_conv_ns = 0.0;
  double redundant_conv_ns = 0.0;
  double radix9_conv_ns = 0.0;
  double radix27_conv_ns = 0.0;
  double radix81_conv_ns = 0.0;

  double canonical_avg_carry_dist = 0.0;
  double canonical_max_carry_dist = 0.0;
  double redundant_avg_carry_dist = 1.0; // By design O(1)
  double redundant_max_carry_dist = 1.0; // By design O(1)

  size_t binary_bytes_per_64bit = 8;
  size_t canonical_bytes_packed_64bit = 11; // 40 trits / 4 = 10..11 bytes
  size_t redundant_bytes_64bit = 40;       // 40 bytes (1 byte/digit)
  size_t radix9_bytes_64bit = 20;          // 20 bytes (2 trits/digit)
  size_t radix27_bytes_64bit = 14;         // 14 bytes (3 trits/digit)
  size_t radix81_bytes_64bit = 10;         // 10 bytes (4 trits/digit)

  bool tests_passed = false;
};

// ----------------------------------------------------------------------------
// Correctness Tests
// ----------------------------------------------------------------------------
bool run_correctness_tests() {
  std::cout << "[+] Running Correctness & Property Test Suite...\n";

  // Special test values
  const std::vector<int64_t> test_values = {
      0, 1, -1, 2, -2, 3, -3, 40, -40, 81, -81, 364, -364, 3280, -3280,
      100000, -100000, 2147483647LL, -2147483648LL, 1215766545905692880LL,
      -1215766545905692880LL};

  for (int64_t v : test_values) {
    // 1. Conversion roundtrip
    CanonicalTernary ct = CanonicalTernary::from_int64(v);
    if (ct.to_int64() != v) {
      std::cerr << "[-] CanonicalTernary roundtrip failed for " << v << " got " << ct.to_int64() << "\n";
      return false;
    }

    RedundantTernary rt = RedundantTernary::from_int64(v);
    if (rt.to_int64() != v) {
      std::cerr << "[-] RedundantTernary roundtrip failed for " << v << " got " << rt.to_int64() << "\n";
      return false;
    }

    Radix9Integer r9 = Radix9Integer::from_int64(v);
    if (r9.to_int64() != v) {
      std::cerr << "[-] Radix9Integer roundtrip failed for " << v << " got " << r9.to_int64() << "\n";
      return false;
    }

    Radix27Integer r27 = Radix27Integer::from_int64(v);
    if (r27.to_int64() != v) {
      std::cerr << "[-] Radix27Integer roundtrip failed for " << v << " got " << r27.to_int64() << "\n";
      return false;
    }

    Radix81Integer r81 = Radix81Integer::from_int64(v);
    if (r81.to_int64() != v) {
      std::cerr << "[-] Radix81Integer roundtrip failed for " << v << " got " << r81.to_int64() << "\n";
      return false;
    }
  }

  // 2. Randomized Addition & Subtraction
  std::mt19937_64 rng(42);
  std::uniform_int_distribution<int64_t> dist(-1000000000LL, 1000000000LL);

  constexpr size_t kNumRandomPairs = 5000;
  for (size_t i = 0; i < kNumRandomPairs; ++i) {
    int64_t a = dist(rng);
    int64_t b = dist(rng);
    int64_t expected_sum = a + b;

    // Binary
    if (Binary64::add(Binary64(a), Binary64(b)).to_int64() != expected_sum) {
      std::cerr << "[-] Binary64 add failed for " << a << " + " << b << "\n";
      return false;
    }

    // Canonical Ternary
    CanonicalTernary ct_a = CanonicalTernary::from_int64(a);
    CanonicalTernary ct_b = CanonicalTernary::from_int64(b);
    CanonicalTernary ct_sum = CanonicalTernary::add(ct_a, ct_b);
    if (ct_sum.to_int64() != expected_sum) {
      std::cerr << "[-] CanonicalTernary add failed for " << a << " + " << b << " got " << ct_sum.to_int64() << " expected " << expected_sum << "\n";
      return false;
    }

    // Redundant Ternary (Carry-Free)
    RedundantTernary rt_a = RedundantTernary::from_int64(a);
    RedundantTernary rt_b = RedundantTernary::from_int64(b);
    RedundantTernary rt_sum = RedundantTernary::add_carry_free(rt_a, rt_b);
    if (rt_sum.to_int64() != expected_sum) {
      std::cerr << "[-] RedundantTernary carry-free add failed for " << a << " + " << b << " got " << rt_sum.to_int64() << " expected " << expected_sum << "\n";
      return false;
    }

    // Radix-9
    Radix9Integer r9_a = Radix9Integer::from_int64(a);
    Radix9Integer r9_b = Radix9Integer::from_int64(b);
    Radix9Integer r9_sum = Radix9Integer::add(r9_a, r9_b);
    if (r9_sum.to_int64() != expected_sum) {
      std::cerr << "[-] Radix9Integer add failed for " << a << " + " << b << "\n";
      return false;
    }

    // Radix-27
    Radix27Integer r27_a = Radix27Integer::from_int64(a);
    Radix27Integer r27_b = Radix27Integer::from_int64(b);
    Radix27Integer r27_sum = Radix27Integer::add(r27_a, r27_b);
    if (r27_sum.to_int64() != expected_sum) {
      std::cerr << "[-] Radix27Integer add failed for " << a << " + " << b << "\n";
      return false;
    }

    // Radix-81
    Radix81Integer r81_a = Radix81Integer::from_int64(a);
    Radix81Integer r81_b = Radix81Integer::from_int64(b);
    Radix81Integer r81_sum = Radix81Integer::add(r81_a, r81_b);
    if (r81_sum.to_int64() != expected_sum) {
      std::cerr << "[-] Radix81Integer add failed for " << a << " + " << b << "\n";
      return false;
    }
  }

  // 3. Long Carry Chain Test
  // Example: Adding 1 to (1, 1, 1, ..., 1)_3 -> carry propagates through all trits.
  // (3^20 - 1) / 2 = 1743392200
  int64_t max_trit_val = 1743392200LL;
  CanonicalTernary max_ct = CanonicalTernary::from_int64(max_trit_val);
  CanonicalTernary one_ct = CanonicalTernary::from_int64(1);
  auto [chain_res, max_chain, total_carries] = CanonicalTernary::add_with_stats(max_ct, one_ct);
  if (chain_res.to_int64() != max_trit_val + 1) {
    std::cerr << "[-] Carry chain test failed! expected " << max_trit_val + 1 << " got " << chain_res.to_int64() << "\n";
    return false;
  }
  if (max_chain < 10) {
    std::cerr << "[-] Carry chain did not trigger long carry propagation as expected. Max chain: " << max_chain << "\n";
    return false;
  }

  std::cout << "  [PASS] All correctness, boundary, carry-chain, and roundtrip tests passed!\n";
  return true;
}

// ----------------------------------------------------------------------------
// Benchmark Suite
// ----------------------------------------------------------------------------
BenchmarkResults run_benchmarks() {
  std::cout << "[+] Running Performance & Latency Benchmarks...\n";
  BenchmarkResults res;

  std::mt19937_64 rng(1337);
  std::uniform_int_distribution<int64_t> dist(-1000000000LL, 1000000000LL);

  constexpr size_t kNumOps = 500000;
  std::vector<int64_t> va(kNumOps), vb(kNumOps);
  for (size_t i = 0; i < kNumOps; ++i) {
    va[i] = dist(rng);
    vb[i] = dist(rng);
  }

  // Pre-convert operands for addition benchmark
  std::vector<Binary64> bin_a(kNumOps), bin_b(kNumOps);
  std::vector<CanonicalTernary> ct_a(kNumOps), ct_b(kNumOps);
  std::vector<RedundantTernary> rt_a(kNumOps), rt_b(kNumOps);
  std::vector<Radix9Integer> r9_a(kNumOps), r9_b(kNumOps);
  std::vector<Radix27Integer> r27_a(kNumOps), r27_b(kNumOps);
  std::vector<Radix81Integer> r81_a(kNumOps), r81_b(kNumOps);

  for (size_t i = 0; i < kNumOps; ++i) {
    bin_a[i] = Binary64(va[i]);
    bin_b[i] = Binary64(vb[i]);

    ct_a[i] = CanonicalTernary::from_int64(va[i]);
    ct_b[i] = CanonicalTernary::from_int64(vb[i]);

    rt_a[i] = RedundantTernary::from_int64(va[i]);
    rt_b[i] = RedundantTernary::from_int64(vb[i]);

    r9_a[i] = Radix9Integer::from_int64(va[i]);
    r9_b[i] = Radix9Integer::from_int64(vb[i]);

    r27_a[i] = Radix27Integer::from_int64(va[i]);
    r27_b[i] = Radix27Integer::from_int64(vb[i]);

    r81_a[i] = Radix81Integer::from_int64(va[i]);
    r81_b[i] = Radix81Integer::from_int64(vb[i]);
  }

  // 1. Conversion Latency Benchmark
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Binary64(va[i]);
      (void)x;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    res.binary_conv_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = CanonicalTernary::from_int64(va[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.canonical_conv_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = RedundantTernary::from_int64(va[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.redundant_conv_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Radix9Integer::from_int64(va[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.radix9_conv_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Radix27Integer::from_int64(va[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.radix27_conv_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Radix81Integer::from_int64(va[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.radix81_conv_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;
  }

  // 2. Addition Latency Benchmark
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Binary64::add(bin_a[i], bin_b[i]);
      (void)x;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    res.binary_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = CanonicalTernary::add(ct_a[i], ct_b[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.canonical_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = RedundantTernary::add_carry_free(rt_a[i], rt_b[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.redundant_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Radix9Integer::add(r9_a[i], r9_b[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.radix9_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Radix27Integer::add(r27_a[i], r27_b[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.radix27_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Radix81Integer::add(r81_a[i], r81_b[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.radix81_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;
  }

  // 3. Carry Propagation Stats
  {
    double total_carry_chain = 0.0;
    size_t max_carry_chain = 0;
    for (size_t i = 0; i < kNumOps; ++i) {
      auto [r, max_c, total_c] = CanonicalTernary::add_with_stats(ct_a[i], ct_b[i]);
      (void)r;
      total_carry_chain += total_c;
      max_carry_chain = std::max(max_carry_chain, max_c);
    }
    res.canonical_avg_carry_dist = total_carry_chain / kNumOps;
    res.canonical_max_carry_dist = static_cast<double>(max_carry_chain);
  }

  return res;
}

// ----------------------------------------------------------------------------
// JSON Exporter
// ----------------------------------------------------------------------------
void export_json(const BenchmarkResults& res, const std::string& path) {
  std::ofstream out(path);
  if (!out) {
    std::cerr << "[-] Failed to open output JSON path: " << path << "\n";
    return;
  }

  out << "{\n"
      << "  \"experiment\": \"t81_radix_comparison\",\n"
      << "  \"status\": \"" << (res.tests_passed ? "PASSED" : "FAILED") << "\",\n"
      << "  \"storage_bytes_64bit\": {\n"
      << "    \"binary64\": " << res.binary_bytes_per_64bit << ",\n"
      << "    \"canonical_ternary_packed\": " << res.canonical_bytes_packed_64bit << ",\n"
      << "    \"redundant_ternary\": " << res.redundant_bytes_64bit << ",\n"
      << "    \"radix9\": " << res.radix9_bytes_64bit << ",\n"
      << "    \"radix27\": " << res.radix27_bytes_64bit << ",\n"
      << "    \"radix81\": " << res.radix81_bytes_64bit << "\n"
      << "  },\n"
      << "  \"addition_latency_ns\": {\n"
      << "    \"binary64\": " << res.binary_add_ns << ",\n"
      << "    \"canonical_ternary\": " << res.canonical_add_ns << ",\n"
      << "    \"redundant_ternary_carry_free\": " << res.redundant_add_ns << ",\n"
      << "    \"radix9\": " << res.radix9_add_ns << ",\n"
      << "    \"radix27\": " << res.radix27_add_ns << ",\n"
      << "    \"radix81\": " << res.radix81_add_ns << "\n"
      << "  },\n"
      << "  \"conversion_latency_ns\": {\n"
      << "    \"binary64\": " << res.binary_conv_ns << ",\n"
      << "    \"canonical_ternary\": " << res.canonical_conv_ns << ",\n"
      << "    \"redundant_ternary\": " << res.redundant_conv_ns << ",\n"
      << "    \"radix9\": " << res.radix9_conv_ns << ",\n"
      << "    \"radix27\": " << res.radix27_conv_ns << ",\n"
      << "    \"radix81\": " << res.radix81_conv_ns << "\n"
      << "  },\n"
      << "  \"carry_propagation_distance\": {\n"
      << "    \"canonical_avg\": " << res.canonical_avg_carry_dist << ",\n"
      << "    \"canonical_max\": " << res.canonical_max_carry_dist << ",\n"
      << "    \"redundant_avg\": " << res.redundant_avg_carry_dist << ",\n"
      << "    \"redundant_max\": " << res.redundant_max_carry_dist << "\n"
      << "  }\n"
      << "}\n";

  std::cout << "[+] Exported machine-readable results to: " << path << "\n";
}

int main(int argc, char** argv) {
  std::string json_out_path = "results.json";
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--json-out" && i + 1 < argc) {
      json_out_path = argv[++i];
    }
  }

  std::cout << "=======================================================================\n";
  std::cout << " T81 Radix Comparison & Ternary Arithmetic Falsification Experiment\n";
  std::cout << "=======================================================================\n\n";

  bool ok = run_correctness_tests();
  if (!ok) {
    std::cerr << "[-] Correctness tests FAILED!\n";
    return 1;
  }

  BenchmarkResults bm = run_benchmarks();
  bm.tests_passed = ok;

  std::cout << "\n=======================================================================\n";
  std::cout << " EMPIRICAL RESULTS SUMMARY\n";
  std::cout << "=======================================================================\n";
  std::cout << std::left << std::setw(30) << "Representation"
            << std::setw(18) << "Add Latency (ns)"
            << std::setw(20) << "Conv Latency (ns)"
            << std::setw(18) << "Storage (Bytes)\n";
  std::cout << "-----------------------------------------------------------------------\n";

  auto print_row = [](const std::string& name, double add_ns, double conv_ns, size_t bytes) {
    std::cout << std::left << std::setw(30) << name
              << std::setw(18) << std::fixed << std::setprecision(2) << add_ns
              << std::setw(20) << std::fixed << std::setprecision(2) << conv_ns
              << std::setw(18) << bytes << "\n";
  };

  print_row("Binary (64-bit ALU)", bm.binary_add_ns, bm.binary_conv_ns, bm.binary_bytes_per_64bit);
  print_row("Canonical Balanced Ternary", bm.canonical_add_ns, bm.canonical_conv_ns, bm.canonical_bytes_packed_64bit);
  print_row("Redundant SD (Carry-Free)", bm.redundant_add_ns, bm.redundant_conv_ns, bm.redundant_bytes_64bit);
  print_row("Grouped Radix-9", bm.radix9_add_ns, bm.radix9_conv_ns, bm.radix9_bytes_64bit);
  print_row("Grouped Radix-27", bm.radix27_add_ns, bm.radix27_conv_ns, bm.radix27_bytes_64bit);
  print_row("Grouped Radix-81 (Tryte)", bm.radix81_add_ns, bm.radix81_conv_ns, bm.radix81_bytes_64bit);

  std::cout << "-----------------------------------------------------------------------\n";
  std::cout << "Carry Propagation Analysis:\n";
  std::cout << "  - Canonical BT Avg Carry Distance : " << bm.canonical_avg_carry_dist << " trits\n";
  std::cout << "  - Canonical BT Max Carry Distance : " << bm.canonical_max_carry_dist << " trits\n";
  std::cout << "  - Redundant SD Max Carry Distance : " << bm.redundant_max_carry_dist << " trit (Strict O(1) Bounded)\n";
  std::cout << "=======================================================================\n\n";

  export_json(bm, json_out_path);

  return 0;
}
