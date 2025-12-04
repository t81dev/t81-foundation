#include "t81/cli/driver.hpp"
#include "t81/tisc/binary_io.hpp"
#include "t81/vm/vm.hpp"

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
    static std::uniform_int_distribution<uint64_t> dist;
    return fs::temp_directory_path() /
           (prefix + "-" + std::to_string(dist(rng)) + extension);
}

void write_source(const fs::path& path, std::string_view contents) {
    std::ofstream out(path, std::ios::binary);
    out << contents;
    out.flush();
}
} // namespace

int main() {
    try {
        const std::string program = R"(
            fn main() -> i32 {
                let maybe: Option[i32] = Some(5);
                return match (maybe) {
                    Some(v) => v;
                    None => 0;
                };
            }
        )";

        auto src = make_temp_path("t81-match", ".t81");
        write_source(src, program);
        auto tisc_path = src;
        tisc_path.replace_extension(".tisc");

        int rc = t81::cli::compile(src, tisc_path);
        if (rc != 0) {
            std::cerr << "Compilation failed with return code " << rc << std::endl;
            return rc;
        }

        auto compiled = t81::tisc::load_program(tisc_path.string());
        assert(!compiled.match_metadata_text.empty());
        assert(compiled.match_metadata_text.find("(payload") != std::string::npos);

        auto vm = t81::vm::make_interpreter_vm();
        vm->load_program(compiled);

        bool saw_match_hint = false;
        for (const auto& entry : vm->state().axion_log) {
            if (entry.verdict.reason.find("match metadata") != std::string::npos) {
                saw_match_hint = true;
                break;
            }
        }
        if (!saw_match_hint) {
            std::cerr << "Axion log missing match metadata hint" << std::endl;
            return 1;
        }

        fs::remove(src);
        if (fs::exists(tisc_path)) {
            fs::remove(tisc_path);
        }

        std::cout << "Axion match metadata test passed!" << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Axion match metadata test threw: " << ex.what() << std::endl;
        return 1;
    }
}
