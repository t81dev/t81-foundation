#include "t81/cli/driver.hpp"
#include "t81/vm/vm.hpp"

#include <iostream>
#include <string_view>

int main() {
    constexpr std::string_view source = R"(
        enum Color {
            Red;
            Blue(i32);
        };

        fn main() -> i32 {
            match (Color.Blue(42)) {
                Red => 0;
                Blue(v) if v > 10 => v;
            };
            return 0;
        }
    )";

    auto program_opt = t81::cli::build_program_from_source(std::string(source), "<axion-e2e>");
    if (!program_opt) {
        std::cerr << "Failed to compile Axion trace program\n";
        return 1;
    }

    auto& program = *program_opt;
    if (program.match_metadata_text.find("guard-expr") == std::string::npos) {
        std::cerr << "Match metadata missing guard expression: " << program.match_metadata_text << '\n';
        return 1;
    }

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    if (!result) {
        std::cerr << "Guard trace VM trapped: " << static_cast<int>(result.error()) << '\n';
        return 1;
    }

    bool saw_match_metadata = false;
    bool saw_guard_pass = false;
    bool saw_payload_entry = false;
    for (const auto& entry : vm->state().axion_log) {
        std::string_view reason(entry.verdict.reason);
        if (entry.opcode == t81::tisc::Opcode::Nop && reason.find("guard-expr") != std::string_view::npos) {
            saw_match_metadata = true;
        }
        if (entry.opcode == t81::tisc::Opcode::EnumIsVariant && reason.find("variant=Blue") != std::string_view::npos) {
            if (reason.find("match=pass") != std::string_view::npos) {
                saw_guard_pass = true;
            }
        }
        if (entry.opcode == t81::tisc::Opcode::EnumUnwrapPayload &&
            reason.find("payload=i32") != std::string_view::npos) {
            saw_payload_entry = true;
        }
    }

    if (!saw_match_metadata || !saw_guard_pass || !saw_payload_entry) {
        std::cerr << "Axion trace missing expectation: "
                  << "match-metadata=" << saw_match_metadata
                  << " guard-pass=" << saw_guard_pass
                  << " payload-entry=" << saw_payload_entry << '\n';
        return 1;
    }

    std::cout << "E2E Axion trace test passed\n";
    return 0;
}
