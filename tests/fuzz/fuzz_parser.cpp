#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 65536) {
        // Skip empty or excessively large inputs to bound memory
        return 0;
    }

    // Convert fuzz bytes to a string
    std::string source(reinterpret_cast<const char*>(data), size);

    // Parse the input (we expect it to fail gracefully without crashing)
    try {
        t81::frontend::Lexer lexer(source);
        
        t81::frontend::Parser parser(lexer);
        auto ast = parser.parse();
        
        // We do not run semantic analysis since AST shapes can be invalid but structurally sound
    } catch (const std::exception& e) {
        // Standard exceptions are acceptable (e.g., Lexer bounds assertions)
        // Crash/SEGFAULT implies a fuzzing failure which LLVMFuzzer captures automatically.
    } catch (...) {
        // Catch all to prevent aborts from non-standard exceptions bubbling up
    }

    return 0;
}

#ifndef T81_FUZZ_TARGET
#include <iostream>
#include <random>
// Standalone main for testing without libFuzzer
int main() {
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> len_dist(2, 64);
    std::uniform_int_distribution<uint16_t> byte_dist(32, 126); // Printable ASCII

    int iterations = 1000;
    std::cout << "Running standalone fuzz (" << iterations << " iterations)...\n";

    for (int i = 0; i < iterations; ++i) {
        size_t len = len_dist(rng);
        std::vector<uint8_t> data(len);
        for (size_t j = 0; j < len; ++j) {
            data[j] = static_cast<uint8_t>(byte_dist(rng));
        }
        LLVMFuzzerTestOneInput(data.data(), data.size());
    }
    std::cout << "Standalone fuzz run completed.\n";
    return 0;
}
#endif
