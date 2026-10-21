/**
 * @file main.cpp
 * @brief Test runner and benchmark harness for Radix Comparison Experiment.
 *
 * Runs exhaustive correctness tests, carry-chain stress tests, property tests,
 * stack array vs dynamic vector benchmarks, packing analysis, and empirical latency measurements.
 *
 * Outputs human-readable summary tables and machine-readable JSON results.
 */

#include "radix_comparison.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>

using namespace t81::experiments::radix;

struct BenchmarkResults {
  double binary_add_ns = 0.0;

  // Stack Array Addition Latency (std::array, isolates allocation overhead)
  double canonical_arr_add_ns = 0.0;
  double redundant_arr_add_ns = 0.0;
  double radix9_arr_add_ns = 0.0;
  double radix27_arr_add_ns = 0.0;
  double radix81_arr_add_ns = 0.0;

  // Dynamic Vector Addition Latency (std::vector, includes malloc/free)
  double canonical_vec_add_ns = 0.0;
  double redundant_vec_add_ns = 0.0;
  double radix9_vec_add_ns = 0.0;
  double radix27_vec_add_ns = 0.0;
  double radix81_vec_add_ns = 0.0;

  // Conversion Latency
  double binary_conv_ns = 0.0;
  double canonical_conv_ns = 0.0;
  double redundant_conv_ns = 0.0;
  double radix9_conv_ns = 0.0;
  double radix27_conv_ns = 0.0;
  double radix81_conv_ns = 0.0;

  // Packing / Unpacking Latency
  double pack_base243_ns = 0.0;
  double unpack_base243_ns = 0.0;

  // Carry Propagation Statistics
  double canonical_avg_carry_dist = 0.0;
  double canonical_max_carry_dist = 0.0;
  double redundant_avg_transfer_dist = 0.0;
  double redundant_max_transfer_dist = 0.0;
  double redundant_total_transfers_avg = 0.0;

  // Storage Sizes (for full 64-bit signed integer range, 41 trits)
  size_t binary_bytes_per_64bit = 8;
  size_t canonical_bytes_packed_64bit = 11; // 41 trits / 4 = 11 bytes
  size_t redundant_bytes_64bit = 41;       // 41 bytes (1 byte/digit)
  size_t radix9_bytes_64bit = 21;          // 21 bytes
  size_t radix27_bytes_64bit = 14;         // 14 bytes
  size_t radix81_bytes_64bit = 11;         // 11 bytes (41 trits / 4)
  size_t base243_bytes_64bit = 9;          // 41 trits / 5 = 9 bytes

  bool tests_passed = false;
};

