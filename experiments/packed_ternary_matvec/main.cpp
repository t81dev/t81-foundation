// experiments/packed_ternary_matvec/main.cpp

#include "matvec_experiment.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace t81::research::matvec;

// Statistical helper struct
struct BenchmarkStats {
  std::string name;
  size_t m{0};
  size_t k{0};
  double median_us{0.0};
  double mean_us{0.0};
  double min_us{0.0};
  double max_us{0.0};
  double stddev_us{0.0};
  double iqr_us{0.0};
  double variation_pct{0.0};
  double gflops{0.0};
  double throughput_gb_per_sec{0.0};
};

static BenchmarkStats compute_stats(const std::string& name, size_t m, size_t k,
                                     size_t bytes_accessed, std::vector<double>& times_us) {
  std::sort(times_us.begin(), times_us.end());
  size_t n = times_us.size();

  double min_val = times_us.front();
  double max_val = times_us.back();
  double median_val = (n % 2 == 1) ? times_us[n / 2] : 0.5 * (times_us[n / 2 - 1] + times_us[n / 2]);

  double sum = 0.0;
  for (double t : times_us) sum += t;
  double mean_val = sum / static_cast<double>(n);

  double sq_sum = 0.0;
  for (double t : times_us) sq_sum += (t - mean_val) * (t - mean_val);
  double stddev_val = std::sqrt(sq_sum / static_cast<double>(n));

  double q1 = times_us[n / 4];
  double q3 = times_us[(3 * n) / 4];
  double iqr_val = q3 - q1;
  double var_pct = (median_val > 0.0) ? (iqr_val / median_val) * 100.0 : 0.0;

  // 2 * M * K FLOPs / ops per matvec
  double ops = 2.0 * static_cast<double>(m) * static_cast<double>(k);
  double gflops = (median_val > 0.0) ? (ops / (median_val * 1e-6)) / 1e9 : 0.0;
  double throughput = (median_val > 0.0) ? (static_cast<double>(bytes_accessed) / (median_val * 1e-6)) / 1e9 : 0.0;

  return {name, m, k, median_val, mean_val, min_val, max_val, stddev_val, iqr_val, var_pct, gflops, throughput};
}

