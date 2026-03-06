#include <string>

#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

namespace {

bool has_tensor_provenance_event(const t81::vm::State& state, std::int64_t handle,
                                 std::string_view action, std::string_view storage_class,
                                 std::string_view numeric_class, bool strict_core_eligible) {
  for (const auto& event : state.axion_log) {
    if (event.structured.event_type != "tensor_provenance") {
      continue;
    }
    if (event.structured.reason_code != "VM_TENSOR_PROVENANCE") {
      continue;
    }
    if (event.structured.handle_id != handle) {
      continue;
    }
    if (event.structured.storage_class != storage_class) {
      continue;
    }
    if (event.structured.numeric_class != numeric_class) {
      continue;
    }
    if (event.structured.strict_core_eligible != strict_core_eligible) {
      continue;
    }
    if (event.verdict.reason.find("action=" + std::string(action)) == std::string::npos) {
      continue;
    }
    return true;
  }
  return false;
}

void run_tensor_provenance_trace_contract() {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 1, 3, 0});  // size
  p.insns.push_back({t81::tisc::Opcode::TNew, 2, 1, 0});     // tensor handle
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 1, 0});  // index
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 4, 7, 0});  // int payload
  p.insns.push_back({t81::tisc::Opcode::TSet, 2, 3, 4});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 5, 9, 0});  // float payload source
  p.insns.push_back({t81::tisc::Opcode::I2F, 6, 5, 0});
  p.insns.push_back({t81::tisc::Opcode::TSet, 2, 3, 6});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());

  const auto& state = vm->state();
  const auto handle = state.contexts[0].registers[2];
  T81_TEST_CHECK(handle > 0);

  T81_TEST_CHECK(has_tensor_provenance_event(state, handle, "alloc", "canonical_fixed",
                                             "exact_trit", true));
  T81_TEST_CHECK(
      has_tensor_provenance_event(state, handle, "set", "canonical_fixed", "exact_int", true));
  T81_TEST_CHECK(
      has_tensor_provenance_event(state, handle, "set", "canonical_fixed", "host_float", false));
}

}  // namespace

int main() {
  run_tensor_provenance_trace_contract();
  return 0;
}
