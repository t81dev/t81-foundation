#include "t81/cli/driver.hpp"
#include "t81/vm/vm.hpp"

#include <iostream>
#include <string>

int main() {
    constexpr std::string_view source = R"(
        enum Color {
            Red;
            Blue(i32);
        };

        fn main() -> i32 {
            return match (Color.Blue(10)) {
                Red => 0;
                Blue(v) if v > 5 => v;
            };
        }
    )";

    auto program_opt = t81::cli::build_program_from_source(std::string(source), "<axion-nested-guard>");
    if (!program_opt) {
        std::cerr << "Failed to compile nested guard program\n";
        return 1;
    }

    auto program = std::move(*program_opt);
    if (program.match_metadata_text.find("(guards true)") == std::string::npos ||
        program.match_metadata_text.find("(guard true)") == std::string::npos) {
        std::cerr << "Match metadata missing guard annotations: " << program.match_metadata_text << std::endl;
        return 1;
    }
    if (program.match_metadata_text.find("guard-expr \"v > 5\"") == std::string::npos) {
        std::cerr << "Match metadata missing guard expression snippet: " << program.match_metadata_text << std::endl;
        return 1;
    }

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    if (!result) {
        std::cerr << "Execution trapped unexpectedly\n";
        return 1;
    }

    bool saw_guard_event = false;
    bool saw_guard_pass = false;
    bool saw_payload_log = false;
    for (const auto& entry : vm->state().axion_log) {
        if (entry.opcode == t81::tisc::Opcode::EnumIsVariant &&
            entry.verdict.reason.find("variant=Blue") != std::string::npos) {
            saw_guard_event = true;
            if (entry.verdict.reason.find("match=pass") != std::string::npos) {
                saw_guard_pass = true;
            }
        }
        if (entry.opcode == t81::tisc::Opcode::EnumUnwrapPayload &&
            entry.verdict.reason.find("payload") != std::string::npos) {
            saw_payload_log = true;
        }
    }

    if (!saw_guard_event || !saw_guard_pass) {
        std::cerr << "Axion log missing Blue guard pass event\n";
        return 1;
    }
    if (!saw_payload_log) {
        std::cerr << "Axion log missing payload unwrap event for guard arm\n";
        return 1;
    }

    std::cout << "Axion nested guard test passed!\n";
    return 0;
}
