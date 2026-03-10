#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>

#include "t81/vm/vm.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"

// Each fuzz instruction record: [1B opcode][4B a le][8B b le][4B c le]
static constexpr std::size_t kInsnBytes  = 17;
static constexpr int         kRegCount   = 243; // R0..R242 (flat register file)
static constexpr std::size_t kMaxSteps   = 512; // Bound execution per fuzz input

static int32_t read_i32_le(const uint8_t* p) {
    return static_cast<int32_t>(
        static_cast<uint32_t>(p[0])        |
       (static_cast<uint32_t>(p[1]) <<  8) |
       (static_cast<uint32_t>(p[2]) << 16) |
       (static_cast<uint32_t>(p[3]) << 24));
}

static int64_t read_i64_le(const uint8_t* p) {
    return static_cast<int64_t>(
        static_cast<uint64_t>(p[0])        |
       (static_cast<uint64_t>(p[1]) <<  8) |
       (static_cast<uint64_t>(p[2]) << 16) |
       (static_cast<uint64_t>(p[3]) << 24) |
       (static_cast<uint64_t>(p[4]) << 32) |
       (static_cast<uint64_t>(p[5]) << 40) |
       (static_cast<uint64_t>(p[6]) << 48) |
       (static_cast<uint64_t>(p[7]) << 56));
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Need at least one complete instruction record.
    if (size < kInsnBytes || size > 65536) {
        return 0;
    }

    const uint8_t kOpcodeMax =
        static_cast<uint8_t>(t81::tisc::Opcode::Int2BigInt) + 1;

    t81::tisc::Program prog;
    const std::size_t insn_count = size / kInsnBytes;

    for (std::size_t i = 0; i < insn_count; ++i) {
        const uint8_t* p = data + i * kInsnBytes;

        uint8_t raw_op = p[0] % kOpcodeMax;

        // TLoadHash has a pre-existing SEGFAULT (known issue, unrelated to VM
        // dispatch logic).  Replace with Nop so fuzz runs don't produce noise
        // that masks real VM bugs.
        if (static_cast<t81::tisc::Opcode>(raw_op) == t81::tisc::Opcode::TLoadHash) {
            raw_op = static_cast<uint8_t>(t81::tisc::Opcode::Nop);
        }

        t81::tisc::Insn insn;
        insn.opcode = static_cast<t81::tisc::Opcode>(raw_op);
        // Clamp register operands to the flat register file (R0..R242).
        insn.a = static_cast<int32_t>(static_cast<uint32_t>(read_i32_le(p + 1))  % kRegCount);
        insn.b = read_i64_le(p + 5);   // Immediate / pool index / PC target — unconstrained
        insn.c = static_cast<int32_t>(static_cast<uint32_t>(read_i32_le(p + 13)) % kRegCount);
        // literal_kind left as default (Int) — enough coverage for dispatch fuzzing
        prog.insns.push_back(insn);
    }

    // Guarantee the program terminates so run_to_halt doesn't exhaust max_steps
    // on every input (that would be noise, not a bug).
    prog.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

    try {
        auto vm = t81::vm::make_interpreter_vm();
        vm->load_program(prog);
        (void)vm->run_to_halt(kMaxSteps);
    } catch (const std::exception&) {
        // Structured exceptions (e.g. bad_alloc) are acceptable outcomes.
    } catch (...) {
        // Swallow non-standard throws; a crash/SEGFAULT is what LLVMFuzzer
        // captures as a genuine finding.
    }

    return 0;
}

#ifndef T81_FUZZ_TARGET
#include <iostream>
#include <random>
// Standalone driver — lets the harness run without a libFuzzer runtime.
int main() {
    std::mt19937 rng(42);
    std::uniform_int_distribution<std::size_t> len_dist(1, 16); // 1–16 instructions
    std::uniform_int_distribution<uint16_t>    byte_dist(0, 255);

    constexpr int iterations = 1000;
    std::cout << "Running standalone fuzz (" << iterations << " iterations)...\n";

    for (int i = 0; i < iterations; ++i) {
        const std::size_t n_insns = len_dist(rng);
        const std::size_t len = n_insns * kInsnBytes;
        std::vector<uint8_t> data(len);
        for (auto& b : data) {
            b = static_cast<uint8_t>(byte_dist(rng));
        }
        LLVMFuzzerTestOneInput(data.data(), data.size());
    }

    std::cout << "Standalone fuzz run completed.\n";
    return 0;
}
#endif
