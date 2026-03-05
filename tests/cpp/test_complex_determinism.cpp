#include <cassert>
#include <fstream>
#include <iostream>
#include "t81/isa/binary_io.hpp"
#include "t81/types/T81Complex.hpp"

using namespace t81;
using namespace t81::tisc;

int main() {
  std::cout << "Testing T81Complex binary serialization determinism..." << std::endl;

  // Create a test program with various complex numbers
  Program program;

  // Add diverse complex numbers to test determinism
  program.complex_pool.push_back(T81Complex<18>::zero());
  program.complex_pool.push_back(T81Complex<18>::one());
  program.complex_pool.push_back(T81Complex<18>::i());

  // Add complex numbers with different values
  program.complex_pool.push_back(T81Complex<18>(T81Complex<18>::FloatType::from_double(1.5),
                                                T81Complex<18>::FloatType::from_double(-2.5)));

  program.complex_pool.push_back(T81Complex<18>(T81Complex<18>::FloatType::from_double(-3.14159),
                                                T81Complex<18>::FloatType::from_double(2.71828)));

  const std::string test_file = "/tmp/complex_determinism_test.bin";

  // Test 1: Save and load multiple times to ensure determinism
  for (int run = 0; run < 3; ++run) {
    // Save the program
    save_program(program, test_file);

    // Load the program
    Program loaded = load_program(test_file);

    // Verify size matches
    assert(program.complex_pool.size() == loaded.complex_pool.size());

    // Verify all complex numbers match exactly
    for (size_t i = 0; i < program.complex_pool.size(); ++i) {
      const auto& orig = program.complex_pool[i];
      const auto& loaded_complex = loaded.complex_pool[i];

      double orig_real = orig.re.to_double();
      double orig_imag = orig.im.to_double();
      double loaded_real = loaded_complex.re.to_double();
      double loaded_imag = loaded_complex.im.to_double();

      assert(orig_real == loaded_real);
      assert(orig_imag == loaded_imag);

      std::cout << "Run " << run + 1 << ", Complex " << i << ": (" << orig_real << ", " << orig_imag
                << "i) ✓" << std::endl;
    }
  }

  // Test 2: Test round-trip consistency
  Program round_trip = load_program(test_file);
  save_program(round_trip, "/tmp/complex_round_trip.bin");
  Program final_round_trip = load_program("/tmp/complex_round_trip.bin");

  assert(program.complex_pool.size() == final_round_trip.complex_pool.size());
  for (size_t i = 0; i < program.complex_pool.size(); ++i) {
    const auto& orig = program.complex_pool[i];
    const auto& final = final_round_trip.complex_pool[i];

    assert(orig.re.to_double() == final.re.to_double());
    assert(orig.im.to_double() == final.im.to_double());
  }

  std::cout << "✅ T81Complex binary serialization determinism test PASSED!" << std::endl;
  std::cout << "✅ Round-trip consistency verified!" << std::endl;
  std::cout << "✅ Multiple runs produce identical results!" << std::endl;

  return 0;
}
