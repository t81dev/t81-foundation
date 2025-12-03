#include "t81/cli/driver.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
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
        record Point {
            x: i32;
            y: i32;
        }

        enum MaybePoint {
            Some(Point);
            None;
        }

        fn sum_point(point: Point, payload: MaybePoint) -> i32 {
            return match (payload) {
                Some(inner) => inner.x + inner.y;
                None => point.x - point.y;
            };
        }

        fn main() -> i32 {
            let base: Point = Point { x: 7; y: 5; };
            let payload: MaybePoint = Some(Point { x: 3; y: 2; });
            return sum_point(base, payload);
        }
    )";

    auto src = make_temp_path("t81-record-enum", ".t81");
    auto tisc_path = src;
    tisc_path.replace_extension(".tisc");

    write_source(src, program);

    assert(t81::cli::compile(src, tisc_path) == 0);
    assert(fs::exists(tisc_path));
    assert(t81::cli::run_tisc(tisc_path) == 0);

    fs::remove(src);
    fs::remove(tisc_path);

    std::cout << "CliRecordEnumTest passed!" << std::endl;
    return 0;
}