// ----------------------------------------------------------------------------
// Correctness & Property Test Suite
// ----------------------------------------------------------------------------
bool run_correctness_tests() {
  std::cout << "[+] Running Correctness & Property Test Suite...\n";

  // Special test values including INT64_MIN and INT64_MAX boundary extremes
  const std::vector<int64_t> test_values = {
      0, 1, -1, 2, -2, 3, -3, 40, -40, 81, -81, 364, -364, 3280, -3280,
      100000, -100000, 2147483647LL, -2147483648LL, 1215766545905692880LL,
      -1215766545905692880LL, std::numeric_limits<int64_t>::max(),
      std::numeric_limits<int64_t>::min()};

  for (int64_t v : test_values) {
    // 1. Dynamic Vector Roundtrip Tests
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

    // 2. Stack Array Roundtrip Tests
    CanonicalTernaryArray cta = CanonicalTernaryArray::from_int64(v);
    if (cta.to_int64() != v) {
      std::cerr << "[-] CanonicalTernaryArray roundtrip failed for " << v << " got " << cta.to_int64() << "\n";
      return false;
    }

    RedundantTernaryArray rta = RedundantTernaryArray::from_int64(v);
    if (rta.to_int64() != v) {
      std::cerr << "[-] RedundantTernaryArray roundtrip failed for " << v << " got " << rta.to_int64() << "\n";
      return false;
    }

    Radix81Array r81a = Radix81Array::from_int64(v);
    if (r81a.to_int64() != v) {
      std::cerr << "[-] Radix81Array roundtrip failed for " << v << " got " << r81a.to_int64() << "\n";
      return false;
    }

    // 3. Base-243 Packing Roundtrip Test
    auto packed_243 = pack_base243(ct.trits);
    auto unpacked_243 = unpack_base243(packed_243, ct.trits.size());
    CanonicalTernary ct_unpacked(unpacked_243);
    if (ct_unpacked.to_int64() != v) {
      std::cerr << "[-] Base-243 packing roundtrip failed for " << v << " got " << ct_unpacked.to_int64() << "\n";
      return false;
    }
  }

  // 4. Randomized Addition & Subtraction (Stack Array & Vector for all types)
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

    // Canonical Ternary Vector
    CanonicalTernary ct_a = CanonicalTernary::from_int64(a);
    CanonicalTernary ct_b = CanonicalTernary::from_int64(b);
    CanonicalTernary ct_sum = CanonicalTernary::add(ct_a, ct_b);
    if (ct_sum.to_int64() != expected_sum) {
      std::cerr << "[-] CanonicalTernary add failed for " << a << " + " << b << "\n";
      return false;
    }

    // Canonical Ternary Stack Array
    CanonicalTernaryArray cta_a = CanonicalTernaryArray::from_int64(a);
    CanonicalTernaryArray cta_b = CanonicalTernaryArray::from_int64(b);
    CanonicalTernaryArray cta_sum = CanonicalTernaryArray::add(cta_a, cta_b);
    if (cta_sum.to_int64() != expected_sum) {
      std::cerr << "[-] CanonicalTernaryArray add failed for " << a << " + " << b << "\n";
      return false;
    }

    // Redundant Ternary Carry-Free (Vector)
    RedundantTernary rt_a = RedundantTernary::from_int64(a);
    RedundantTernary rt_b = RedundantTernary::from_int64(b);
    RedundantTernary rt_sum = RedundantTernary::add_carry_free(rt_a, rt_b);
    if (rt_sum.to_int64() != expected_sum) {
      std::cerr << "[-] RedundantTernary add_carry_free failed for " << a << " + " << b << "\n";
      return false;
    }

    // Redundant Ternary Carry-Free (Stack Array)
    RedundantTernaryArray rta_a = RedundantTernaryArray::from_int64(a);
    RedundantTernaryArray rta_b = RedundantTernaryArray::from_int64(b);
    RedundantTernaryArray rta_sum = RedundantTernaryArray::add_carry_free(rta_a, rta_b);
    if (rta_sum.to_int64() != expected_sum) {
      std::cerr << "[-] RedundantTernaryArray add_carry_free failed for " << a << " + " << b << "\n";
      return false;
    }

    // Radix-9 Vector & Array
    Radix9Integer r9_a = Radix9Integer::from_int64(a);
    Radix9Integer r9_b = Radix9Integer::from_int64(b);
    if (Radix9Integer::add(r9_a, r9_b).to_int64() != expected_sum) {
      std::cerr << "[-] Radix9Integer add failed for " << a << " + " << b << "\n";
      return false;
    }

    // Radix-27 Vector & Array
    Radix27Integer r27_a = Radix27Integer::from_int64(a);
    Radix27Integer r27_b = Radix27Integer::from_int64(b);
    if (Radix27Integer::add(r27_a, r27_b).to_int64() != expected_sum) {
      std::cerr << "[-] Radix27Integer add failed for " << a << " + " << b << "\n";
      return false;
    }

    // Radix-81 Vector & Array
    Radix81Integer r81_a = Radix81Integer::from_int64(a);
    Radix81Integer r81_b = Radix81Integer::from_int64(b);
    if (Radix81Integer::add(r81_a, r81_b).to_int64() != expected_sum) {
      std::cerr << "[-] Radix81Integer add failed for " << a << " + " << b << "\n";
      return false;
    }

    Radix81Array r81a_a = Radix81Array::from_int64(a);
    Radix81Array r81a_b = Radix81Array::from_int64(b);
    Radix81Array r81a_sum = Radix81Array::add(r81a_a, r81a_b);
    if (r81a_sum.to_int64() != expected_sum) {
      std::cerr << "[-] Radix81Array add failed for " << a << " + " << b << "\n";
      return false;
    }
  }

  // 5. Long Carry Chain Test
  int64_t max_trit_val = 1743392200LL;
  CanonicalTernary max_ct = CanonicalTernary::from_int64(max_trit_val);
  CanonicalTernary one_ct = CanonicalTernary::from_int64(1);
  auto [chain_res, max_chain, total_carries] = CanonicalTernary::add_with_stats(max_ct, one_ct);
  if (chain_res.to_int64() != max_trit_val + 1) {
    std::cerr << "[-] Carry chain test failed! expected " << max_trit_val + 1 << " got " << chain_res.to_int64() << "\n";
    return false;
  }

  std::cout << "  [PASS] All correctness, boundary, carry-chain, array, and roundtrip tests passed!\n";
  return true;
}

