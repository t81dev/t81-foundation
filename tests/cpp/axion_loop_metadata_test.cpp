#include "t81/axion/policy.hpp"
#include "t81/cli/driver.hpp"
#include "t81/tisc/binary_io.hpp"
#include "t81/vm/vm.hpp"

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
            @bounded(infinite)
            loop {
                return 0;
            }
        }
    )";

    auto src = make_temp_path("t81-loop", ".t81");
    write_source(src, program);
    auto tisc_path = src;
    tisc_path.replace_extension(".tisc");

    int rc = 0;
    try {
        rc = t81::cli::compile(src, tisc_path);
    } catch (const std::exception& ex) {
        std::cerr << "compile threw: " << ex.what() << std::endl;
        throw;
    }
    if (rc != 0) {
        std::cerr << "Failed to compile loop test source\n";
        return rc;
    }
    assert(fs::exists(tisc_path));
    std::cerr << "after compile" << std::endl;

    auto compiled = t81::tisc::load_program(tisc_path.string());
    std::cerr << "after load_program load" << std::endl;
    std::cerr << "policy text: " << compiled.axion_policy_text << std::endl;
    assert(!compiled.axion_policy_text.empty());
    assert(compiled.axion_policy_text.find("(policy") != std::string::npos);
    assert(compiled.axion_policy_text.find("(loop") != std::string::npos);

    auto parsed = t81::axion::parse_policy(compiled.axion_policy_text);
    if (!parsed.has_value()) {
        std::cerr << "parse_policy error: " << parsed.error() << std::endl;
        return 1;
    }
    if (parsed->loops.empty()) {
        std::cerr << "Policy did not preserve loop metadata\n";
        return 1;
    }
    std::cerr << "after parse" << std::endl;

    auto vm = t81::vm::make_interpreter_vm();
    try {
        vm->load_program(compiled);
    } catch (const std::exception& ex) {
        std::cerr << "load_program exception: " << ex.what() << std::endl;
        throw;
    }
    std::cerr << "after load_program" << std::endl;
    if (!vm->state().policy.has_value()) {
        std::cerr << "VM failed to capture policy\n";
        return 1;
    }
    if (vm->state().policy->loops.empty()) {
        std::cerr << "VM policy missing loop hints\n";
        return 1;
    }
    auto run_rc = [&]() {
        try {
            return vm->run_to_halt();
        } catch (const std::exception& ex) {
            std::cerr << "run_to_halt exception: " << ex.what() << std::endl;
            throw;
        }
    }();
    std::cerr << "after run_to_halt" << std::endl;
    if (!run_rc.has_value()) {
        std::cerr << "VM trapped while running loop program\n";
        return 1;
    }

    const auto& log = vm->state().axion_log;
    std::cerr << "axion log entries: " << log.size() << std::endl;
    for (const auto& entry : log) {
        std::cerr << "  reason: " << entry.verdict.reason << std::endl;
    }
    bool saw_loop_hint = false;
    for (const auto& entry : log) {
        if (entry.verdict.reason.find("loop hint") != std::string::npos) {
            saw_loop_hint = true;
            break;
        }
    }
    if (!saw_loop_hint) {
        std::cerr << "Axion log did not capture loop hint\n";
        return 1;
    }

    fs::remove(src);
    if (fs::exists(tisc_path)) {
        fs::remove(tisc_path);
    }

        std::cout << "Axion loop metadata test passed!" << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Axion loop metadata test threw: " << ex.what() << std::endl;
        return 1;
    }
}
