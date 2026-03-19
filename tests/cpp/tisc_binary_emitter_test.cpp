#include <cassert>
#include <iostream>
#include "t81/isa/binary_emitter.hpp"
#include "t81/isa/ir.hpp"
#include "t81/isa/pretty_printer.hpp"

using namespace t81::tisc::ir;

void test_simple_program() {
  [[maybe_unused]] IntermediateProgram ir_program;
  ir_program.add_instruction({Opcode::LOADI, {Register{0}, Immediate{10}}});
  ir_program.add_instruction({Opcode::HALT, {}});

  [[maybe_unused]] t81::tisc::BinaryEmitter emitter;
  [[maybe_unused]] auto program = emitter.emit(ir_program);

  assert(program.insns.size() == 2);
  assert(program.insns[0].opcode == t81::tisc::Opcode::LoadImm);
  assert(program.insns[0].a == 0);
  assert(program.insns[0].b == 10);
  assert(program.insns[1].opcode == t81::tisc::Opcode::Halt);

  std::cout << "BinaryEmitterTest test_simple_program passed!" << std::endl;
}

void test_jump() {
  [[maybe_unused]] IntermediateProgram ir_program;
  ir_program.add_instruction({Opcode::JMP, {Label{0}}});
  ir_program.add_instruction({Opcode::LABEL, {Label{0}}});
  ir_program.add_instruction({Opcode::HALT, {}});

  [[maybe_unused]] t81::tisc::BinaryEmitter emitter;
  [[maybe_unused]] auto program = emitter.emit(ir_program);

  assert(program.insns.size() == 2);
  assert(program.insns[0].opcode == t81::tisc::Opcode::Jump);
  assert(program.insns[0].a == 1);  // address of HALT
  assert(program.insns[1].opcode == t81::tisc::Opcode::Halt);

  std::cout << "BinaryEmitterTest test_jump passed!" << std::endl;
}

void test_comparison_relation() {
  [[maybe_unused]] IntermediateProgram ir_program;
  Instruction cmp{Opcode::CMP, {Register{0}, Register{1}, Register{2}}};
  cmp.boolean_result = true;
  cmp.relation = ComparisonRelation::LessEqual;
  ir_program.add_instruction(cmp);
  ir_program.add_instruction({Opcode::HALT, {}});

  [[maybe_unused]] t81::tisc::BinaryEmitter emitter;
  [[maybe_unused]] auto program = emitter.emit(ir_program);

  assert(program.insns.size() == 2);
  assert(program.insns[0].opcode == t81::tisc::Opcode::LessEqual);
  assert(program.insns[0].a == 0);
  assert(program.insns[0].b == 1);
  assert(program.insns[0].c == 2);

  std::cout << "BinaryEmitterTest test_comparison_relation passed!" << std::endl;
}

void test_all_comparison_relations() {
  std::vector<std::pair<ComparisonRelation, t81::tisc::Opcode>> cases = {
      {ComparisonRelation::Less, t81::tisc::Opcode::Less},
      {ComparisonRelation::LessEqual, t81::tisc::Opcode::LessEqual},
      {ComparisonRelation::Greater, t81::tisc::Opcode::Greater},
      {ComparisonRelation::GreaterEqual, t81::tisc::Opcode::GreaterEqual},
      {ComparisonRelation::Equal, t81::tisc::Opcode::Equal},
      {ComparisonRelation::NotEqual, t81::tisc::Opcode::NotEqual},
  };

  for (const auto& [relation, expected_opcode] : cases) {
    [[maybe_unused]] IntermediateProgram ir_program;
    Instruction cmp{Opcode::CMP, {Register{0}, Register{1}, Register{2}}};
    cmp.boolean_result = true;
    cmp.relation = relation;
    ir_program.add_instruction(cmp);
    ir_program.add_instruction({Opcode::HALT, {}});

    [[maybe_unused]] t81::tisc::BinaryEmitter emitter;
    [[maybe_unused]] auto program = emitter.emit(ir_program);

    assert(program.insns.size() == 2);
    assert(program.insns[0].opcode == expected_opcode);
    assert(program.insns[0].a == 0);
    assert(program.insns[0].b == 1);
    assert(program.insns[0].c == 2);
  }

  std::cout << "BinaryEmitterTest test_all_comparison_relations passed!" << std::endl;
}

