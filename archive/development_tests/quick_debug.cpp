#include "t81/canonfs/interchange_ops.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace t81::canonfs;

int main() {
    namespace fs = std::filesystem;
    
    // Create test directory
    std::filesystem::create_directories("/tmp/quick_debug");
    
    ImportOptions options;
    options.canonfs_root = "/tmp/quick_debug/.t81_canonfs";
    options.policy_profile = InterchangePolicyProfile::Permissive;
    options.policy_evaluator = [](std::string_view, std::string_view, std::string_view) -> InterchangePolicyDecision {
      return InterchangePolicyDecision{true, "allow"};
    };
    
    // Test 1: Nonexistent file
    std::cout << "=== Test 1: Nonexistent file ===" << std::endl;
    auto outcome1 = import_path("/tmp/quick_debug/nonexistent.txt", options);
    std::cout << "Result: " << outcome1.ok() << std::endl;
    std::cout << "Errors: " << outcome1.errors.size() << std::endl;
    for (const auto& error : outcome1.errors) {
      std::cout << "  Reason: '" << error.reason << "', Message: '" << error.message << "'" << std::endl;
    }
    
    // Test 2: Empty directory
    std::cout << "\n=== Test 2: Empty directory ===" << std::endl;
    std::filesystem::create_directories("/tmp/quick_debug/empty");
    auto outcome2 = import_path("/tmp/quick_debug/empty", options);
    std::cout << "Result: " << outcome2.ok() << std::endl;
    std::cout << "Errors: " << outcome2.errors.size() << std::endl;
    for (const auto& error : outcome2.errors) {
      std::cout << "  Reason: '" << error.reason << "', Message: '" << error.message << "'" << std::endl;
    }
    
    // Test 3: Special file (neither regular nor directory)
    std::cout << "\n=== Test 3: Special file ===" << std::endl;
    // This will be handled by the filesystem as non-existent
    auto outcome3 = import_path("/tmp/quick_debug/special", options);
    std::cout << "Result: " << outcome3.ok() << std::endl;
    std::cout << "Errors: " << outcome3.errors.size() << std::endl;
    for (const auto& error : outcome3.errors) {
      std::cout << "  Reason: '" << error.reason << "', Message: '" << error.message << "'" << std::endl;
    }
    
    return 0;
}
