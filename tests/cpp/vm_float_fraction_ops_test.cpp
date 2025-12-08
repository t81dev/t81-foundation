#include <cassert>
#include <cmath>

#include "t81/bigint.hpp"
#include "t81/fraction.hpp"
#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"

namespace {
t81::T81Fraction make_fraction(int num, int den) {
  return t81::T81Fraction(t81::T81BigInt::from_i64(num),
                          t81::T81BigInt::from_i64(den));
}
}  // namespace

int main() {
  using namespace t81;

  // Float opcodes produce deterministic handles.
  {
    tisc::Program program;
    program.float_pool = {1.5, -0.5};
    program.insns.push_back({tisc::Opcode::LoadImm, 1, 1, 0});
    program.insns.push_back({tisc::Opcode::LoadImm, 2, 2, 0});
    program.insns.push_back({tisc::Opcode::FAdd, 3, 1, 2});
    program.insns.push_back({tisc::Opcode::FSub, 4, 1, 2});
    program.insns.push_back({tisc::Opcode::FMul, 5, 1, 2});
    program.insns.push_back({tisc::Opcode::FDiv, 6, 1, 2});
    program.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(program);
    [[maybe_unused]] auto run = vm->run_to_halt();
    assert(run.has_value());
    [[maybe_unused]] const auto& floats = vm->state().floats;
    assert(floats.size() == 6);
    [[maybe_unused]] auto nearly_equal = [](double lhs, double rhs) {
      return std::fabs(lhs - rhs) < 1e-12;
    };
    assert(nearly_equal(floats[2], 1.0));    // FAdd
    assert(nearly_equal(floats[3], 2.0));    // FSub
    assert(nearly_equal(floats[4], -0.75));  // FMul
    assert(nearly_equal(floats[5], -3.0));   // FDiv
  }

  // Float divide-by-zero traps.
  {
    tisc::Program program;
    program.float_pool = {1.0, 0.0};
    program.insns.push_back({tisc::Opcode::LoadImm, 1, 1, 0});
    program.insns.push_back({tisc::Opcode::LoadImm, 2, 2, 0});
    program.insns.push_back({tisc::Opcode::FDiv, 3, 1, 2});
    auto vm = vm::make_interpreter_vm();
    vm->load_program(program);
    [[maybe_unused]] auto run = vm->run_to_halt();
    assert(!run.has_value());
    assert(run.error() == vm::Trap::DivideByZero);
  }

  // Fraction opcodes mirror float behavior.
  {
    tisc::Program program;
    program.fraction_pool = {make_fraction(1, 2), make_fraction(2, 3)};
    program.insns.push_back({tisc::Opcode::LoadImm, 1, 1, 0});
    program.insns.push_back({tisc::Opcode::LoadImm, 2, 2, 0});
    program.insns.push_back({tisc::Opcode::FracAdd, 3, 1, 2});
    program.insns.push_back({tisc::Opcode::FracSub, 4, 1, 2});
    program.insns.push_back({tisc::Opcode::FracMul, 5, 1, 2});
    program.insns.push_back({tisc::Opcode::FracDiv, 6, 1, 2});
    program.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(program);
    [[maybe_unused]] auto run = vm->run_to_halt();
    assert(run.has_value());
    [[maybe_unused]] const auto& fracs = vm->state().fractions;
    assert(fracs.size() == 6);
    assert(fracs[2].num.to_int64() == 7 && fracs[2].den.to_int64() == 6);   // add
    assert(fracs[3].num.to_int64() == -1 && fracs[3].den.to_int64() == 6);  // sub
    assert(fracs[4].num.to_int64() == 1 && fracs[4].den.to_int64() == 3);   // mul
    assert(fracs[5].num.to_int64() == 3 && fracs[5].den.to_int64() == 4);   // div
  }

  // Fraction divide-by-zero traps.
  {
    tisc::Program program;
    program.fraction_pool = {make_fraction(1, 2), make_fraction(0, 1)};
    program.insns.push_back({tisc::Opcode::LoadImm, 1, 1, 0});
    program.insns.push_back({tisc::Opcode::LoadImm, 2, 2, 0});
    program.insns.push_back({tisc::Opcode::FracDiv, 3, 1, 2});
    auto vm = vm::make_interpreter_vm();
    vm->load_program(program);
    [[maybe_unused]] auto run = vm->run_to_halt();
    assert(!run.has_value());
    assert(run.error() == vm::Trap::DivideByZero);
  }

  // Float comparisons influence flags.
  {
    tisc::Program program;
    program.float_pool = {1.0, 2.0};
    program.insns.push_back({tisc::Opcode::LoadImm, 1, 1, 0,
                             tisc::LiteralKind::FloatHandle});
    program.insns.push_back({tisc::Opcode::LoadImm, 2, 2, 0,
                             tisc::LiteralKind::FloatHandle});
    program.insns.push_back({tisc::Opcode::Cmp, 1, 2, 0});
    program.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});
    auto vm = vm::make_interpreter_vm();
    vm->load_program(program);
    [[maybe_unused]] auto run = vm->run_to_halt();
    assert(run.has_value());
    assert(vm->state().flags.zero == false);
    assert(vm->state().flags.negative == true);
  }

  // Fraction comparisons influence flags.
  {
    tisc::Program program;
    program.fraction_pool = {make_fraction(1, 2), make_fraction(3, 4)};
    program.insns.push_back({tisc::Opcode::LoadImm, 1, 1, 0,
                             tisc::LiteralKind::FractionHandle});
    program.insns.push_back({tisc::Opcode::LoadImm, 2, 2, 0,
                             tisc::LiteralKind::FractionHandle});
    program.insns.push_back({tisc::Opcode::Cmp, 2, 1, 0});
    program.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});
    auto vm = vm::make_interpreter_vm();
    vm->load_program(program);
    [[maybe_unused]] auto run = vm->run_to_halt();
    assert(run.has_value());
    assert(vm->state().flags.zero == false);
    assert(vm->state().flags.negative == false);
  }

  return 0;
}
