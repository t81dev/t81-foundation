#include "exact_rational.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  std::cout << "========================================================================\n"
            << "T81 Exact Rational Conversion Prototype Benchmark\n"
            << "========================================================================"
            << std::endl;

  using namespace t81::experiments::exact_rational;

  std::vector<std::string> inputs_hex = {
      "0000000000000000",  // +0.0
      "8000000000000000",  // -0.0
      "3fe0000000000000",  // 0.5
      "3ff0000000000000",  // 1.0
      "4000000000000000",  // 2.0
      "4008000000000000",  // 3.0
      "0000000000000001",  // Min subnormal
      "000fffffffffffff",  // Max subnormal
      "0010000000000000",  // Min normal
      "7fefffffffffffff"   // Max finite
  };

  const size_t iterations_per_input = 10000;
  const size_t total_ops = inputs_hex.size() * iterations_per_input;

  std::cout << "[+] Benchmarking C++23 Prototype with " << total_ops << " total conversions..."
            << std::endl;

  // Warmup
  for (const auto& hex : inputs_hex) {
    auto res1 = from_binary64(hex, 27, 9, "nearest_even");
    (void)res1;
  }

  // Measure from_binary64
  auto start_fb = std::chrono::high_resolution_clock::now();
  for (size_t iter = 0; iter < iterations_per_input; ++iter) {
    for (const auto& hex : inputs_hex) {
      auto res = from_binary64(hex, 27, 9, "nearest_even");
      (void)res;
    }
  }
  auto end_fb = std::chrono::high_resolution_clock::now();
  double duration_fb_ms = std::chrono::duration<double, std::milli>(end_fb - start_fb).count();
  double us_per_op_fb = (duration_fb_ms * 1000.0) / total_ops;
  double ops_per_sec_fb = (total_ops / duration_fb_ms) * 1000.0;

  // Measure to_binary64
  std::vector<std::string> ternary_inputs = {"+0E0", "++++++++++++++++++++++++++++E-1",
                                             "++----00++++-0--00-+++0++000E-677",
                                             "++0+-+-00+--+-0+--+0+---000+E646"};
  const size_t total_ops_tb = ternary_inputs.size() * iterations_per_input;

  auto start_tb = std::chrono::high_resolution_clock::now();
  for (size_t iter = 0; iter < iterations_per_input; ++iter) {
    for (const auto& text : ternary_inputs) {
      auto res = to_binary64(text, 27, 9, "nearest_even");
      (void)res;
    }
  }
  auto end_tb = std::chrono::high_resolution_clock::now();
  double duration_tb_ms = std::chrono::duration<double, std::milli>(end_tb - start_tb).count();
  double us_per_op_tb = (duration_tb_ms * 1000.0) / total_ops_tb;
  double ops_per_sec_tb = (total_ops_tb / duration_tb_ms) * 1000.0;

  std::cout << std::fixed << std::setprecision(3);
  std::cout << "\n=== Actual Benchmark Results (C++23 Prototype) ===" << std::endl;
  std::cout << "from_binary64 (<27,9> nearest_even):\n"
            << "  Total Operations: " << total_ops << "\n"
            << "  Total Time      : " << duration_fb_ms << " ms\n"
            << "  Latency per Op  : " << us_per_op_fb << " us/op\n"
            << "  Throughput      : " << ops_per_sec_fb << " ops/sec\n\n";

  std::cout << "to_binary64 (<27,9> nearest_even):\n"
            << "  Total Operations: " << total_ops_tb << "\n"
            << "  Total Time      : " << duration_tb_ms << " ms\n"
            << "  Latency per Op  : " << us_per_op_tb << " us/op\n"
            << "  Throughput      : " << ops_per_sec_tb << " ops/sec\n\n";

  return 0;
}
