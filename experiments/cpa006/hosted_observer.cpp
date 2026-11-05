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

static void terminal(const char* classification, const char* reason, unsigned steps) {
  std::cout << "CPA006_RESULT={\"classification\":\"" << classification << "\",\"reason\":";
  if(reason) std::cout << '\"' << reason << '\"'; else std::cout << "null";
  std::cout << ",\"steps\":" << steps << "}\nCPA006_DONE\n";
}
int main(int argc,char** argv) {
  std::cout << std::boolalpha;
  if(argc!=3) return 2;
  if(const char* root=std::getenv("T81_CANONFS_ROOT");root && root[0]) return 2;
  unsigned limit=static_cast<unsigned>(std::stoul(argv[2]));
  std::ifstream input(argv[1],std::ios::binary);
  if(!input) return 2;
  std::vector<unsigned char> chars((std::istreambuf_iterator<char>(input)),{});
  const char hex[]="0123456789abcdef";
  std::cout << "CPA006_ARTIFACT_HEX=";
  for(unsigned char ch:chars) std::cout << hex[ch>>4] << hex[ch&15];
  std::cout << '\n';
  if(chars.empty() || chars.size()%13 || chars.size()>64*13) {
    terminal("admission_rejection","malformed",0);return 0;
  }
  if(limit>64) { terminal("admission_rejection","outside_profile",0);return 0; }
  std::vector<std::byte> bytes;
  for(auto ch:chars) bytes.push_back(static_cast<std::byte>(ch));
  auto program=t81::tisc::decode(bytes);
  if(!program) { terminal("admission_rejection","undefined_opcode",0);return 0; }
  for(const auto& insn:program->insns) {
    switch(static_cast<unsigned>(insn.opcode)) {
      case 1:case 2:case 3:case 4:case 5:case 7:case 8:case 9:case 11:case 15:
      case 18:case 19:case 20:case 21:case 25:case 28:case 60:break;
      default:terminal("admission_rejection","unsupported_profile",0);return 0;
    }
  }
  if(t81::tisc::encode(*program)!=bytes) return 3;
  auto vm=t81::vm::make_interpreter_vm(t81::axion::make_allow_all_engine());
  vm->load_program(*program);
  std::cout << "CPA006_OBSERVATION={\"sequence\":0,\"pc_before\":null,\"instruction\":null,\"status\":\"running\",\"trap\":null,\"state\":";
  snapshot(vm->state()); std::cout << "}\n";
  for(unsigned tick=1;tick<=limit;++tick) {
    const auto pc=vm->state().contexts[0].pc;
    if(pc>=program->insns.size()) return 4; // outside sealed suite, never invent a step
    const auto& insn=program->insns[pc];auto result=vm->step();const auto& state=vm->state();
    std::cout << "CPA006_OBSERVATION={\"sequence\":" << tick << ",\"pc_before\":" << pc
      << ",\"instruction\":{\"opcode\":" << static_cast<unsigned>(insn.opcode)
      << ",\"name\":\"" << t81::tisc::opcode_name(insn.opcode) << "\",\"a\":" << insn.a
      << ",\"b\":" << insn.b << ",\"c\":" << insn.c << "},\"status\":\""
      << (!result?"trap":state.halted?"halted":"running") << "\",\"trap\":";
    if(result) std::cout << "null"; else std::cout << '\"' << t81::vm::to_string(result.error()) << '\"';
    std::cout << ",\"state\":";snapshot(state);std::cout << "}\n";
    if(!result) { terminal("runtime_trap",t81::vm::to_string(result.error()).c_str(),tick);return 0; }
    if(state.halted) { terminal("halted",nullptr,tick);return 0; }
  }
  terminal("instruction_limit",nullptr,limit);return 0;
}
