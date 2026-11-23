#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>

#include "t81/isa/encoding.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/vm/vm.hpp"
#include "test_runtime_check.hpp"

// Differential evidence for disputed frozen TISC semantics.
//
// This file does not choose a normative winner and does not change the VM.
// It executes the hosted interpreter on fixtures where two readings disagree,
// and it checks both readings explicitly:
//
//   * spec/tisc-spec.md is the frozen ISA named by spec/README.md and
//     docs/governance/SPEC_AUTHORITY_MODEL.md. §5.6 is register-indirect
//     memory. §5.7 tests FLAGS and writes PC from a register. §5.3 names
//     tritwise XOR and gives no truth table.
//   * spec/tisc/opcode-semantics.md and the interpreter in vm/vm.cpp use
//     direct addresses, a register predicate, an immediate branch target,
//     and wrapped balanced difference for scalar TXor.
//
// The same small integer is a legal address and a legal register index, so
// these fixtures overlap on purpose. A 13-byte instruction has no field that
// selects between the two readings.
//
// CTest runs this executable with no arguments. Exit 0 means the six
// Load/Store/JZ/JNZ disagreements are still present. That is evidence, not
// conformance.
//
// --require-normative is the audit mode. It exits 1 while those six
// disagreements remain. It is not registered with CTest. TXor is reported
// and is not one of the six: the frozen core text does not decide it.

