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
            enum Color {
                Red;
                Blue(i32);
            };

            fn main() -> i32 {
                return match (Color.Blue(9)) {
                    Red => 0;
                    Blue(_) => 9;
                };
            }
        )";

        auto src = make_temp_path("t81-enum-guard", ".t81");
        write_source(src, program);
        auto tisc_path = src;
        tisc_path.replace_extension(".tisc");

        int rc = t81::cli::compile(src, tisc_path);
        if (rc != 0) {
            std::cerr << "Compilation failed with return code " << rc << std::endl;
            return rc;
        }

        auto compiled = t81::tisc::load_program(tisc_path.string());
        auto vm = t81::vm::make_interpreter_vm();
        vm->load_program(compiled);
        if (!vm->run_to_halt()) {
            std::cerr << "Enum guard test VM trapped unexpectedly" << std::endl;
            return 1;
        }

        bool saw_variant_guard = false;
        bool saw_payload_unwrap = false;
        for (const auto& entry : vm->state().axion_log) {
            if (entry.opcode == t81::tisc::Opcode::EnumIsVariant &&
                entry.tag == 1 && entry.value == 1) {
                saw_variant_guard = true;
            }
            if (entry.opcode == t81::tisc::Opcode::EnumUnwrapPayload &&
                entry.tag == 1 && entry.value == 9) {
                saw_payload_unwrap = true;
            }
        }

        if (!saw_variant_guard) {
            std::cerr << "Axion log missing enum variant guard event" << std::endl;
            return 1;
        }
        if (!saw_payload_unwrap) {
            std::cerr << "Axion log missing enum payload unwrap event" << std::endl;
            return 1;
        }

        fs::remove(src);
        if (fs::exists(tisc_path)) {
            fs::remove(tisc_path);
        }

        std::cout << "Axion enum guard test passed!" << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Axion enum guard test threw: " << ex.what() << std::endl;
        return 1;
    }
}
