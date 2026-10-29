#include "exact_rational.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct CorpusCase {
  std::string id;
  std::string operation;
  std::string input;
  std::string numerator;
  std::string denominator;
  int m = 27;
  int e = 9;
  std::string mode = "exact_only";
  t81::experiments::exact_rational::ConversionResult expected;
};

static std::vector<CorpusCase> load_corpus(const std::string& filepath) {
  std::ifstream ifs(filepath);
  if (!ifs.is_open()) {
    std::cerr << "Failed to open corpus file: " << filepath << std::endl;
    return {};
  }
  std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

  std::vector<CorpusCase> cases;
  size_t pos = 0;
  while ((pos = content.find("\"id\":", pos)) != std::string::npos) {
    size_t case_start = content.rfind('{', pos);
    size_t case_end = content.find("}\n", pos);
    if (case_end == std::string::npos) case_end = content.find("}", pos);

    std::string block = content.substr(case_start, case_end - case_start + 1);
    CorpusCase c;

    auto get_val = [&](const std::string& key) -> std::string {
      size_t kpos = block.find("\"" + key + "\":");
      if (kpos == std::string::npos) return "";
      size_t vstart = block.find(':', kpos) + 1;
      while (vstart < block.length() &&
             (block[vstart] == ' ' || block[vstart] == '\n' || block[vstart] == '\r'))
        vstart++;
      if (block[vstart] == '"') {
        size_t vend = block.find('"', vstart + 1);
        return block.substr(vstart + 1, vend - vstart - 1);
      } else {
        size_t vend = block.find_first_of(",}\n\r", vstart);
        return block.substr(vstart, vend - vstart);
      }
    };

    c.id = get_val("id");
    c.operation = get_val("operation");
    c.input = get_val("input");
    c.numerator = get_val("numerator");
    c.denominator = get_val("denominator");

    std::string m_str = get_val("M");
    if (!m_str.empty()) {
      try {
        c.m = std::stoi(m_str);
      } catch (...) {
        c.m = -1;
      }
    }

    std::string e_str = get_val("E");
    if (!e_str.empty()) {
      try {
        c.e = std::stoi(e_str);
      } catch (...) {
        c.e = -1;
      }
    }

    std::string mode_str = get_val("mode");
    if (!mode_str.empty()) c.mode = mode_str;

    // Parse expected block
    size_t exp_pos = block.find("\"expected\":");
    if (exp_pos != std::string::npos) {
      std::string exp_block = block.substr(exp_pos);
      auto get_exp_val = [&](const std::string& key) -> std::string {
        size_t kpos = exp_block.find("\"" + key + "\":");
        if (kpos == std::string::npos) return "";
        size_t vstart = exp_block.find(':', kpos) + 1;
        while (vstart < exp_block.length() &&
               (exp_block[vstart] == ' ' || exp_block[vstart] == '\n' || exp_block[vstart] == '\r'))
          vstart++;
        if (exp_block.substr(vstart, 4) == "null") return "null";
        if (exp_block[vstart] == '"') {
          size_t vend = exp_block.find('"', vstart + 1);
          return exp_block.substr(vstart + 1, vend - vstart - 1);
        } else {
          size_t vend = exp_block.find_first_of(",}\n\r", vstart);
          return exp_block.substr(vstart, vend - vstart);
        }
      };

      std::string status = get_exp_val("status");
      std::string val = get_exp_val("value");
      std::string err = get_exp_val("error");

      c.expected.status = status;
      if (val != "null" && !val.empty()) c.expected.value = val;
      if (err != "null" && !err.empty()) c.expected.error = err;

      if (exp_block.find("negative_zero_normalized") != std::string::npos) {
        c.expected.flags.push_back("negative_zero_normalized");
      }
    }

    cases.push_back(c);
    pos += 5;
  }
  return cases;
}

