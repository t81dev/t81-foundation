#include "t81/cli/driver.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

int main() {
    // Create temporary files for the test
    fs::path temp_dir = fs::temp_directory_path();
    fs::path t81_file = temp_dir / "test.t81";
    fs::path tisc_file = temp_dir / "test.tisc";

    // Write a simple T81Lang program to the source file
    {
        std::ofstream out(t81_file);
        out << R"(
            fn main() -> i32 {
                return 42;
            }
        )";
    }

    // Compile the source file
    int compile_result = t81::cli::compile(t81_file, tisc_file);
    assert(compile_result == 0 && "CLI compile command failed");

    // Run the compiled TISC binary
    int run_result = t81::cli::run_tisc(tisc_file);
    assert(run_result == 0 && "CLI run command failed");

    // Clean up temporary files
    fs::remove(t81_file);
    fs::remove(tisc_file);

    std::cout << "CLI e2e test passed!" << std::endl;
    return 0;
}
