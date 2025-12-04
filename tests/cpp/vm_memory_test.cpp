#include "t81/tisc/program.hpp"
#include "t81/tisc/opcodes.hpp"
#include "t81/vm/vm.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::unique_ptr<t81::vm::IVirtualMachine> run_program(const std::vector<t81::tisc::Insn>& insns) {
    t81::tisc::Program program;
    program.insns = insns;
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    assert(result.has_value());
    return vm;
}

t81::vm::Trap run_expected_trap(const std::vector<t81::tisc::Insn>& insns) {
    t81::tisc::Program program;
    program.insns = insns;
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    auto result = vm->run_to_halt();
    assert(!result.has_value());
    return result.error();
}

int dump_axion_log_and_fail(const t81::vm::State& state, const char* label) {
    std::cerr << "[vm_memory_test] " << label << " axion log (size=" << state.axion_log.size() << ")\n";
    for (const auto& entry : state.axion_log) {
        std::cerr << "  opcode=" << static_cast<int>(entry.opcode)
                  << " tag=" << entry.tag
                  << " value=" << entry.value
                  << " reason=\"" << entry.verdict.reason << "\"\n";
    }
    return 1;
}

}  // namespace

int main() {
    t81::tisc::Insn stack_alloc{};
    stack_alloc.opcode = t81::tisc::Opcode::StackAlloc;
    stack_alloc.a = 0;
    stack_alloc.b = 16;

    t81::tisc::Insn stack_alloc2{};
    stack_alloc2.opcode = t81::tisc::Opcode::StackAlloc;
    stack_alloc2.a = 1;
    stack_alloc2.b = 32;

    t81::tisc::Insn stack_free{};
    stack_free.opcode = t81::tisc::Opcode::StackFree;
    stack_free.a = 1;
    stack_free.b = 32;

    t81::tisc::Insn stack_free0{};
    stack_free0.opcode = t81::tisc::Opcode::StackFree;
    stack_free0.a = 0;
    stack_free0.b = 16;

    t81::tisc::Insn halt{};
    halt.opcode = t81::tisc::Opcode::Halt;

    {
        auto vm = run_program({stack_alloc, stack_free0, halt});
        assert(vm->state().stack_frames.empty());
        assert(vm->state().sp == vm->state().layout.stack.limit);
        assert(vm->state().registers[0] >= static_cast<std::int64_t>(vm->state().layout.code.limit));
        const auto& log = vm->state().axion_log;
        bool saw_alloc = false;
        bool saw_free = false;
        for (const auto& entry : log) {
            if (entry.opcode == t81::tisc::Opcode::StackAlloc &&
                entry.verdict.reason.find("stack frame allocated") != std::string::npos) {
                saw_alloc = true;
            }
            if (entry.opcode == t81::tisc::Opcode::StackFree &&
                entry.verdict.reason.find("stack frame freed") != std::string::npos) {
                saw_free = true;
            }
        }
        if (!saw_alloc || !saw_free) {
            return dump_axion_log_and_fail(vm->state(), "stack frame");
        }
    }

    {
        auto vm = run_program({stack_alloc, stack_alloc2, stack_free, stack_free0, halt});
        assert(vm->state().stack_frames.empty());
        assert(vm->state().sp == vm->state().layout.stack.limit);
    }

    {
        auto trap = run_expected_trap({stack_alloc, stack_alloc2, stack_free0, halt});
        assert(trap == t81::vm::Trap::IllegalInstruction);
    }

    t81::tisc::Insn stack_overflow{};
    stack_overflow.opcode = t81::tisc::Opcode::StackAlloc;
    stack_overflow.a = 2;
    stack_overflow.b = 512;
    {
        auto trap = run_expected_trap({stack_overflow, halt});
        assert(trap == t81::vm::Trap::BoundsFault);
    }

    t81::tisc::Insn heap_alloc{};
    heap_alloc.opcode = t81::tisc::Opcode::HeapAlloc;
    heap_alloc.a = 3;
    heap_alloc.b = 32;

    t81::tisc::Insn heap_free{};
    heap_free.opcode = t81::tisc::Opcode::HeapFree;
    heap_free.a = 3;
    heap_free.b = 32;

    {
        auto vm = run_program({heap_alloc, heap_free, halt});
        assert(vm->state().heap_frames.empty());
        assert(vm->state().heap_ptr == vm->state().layout.heap.start);
        const auto& log = vm->state().axion_log;
        bool saw_alloc = false;
        bool saw_free = false;
        for (const auto& entry : log) {
            if (entry.opcode == t81::tisc::Opcode::HeapAlloc &&
                entry.verdict.reason.find("heap block allocated") != std::string::npos) {
                saw_alloc = true;
            }
            if (entry.opcode == t81::tisc::Opcode::HeapFree &&
                entry.verdict.reason.find("heap block freed") != std::string::npos) {
                saw_free = true;
            }
        }
        if (!saw_alloc || !saw_free) {
            return dump_axion_log_and_fail(vm->state(), "heap block");
        }
    }

    {
        auto trap = run_expected_trap({heap_alloc, heap_alloc, heap_free, halt});
        assert(trap == t81::vm::Trap::IllegalInstruction);
    }

    t81::tisc::Insn heap_big{};
    heap_big.opcode = t81::tisc::Opcode::HeapAlloc;
    heap_big.a = 4;
    heap_big.b = 1024;
    {
        auto trap = run_expected_trap({heap_big, halt});
        assert(trap == t81::vm::Trap::BoundsFault);
    }

    {
        constexpr int kDefaultStackSize = 256;
        constexpr int kDefaultHeapSize = 768;
        constexpr int kDefaultTensorSpace = 256;
        constexpr int instructions = 4;
        constexpr int stack_start = instructions;
        constexpr int heap_start = stack_start + kDefaultStackSize;
        constexpr int tensor_start = heap_start + kDefaultHeapSize;
        constexpr int meta_start = tensor_start + kDefaultTensorSpace;

        t81::tisc::Insn load_val{};
        load_val.opcode = t81::tisc::Opcode::LoadImm;
        load_val.a = 1;
        load_val.b = 123;

        t81::tisc::Insn store_meta{};
        store_meta.opcode = t81::tisc::Opcode::Store;
        store_meta.a = meta_start;
        store_meta.b = 1;

        t81::tisc::Insn load_meta{};
        load_meta.opcode = t81::tisc::Opcode::Load;
        load_meta.a = 2;
        load_meta.b = meta_start;

        auto vm = run_program({load_val, store_meta, load_meta, halt});
        bool saw_store = false;
        bool saw_load = false;
        for (const auto& entry : vm->state().axion_log) {
            if (entry.opcode == t81::tisc::Opcode::Store &&
                entry.verdict.reason.find("meta") != std::string::npos) {
                saw_store = true;
            }
            if (entry.opcode == t81::tisc::Opcode::Load &&
                entry.verdict.reason.find("meta") != std::string::npos) {
                saw_load = true;
            }
        }
        if (!saw_store || !saw_load)
            return dump_axion_log_and_fail(vm->state(), "meta segment access");
        assert(saw_store && saw_load);
    }

    return 0;
}
