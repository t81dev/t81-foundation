#include "t81/cli/driver.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
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
    constexpr std::string_view program = R"(
        fn option_match(value: Option[i32]) -> i32 {
            return match (value) {
                Some(v) => v * 2;
                None => -1;
            };
        }

        fn result_match(res: Result[i32, T81String]) -> i32 {
            let fallback: Option[i32] = None;
            return match (res) {
                Ok(v) => v + 1;
                Err(_) => match (fallback) {
                    Some(x) => x + 5;
                    None => -2;
                }
            };
        }

        fn main() -> i32 {
            let first: Option[i32] = Some(3);
            let second: Option[i32] = None;
            let ok_result: Result[i32, T81String] = Ok(10);
            let err_result: Result[i32, T81String] = Err("boom");
            let first_total = option_match(first);
            let second_total = option_match(second);
            let ok_total = result_match(ok_result);
            let err_total = result_match(err_result);
            return first_total + second_total + ok_total + err_total;
        }
    )";

    auto src = make_temp_path("t81-option-result", ".t81");
    auto tisc_path = src;
    tisc_path.replace_extension(".tisc");

    write_source(src, program);

    assert(t81::cli::compile(src, tisc_path) == 0);
    assert(fs::exists(tisc_path));
    assert(t81::cli::run_tisc(tisc_path) == 0);

    fs::remove(src);
    fs::remove(tisc_path);

    std::cout << "CliOptionResultTest passed!" << std::endl;
    return 0;
}