namespace {
using namespace t81;
using tisc::Opcode;

int core_disagreements = 0;

tisc::Program round_trip(const tisc::Program& program) {
  const auto bytes = tisc::encode(program);
  T81_TEST_CHECK(bytes.size() == program.insns.size() * 13);
  for (std::size_t i = 0; i < program.insns.size(); ++i) {
    T81_TEST_CHECK(std::to_integer<unsigned>(bytes[i * 13]) ==
                   static_cast<unsigned>(program.insns[i].opcode));
  }
  const auto decoded = tisc::decode(bytes);
  T81_TEST_CHECK(decoded.has_value());
  T81_TEST_CHECK(tisc::encode(*decoded) == bytes);
  for (std::size_t i = 0; i < program.insns.size(); ++i) {
    T81_TEST_CHECK(decoded->insns[i].a == program.insns[i].a);
    T81_TEST_CHECK(decoded->insns[i].b == program.insns[i].b);
    T81_TEST_CHECK(decoded->insns[i].c == program.insns[i].c);
  }
  return *decoded;
}

void record_core_disagreement(bool disagree) {
  T81_TEST_CHECK(disagree);
  if (disagree) {
    ++core_disagreements;
  }
}

// Load A=2, B=5. Store A=5, B=1.
// R[5]=6, R[1]=555, MEM[5]=111, MEM[6]=222.
// Addresses 5 and 6 are both in range, so the split is not a bounds fault.
//
// §5.6 LOAD RD,[RS] reads MEM[R[RS]]. Binding RD=A and RS=B yields
// R[2] = MEM[R[5]] = MEM[6] = 222.
// The hosted interpreter executes R[A] = MEM[B], so R[2] = MEM[5] = 111.
//
// §5.6 STORE [RS],RD writes MEM[R[RS]] = R[RD]. Binding RS=A and RD=B yields
// MEM[R[5]] = R[1], a write of 555 at address 6.
// The hosted interpreter executes MEM[A] = R[B], a write of 555 at address 5.
void memory_cases() {
  constexpr std::int64_t kRegisterFive = 6;
  constexpr std::int64_t kStoredValue = 555;
  constexpr std::int64_t kDirectCell = 111;
  constexpr std::int64_t kIndirectCell = 222;

  {
    tisc::Program program;
    program.insns = {{Opcode::Load, 2, 5}, {Opcode::Halt}};
    auto machine = vm::make_interpreter_vm();
    machine->load_program(round_trip(program));
    machine->set_register(5, kRegisterFive);
    machine->set_register(1, kStoredValue);
    machine->set_memory_word(5, kDirectCell);
    machine->set_memory_word(6, kIndirectCell);
    T81_TEST_CHECK(machine->step().has_value());
    const auto actual = machine->state().contexts[0].registers[2];
    const bool disagree = actual == kDirectCell && actual != kIndirectCell;
    std::cout << "Load opcode=0x03 form=A:2,B:5 spec=R[A]=MEM[R[B]] spec_value=" << kIndirectCell
              << " vm=R[A]=MEM[B] vm_value=" << actual << " disagree=" << disagree << '\n';
    record_core_disagreement(disagree);
  }

  {
    tisc::Program program;
    program.insns = {{Opcode::Store, 5, 1}, {Opcode::Halt}};
    auto machine = vm::make_interpreter_vm();
    machine->load_program(round_trip(program));
    machine->set_register(5, kRegisterFive);
    machine->set_register(1, kStoredValue);
    machine->set_memory_word(5, kDirectCell);
    machine->set_memory_word(6, kIndirectCell);
    T81_TEST_CHECK(machine->step().has_value());
    const auto& memory = machine->state().memory;
    const bool wrote_direct = memory[5] == kStoredValue && memory[6] == kIndirectCell;
    const bool wrote_indirect = memory[6] == kStoredValue;
    std::cout << "Store opcode=0x04 form=A:5,B:1 spec=MEM[R[A]]=R[B] spec_addr=6"
              << " vm=MEM[A]=R[B] vm_addr=5 mem5=" << memory[5] << " mem6=" << memory[6]
              << " disagree=" << (wrote_direct && !wrote_indirect) << '\n';
    record_core_disagreement(wrote_direct && !wrote_indirect);
  }
}

// Seven instructions, so both candidate targets are inside the program:
//   0 Cmp R[7], R[8]     sets FLAGS; hosted Cmp compares R[A] with R[B]
//   1 JZ/JNZ A=5, B=6
//   2 Halt               hosted fall-through (step() already advanced PC)
//   5 Halt               hosted taken target, the immediate in A
//   6 Halt               §5.7 taken target, R[5]
//
// §5.7: JZ RS / JNZ RS tests FLAGS.ZERO and, when taken, PC := R[RS].
// The one written operand is bound to A here, so a taken prose branch is
// PC = R[5] = 6. Every fixture below makes that prose condition true.
// The hosted interpreter tests R[B] and, when taken, sets PC to immediate A.
void branch_case(Opcode opcode, bool flags_zero, std::int64_t condition_register,
                 std::size_t hosted_pc) {
  tisc::Program program;
  program.insns = {{Opcode::Cmp, 7, 8}, {opcode, 5, 6}, {Opcode::Halt}, {Opcode::Halt},
                   {Opcode::Halt},      {Opcode::Halt}, {Opcode::Halt}};
  auto machine = vm::make_interpreter_vm();
  machine->load_program(round_trip(program));
  machine->set_register(5, 6);
  machine->set_register(7, 0);
  machine->set_register(8, flags_zero ? 0 : 1);
  machine->set_register(6, condition_register);
  T81_TEST_CHECK(machine->step().has_value());
  T81_TEST_CHECK(machine->state().contexts[0].flags.zero == flags_zero);
  T81_TEST_CHECK(machine->step().has_value());
  const auto actual_pc = machine->state().contexts[0].pc;
  constexpr std::size_t kSpecPc = 6;
  const bool disagree = actual_pc == hosted_pc && actual_pc != kSpecPc;
  std::cout << tisc::opcode_name(opcode) << " flags_zero=" << flags_zero
            << " R[B]=" << condition_register << " spec=FLAGS,PC=R[A] spec_pc=" << kSpecPc
            << " vm=R[B],PC=A vm_pc=" << actual_pc << " disagree=" << disagree << '\n';
  record_core_disagreement(disagree);
}

void branch_cases() {
  // Prose condition is true in all four rows. The register predicate agrees
  // with it in the taken rows and contradicts it in the fall-through rows.
  branch_case(Opcode::JumpIfZero, true, 0, 5);
  branch_case(Opcode::JumpIfZero, true, 1, 2);
  branch_case(Opcode::JumpIfNotZero, false, 1, 5);
  branch_case(Opcode::JumpIfNotZero, false, 0, 2);
}

// §5.3 says tritwise XOR and specifies padding, not a truth table.
// opcode-semantics.md says "Ternary XOR / Sum" and does not define Sum.
// The interpreter clamps each scalar to {-1,0,1} and stores wrap(lhs - rhs),
// where a difference above 1 becomes -1 and a difference below -1 becomes 1.
// wrapped_sum is only a candidate reading of the word "Sum". Six of the nine
// pairs reject that candidate. This is not a finding that Sum is required.
void txor_cases() {
  constexpr int kDifference[3][3] = {{0, -1, 1}, {1, 0, -1}, {-1, 1, 0}};
  constexpr int kSumCandidate[3][3] = {{1, -1, 0}, {-1, 0, 1}, {0, 1, -1}};
  int sum_mismatches = 0;
  for (int lhs = -1; lhs <= 1; ++lhs) {
    for (int rhs = -1; rhs <= 1; ++rhs) {
      tisc::Program program;
      program.insns = {{Opcode::TXor, 3, 1, 2}, {Opcode::Halt}};
      auto machine = vm::make_interpreter_vm();
      machine->load_program(round_trip(program));
      machine->set_register(1, lhs);
      machine->set_register(2, rhs);
      T81_TEST_CHECK(machine->step().has_value());
      const auto actual = machine->state().contexts[0].registers[3];
      const int difference = kDifference[lhs + 1][rhs + 1];
      const int sum_candidate = kSumCandidate[lhs + 1][rhs + 1];
      T81_TEST_CHECK(actual == difference);
      if (actual != sum_candidate) {
        ++sum_mismatches;
      }
      std::cout << "TXor opcode=0x15 lhs=" << lhs << " rhs=" << rhs << " vm=" << actual
                << " wrapped_difference=" << difference << " wrapped_sum=" << sum_candidate
                << " spec_table=absent\n";
    }
  }
  T81_TEST_CHECK(sum_mismatches == 6);
  std::cout << "TXor status=underspecified scalar_pairs=9 wrapped_sum_mismatches=" << sum_mismatches
            << " counted_in_audit=0\n";
}
}  // namespace

int main(int argc, char** argv) {
  bool require_normative = false;
  if (argc == 2 && std::string_view(argv[1]) == "--require-normative") {
    require_normative = true;
  } else if (argc != 1) {
    std::cerr << "usage: t81_vm_frozen_semantics_differential_test [--require-normative]\n";
    return 2;
  }

  memory_cases();
  branch_cases();
  txor_cases();
  T81_TEST_CHECK(core_disagreements == 6);

  if (require_normative) {
    std::cout << "audit=fail disagreements=" << core_disagreements
              << " reason=Load,Store,JumpIfZero,JumpIfNotZero still disagree with "
                 "spec/tisc-spec.md §5.6 and §5.7. No normative decision is recorded here.\n";
    return 1;
  }

  std::cout << "characterization=pass disagreements=" << core_disagreements
            << " note=ordinary CI does not pass --require-normative\n";
  return 0;
}
