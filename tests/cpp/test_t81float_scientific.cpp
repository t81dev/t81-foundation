#include "t81/frontend/ir_generator.hpp"  // For parse_base81_float_literal
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
// Use relative include for the header found in tests/cpp/
#include <cmath>
#include <iostream>
#include "test_runtime_check.hpp"

using namespace t81::frontend;

void test_parse_scientific_notation() {
  T81_TEST_CHECK(std::abs(parse_base81_float_literal("1.0e3") - 1000.0) < 1e-9);
  T81_TEST_CHECK(std::abs(parse_base81_float_literal("1.0e-3") - 0.001) < 1e-9);
  T81_TEST_CHECK(std::abs(parse_base81_float_literal("1.23e2") - 123.0) < 1e-9);
  T81_TEST_CHECK(std::abs(parse_base81_float_literal("5.0E1") - 50.0) < 1e-9);
  T81_TEST_CHECK(std::abs(parse_base81_float_literal("1e3") - 1000.0) < 1e-9);
}

void test_parse_with_suffix() {
  T81_TEST_CHECK(std::abs(parse_base81_float_literal("1.0e3t81") - 1000.0) < 1e-9);
  T81_TEST_CHECK(std::abs(parse_base81_float_literal("1.0e-3t81") - 0.001) < 1e-9);
}

void test_canonical_representation() {
  double v1 = parse_base81_float_literal("1000.0t81");
  double v2 = parse_base81_float_literal("1.0e3t81");
  T81_TEST_CHECK(std::abs(v1 - v2) < 1e-9);
}

int main() {
  try {
    test_parse_scientific_notation();
    test_parse_with_suffix();
    test_canonical_representation();
    std::cout << "All scientific float tests passed!\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Test failed with exception: " << e.what() << "\n";
    return 1;
  }
}
