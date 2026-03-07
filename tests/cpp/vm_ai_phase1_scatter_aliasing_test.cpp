#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/tensor/llama.hpp"
#include "t81/vm/vm.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>

namespace {

std::int32_t pack_reg_pair(std::int32_t first, std::int32_t second) {
  return static_cast<std::int32_t>((first & 0xFF) | ((second & 0xFF) << 8));
}

}  // namespace

int main() {
  // Success path: two SCATTER ops targeting DIFFERENT rows — both must succeed.
  {
    t81::tisc::Program p;
    // dst (3×2), all zeros.
    p.tensor_pool.push_back(
        t81::T729DynamicTensor({3, 2}, {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F}));
    // src0 (rank-1, length 2): scatter into row 0.
    p.tensor_pool.push_back(t81::T729DynamicTensor({2}, {1.0F, 2.0F}));
    // src1 (rank-1, length 2): scatter into row 2 (different row — no alias).
    p.tensor_pool.push_back(t81::T729DynamicTensor({2}, {3.0F, 4.0F}));

    // R1 = dst handle
    t81::tisc::Insn ldst{t81::tisc::Opcode::LoadImm, 1, 1, 0};
    ldst.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(ldst);
    // R2 = src0 handle
    t81::tisc::Insn lsrc0{t81::tisc::Opcode::LoadImm, 2, 2, 0};
    lsrc0.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(lsrc0);
    // R3 = src1 handle
    t81::tisc::Insn lsrc1{t81::tisc::Opcode::LoadImm, 3, 3, 0};
    lsrc1.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(lsrc1);
    // R4 = 0 (row 0 index)
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 4, 0, 0});
    // R5 = 2 (row 2 index)
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 5, 2, 0});
    // SCATTER R6, R1, PACK(R4=idx0, R2=src0)
    p.insns.push_back({t81::tisc::Opcode::SCATTER, 6, 1, pack_reg_pair(4, 2)});
    // SCATTER R7, R1, PACK(R5=idx2, R3=src1) — different row, no alias.
    p.insns.push_back({t81::tisc::Opcode::SCATTER, 7, 1, pack_reg_pair(5, 3)});
    p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(p);
    auto res = vm->run_to_halt();
    T81_TEST_CHECK(res.has_value());
    T81_TEST_CHECK(vm->state().contexts[0].register_tags[6] == t81::vm::ValueTag::TensorHandle);
    T81_TEST_CHECK(vm->state().contexts[0].register_tags[7] == t81::vm::ValueTag::TensorHandle);

    std::cout << "AI_M5_ALIASING SCATTER different_rows=pass\n";
  }

  // Aliasing fault path: two SCATTER ops targeting the SAME row of the same dst.
  // The second SCATTER must raise SecurityFault via Axion deny.
  {
    t81::tisc::Program p;
    p.tensor_pool.push_back(
        t81::T729DynamicTensor({3, 2}, {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F}));
    p.tensor_pool.push_back(t81::T729DynamicTensor({2}, {1.0F, 2.0F}));
    p.tensor_pool.push_back(t81::T729DynamicTensor({2}, {3.0F, 4.0F}));

    t81::tisc::Insn ldst{t81::tisc::Opcode::LoadImm, 1, 1, 0};
    ldst.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(ldst);
    t81::tisc::Insn lsrc0{t81::tisc::Opcode::LoadImm, 2, 2, 0};
    lsrc0.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(lsrc0);
    t81::tisc::Insn lsrc1{t81::tisc::Opcode::LoadImm, 3, 3, 0};
    lsrc1.literal_kind = t81::tisc::LiteralKind::TensorHandle;
    p.insns.push_back(lsrc1);
    // Both SCATTER ops target row 1 of the same dst — aliasing violation.
    p.insns.push_back({t81::tisc::Opcode::LoadImm, 4, 1, 0});  // index = 1
    // First SCATTER R5, R1, PACK(R4, R2) — should succeed.
    p.insns.push_back({t81::tisc::Opcode::SCATTER, 5, 1, pack_reg_pair(4, 2)});
    // Second SCATTER R6, R1, PACK(R4, R3) — same dst handle, same index → SecurityFault.
    p.insns.push_back({t81::tisc::Opcode::SCATTER, 6, 1, pack_reg_pair(4, 3)});
    p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(p);
    auto res = vm->run_to_halt();
    T81_TEST_CHECK(!res.has_value());
    T81_TEST_CHECK(res.error() == t81::vm::Trap::SecurityFault);

    // Verify the Axion deny event is present in the log.
    const auto& log = vm->state().axion_log;
    const bool has_alias_deny =
        std::any_of(log.begin(), log.end(), [](const t81::vm::AxionEvent& ev) {
          return ev.verdict.kind == t81::axion::VerdictKind::Deny &&
                 ev.verdict.reason.find("aliasing") != std::string::npos;
        });
    T81_TEST_CHECK(has_alias_deny);

    std::cout << "AI_M5_ALIASING SCATTER same_row_alias=SecurityFault\n";
  }

  return 0;
}
