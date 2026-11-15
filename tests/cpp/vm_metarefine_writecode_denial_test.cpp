#include <array>
#include <iostream>
#include "t81/axion/engine.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/vm/vm.hpp"
#include "test_runtime_check.hpp"

using namespace t81::vm;
using namespace t81::tisc;

static void check_batch(bool deny, bool invalid_target) {
  auto engine = t81::axion::make_policy_engine(
      t81::axion::parse_policy("(policy (tier 4) (max-reflections 10) (max-meta-writes 100))")
          .value());
  auto vm = make_interpreter_vm(std::move(engine));
  Program program;
  program.insns = {{Opcode::MetaRefine, 3, 1, 2}, {Opcode::Halt}};
  vm->load_program(program);
  auto& state = const_cast<State&>(vm->state());
  const auto base = state.layout.heap.start;
  const auto target = base + 64;
  vm->set_register(1, static_cast<std::int64_t>(base));
  vm->set_register(2, 3);
  vm->set_register(4, 7);
  state.memory[target] = 11;
  const auto meta_writes = state.meta_write_count;
  const std::array<std::array<std::int64_t, 4>, 3> commands = {
      {{static_cast<std::int64_t>(RefinementCommand::Op::WriteReg), 4, 42,
        static_cast<std::int64_t>(ValueTag::Int)},
       {static_cast<std::int64_t>(deny ? RefinementCommand::Op::WriteCode
                                       : RefinementCommand::Op::Noop),
        invalid_target ? -1 : 0, static_cast<std::int64_t>(Opcode::Halt), 0},
       {static_cast<std::int64_t>(RefinementCommand::Op::WriteMem),
        static_cast<std::int64_t>(target), 99, static_cast<std::int64_t>(ValueTag::Int)}}};
  for (std::size_t i = 0; i < commands.size(); ++i)
    for (std::size_t j = 0; j < 4; ++j) state.memory[base + i * 4 + j] = commands[i][j];
  const auto result = vm->run_to_halt(10);
  if (deny) {
    T81_TEST_CHECK(!result && result.error() == Trap::SecurityFault);
    T81_TEST_CHECK(state.contexts[0].registers[4] == 7);
    T81_TEST_CHECK(state.memory[target] == 11);
    T81_TEST_CHECK(!state.axion_log.empty());
    T81_TEST_CHECK(state.axion_log.back().verdict.reason ==
                   "MetaRefine WriteCode denied (protected)");
  } else {
    T81_TEST_CHECK(result);
    T81_TEST_CHECK(state.contexts[0].registers[4] == 42);
    T81_TEST_CHECK(state.memory[target] == 99);
  }
  T81_TEST_CHECK(state.meta_write_count == meta_writes);
}

int main() {
  check_batch(true, false);
  check_batch(true, true);
  check_batch(false, false);
  std::cout << "WriteCode denial and mixed-batch atomicity: PASS\n";
}