void test_print_opcode_mapping() {
  IntermediateProgram ir_program;
  ir_program.add_instruction({Opcode::PRINT, {Register{3}}});
  ir_program.add_instruction({Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  assert(program.insns.size() == 2);
  assert(program.insns[0].opcode == t81::tisc::Opcode::Print);
  assert(program.insns[0].a == 3);
  assert(program.insns[1].opcode == t81::tisc::Opcode::Halt);

  std::cout << "BinaryEmitterTest test_print_opcode_mapping passed!" << std::endl;
}

void test_float_literal_pool_mapping() {
  IntermediateProgram ir_program;
  Instruction load_float{Opcode::LOADI, {Register{1}}};
  load_float.literal_kind = t81::tisc::LiteralKind::FloatHandle;
  load_float.text_literal = "1.25";
  ir_program.add_instruction(load_float);
  ir_program.add_instruction({Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  assert(program.float_pool.size() == 1);
  assert(program.float_pool[0] == 1.25);
  assert(program.insns[0].opcode == t81::tisc::Opcode::LoadImm);
  assert(program.insns[0].literal_kind == t81::tisc::LiteralKind::FloatHandle);
  assert(program.insns[0].b == 1);

  std::cout << "BinaryEmitterTest test_float_literal_pool_mapping passed!" << std::endl;
}

void test_bigint_literal_pool_mapping() {
  IntermediateProgram ir_program;
  Instruction load_bigint{Opcode::LOADI, {Register{1}}};
  load_bigint.literal_kind = t81::tisc::LiteralKind::BigIntHandle;
  load_bigint.text_literal = "9223372036854775808";
  ir_program.add_instruction(load_bigint);
  ir_program.add_instruction({Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  assert(program.bigint_pool.size() == 1);
  assert(program.bigint_pool[0].to_string() == "9223372036854775808");
  assert(program.insns[0].opcode == t81::tisc::Opcode::LoadImm);
  assert(program.insns[0].literal_kind == t81::tisc::LiteralKind::BigIntHandle);
  assert(program.insns[0].b == 1);

  std::cout << "BinaryEmitterTest test_bigint_literal_pool_mapping passed!" << std::endl;
}

void test_ffi_call_symbol_pool_mapping() {
  IntermediateProgram ir_program;
  Instruction ffi_call{Opcode::FFI_CALL, {Register{7}, Immediate{3}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = "bridge_target";
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  assert(program.symbol_pool.size() == 1);
  assert(program.symbol_pool[0] == "bridge_target");
  assert(program.insns[0].opcode == t81::tisc::Opcode::FFICall);
  assert(program.insns[0].a == 7);
  assert(program.insns[0].b == 3);
  assert(program.insns[0].c == 1);
  assert(program.insns[0].literal_kind == t81::tisc::LiteralKind::SymbolHandle);

  std::cout << "BinaryEmitterTest test_ffi_call_symbol_pool_mapping passed!" << std::endl;
}

void test_string_opcode_mappings() {
  IntermediateProgram ir_program;
  ir_program.add_instruction({Opcode::STRCONCAT, {Register{1}, Register{2}, Register{3}}});
  ir_program.add_instruction({Opcode::STRSTARTSWITH, {Register{4}, Register{5}, Register{6}}});
  ir_program.add_instruction({Opcode::STRENDSWITH, {Register{7}, Register{8}, Register{9}}});
  ir_program.add_instruction({Opcode::STRCONTAINS, {Register{10}, Register{11}, Register{12}}});
  ir_program.add_instruction({Opcode::STRINDEXOF, {Register{13}, Register{14}, Register{15}}});
  ir_program.add_instruction({Opcode::STRREPLACE, {Register{16}, Register{17}, Register{18}}});
  ir_program.add_instruction({Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  assert(program.insns.size() == 7);
  assert(program.insns[0].opcode == t81::tisc::Opcode::StrConcat);
  assert(program.insns[1].opcode == t81::tisc::Opcode::StrStartsWith);
  assert(program.insns[2].opcode == t81::tisc::Opcode::StrEndsWith);
  assert(program.insns[3].opcode == t81::tisc::Opcode::StrContains);
  assert(program.insns[4].opcode == t81::tisc::Opcode::StrIndexOf);
  assert(program.insns[5].opcode == t81::tisc::Opcode::StrReplace);
  assert(program.insns[6].opcode == t81::tisc::Opcode::Halt);

  std::cout << "BinaryEmitterTest test_string_opcode_mappings passed!" << std::endl;
}

void test_bitwise_opcode_mappings() {
  IntermediateProgram ir_program;
  ir_program.add_instruction({Opcode::BITAND, {Register{1}, Register{2}, Register{3}}});
  ir_program.add_instruction({Opcode::BITOR, {Register{4}, Register{5}, Register{6}}});
  ir_program.add_instruction({Opcode::BITXOR, {Register{7}, Register{8}, Register{9}}});
  ir_program.add_instruction({Opcode::BITNOT, {Register{10}, Register{11}, Register{12}}});
  ir_program.add_instruction({Opcode::BITSHL, {Register{13}, Register{14}, Register{15}}});
  ir_program.add_instruction({Opcode::BITSHR, {Register{16}, Register{17}, Register{18}}});
  ir_program.add_instruction({Opcode::BITUSHR, {Register{19}, Register{20}, Register{21}}});
  ir_program.add_instruction({Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  assert(program.insns.size() == 8);
  assert(program.insns[0].opcode == t81::tisc::Opcode::BitAnd);
  assert(program.insns[1].opcode == t81::tisc::Opcode::BitOr);
  assert(program.insns[2].opcode == t81::tisc::Opcode::BitXor);
  assert(program.insns[3].opcode == t81::tisc::Opcode::BitNot);
  assert(program.insns[4].opcode == t81::tisc::Opcode::BitShl);
  assert(program.insns[5].opcode == t81::tisc::Opcode::BitShr);
  assert(program.insns[6].opcode == t81::tisc::Opcode::BitUShr);
  assert(program.insns[7].opcode == t81::tisc::Opcode::Halt);

  std::cout << "BinaryEmitterTest test_bitwise_opcode_mappings passed!" << std::endl;
}

void test_bitwise_pretty_printer() {
  IntermediateProgram ir_program;
  ir_program.add_instruction({Opcode::BITAND, {Register{1}, Register{2}, Register{3}}});
  ir_program.add_instruction({Opcode::BITOR, {Register{4}, Register{5}, Register{6}}});
  ir_program.add_instruction({Opcode::BITXOR, {Register{7}, Register{8}, Register{9}}});
  ir_program.add_instruction({Opcode::BITNOT, {Register{10}, Register{11}, Register{12}}});
  ir_program.add_instruction({Opcode::BITSHL, {Register{13}, Register{14}, Register{15}}});
  ir_program.add_instruction({Opcode::BITSHR, {Register{16}, Register{17}, Register{18}}});
  ir_program.add_instruction({Opcode::BITUSHR, {Register{19}, Register{20}, Register{21}}});
  ir_program.add_instruction({Opcode::HALT, {}});

  std::string output = t81::tisc::pretty_print(ir_program);

  assert(output.find("BitAnd") != std::string::npos);
  assert(output.find("BitOr") != std::string::npos);
  assert(output.find("BitXor") != std::string::npos);
  assert(output.find("BitNot") != std::string::npos);
  assert(output.find("BitShl") != std::string::npos);
  assert(output.find("BitShr") != std::string::npos);
  assert(output.find("BitUShr") != std::string::npos);

  std::cout << "BinaryEmitterTest test_bitwise_pretty_printer passed!" << std::endl;
}

int main() {
  test_simple_program();
  test_jump();
  test_comparison_relation();
  test_all_comparison_relations();
  test_print_opcode_mapping();
  test_float_literal_pool_mapping();
  test_bigint_literal_pool_mapping();
  test_ffi_call_symbol_pool_mapping();
  test_string_opcode_mappings();
  test_bitwise_opcode_mappings();
  test_bitwise_pretty_printer();
  return 0;
}
