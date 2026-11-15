#include <cstdlib>
#include <iostream>
#include <string>
#include <t81/crypto/sha3.hpp>
#include <vector>

using t81::crypto::sha3_512;
using t81::crypto::sha3_512_hex;

static void run_test(const std::string& input, const std::string& expected_hex,
                     const std::string& label) {
  std::vector<uint8_t> input_bytes(input.begin(), input.end());
  std::string actual_hex = sha3_512_hex(input_bytes);

  if (actual_hex != expected_hex) {
    std::cerr << "Test '" << label << "' failed!" << std::endl;
    std::cerr << "Expected: " << expected_hex << std::endl;
    std::cerr << "Actual:   " << actual_hex << std::endl;
    std::exit(EXIT_FAILURE);
  } else {
    std::cout << "Test '" << label << "' passed." << std::endl;
  }
}

int main() {
  std::cout << "Running SHA3-512 tests..." << std::endl;

  // Test 1: Empty string
  run_test("",
           "a69f73cca23a9ac5c8b567dc185a756e97c982164fe25859e0d1dcc1475c80a615b2123af1f5f94c11e3e94"
           "02c3ac558f500199d95b6d3e301758586281dcd26",
           "Empty string");

  // Test 2: "abc"
  run_test("abc",
           "b751850b1a57168a5693cd924b6b096e08f621827444f70d884f5d0240d2712e10e116e9192af3c91a7ec57"
           "647e3934057340b4cf408d5a56592f8274eec53f0",
           "abc");

  // Test 3: "The quick brown fox jumps over the lazy dog"
  run_test("The quick brown fox jumps over the lazy dog",
           "01dedd5de4ef14642445ba5f5b97c15e47b9ad931326e4b0727cd94cefc44fff23f07bf543139939b49128c"
           "af436dc1bdee54fcb24023a08d9403f9b4bf0d450",
           "The quick brown fox...");

  // Test 4: Long input (> 72 bytes)
  std::string long_input = "The quick brown fox jumps over the lazy dog"
                           "The quick brown fox jumps over the lazy dog"
                           "The quick brown fox jumps over the lazy dog";
  run_test(long_input,
           "3cda341b539bd2e963e0221fc022b9411ee15dbb2fb64a96a1a75d233c1f34566ddf4a8879f490ea6f59b53"
           "8155fa38c803f8e69d9ef4fd1ecd9e52744690aa3",
           "Long input (3x repeat)");

  std::cout << "All SHA3-512 tests passed!" << std::endl;
  return 0;
}