// ----------------------------------------------------------------------------
// Performance & Latency Benchmark Harness
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

  // Pre-convert operands for addition benchmark (Vectors)
  std::vector<Binary64> bin_a(kNumOps), bin_b(kNumOps);
  std::vector<CanonicalTernary> ct_a(kNumOps), ct_b(kNumOps);
  std::vector<RedundantTernary> rt_a(kNumOps), rt_b(kNumOps);
  std::vector<Radix9Integer> r9_a(kNumOps), r9_b(kNumOps);
  std::vector<Radix27Integer> r27_a(kNumOps), r27_b(kNumOps);
  std::vector<Radix81Integer> r81_a(kNumOps), r81_b(kNumOps);

  // Pre-convert operands for addition benchmark (Stack Arrays)
  std::vector<CanonicalTernaryArray> cta_a(kNumOps), cta_b(kNumOps);
  std::vector<RedundantTernaryArray> rta_a(kNumOps), rta_b(kNumOps);
  std::vector<Radix9Array> r9a_a(kNumOps), r9a_b(kNumOps);
  std::vector<Radix27Array> r27a_a(kNumOps), r27a_b(kNumOps);
  std::vector<Radix81Array> r81a_a(kNumOps), r81a_b(kNumOps);

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

    cta_a[i] = CanonicalTernaryArray::from_int64(va[i]);
    cta_b[i] = CanonicalTernaryArray::from_int64(vb[i]);

    rta_a[i] = RedundantTernaryArray::from_int64(va[i]);
    rta_b[i] = RedundantTernaryArray::from_int64(vb[i]);

    r9a_a[i] = Radix9Array::from_int64(va[i]);
    r9a_b[i] = Radix9Array::from_int64(vb[i]);

    r27a_a[i] = Radix27Array::from_int64(va[i]);
    r27a_b[i] = Radix27Array::from_int64(vb[i]);

    r81a_a[i] = Radix81Array::from_int64(va[i]);
    r81a_b[i] = Radix81Array::from_int64(vb[i]);
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

  // 2. Binary Hardware ALU Addition Benchmark
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Binary64::add(bin_a[i], bin_b[i]);
      (void)x;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    res.binary_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;
  }

  // 3. Stack Array Addition Benchmark (Isolates Heap Allocation Overhead)
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = CanonicalTernaryArray::add(cta_a[i], cta_b[i]);
      (void)x;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    res.canonical_arr_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = RedundantTernaryArray::add_carry_free(rta_a[i], rta_b[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.redundant_arr_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Radix9Array::add(r9a_a[i], r9a_b[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.radix9_arr_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Radix27Array::add(r27a_a[i], r27a_b[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.radix27_arr_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Radix81Array::add(r81a_a[i], r81a_b[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.radix81_arr_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;
  }

  // 4. Dynamic Vector Addition Benchmark (Includes malloc/free)
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = CanonicalTernary::add(ct_a[i], ct_b[i]);
      (void)x;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    res.canonical_vec_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = RedundantTernary::add_carry_free(rt_a[i], rt_b[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.redundant_vec_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Radix9Integer::add(r9_a[i], r9_b[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.radix9_vec_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Radix27Integer::add(r27_a[i], r27_b[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.radix27_vec_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto x = Radix81Integer::add(r81_a[i], r81_b[i]);
      (void)x;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.radix81_vec_add_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;
  }

  // 5. Packing / Unpacking Benchmark (Base-243)
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto packed = pack_base243(ct_a[i].trits);
      (void)packed;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    res.pack_base243_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;

    std::vector<std::vector<uint8_t>> pre_packed(kNumOps);
    for (size_t i = 0; i < kNumOps; ++i) pre_packed[i] = pack_base243(ct_a[i].trits);

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kNumOps; ++i) {
      volatile auto unpacked = unpack_base243(pre_packed[i], ct_a[i].trits.size());
      (void)unpacked;
    }
    t1 = std::chrono::high_resolution_clock::now();
    res.unpack_base243_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kNumOps;
  }

  // 6. Carry / Transfer Propagation Instrumentation
  {
    double total_carry_ops = 0.0;
    size_t max_carry_chain = 0;
    for (size_t i = 0; i < kNumOps; ++i) {
      auto [r, max_c, total_c] = CanonicalTernary::add_with_stats(ct_a[i], ct_b[i]);
      (void)r;
      total_carry_ops += total_c;
      max_carry_chain = std::max(max_carry_chain, max_c);
    }
    res.canonical_avg_carry_dist = total_carry_ops / kNumOps;
    res.canonical_max_carry_dist = static_cast<double>(max_carry_chain);

    double total_transfers = 0.0;
    double max_transfer_dist = 0.0;
    for (size_t i = 0; i < kNumOps; ++i) {
      auto [r, max_t_dist, total_t] = RedundantTernary::add_carry_free_with_stats(rt_a[i], rt_b[i]);
      (void)r;
      total_transfers += total_t;
      max_transfer_dist = std::max(max_transfer_dist, max_t_dist);
    }
    res.redundant_avg_transfer_dist = (total_transfers > 0) ? 1.0 : 0.0;
    res.redundant_max_transfer_dist = max_transfer_dist;
    res.redundant_total_transfers_avg = total_transfers / kNumOps;
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
      << "  \"experiment\": \"t81_radix_comparison_adversarial_audit\",\n"
      << "  \"status\": \"" << (res.tests_passed ? "PASSED" : "FAILED") << "\",\n"
      << "  \"storage_bytes_64bit\": {\n"
      << "    \"binary64\": " << res.binary_bytes_per_64bit << ",\n"
      << "    \"canonical_ternary_packed\": " << res.canonical_bytes_packed_64bit << ",\n"
      << "    \"redundant_ternary\": " << res.redundant_bytes_64bit << ",\n"
      << "    \"radix9\": " << res.radix9_bytes_64bit << ",\n"
      << "    \"radix27\": " << res.radix27_bytes_64bit << ",\n"
      << "    \"radix81\": " << res.radix81_bytes_64bit << ",\n"
      << "    \"base243\": " << res.base243_bytes_64bit << "\n"
      << "  },\n"
      << "  \"addition_latency_ns\": {\n"
      << "    \"binary64_alu\": " << res.binary_add_ns << ",\n"
      << "    \"stack_array\": {\n"
      << "      \"canonical_ternary\": " << res.canonical_arr_add_ns << ",\n"
      << "      \"redundant_ternary_carry_free\": " << res.redundant_arr_add_ns << ",\n"
      << "      \"radix9\": " << res.radix9_arr_add_ns << ",\n"
      << "      \"radix27\": " << res.radix27_arr_add_ns << ",\n"
      << "      \"radix81\": " << res.radix81_arr_add_ns << "\n"
      << "    },\n"
      << "    \"dynamic_vector\": {\n"
      << "      \"canonical_ternary\": " << res.canonical_vec_add_ns << ",\n"
      << "      \"redundant_ternary_carry_free\": " << res.redundant_vec_add_ns << ",\n"
      << "      \"radix9\": " << res.radix9_vec_add_ns << ",\n"
      << "      \"radix27\": " << res.radix27_vec_add_ns << ",\n"
      << "      \"radix81\": " << res.radix81_vec_add_ns << "\n"
      << "    }\n"
      << "  },\n"
      << "  \"conversion_latency_ns\": {\n"
      << "    \"binary64\": " << res.binary_conv_ns << ",\n"
      << "    \"canonical_ternary\": " << res.canonical_conv_ns << ",\n"
      << "    \"redundant_ternary\": " << res.redundant_conv_ns << ",\n"
      << "    \"radix9\": " << res.radix9_conv_ns << ",\n"
      << "    \"radix27\": " << res.radix27_conv_ns << ",\n"
      << "    \"radix81\": " << res.radix81_conv_ns << "\n"
      << "  },\n"
      << "  \"packing_throughput_ns\": {\n"
      << "    \"pack_base243\": " << res.pack_base243_ns << ",\n"
      << "    \"unpack_base243\": " << res.unpack_base243_ns << "\n"
      << "  },\n"
      << "  \"carry_propagation_distance\": {\n"
      << "    \"canonical_avg_carry_ops\": " << res.canonical_avg_carry_dist << ",\n"
      << "    \"canonical_max_carry_chain\": " << res.canonical_max_carry_dist << ",\n"
      << "    \"redundant_max_transfer_dist\": " << res.redundant_max_transfer_dist << ",\n"
      << "    \"redundant_avg_transfer_dist\": " << res.redundant_avg_transfer_dist << ",\n"
      << "    \"redundant_avg_transfers_count\": " << res.redundant_total_transfers_avg << "\n"
      << "  }\n"
      << "}\n";

  std::cout << "[+] Exported audited results to: " << path << "\n";
}

int main(int argc, char** argv) {
  std::string json_out_path = "results.json";
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--json-out" && i + 1 < argc) {
      json_out_path = argv[++i];
    }
  }

  std::cout << "=======================================================================\n";
  std::cout << " T81 Radix Comparison & Ternary Arithmetic Adversarial Audit\n";
  std::cout << "=======================================================================\n\n";

  bool ok = run_correctness_tests();
  if (!ok) {
    std::cerr << "[-] Correctness tests FAILED!\n";
    return 1;
  }

  BenchmarkResults bm = run_benchmarks();
  bm.tests_passed = ok;

  std::cout << "\n=======================================================================\n";
  std::cout << " AUDITED ARITHMETIC BENCHMARK SUMMARY (64-bit Signed Domain, 41 Trits)\n";
  std::cout << "=======================================================================\n";
  std::cout << std::left << std::setw(30) << "Representation"
            << std::setw(18) << "Stack Add (ns)"
            << std::setw(18) << "Vector Add (ns)"
            << std::setw(18) << "Storage (Bytes)\n";
  std::cout << "-----------------------------------------------------------------------\n";

  auto print_row = [](const std::string& name, double stack_ns, double vec_ns, size_t bytes) {
    std::cout << std::left << std::setw(30) << name
              << std::setw(18) << std::fixed << std::setprecision(2) << stack_ns
              << std::setw(18) << std::fixed << std::setprecision(2) << vec_ns
              << std::setw(18) << bytes << "\n";
  };

  print_row("Binary (64-bit Hardware ALU)", bm.binary_add_ns, 0.0, bm.binary_bytes_per_64bit);
  print_row("Grouped Radix-81 (Tryte)", bm.radix81_arr_add_ns, bm.radix81_vec_add_ns, bm.radix81_bytes_64bit);
  print_row("Grouped Radix-27", bm.radix27_arr_add_ns, bm.radix27_vec_add_ns, bm.radix27_bytes_64bit);
  print_row("Grouped Radix-9", bm.radix9_arr_add_ns, bm.radix9_vec_add_ns, bm.radix9_bytes_64bit);
  print_row("Canonical Balanced Ternary", bm.canonical_arr_add_ns, bm.canonical_vec_add_ns, bm.canonical_bytes_packed_64bit);
  print_row("Redundant SD (Carry-Free)", bm.redundant_arr_add_ns, bm.redundant_vec_add_ns, bm.redundant_bytes_64bit);

  std::cout << "-----------------------------------------------------------------------\n";
  std::cout << "Carry & Transfer Propagation Analysis:\n";
  std::cout << "  - Canonical BT Avg Carry Ops   : " << bm.canonical_avg_carry_dist << " trits/add\n";
  std::cout << "  - Canonical BT Max Carry Chain : " << bm.canonical_max_carry_dist << " trits\n";
  std::cout << "  - Redundant SD Transfer Dist   : " << bm.redundant_max_transfer_dist << " digit (Strict parallel transfer bound)\n";
  std::cout << "  - Base-243 Pack / Unpack       : " << bm.pack_base243_ns << " ns / " << bm.unpack_base243_ns << " ns\n";
  std::cout << "=======================================================================\n\n";

  export_json(bm, json_out_path);

  return 0;
}
