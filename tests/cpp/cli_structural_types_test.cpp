#include "t81/cli/driver.hpp"
#include "t81/tisc/binary_io.hpp"

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
    constexpr std::string_view source = R"(
        @schema(2) @module(Core.Points)
        record Point {
            x: i32;
            y: i32;
        }

        @schema(3)
        enum Flag {
            On;
            Off;
        }

        fn main() -> i32 {
            let p: Point = Point { x: 3; y: 4; };
            let _ = p.x;
            return 0;
        }
    )";

    auto src = make_temp_path("t81-structural", ".t81");
    auto tisc_path = src;
    tisc_path.replace_extension(".tisc");

    write_source(src, source);

    assert(t81::cli::compile(src, tisc_path) == 0);
    assert(fs::exists(tisc_path));
    assert(t81::cli::run_tisc(tisc_path) == 0);
    auto program = t81::tisc::load_program(tisc_path);
    bool saw_point = false;
    bool saw_flag = false;
    std::string point_module = "Core.Points";
    std::string default_module = src.string();
    for (const auto& alias : program.type_aliases) {
        if (alias.kind == t81::tisc::StructuralKind::Record && alias.name == "Point") {
            if (alias.fields.size() == 2 &&
                alias.fields[0].name == "x" &&
                alias.fields[1].name == "y") {
                assert(alias.schema_version == 2);
                assert(alias.module_path == point_module);
                saw_point = true;
            }
        }
        if (alias.kind == t81::tisc::StructuralKind::Enum && alias.name == "Flag") {
            if (alias.variants.size() == 2) {
                assert(alias.schema_version == 3);
                assert(alias.module_path == default_module);
                saw_flag = true;
            }
        }
    }
    if (!saw_point || !saw_flag) {
        throw std::runtime_error("Structural metadata missing in TISC program");
    }

    fs::remove(src);
    fs::remove(tisc_path);

    std::cout << "CliStructuralTypesTest passed!" << std::endl;
    return 0;
}
