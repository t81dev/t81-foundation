// CPA-003 hosted observation only. Links the unchanged in-tree interpreter.
// No instruction execution semantics or expected answers are implemented here.
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include "t81/isa/encoding.hpp"
#include "t81/vm/vm.hpp"

using t81::tisc::Program;
using t81::vm::State;

template <typename T>
void array_json(const T& values, std::size_t count) {
  std::cout << '[';
  for (std::size_t i = 0; i < count; ++i) {
    if (i) std::cout << ',';
    std::cout << static_cast<std::int64_t>(values[i]);
  }
  std::cout << ']';
}

void snapshot(const State& state) {
  const auto& c = state.contexts.at(0);
  std::cout << "{\"pc\":" << c.pc << ",\"sp\":" << c.sp << ",\"halted\":" << state.halted
            << ",\"flags\":{\"zero\":" << c.flags.zero << ",\"negative\":" << c.flags.negative
            << ",\"positive\":" << c.flags.positive << "},\"register_values\":";
  array_json(c.registers, 81);
  std::cout << ",\"register_tags\":";
  array_json(c.register_tags, 81);
  std::cout << ",\"memory_values\":";
  array_json(state.memory, state.memory.size());
  std::cout << ",\"memory_tags\":";
  array_json(state.memory_tags, state.memory_tags.size());
  std::cout << '}';
}

void diagnostics(const State& s) {
  const auto& c = s.contexts.at(0);
  std::cout << ",\"diagnostics\":{\"tier\":" << static_cast<int>(c.tier_status.current)
            << ",\"call_depth\":" << c.call_depth << ",\"stack_frames\":" << c.stack_frames.size()
            << ",\"contradiction_events\":" << s.contradiction_events
            << ",\"context_halted\":" << c.halted << ",\"gc_cycles\":" << s.gc_cycles
            << ",\"policy_loaded\":" << s.policy.has_value()
            << ",\"axion_events\":" << s.axion_log.size() << '}';
}

int main(int argc, char** argv) {
  std::cout << std::boolalpha;
  if (argc != 2) return 2;
  const char* root = std::getenv("T81_CANONFS_ROOT");
  if (root && root[0]) {
    std::cerr << "T81_CANONFS_ROOT must be absent or empty\n";
    return 2;
  }
  std::ifstream input(argv[1], std::ios::binary);
  if (!input) return 2;
  const std::vector<char> chars((std::istreambuf_iterator<char>(input)), {});
  std::vector<std::byte> bytes;
  for (unsigned char ch : chars) bytes.push_back(static_cast<std::byte>(ch));
  auto program = t81::tisc::decode(bytes);
  if (!program) {
    std::cerr << "existing decoder rejected artifact\n";
    return 3;
  }
  // Exercise the existing encoder as a third byte-identity check.
  if (t81::tisc::encode(*program) != bytes) return 4;
  if (!program->float_pool.empty() || !program->fraction_pool.empty() ||
      !program->bigint_pool.empty() || !program->symbol_pool.empty() ||
      !program->tensor_pool.empty() || !program->shape_pool.empty() ||
      !program->complex_pool.empty() || !program->axion_policy_text.empty() ||
      !program->match_metadata_text.empty() || !program->type_aliases.empty() ||
      !program->enum_metadata.empty() || !program->function_metadata.empty() ||
      program->weights_model)
    return 5;
  auto vm = t81::vm::make_interpreter_vm(t81::axion::make_allow_all_engine());
  vm->load_program(*program);

  std::cout << "{\"sequence\":0,\"pc_before\":null,\"instruction\":null,"
               "\"status\":\"running\",\"trap\":null,\"state\":";
  snapshot(vm->state());
  diagnostics(vm->state());
  std::cout << ",\"input_bytes\":" << bytes.size()
            << ",\"instruction_records\":" << program->insns.size()
            << ",\"existing_encoder_roundtrip\":true,\"all_pools_metadata_empty\":true,"
               "\"admission\":\"explicit_allow_all\",\"canonfs_root\":null,\"decoded_records\":[";
  for (std::size_t i = 0; i < program->insns.size(); ++i) {
    if (i) std::cout << ',';
    const auto& insn = program->insns[i];
    std::cout << '[' << static_cast<unsigned>(insn.opcode) << ',' << insn.a << ',' << insn.b << ','
              << insn.c << ',' << static_cast<unsigned>(insn.literal_kind) << ']';
  }
  std::cout << "]}\n";

  for (unsigned tick = 1; tick <= 64; ++tick) {
    const auto pc = vm->state().contexts[0].pc;
    if (pc >= program->insns.size()) {
      std::cerr << "outside frozen artifact instruction path\n";
      return 6;
    }
    const auto& insn = program->insns[pc];
    const auto result = vm->step();
    const auto& s = vm->state();
    std::cout << "{\"sequence\":" << tick << ",\"pc_before\":" << pc
              << ",\"instruction\":{\"opcode\":" << static_cast<unsigned>(insn.opcode)
              << ",\"name\":\"" << t81::tisc::opcode_name(insn.opcode) << "\",\"a\":" << insn.a
              << ",\"b\":" << insn.b << ",\"c\":" << insn.c << "},"
              << "\"status\":\"" << (!result ? "trap" : (s.halted ? "halted" : "running"))
              << "\",\"trap\":";
    if (result)
      std::cout << "null";
    else
      std::cout << '"' << t81::vm::to_string(result.error()) << '"';
    std::cout << ",\"state\":";
    snapshot(s);
    diagnostics(s);
    std::cout << ",\"trace_last\":";
    if (s.trace.empty())
      std::cout << "null";
    else {
      const auto& t = s.trace.back();
      std::cout << "{\"pc\":" << t.pc << ",\"opcode\":" << static_cast<unsigned>(t.opcode)
                << ",\"trap\":";
      if (t.trap)
        std::cout << '"' << t81::vm::to_string(*t.trap) << '"';
      else
        std::cout << "null";
      std::cout << '}';
    }
    std::cout << "}\n";
    if (!result) return 7;
    if (s.halted) return 0;
  }
  std::cerr << "instruction limit reached without Halt\n";
  return 8;
}
