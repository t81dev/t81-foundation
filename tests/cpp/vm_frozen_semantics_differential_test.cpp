#include <array>
#include <cstddef>
#include <iostream>
#include <string_view>
#include "test_runtime_check.hpp"
#include "t81/isa/encoding.hpp"
#include "t81/vm/vm.hpp"

// Characterize compatibility without silently choosing between conflicting frozen
// documents. --require-normative fails on the four unambiguous prose divergences.
// TXor's sum candidate is diagnostic: no authoritative truth table was found.
namespace {
using namespace t81;
using tisc::Opcode;
int divergences = 0;

tisc::Program round_trip(const tisc::Program& p) {
  const auto bytes = tisc::encode(p);
  T81_TEST_CHECK(bytes.size() == p.insns.size() * 13);
  for (std::size_t i = 0; i < p.insns.size(); ++i) {
    T81_TEST_CHECK(std::to_integer<unsigned>(bytes[i * 13]) ==
                   static_cast<unsigned>(p.insns[i].opcode));
  }
  const auto decoded = tisc::decode(bytes);
  T81_TEST_CHECK(decoded.has_value());
  T81_TEST_CHECK(tisc::encode(*decoded) == bytes);
  for (std::size_t i = 0; i < p.insns.size(); ++i) {
    T81_TEST_CHECK(decoded->insns[i].a == p.insns[i].a);
    T81_TEST_CHECK(decoded->insns[i].b == p.insns[i].b);
    T81_TEST_CHECK(decoded->insns[i].c == p.insns[i].c);
  }
  return *decoded;
}

void memory_cases() {
  for (auto op : {Opcode::Load, Opcode::Store}) {
    tisc::Program p;
    p.insns = {{op, op == Opcode::Load ? 2 : 5, op == Opcode::Load ? 5 : 1},
               {Opcode::Halt}};
    auto machine = vm::make_interpreter_vm();
    machine->load_program(round_trip(p));
    machine->set_register(5, 6);
    machine->set_register(1, 555);
    machine->set_memory_word(5, 111);
    machine->set_memory_word(6, 222);
    T81_TEST_CHECK(machine->step().has_value());
    const auto& st = machine->state();
    if (op == Opcode::Load) {
      const auto actual = st.contexts[0].registers[2];
      T81_TEST_CHECK(actual == 111);
      T81_TEST_CHECK(actual != 222);  // §5.6 MEM[R[5]] oracle
      std::cout << "Load actual=111 normative=222\n";
    } else {
      T81_TEST_CHECK(st.memory[5] == 555);
      T81_TEST_CHECK(st.memory[6] == 222);  // §5.6 would write MEM[R[5]]
      std::cout << "Store actual_destination=5 normative_destination=6\n";
    }
    ++divergences;
  }
}

void branch_cases() {
  for (auto op : {Opcode::JumpIfZero, Opcode::JumpIfNotZero}) {
    for (bool same_predicate : {false, true}) {
      tisc::Program p;
      p.insns = {{Opcode::Cmp, 7, 8}, {op, 5, 6}, {Opcode::Halt}, {Opcode::Halt},
                 {Opcode::Halt}, {Opcode::Halt}, {Opcode::Halt}};
      auto machine = vm::make_interpreter_vm();
      machine->load_program(round_trip(p));
      machine->set_register(5, 6);  // §5.7 target R[A]; hosted target A
      const bool zero = op == Opcode::JumpIfZero;
      machine->set_register(7, 0);
      machine->set_register(8, zero ? 0 : 1);
      machine->set_register(6, zero == same_predicate ? 0 : 1);
      T81_TEST_CHECK(machine->step().has_value());
      T81_TEST_CHECK(machine->state().contexts[0].flags.zero == zero);
      T81_TEST_CHECK(machine->step().has_value());
      auto actual = machine->state().contexts[0].pc;
      T81_TEST_CHECK(actual == (same_predicate ? 5u : 2u));
      T81_TEST_CHECK(actual != 6);  // prose condition is true; target R[5]=6
      std::cout << (zero ? "JumpIfZero" : "JumpIfNotZero")
                << " same_predicate=" << same_predicate << " actual_pc=" << actual
                << " normative_pc=6\n";
      ++divergences;
    }
  }
}

void txor_cases() {
  constexpr int difference[3][3] = {{0,-1,1}, {1,0,-1}, {-1,1,0}};
  constexpr int sum_candidate[3][3] = {{1,-1,0}, {-1,0,1}, {0,1,-1}};
  int sum_mismatches = 0;
  for (int lhs = -1; lhs <= 1; ++lhs) for (int rhs = -1; rhs <= 1; ++rhs) {
    tisc::Program p;
    p.insns = {{Opcode::TXor, 3, 1, 2}, {Opcode::Halt}};
    auto machine = vm::make_interpreter_vm();
    machine->load_program(round_trip(p));
    machine->set_register(1, lhs);
    machine->set_register(2, rhs);
    T81_TEST_CHECK(machine->step().has_value());
    auto actual = machine->state().contexts[0].registers[3];
    T81_TEST_CHECK(actual == difference[lhs+1][rhs+1]);
    sum_mismatches += actual != sum_candidate[lhs+1][rhs+1];
    std::cout << "TXor lhs=" << lhs << " rhs=" << rhs << " actual=" << actual
              << " sum_candidate=" << sum_candidate[lhs+1][rhs+1] << '\n';
  }
  T81_TEST_CHECK(sum_mismatches == 6);
}
}  // namespace

int main(int argc, char** argv) {
  memory_cases();
  branch_cases();
  txor_cases();
  T81_TEST_CHECK(divergences == 6);
  std::cout << "Characterization passed; 6 prose divergences remain unresolved.\n";
  return argc == 2 && std::string_view(argv[1]) == "--require-normative" ? 1 : 0;
}
