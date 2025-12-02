#include "t81/cli/driver.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

namespace fs = std::filesystem;

namespace {

fs::path make_temp_path(const std::string& prefix, const std::string& extension) {
    static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist;
    fs::path path = fs::temp_directory_path() /
                    (prefix + "-" + std::to_string(dist(rng)) + extension);
    return path;
}

void write_source(const fs::path& path, std::string_view contents) {
    std::ofstream out(path, std::ios::binary);
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

    auto success_src = make_temp_path("t81-success", ".t81");
    write_source(success_src, minimal_program);
    auto success_tisc = success_src;
    success_tisc.replace_extension(".tisc");

    int compile_rc = t81::cli::compile(success_src, success_tisc);
    assert(compile_rc == 0);
    assert(fs::exists(success_tisc));

    int run_rc = t81::cli::run_tisc(success_tisc);
    assert(run_rc == 0);

    fs::remove(success_src);
    fs::remove(success_tisc);

    const std::string bad_program = R"(
        fn main() -> i32 {
            let bad: i2 = 1.5;
            return 0;
        }
    )";

    auto fail_src = make_temp_path("t81-fail", ".t81");
    write_source(fail_src, bad_program);
    auto fail_tisc = fail_src;
    fail_tisc.replace_extension(".tisc");

    std::ostringstream captured;
    auto* old = std::cerr.rdbuf(captured.rdbuf());
    int bad_rc = t81::cli::compile(fail_src, fail_tisc);
    std::cerr.rdbuf(old);

    assert(bad_rc != 0);
    std::string output = captured.str();
    assert(output.find("Cannot assign initializer") != std::string::npos);

    size_t path_pos = output.find(fail_src.string());
    assert(path_pos != std::string::npos);
    size_t first_colon = output.find(':', path_pos + fail_src.string().size());
    assert(first_colon != std::string::npos);
    size_t second_colon = output.find(':', first_colon + 1);
    assert(second_colon != std::string::npos);
    assert(output.find("error:", second_colon) != std::string::npos);

    fs::remove(fail_src);
    if (fs::exists(fail_tisc)) {
        fs::remove(fail_tisc);
    }
    return 0;
}