// Correctness verification
static bool verify_matrix_dimensions(size_t m, size_t k, std::mt19937& rng) {
  std::vector<int8_t> trits(m * k);
  std::uniform_int_distribution<int> dist(-1, 1);
  for (size_t i = 0; i < m * k; ++i) {
    trits[i] = static_cast<int8_t>(dist(rng));
  }

  MatrixData data = MatrixData::create(m, k, trits);

  std::vector<float> x(k);
  std::uniform_real_distribution<float> fdist(-1.0f, 1.0f);
  for (size_t j = 0; j < k; ++j) {
    x[j] = fdist(rng);
  }

  std::vector<float> y_oracle(m, 0.0f);
  std::vector<float> y_test(m, 0.0f);

  matvec_oracle_f64(m, k, data.logical_trits.data(), x.data(), y_oracle.data());

  auto check_close = [&](const std::string& name, const std::vector<float>& y) {
    for (size_t i = 0; i < m; ++i) {
      float diff = std::abs(y[i] - y_oracle[i]);
      float tol = 1e-3f * (1.0f + std::abs(y_oracle[i]));
      if (diff > tol) {
        std::cerr << "Mismatch in " << name << " at row " << i << " (M=" << m << ", K=" << k
                  << "): got " << y[i] << ", expected " << y_oracle[i] << " (diff " << diff << ")\n";
        return false;
      }
    }
    return true;
  };

  bool ok = true;

  matvec_fp32(m, k, data.w_fp32.data(), x.data(), y_test.data());
  if (!check_close("matvec_fp32", y_test)) ok = false;

  matvec_fp32_avx2(m, k, data.w_fp32.data(), x.data(), y_test.data());
  if (!check_close("matvec_fp32_avx2", y_test)) ok = false;

  matvec_int8(m, k, data.w_int8.data(), x.data(), y_test.data());
  if (!check_close("matvec_int8", y_test)) ok = false;

  matvec_int8_avx2(m, k, data.w_int8.data(), x.data(), y_test.data());
  if (!check_close("matvec_int8_avx2", y_test)) ok = false;

  matvec_ternary_byte_cond(m, k, data.w_ternary_byte.data(), x.data(), y_test.data());
  if (!check_close("matvec_ternary_byte_cond", y_test)) ok = false;

  matvec_ternary_byte_branchless(m, k, data.w_ternary_byte.data(), x.data(), y_test.data());
  if (!check_close("matvec_ternary_byte_branchless", y_test)) ok = false;

  matvec_ternary_2bit_onthefly(m, k, data.w_packed_2bit.data(), x.data(), y_test.data());
  if (!check_close("matvec_ternary_2bit_onthefly", y_test)) ok = false;

  std::vector<int8_t> scratch;
  matvec_ternary_2bit_preunpacked(m, k, data.w_packed_2bit.data(), x.data(), y_test.data(), scratch);
  if (!check_close("matvec_ternary_2bit_preunpacked", y_test)) ok = false;

  matvec_ternary_2bit_swar(m, k, data.w_packed_2bit.data(), x.data(), y_test.data());
  if (!check_close("matvec_ternary_2bit_swar", y_test)) ok = false;

  matvec_ternary_2bit_lut(m, k, data.w_packed_2bit.data(), x.data(), y_test.data());
  if (!check_close("matvec_ternary_2bit_lut", y_test)) ok = false;

  matvec_ternary_2bit_avx2(m, k, data.w_packed_2bit.data(), x.data(), y_test.data());
  if (!check_close("matvec_ternary_2bit_avx2", y_test)) ok = false;

  // Verify roundtrip packing/unpacking
  std::vector<int8_t> unpacked(m * k, 0);
  unpack_ternary_2bit(data.w_packed_2bit.data(), m * k, unpacked.data());
  for (size_t i = 0; i < m * k; ++i) {
    if (unpacked[i] != data.logical_trits[i]) {
      std::cerr << "Unpack mismatch at idx " << i << ": got " << static_cast<int>(unpacked[i])
                << ", expected " << static_cast<int>(data.logical_trits[i]) << "\n";
      ok = false;
      break;
    }
  }

  // Verify invalid code handling (0x02 -> 0)
  uint8_t invalid_packed[1] = { 0x02 }; // lane 0 has invalid code 10
  int8_t invalid_out[1] = { 99 };
  unpack_ternary_2bit(invalid_packed, 1, invalid_out);
  if (invalid_out[0] != 0) {
    std::cerr << "Invalid trit code 0x02 failed to decode to 0, got: " << static_cast<int>(invalid_out[0]) << "\n";
    ok = false;
  }

  // Verify matvec_ternary_2bit_avx2 on matrix filled with invalid code 0x02 (10)
  std::vector<uint8_t> all_invalid_packed((m * k + 3) / 4, 0xAA); // 0xAA = 10101010 in binary = all invalid trits
  std::vector<float> y_invalid(m, 999.0f);
  matvec_ternary_2bit_avx2(m, k, all_invalid_packed.data(), x.data(), y_invalid.data());
  for (size_t i = 0; i < m; ++i) {
    if (std::abs(y_invalid[i]) > 1e-6f) {
      std::cerr << "Invalid code matrix execution in matvec_ternary_2bit_avx2 produced non-zero result at row "
                << i << ": " << y_invalid[i] << "\n";
      ok = false;
      break;
    }
  }

  return ok;
}

static bool run_all_verifications() {
  std::mt19937 rng(42);
  struct Shape { size_t m; size_t k; std::string desc; };
  std::vector<Shape> shapes = {
    {1, 1, "1x1 scalar edge case"},
    {7, 13, "7x13 prime/irregular shape"},
    {100, 33, "100x33 odd col alignment"},
    {127, 255, "127x255 prime - 1 shape"},
    {128, 512, "128x512 regular powers-of-two"},
    {513, 1007, "513x1007 large irregular shape"}
  };

  std::cout << "=== Running Correctness Verification Suite ===\n";
  bool overall_pass = true;
  for (const auto& s : shapes) {
    bool ok = verify_matrix_dimensions(s.m, s.k, rng);
    std::cout << "Verification [" << s.desc << " (" << s.m << "x" << s.k << ")]: "
              << (ok ? "PASS" : "FAIL") << "\n";
    if (!ok) overall_pass = false;
  }
  return overall_pass;
}

