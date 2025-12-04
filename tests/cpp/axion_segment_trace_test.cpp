#include "t81/tisc/program.hpp"
#include "t81/tisc/opcodes.hpp"
#include "t81/vm/vm.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main() {
    t81::tisc::Program program;
    program.tensor_pool.emplace_back(std::vector<int>{1}, std::vector<float>{1.0f});
    program.tensor_pool.emplace_back(std::vector<int>{1}, std::vector<float>{2.0f});

    t81::tisc::Insn load_tensor0{};
    load_tensor0.opcode = t81::tisc::Opcode::LoadImm;
    load_tensor0.a = 1;
    load_tensor0.b = 1;  // handles are 1-indexed
    load_tensor0.literal_kind = t81::tisc::LiteralKind::TensorHandle;

    t81::tisc::Insn load_tensor1{};
    load_tensor1.opcode = t81::tisc::Opcode::LoadImm;
    load_tensor1.a = 2;
    load_tensor1.b = 2;
    load_tensor1.literal_kind = t81::tisc::LiteralKind::TensorHandle;

    t81::tisc::Insn vec_add{};
    vec_add.opcode = t81::tisc::Opcode::TVecAdd;
    vec_add.a = 3;
    vec_add.b = 1;
    vec_add.c = 2;

    t81::tisc::Insn load_addr{};
    load_addr.opcode = t81::tisc::Opcode::LoadImm;
    load_addr.a = 4;
    load_addr.b = 128;

    t81::tisc::Insn load_value{};
    load_value.opcode = t81::tisc::Opcode::LoadImm;
    load_value.a = 5;
    load_value.b = 7;

    t81::tisc::Insn ax_set{};
    ax_set.opcode = t81::tisc::Opcode::AxSet;
    ax_set.a = 4;
    ax_set.b = 5;

    t81::tisc::Insn ax_read{};
    ax_read.opcode = t81::tisc::Opcode::AxRead;
    ax_read.a = 6;
    ax_read.b = 200;

    t81::tisc::Insn halt{};
    halt.opcode = t81::tisc::Opcode::Halt;

    program.insns = {load_tensor0, load_tensor1, vec_add, load_addr, load_value, ax_set, ax_read, halt};

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    assert(result.has_value());

    bool saw_tensor = false;
    bool saw_axread = false;
    bool saw_axset = false;
    bool saw_meta = false;
    for (const auto& entry : vm->state().axion_log) {
        if (entry.verdict.reason.find("tensor slot allocated") != std::string::npos) {
            saw_tensor = true;
        }
        if (entry.opcode == t81::tisc::Opcode::AxRead &&
            entry.verdict.reason.find("AxRead guard") != std::string::npos) {
            saw_axread = true;
        }
        if (entry.opcode == t81::tisc::Opcode::AxSet &&
            entry.verdict.reason.find("AxSet guard") != std::string::npos) {
            saw_axset = true;
        }
        if (entry.verdict.reason.find("meta slot") != std::string::npos) {
            saw_meta = true;
        }
    }
    assert(saw_tensor);
    assert(saw_axread);
    assert(saw_axset);
    assert(saw_meta);

    std::cout << "Axion segment trace snippet:\n";
    for (const auto& entry : vm->state().axion_log) {
        std::cout << "  opcode=" << static_cast<int>(entry.opcode)
                  << " reason=\"" << entry.verdict.reason << "\"\n";
    }

    return 0;
}
