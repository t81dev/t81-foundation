#include "t81/axion/reasons.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/state.hpp"
#include "t81/vm/vm.hpp"

#include <iostream>
#include <vector>
#include "test_runtime_check.hpp"

namespace {

std::unique_ptr<t81::vm::IVirtualMachine> run_program(const std::vector<t81::tisc::Insn>& insns,
                                                      const std::vector<std::string>& symbols) {
  t81::tisc::Program program;
  program.insns = insns;
  program.symbol_pool = symbols;
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "Execution failed with trap: " << t81::vm::to_string(result.error()) << "\n";
  }
  T81_TEST_CHECK(result.has_value());
  return vm;
}

}  // namespace

int main() {
  using namespace t81::tisc;
  using namespace t81::vm;

  std::cout << "Testing ReflCap/ReflJustify...\n";
  {
    std::vector<std::string> symbols = {"MyDescription", "Because it is true"};

    // R1 = "MyDescription" (SymbolHandle)
    Insn load_desc{Opcode::LoadImm, 1, 1};
    load_desc.literal_kind = LiteralKind::SymbolHandle;

    // ReflCap R2, R1
    Insn capture{Opcode::ReflCap, 2, 1};  // dst=R2, src=R1

    // R3 = "Because it is true" (SymbolHandle)
    Insn load_just{Opcode::LoadImm, 3, 2};
    load_just.literal_kind = LiteralKind::SymbolHandle;

    // ReflJustify R2, R3
    Insn justify{Opcode::ReflJustify, 2, 3};  // handle=R2, text=R3

    Insn halt{Opcode::Halt};

    auto vm = run_program({load_desc, capture, load_just, justify, halt}, symbols);

    const auto& state = vm->state();
    T81_TEST_CHECK(state.contexts[0].register_tags[2] == ValueTag::Tier2FrameHandle);
    T81_TEST_CHECK(state.contexts[0].tier2_frames.size() == 1);

    const auto& frame = state.contexts[0].tier2_frames[0];
    T81_TEST_CHECK(frame.description == "MyDescription");
    T81_TEST_CHECK(frame.justification.steps.size() == 2);

    T81_TEST_CHECK(frame.justification.steps[0] == "Captured state: MyDescription");
    T81_TEST_CHECK(frame.justification.steps[1] == "Because it is true");
    T81_TEST_CHECK(frame.justification.entries.size() == 2);
    T81_TEST_CHECK(frame.justification.entries[0].kind == "capture");
    T81_TEST_CHECK(frame.justification.entries[1].kind == "justify");
    T81_TEST_CHECK(frame.justification.entries[0].pc == 1);
    T81_TEST_CHECK(frame.justification.entries[1].pc == 3);

    bool saw_tier2_capture_event = false;
    bool saw_tier2_justify_event = false;
    for (const auto& event : state.axion_log) {
      if (event.verdict.reason.find(t81::axion::reasons::kCogTier2Reflect.data()) ==
          std::string::npos) {
        continue;
      }
      if (event.verdict.reason.find("action=capture") != std::string::npos) {
        saw_tier2_capture_event = true;
      }
      if (event.verdict.reason.find("action=justify") != std::string::npos) {
        saw_tier2_justify_event = true;
      }
    }
    T81_TEST_CHECK(saw_tier2_capture_event);
    T81_TEST_CHECK(saw_tier2_justify_event);

    std::cout << "  ReflCap/ReflJustify: PASS\n";
  }

  std::cout << "All Tier 2 reflection tests passed!\n";
  return 0;
}