static void run_benchmarks(const std::string& json_path) {
  constexpr int REPETITIONS = 100;
  std::mt19937 rng(12345);

  struct Workload {
    std::string name;
    size_t m;
    size_t k;
  };

  std::vector<Workload> workloads = {
    {"Cache-Resident (L1/L2)", 64, 256},
    {"Medium (L3 Transition)", 512, 512},
    {"Memory-Bound (DRAM)", 4096, 4096}
  };

  std::vector<BenchmarkStats> all_stats;

  std::cout << "\n=== Running Performance Benchmark Suite (" << REPETITIONS << " repetitions) ===\n";

  for (const auto& wl : workloads) {
    size_t m = wl.m;
    size_t k = wl.k;
    size_t total = m * k;

    std::vector<int8_t> trits(total);
    std::uniform_int_distribution<int> dist(-1, 1);
    for (size_t i = 0; i < total; ++i) trits[i] = static_cast<int8_t>(dist(rng));

    MatrixData data = MatrixData::create(m, k, trits);

    std::vector<float> x(k);
    std::uniform_real_distribution<float> fdist(-1.0f, 1.0f);
    for (size_t j = 0; j < k; ++j) x[j] = fdist(rng);

    std::vector<float> y(m, 0.0f);
    std::vector<int8_t> scratch(total);

    size_t bytes_fp32 = total * sizeof(float) + k * sizeof(float) + m * sizeof(float);
    size_t bytes_int8 = total * sizeof(int8_t) + k * sizeof(float) + m * sizeof(float);
    size_t bytes_packed = (total + 3) / 4 + k * sizeof(float) + m * sizeof(float);

    std::cout << "\n--- Workload: " << wl.name << " (M=" << m << ", K=" << k << ") ---\n";

    // 1. Packing Cost Alone
    {
      std::vector<double> times;
      times.reserve(REPETITIONS);
      for (int r = 0; r < REPETITIONS; ++r) {
        auto t0 = std::chrono::high_resolution_clock::now();
        auto packed = pack_ternary_2bit(trits.data(), total);
        auto t1 = std::chrono::high_resolution_clock::now();
        do_not_optimize(packed.data());
        times.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
      }
      all_stats.push_back(compute_stats(wl.name + " :: Packing Cost Alone", m, k, total, times));
    }

    // 2. Scratch Alloc + Unpack Cost Alone
    {
      std::vector<double> times;
      times.reserve(REPETITIONS);
      for (int r = 0; r < REPETITIONS; ++r) {
        auto t0 = std::chrono::high_resolution_clock::now();
        std::vector<int8_t> buf(total);
        unpack_ternary_2bit(data.w_packed_2bit.data(), total, buf.data());
        auto t1 = std::chrono::high_resolution_clock::now();
        do_not_optimize(buf.data());
        times.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
      }
      all_stats.push_back(compute_stats(wl.name + " :: Unpack & Alloc Cost Alone", m, k, total, times));
    }

    // Benchmark Kernel Function helper
    auto bench_kernel = [&](const std::string& name, size_t bytes, auto func) {
      std::vector<double> times;
      times.reserve(REPETITIONS);
      // Warmup
      func();
      for (int r = 0; r < REPETITIONS; ++r) {
        auto t0 = std::chrono::high_resolution_clock::now();
        func();
        auto t1 = std::chrono::high_resolution_clock::now();
        do_not_optimize(y.data());
        times.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
      }
      auto stats = compute_stats(wl.name + " :: " + name, m, k, bytes, times);
      std::cout << std::left << std::setw(45) << stats.name
                << " Median: " << std::right << std::setw(9) << std::fixed << std::setprecision(2) << stats.median_us << " us | "
                << std::setw(7) << std::setprecision(2) << stats.gflops << " GFLOP/s | "
                << "IQR: " << std::setw(5) << std::setprecision(1) << stats.variation_pct << "%\n";
      all_stats.push_back(stats);
    };

    bench_kernel("1a. FP32 Scalar", bytes_fp32, [&]() {
      matvec_fp32(m, k, data.w_fp32.data(), x.data(), y.data());
    });

    bench_kernel("1b. FP32 AVX2", bytes_fp32, [&]() {
      matvec_fp32_avx2(m, k, data.w_fp32.data(), x.data(), y.data());
    });

    bench_kernel("2a. INT8 Scalar", bytes_int8, [&]() {
      matvec_int8(m, k, data.w_int8.data(), x.data(), y.data());
    });

    bench_kernel("2b. INT8 AVX2", bytes_int8, [&]() {
      matvec_int8_avx2(m, k, data.w_int8.data(), x.data(), y.data());
    });

    bench_kernel("3a. Ternary 1-Byte Cond", bytes_int8, [&]() {
      matvec_ternary_byte_cond(m, k, data.w_ternary_byte.data(), x.data(), y.data());
    });

    bench_kernel("3b. Ternary 1-Byte Branchless", bytes_int8, [&]() {
      matvec_ternary_byte_branchless(m, k, data.w_ternary_byte.data(), x.data(), y.data());
    });

    bench_kernel("4a. Ternary 2-bit On-The-Fly", bytes_packed, [&]() {
      matvec_ternary_2bit_onthefly(m, k, data.w_packed_2bit.data(), x.data(), y.data());
    });

    bench_kernel("4b. Ternary 2-bit Pre-Unpacked", bytes_packed, [&]() {
      matvec_ternary_2bit_preunpacked(m, k, data.w_packed_2bit.data(), x.data(), y.data(), scratch);
    });

    bench_kernel("4c. Ternary 2-bit SWAR (4-trit)", bytes_packed, [&]() {
      matvec_ternary_2bit_swar(m, k, data.w_packed_2bit.data(), x.data(), y.data());
    });

    bench_kernel("4d. Ternary 2-bit LUT (4-trit)", bytes_packed, [&]() {
      matvec_ternary_2bit_lut(m, k, data.w_packed_2bit.data(), x.data(), y.data());
    });

    bench_kernel("4e. Ternary 2-bit AVX2 Candidate", bytes_packed, [&]() {
      matvec_ternary_2bit_avx2(m, k, data.w_packed_2bit.data(), x.data(), y.data());
    });

    // 5. End-to-End Single-Use (Pack + SWAR Run)
    bench_kernel("5a. Single-Use End-to-End (Pack+Run)", bytes_packed, [&]() {
      auto packed = pack_ternary_2bit(trits.data(), total);
      matvec_ternary_2bit_swar(m, k, packed.data(), x.data(), y.data());
      do_not_optimize(packed.data());
    });
  }

  // Generate JSON Report
  if (!json_path.empty()) {
    std::ofstream ofs(json_path);
    if (!ofs.is_open()) {
      std::cerr << "Failed to open JSON output path: " << json_path << "\n";
      return;
    }

    ofs << "{\n";
    ofs << "  \"experiment\": \"t81_packed_ternary_matvec\",\n";
    ofs << "  \"repetitions\": " << REPETITIONS << ",\n";
#if defined(__AVX2__)
    ofs << "  \"compiler_avx2_enabled\": true,\n";
#else
    ofs << "  \"compiler_avx2_enabled\": false,\n";
#endif
    ofs << "  \"results\": [\n";
    for (size_t i = 0; i < all_stats.size(); ++i) {
      const auto& st = all_stats[i];
      ofs << "    {\n";
      ofs << "      \"name\": \"" << st.name << "\",\n";
      ofs << "      \"m\": " << st.m << ",\n";
      ofs << "      \"k\": " << st.k << ",\n";
      ofs << "      \"median_us\": " << st.median_us << ",\n";
      ofs << "      \"mean_us\": " << st.mean_us << ",\n";
      ofs << "      \"min_us\": " << st.min_us << ",\n";
      ofs << "      \"max_us\": " << st.max_us << ",\n";
      ofs << "      \"stddev_us\": " << st.stddev_us << ",\n";
      ofs << "      \"iqr_us\": " << st.iqr_us << ",\n";
      ofs << "      \"variation_pct\": " << st.variation_pct << ",\n";
      ofs << "      \"gflops\": " << st.gflops << ",\n";
      ofs << "      \"throughput_gb_per_sec\": " << st.throughput_gb_per_sec << "\n";
      ofs << "    }" << (i + 1 < all_stats.size() ? "," : "") << "\n";
    }
    ofs << "  ]\n";
    ofs << "}\n";
    std::cout << "\nResults successfully saved to JSON: " << json_path << "\n";
  }
}

int main(int argc, char** argv) {
  std::string json_path = "";
  bool verify_only = false;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--json-out" && i + 1 < argc) {
      json_path = argv[++i];
    } else if (arg == "--verify-only") {
      verify_only = true;
    }
  }

  bool pass = run_all_verifications();
  if (!pass) {
    std::cerr << "Verification failed!\n";
    return 1;
  }

  if (verify_only) {
    std::cout << "Verification suite passed successfully.\n";
    return 0;
  }

  run_benchmarks(json_path);
  return 0;
}