int main(int argc, char** argv) {
  std::cout << "========================================================================\n"
            << "T81 Scientific Investigation 03 — Exact Rational Prototype Verification\n"
            << "========================================================================"
            << std::endl;

  std::string corpus_path = "experiments/checked_float_contract/corpus.json";
  if (argc > 1) corpus_path = argv[1];

  auto cases = load_corpus(corpus_path);
  std::cout << "[+] Loaded " << cases.size() << " test cases from " << corpus_path << std::endl;

  size_t passed = 0;
  size_t failed = 0;

  using namespace t81::experiments::exact_rational;

  std::ofstream report("experiments/exact_rational_prototype/detailed_evaluation_report.txt");
  report << "========================================================================\n"
         << "Detailed Evaluation Report — Corpus & Boundary Verification\n"
         << "========================================================================\n\n";

  for (const auto& c : cases) {
    ConversionResult actual;
    if (c.operation == "from_binary64") {
      actual = from_binary64(c.input, c.m, c.e, c.mode);
    } else if (c.operation == "to_binary64") {
      actual = to_binary64(c.input, c.m, c.e, c.mode);
    } else if (c.operation == "quantize_rational") {
      BigRational rat = BigRational::from_strings(c.numerator, c.denominator);
      actual = quantize_rational(rat, c.m, c.e, c.mode);
    } else {
      std::cerr << "Unknown operation: " << c.operation << " in case " << c.id << std::endl;
      failed++;
      continue;
    }

    bool ok = (actual == c.expected);
    if (ok)
      passed++;
    else
      failed++;

    report << "Case ID  : " << c.id << "\n"
           << "Operation: " << c.operation << " (M=" << c.m << ", E=" << c.e << ", mode=" << c.mode
           << ")\n"
           << "Expected : status=" << c.expected.status
           << ", value=" << (c.expected.value ? *c.expected.value : "null")
           << ", error=" << (c.expected.error ? *c.expected.error : "null") << ", flags=["
           << (c.expected.flags.empty() ? "" : c.expected.flags[0]) << "]\n"
           << "Actual   : status=" << actual.status
           << ", value=" << (actual.value ? *actual.value : "null")
           << ", error=" << (actual.error ? *actual.error : "null") << ", flags=["
           << (actual.flags.empty() ? "" : actual.flags[0]) << "]\n"
           << "Result   : " << (ok ? "PASS" : "FAIL") << "\n\n";
  }

  std::cout << "[+] Corpus Conformance: " << passed << " PASSED, " << failed << " FAILED out of "
            << cases.size() << " cases.\n"
            << std::endl;

  std::cout << "=== Gate 1 Counterexample Verification ===" << std::endl;
  struct Counterexample {
    std::string id;
    std::string bits;
    std::string description;
    std::string expected_nearest;
  };

  std::vector<Counterexample> counterexamples = {
      {"CE-1", "7fefffffffffffff", "Max Finite Positive (2^53-1)*2^971",
       "++0+-+-00+--+-0+--+0+---000+E646"},
      {"CE-2", "0000000000000001", "Min Positive Subnormal 2^-1074",
       "++----00++++-0--00-+++0++000E-677"},
      {"CE-3", "3fe0000000000000", "Positive One-Half 0.5", "++++++++++++++++++++++++++++E-1"}};

  for (const auto& ce : counterexamples) {
    auto res_exact = from_binary64(ce.bits, 27, 9, "exact_only");
    auto res_nearest = from_binary64(ce.bits, 27, 9, "nearest_even");

    bool ce_ok = (res_exact.error == "inexact" && res_nearest.value == ce.expected_nearest);

    std::cout << ce.id << " (" << ce.description << "):\n"
              << "  Hex Input      : " << ce.bits << "\n"
              << "  exact_only Mode: status=" << res_exact.status
              << ", error=" << (res_exact.error ? *res_exact.error : "null") << "\n"
              << "  nearest_even   : status=" << res_nearest.status
              << ", value=" << (res_nearest.value ? *res_nearest.value : "null") << "\n"
              << "  Expected Value : " << ce.expected_nearest << "\n"
              << "  Disposition    : " << (ce_ok ? "RESOLVED (MATCH)" : "DEFECT (MISMATCH)") << "\n"
              << std::endl;

    report << "Counterexample " << ce.id << " (" << ce.description << "):\n"
           << "  exact_only error=" << (res_exact.error ? *res_exact.error : "null") << "\n"
           << "  nearest_even value=" << (res_nearest.value ? *res_nearest.value : "null") << "\n"
           << "  expected value=" << ce.expected_nearest << "\n"
           << "  Match: " << (ce_ok ? "YES" : "NO") << "\n\n";
  }

  std::cout << "=== Additional Mathematical Boundary Cases ===" << std::endl;
  struct BoundaryCase {
    std::string description;
    ConversionResult actual;
    ConversionResult expected;
  };

  std::vector<BoundaryCase> boundaries = {
      {"Subnormal Underflow Threshold (<27,9> 3^-9868)",
       quantize_rational(BigRational(BigInt(1), BigInt::pow3(9868)), 27, 9, "nearest_even"),
       make_result(std::nullopt, "error", "underflow")},
      {"Subnormal Smallest Positive (<27,9> 3^-9867)",
       quantize_rational(BigRational(BigInt(1), BigInt::pow3(9867)), 27, 9, "exact_only"),
       make_result("+00000000000000000000000000+E-9841", "exact", std::nullopt)},
      {"Max Finite Overflow Threshold (<27,9> M_max*3^9815)",
       quantize_rational(
           BigRational(((BigInt::pow3(27) - BigInt(1)) / BigInt(2)) * BigInt::pow3(9815),
                       BigInt(1)),
           27, 9, "nearest_even"),
       make_result(std::nullopt, "error", "overflow")},
      {"Quiet NaN Input (0x7ff8000000000000)",
       from_binary64("7ff8000000000000", 27, 9, "exact_only"),
       make_result(std::nullopt, "error", "nonfinite_input")},
      {"Signaling NaN Input (0x7ff0000000000001)",
       from_binary64("7ff0000000000001", 27, 9, "exact_only"),
       make_result(std::nullopt, "error", "nonfinite_input")},
      {"Positive Infinity Input (0x7ff0000000000000)",
       from_binary64("7ff0000000000000", 27, 9, "exact_only"),
       make_result(std::nullopt, "error", "nonfinite_input")},
      {"Negative Infinity Input (0xfff0000000000000)",
       from_binary64("fff0000000000000", 27, 9, "exact_only"),
       make_result(std::nullopt, "error", "nonfinite_input")}};

  size_t bound_pass = 0;
  for (const auto& b : boundaries) {
    bool ok = (b.actual == b.expected);
    if (ok) bound_pass++;
    std::cout << "Boundary: " << b.description << "\n"
              << "  Actual  : status=" << b.actual.status
              << ", val=" << (b.actual.value ? *b.actual.value : "null")
              << ", err=" << (b.actual.error ? *b.actual.error : "null") << "\n"
              << "  Expected: status=" << b.expected.status
              << ", val=" << (b.expected.value ? *b.expected.value : "null")
              << ", err=" << (b.expected.error ? *b.expected.error : "null") << "\n"
              << "  Status  : " << (ok ? "PASS" : "FAIL") << "\n\n";
  }

  std::cout << "[+] Boundary Verification: " << bound_pass << " PASSED out of " << boundaries.size()
            << " cases." << std::endl;

  report.close();
  std::cout << "\n[+] Detailed evaluation report saved to "
               "experiments/exact_rational_prototype/detailed_evaluation_report.txt"
            << std::endl;

  return (failed == 0 && bound_pass == boundaries.size()) ? 0 : 1;
}
