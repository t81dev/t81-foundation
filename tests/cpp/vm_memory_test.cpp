#include "t81/tensor.hpp"
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
    [[maybe_unused]] auto result = vm->run_to_halt();
    assert(result.has_value());
    return vm;
}

t81::vm::Trap run_expected_trap(const std::vector<t81::tisc::Insn>& insns) {
    t81::tisc::Program program;
    program.insns = insns;
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(program);
    [[maybe_unused]] auto result = vm->run_to_halt();
    assert(!result.has_value());
    return result.error();
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
        [[maybe_unused]] auto trap = run_expected_trap({stack_alloc, stack_alloc2, stack_free0, halt});
        assert(trap == t81::vm::Trap::IllegalInstruction);
    }

    t81::tisc::Insn stack_overflow{};
    stack_overflow.opcode = t81::tisc::Opcode::StackAlloc;
    stack_overflow.a = 2;
    stack_overflow.b = 512;
    {
        t81::tisc::Program program;
        program.insns = {stack_overflow, halt};
        auto vm = t81::vm::make_interpreter_vm();
        vm->load_program(program);
        auto result = vm->run_to_halt();
        assert(!result.has_value());
        assert(result.error() == t81::vm::Trap::BoundsFault);
        const auto& log = vm->state().axion_log;
        bool saw_stack_bounds = false;
        for (const auto& entry : log) {
            if (entry.verdict.reason.find("bounds fault") != std::string::npos &&
                entry.verdict.reason.find("stack frame allocate") != std::string::npos) {
                saw_stack_bounds = true;
                break;
            }
        }
        if (!saw_stack_bounds) {
            return dump_axion_log_and_fail(vm->state(), "stack bounds fault");
        }
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
        [[maybe_unused]] auto trap = run_expected_trap({heap_alloc, heap_alloc, heap_free, halt});
        assert(trap == t81::vm::Trap::IllegalInstruction);
    }

    t81::tisc::Insn heap_big{};
    heap_big.opcode = t81::tisc::Opcode::HeapAlloc;
    heap_big.a = 4;
    heap_big.b = 1024;
    {
        t81::tisc::Program program;
        program.insns = {heap_big, halt};
        auto vm = t81::vm::make_interpreter_vm();
        vm->load_program(program);
        auto result = vm->run_to_halt();
        assert(!result.has_value());
        assert(result.error() == t81::vm::Trap::BoundsFault);
        const auto& log = vm->state().axion_log;
        bool saw_heap_bounds = false;
        for (const auto& entry : log) {
            if (entry.verdict.reason.find("bounds fault") != std::string::npos &&
                entry.verdict.reason.find("heap block allocate") != std::string::npos) {
                saw_heap_bounds = true;
                break;
            }
        }
        if (!saw_heap_bounds) {
            return dump_axion_log_and_fail(vm->state(), "heap bounds fault");
        }
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

    {
        t81::tisc::Insn bad_load{};
        bad_load.opcode = t81::tisc::Opcode::Load;
        bad_load.a = 0;
        bad_load.b = -1;

        t81::tisc::Program program;
        program.insns = {bad_load, halt};
        auto vm = t81::vm::make_interpreter_vm();
        vm->load_program(program);
        auto result = vm->run_to_halt();
        assert(!result.has_value());

        bool saw_bounds = false;
        for (const auto& entry : vm->state().axion_log) {
            if (entry.verdict.reason.find("bounds fault") != std::string::npos &&
                entry.verdict.reason.find("memory load") != std::string::npos) {
                saw_bounds = true;
                break;
            }
        }
        if (!saw_bounds) {
            return dump_axion_log_and_fail(vm->state(), "bounds fault load");
        }
    }

    {
        t81::tisc::Insn bad_store{};
        bad_store.opcode = t81::tisc::Opcode::Store;
        bad_store.a = 0;
        bad_store.b = 0;

        t81::tisc::Program program;
        program.insns = {bad_store, halt};

        auto vm = t81::vm::make_interpreter_vm();
        vm->load_program(program);
        int bad_addr = static_cast<int>(vm->state().layout.total_size());
        program.insns[0].a = bad_addr;
        vm->load_program(program);

        auto result = vm->run_to_halt();
        assert(!result.has_value());

        bool saw_bounds = false;
        for (const auto& entry : vm->state().axion_log) {
            if (entry.verdict.reason.find("bounds fault") != std::string::npos &&
                entry.verdict.reason.find("memory store") != std::string::npos) {
                saw_bounds = true;
                break;
            }
        }
        if (!saw_bounds) {
            return dump_axion_log_and_fail(vm->state(), "bounds fault store");
        }
    }

    {
        t81::tisc::Insn load_tensor_a{};
        load_tensor_a.opcode = t81::tisc::Opcode::LoadImm;
        load_tensor_a.a = 1;
        load_tensor_a.b = 1;
        load_tensor_a.literal_kind = t81::tisc::LiteralKind::TensorHandle;

        t81::tisc::Insn load_tensor_b{};
        load_tensor_b.opcode = t81::tisc::Opcode::LoadImm;
        load_tensor_b.a = 2;
        load_tensor_b.b = 2;
        load_tensor_b.literal_kind = t81::tisc::LiteralKind::TensorHandle;

        t81::tisc::Insn vec_add{};
        vec_add.opcode = t81::tisc::Opcode::TVecAdd;
        vec_add.a = 0;
        vec_add.b = 1;
        vec_add.c = 2;

        t81::tisc::Insn corrupt_handle{};
        corrupt_handle.opcode = t81::tisc::Opcode::LoadImm;
        corrupt_handle.a = 2;
        corrupt_handle.b = 42;
        corrupt_handle.literal_kind = t81::tisc::LiteralKind::Int;

        t81::tisc::Program program;
        t81::T729Tensor dummy_tensor({1}, {0.0f});
        program.tensor_pool.push_back(dummy_tensor);
        program.tensor_pool.push_back(dummy_tensor);
        program.insns = {load_tensor_a, load_tensor_b, corrupt_handle, vec_add, halt};

        auto vm = t81::vm::make_interpreter_vm();
        vm->load_program(program);
        auto result = vm->run_to_halt();
        assert(!result.has_value());

        bool saw_tensor_bounds = false;
        for (const auto& entry : vm->state().axion_log) {
            if (entry.verdict.reason.find("bounds fault") != std::string::npos &&
                entry.verdict.reason.find("tensor handle access") != std::string::npos) {
                saw_tensor_bounds = true;
                break;
            }
        }
        if (!saw_tensor_bounds) {
            return dump_axion_log_and_fail(vm->state(), "tensor bounds fault");
        }
    }

    return 0;
}
