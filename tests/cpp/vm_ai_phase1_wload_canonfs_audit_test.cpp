#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/tensor/llama.hpp"
#include "t81/vm/vm.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void set_env(const char* name, const std::string& value) {
#if defined(_WIN32)
  _putenv_s(name, value.c_str());
#else
  setenv(name, value.c_str(), 1);
#endif
}

void unset_env(const char* name) {
#if defined(_WIN32)
  _putenv_s(name, "");
#else
  unsetenv(name);
#endif
}

}  // namespace

int main() {
  // AI-M4 audit path: WLOAD emits "meta slot axion event ... action=WeightLoad"
  // when a CanonFS driver is attached (via T81_CANONFS_ROOT env override).
  {
    namespace fs = std::filesystem;
    const fs::path workdir =
        fs::temp_directory_path() / "t81-wload-canonfs-audit-test";
    std::error_code ec;
    fs::remove_all(workdir, ec);
    fs::create_directories(workdir, ec);
    T81_TEST_CHECK(!ec);

    set_env("T81_CANONFS_ROOT", workdir.string());

    t81::tisc::Program p;
    p.tensor_pool.push_back(
        t81::T729DynamicTensor({2, 3}, {1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F}));

    // R1 = TensorHandle(1)
    t81::tisc::Insn lsrc{t81::tisc::Opcode::LoadImm, 1, 1, 0};
    lsrc.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(lsrc);
    // R2 = 0  (policy register, ignored in phase-1)
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 0, 0});
    // WLOAD R3, R1, R2
    p.insns.push_back({t81::tisc::Opcode::WLOAD, 3, 1, 2});
    p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(p);
    auto res = vm->run_to_halt();

    unset_env("T81_CANONFS_ROOT");
    fs::remove_all(workdir, ec);

    T81_TEST_CHECK(res.has_value());

    // Verify that the axion log contains a WeightLoad CanonFS audit entry.
    const auto& log = vm->state().axion_log;
    const bool has_weight_load_event =
        std::any_of(log.begin(), log.end(), [](const t81::vm::AxionEvent& ev) {
          return ev.verdict.reason.find("action=WeightLoad") != std::string::npos;
        });
    T81_TEST_CHECK(has_weight_load_event);

    // Verify the canonical meta-slot prefix is present.
    const bool has_meta_slot_prefix =
        std::any_of(log.begin(), log.end(), [](const t81::vm::AxionEvent& ev) {
          return ev.verdict.reason.find("meta slot axion event segment=meta addr=") !=
                 std::string::npos;
        });
    T81_TEST_CHECK(has_meta_slot_prefix);

    // Verify the WeightLoad event is tagged to the WLOAD opcode.
    const bool wload_opcode_present =
        std::any_of(log.begin(), log.end(), [](const t81::vm::AxionEvent& ev) {
          return ev.opcode == t81::tisc::Opcode::WLOAD &&
                 ev.verdict.reason.find("action=WeightLoad") != std::string::npos;
        });
    T81_TEST_CHECK(wload_opcode_present);

    // Basic tensor integrity: output must be correct weight data.
    const auto out_handle = vm->state().contexts[0].registers[3];
    T81_TEST_CHECK(out_handle > 0);
    const auto& out_opt =
        vm->state().tensors[static_cast<std::size_t>(out_handle - 1)];
    T81_TEST_CHECK(out_opt.has_value());
    const auto& out = out_opt.value();
    T81_TEST_CHECK(out.rank() == 2);
    T81_TEST_CHECK(out.shape()[0] == 2);
    T81_TEST_CHECK(out.shape()[1] == 3);
    T81_TEST_CHECK(out.data()[0] == 1.0F);
    T81_TEST_CHECK(out.data()[5] == 6.0F);

    std::cout << "AI_M4_AUDIT WLOAD canonfs_event=present\n";
  }

  // No-CanonFS path: audit event absent when no driver is attached.
  // (This verifies the guard is conditional, not always-on.)
  {
    // Ensure no CanonFS root is set.
    unset_env("T81_CANONFS_ROOT");

    t81::tisc::Program p;
    p.tensor_pool.push_back(
        t81::T729DynamicTensor({2, 3}, {1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F}));

    t81::tisc::Insn lsrc{t81::tisc::Opcode::LoadImm, 1, 1, 0};
    lsrc.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(lsrc);
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 0, 0});
    p.insns.push_back({t81::tisc::Opcode::WLOAD, 3, 1, 2});
    p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(p);
    auto res = vm->run_to_halt();
    T81_TEST_CHECK(res.has_value());

    const auto& log = vm->state().axion_log;
    const bool has_weight_load_event =
        std::any_of(log.begin(), log.end(), [](const t81::vm::AxionEvent& ev) {
          return ev.verdict.reason.find("action=WeightLoad") != std::string::npos;
        });
    // Without CanonFS driver, no WeightLoad meta-slot event should be emitted.
    T81_TEST_CHECK(!has_weight_load_event);

    std::cout << "AI_M4_AUDIT WLOAD no_canonfs_driver=confirmed\n";
  }

  return 0;
}
