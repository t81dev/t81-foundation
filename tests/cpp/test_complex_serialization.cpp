#include <cassert>
#include <fstream>
#include <iostream>
#include "t81/isa/binary_io.hpp"
#include "t81/types/T81Complex.hpp"

using namespace t81;
using namespace t81::tisc;

int main() {
  std::cout << "Testing T81Complex binary serialization..." << std::endl;

  // Create a test program with complex numbers
  Program original_program;

  // Add some complex numbers to the complex pool
  original_program.complex_pool.push_back(T81Complex<18>::zero());
  original_program.complex_pool.push_back(T81Complex<18>::one());
  original_program.complex_pool.push_back(T81Complex<18>::i());

  // Create a complex number with real and imaginary parts
  T81Complex<18> c1(T81Complex<18>::FloatType::from_double(3.0),
                    T81Complex<18>::FloatType::from_double(4.0));
  original_program.complex_pool.push_back(c1);

  std::cout << "Original program has " << original_program.complex_pool.size() << " complex numbers"
            << std::endl;

  // Save the program to a file
  const std::string test_file = "/tmp/test_complex_serialization.bin";
  save_program(original_program, test_file);

  // Load the program back
  Program loaded_program = load_program(test_file);

  std::cout << "Loaded program has " << loaded_program.complex_pool.size() << " complex numbers"
            << std::endl;

  // Verify the complex numbers are the same
  assert(original_program.complex_pool.size() == loaded_program.complex_pool.size());

  for (size_t i = 0; i < original_program.complex_pool.size(); ++i) {
    const auto& orig = original_program.complex_pool[i];
    const auto& loaded = loaded_program.complex_pool[i];

    // Compare real and imaginary parts
    assert(orig.re.to_double() == loaded.re.to_double());
    assert(orig.im.to_double() == loaded.im.to_double());

    std::cout << "Complex " << i << ": (" << orig.re.to_double() << ", " << orig.im.to_double()
              << "i) ✓" << std::endl;
  }

  std::cout << "✅ T81Complex binary serialization test PASSED!" << std::endl;
  return 0;
}
