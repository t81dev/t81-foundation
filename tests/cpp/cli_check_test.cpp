#include "t81/cli/driver.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {
fs::path make_temp_path(const std::string& prefix, const std::string& extension) {
    static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist;
    return fs::temp_directory_path() /
           (prefix + "-" + std::to_string(dist(rng)) + extension);
}

void write_source(const fs::path& path, std::string_view contents) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to open source file: " + path.string());
    }
    out << contents;
    out.flush();
}
} // namespace

int main() {
    const std::string minimal_program = R"(
        fn main() -> i32 {
            return 0;
        }
    )";

    auto source_path = make_temp_path("t81-check", ".t81");
    write_source(source_path, minimal_program);
    assert(t81::cli::check_syntax(source_path) == 0);
    fs::remove(source_path);

    const std::string broken_program = R"(
        fn main() -> i32 {
            let bad: i8 = 1.5;
            return 0;
        }
    )";

    auto broken_path = make_temp_path("t81-check-fail", ".t81");
    write_source(broken_path, broken_program);

    std::ostringstream captured;
    auto* old_buf = std::cerr.rdbuf(captured.rdbuf());
    int rc = t81::cli::check_syntax(broken_path);
    std::cerr.rdbuf(old_buf);

    if (rc == 0) {
        std::cerr << "Expected `t81 check` to fail on invalid input\n";
        return 1;
    }
    std::string output = captured.str();
    assert(output.find(broken_path.string()) != std::string::npos);
    assert(output.find("Cannot assign initializer") != std::string::npos);

    fs::remove(broken_path);

    std::cout << "CliCheckTest passed!" << std::endl;
    return 0;
}
