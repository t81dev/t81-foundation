#include <cassert>
#include <memory>
#include <t81/vm/vm.hpp>
#include <t81/tisc/program.hpp>
#include <t81/axion/engine.hpp>

using namespace t81;

namespace {
class DenyEngine : public t81::axion::Engine {
 public:
  t81::axion::Verdict evaluate(const t81::axion::SyscallContext&) override {
    return {t81::axion::VerdictKind::Deny, "blocked"};
  }
};
}  // namespace

int main() {
  {
    tisc::Program p;
    // r1 = 5
    p.insns.push_back({tisc::Opcode::LoadImm, 1, 5, 0});
    // r2 = r1
    p.insns.push_back({tisc::Opcode::Mov, 2, 1, 0});
    // r1++
    p.insns.push_back({tisc::Opcode::Inc, 1, 0, 0});
    // r2--
    p.insns.push_back({tisc::Opcode::Dec, 2, 0, 0});
    // cmp r1,r2 (should set zero=false, negative=false)
    p.insns.push_back({tisc::Opcode::Cmp, 1, 2, 0});
    // push r1, then r2
    p.insns.push_back({tisc::Opcode::Push, 1, 0, 0});
    p.insns.push_back({tisc::Opcode::Push, 2, 0, 0});
    // pop into r3 (expect r2 value)
    p.insns.push_back({tisc::Opcode::Pop, 3, 0, 0});
    // pop into r4 (expect r1 value)
    p.insns.push_back({tisc::Opcode::Pop, 4, 0, 0});
    // Axion ops
    p.insns.push_back({tisc::Opcode::AxRead, 5, 42, 0});
    p.insns.push_back({tisc::Opcode::AxSet, 7, 1, 0});
    p.insns.push_back({tisc::Opcode::AxVerify, 6, 0, 0});
    p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    auto r = vm->run_to_halt();
    assert(r.has_value());
    assert(vm->state().registers[1] == 6);
    assert(vm->state().registers[2] == 4);
    assert(vm->state().registers[3] == 4);
    assert(vm->state().registers[4] == 6);
    assert(vm->state().registers[5] == 42);
    assert(vm->state().registers[6] == 0);
    assert(!vm->state().flags.zero);
    assert(!vm->state().flags.negative);
    assert(vm->state().axion_log.size() == 3);
    assert(vm->state().axion_log[0].opcode == tisc::Opcode::AxRead);
  }

  // Pop with empty stack must trap.
  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::Pop, 0, 0, 0});
    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    auto step = vm->step();
    assert(!step.has_value());
    assert(step.error() == vm::Trap::BoundsFault);
  }

  // Axion privilege denial via custom engine.
  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::AxRead, 0, 1, 0});
    auto vm = vm::make_interpreter_vm(std::make_unique<DenyEngine>());
    vm->load_program(p);
    auto res = vm->step();
    assert(!res.has_value());
    assert(res.error() == vm::Trap::SecurityFault);
  }

  return 0;
}
