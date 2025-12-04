#include "t81/cli/driver.hpp"
#include "t81/vm/vm.hpp"

namespace {
std::string trap_to_string(t81::vm::Trap trap) {
    using Trap = t81::vm::Trap;
    switch (trap) {
        case Trap::None: return "None";
        case Trap::InvalidMemory: return "InvalidMemory";
        case Trap::IllegalInstruction: return "IllegalInstruction";
        case Trap::DivideByZero: return "DivideByZero";
        case Trap::BoundsFault: return "BoundsFault";
        case Trap::SecurityFault: return "SecurityFault";
        case Trap::TrapInstruction: return "TrapInstruction";
    }
    return "UnknownTrap";
}
} // namespace

#include <iostream>

int main() {
    constexpr std::string_view source = R"(
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

    auto program = t81::cli::build_program_from_source(std::string(source), "<axion-guard-trace>");
    if (!program) {
        std::cerr << "Failed to compile guard trace program\n";
        return 1;
    }

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(*program);
    auto result = vm->run_to_halt();
    if (!result) {
        std::cerr << "Execution trapped: " << trap_to_string(result.error()) << '\n';
        return 1;
    }

    std::cout << "Enum metadata:\n";
    for (const auto& entry : program->enum_metadata) {
        std::cout << "  enum " << entry.name << " (id " << entry.enum_id << ")\n";
        for (const auto& variant : entry.variants) {
            std::cout << "    variant " << variant.name << " (id " << variant.variant_id << ")";
            if (variant.payload) {
                std::cout << " payload=" << *variant.payload;
            }
            std::cout << '\n';
        }
    }

    std::cout << "Axion log entries:\n";
    for (const auto& entry : vm->state().axion_log) {
        std::cout << "  opcode=" << static_cast<int>(entry.opcode)
                  << " tag=" << entry.tag
                  << " value=" << entry.value
                  << " reason=\"" << entry.verdict.reason << "\"\n";
    }

    return 0;
}
