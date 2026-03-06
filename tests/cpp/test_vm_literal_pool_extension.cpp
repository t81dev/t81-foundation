#include <cassert>
#include <cmath>
#include <iostream>

#include <t81/isa/program.hpp>
#include <t81/vm/state.hpp>
#include <t81/vm/vm.hpp>

using namespace t81::tisc;
using namespace t81::vm;

void test_i2f_deterministic_extension() {
  std::cout << "Testing I2F deterministic pool extension..." << std::endl;

  // Manually construct a program that performs I2F conversions
  t81::tisc::Program program;

  // Instructions:
  // 1. Load Imm 42 into R1
  // 2. I2F R1 -> R2 (should create a float in pool)
  // 3. I2F R1 -> R3 (should create another float in pool, even if same value)
  // 4. Load Imm 100 into R4
  // 5. I2F R4 -> R5 (should create another float in pool)
  // 6. Halt

  program.insns.push_back({Opcode::LoadImm, 1, 42, 0, LiteralKind::Int});
  program.insns.push_back({Opcode::I2F, 2, 1, 0, LiteralKind::Int});
  program.insns.push_back({Opcode::I2F, 3, 1, 0, LiteralKind::Int});
  program.insns.push_back({Opcode::LoadImm, 4, 100, 0, LiteralKind::Int});
  program.insns.push_back({Opcode::I2F, 5, 4, 0, LiteralKind::Int});
  program.insns.push_back({Opcode::Halt, 0, 0, 0, LiteralKind::Int});

  auto vm = make_interpreter_vm();
  vm->load_program(program);

  // Initial state check
  [[maybe_unused]] const auto& state = vm->state();
  assert(state.floats.empty());

  // Run step by step or run to halt
  [[maybe_unused]] auto res = vm->run_to_halt();
  assert(res.has_value());

  // Check state after run
  // We expect 3 floats in the pool: 42.0, 42.0, 100.0
  assert(state.floats.size() == 3);

  // Verify values
  assert(std::fabs(state.floats[0] - 42.0) < 1e-9);
  assert(std::fabs(state.floats[1] - 42.0) < 1e-9);
  assert(std::fabs(state.floats[2] - 100.0) < 1e-9);

  // Verify registers point to correct handles (1-based index)
  assert(state.contexts[0].registers[2] == 1);  // handle to first float
  assert(state.contexts[0].register_tags[2] == ValueTag::FloatHandle);

  assert(state.contexts[0].registers[3] == 2);  // handle to second float
  assert(state.contexts[0].register_tags[3] == ValueTag::FloatHandle);

  assert(state.contexts[0].registers[5] == 3);  // handle to third float
  assert(state.contexts[0].register_tags[5] == ValueTag::FloatHandle);

  std::cout << "I2F deterministic pool extension passed." << std::endl;
}

void test_i2frac_deterministic_extension() {
  std::cout << "Testing I2Frac deterministic pool extension..." << std::endl;

  t81::tisc::Program program;

  // Instructions:
  // 1. Load Imm 3 into R1
  // 2. I2Frac R1 -> R2 (3/1)
  // 3. I2Frac R1 -> R3 (3/1)
  // 4. Halt

  program.insns.push_back({Opcode::LoadImm, 1, 3, 0, LiteralKind::Int});
  program.insns.push_back({Opcode::I2Frac, 2, 1, 0, LiteralKind::Int});
  program.insns.push_back({Opcode::I2Frac, 3, 1, 0, LiteralKind::Int});
  program.insns.push_back({Opcode::Halt, 0, 0, 0, LiteralKind::Int});

  auto vm = make_interpreter_vm();
  vm->load_program(program);

  [[maybe_unused]] const auto& state = vm->state();
  assert(state.fractions.empty());

  [[maybe_unused]] auto res = vm->run_to_halt();
  assert(res.has_value());

  // Expect 2 fractions
  assert(state.fractions.size() == 2);

  // Verify values
  assert(state.fractions[0].num.to_int64() == 3);
  assert(state.fractions[0].den.to_int64() == 1);

  assert(state.fractions[1].num.to_int64() == 3);
  assert(state.fractions[1].den.to_int64() == 1);

  // Verify registers
  assert(state.contexts[0].registers[2] == 1);
  assert(state.contexts[0].register_tags[2] == ValueTag::FractionHandle);

  assert(state.contexts[0].registers[3] == 2);
  assert(state.contexts[0].register_tags[3] == ValueTag::FractionHandle);

  std::cout << "I2Frac deterministic pool extension passed." << std::endl;
}

void test_loadimm_bigint_handle_extension() {
  std::cout << "Testing LOADI BigIntHandle deterministic extension..." << std::endl;

  t81::tisc::Program program;
  program.bigint_pool.push_back(t81::T81BigInt::from_i64(9223372036854775807LL) +
                                t81::T81BigInt::one());

  program.insns.push_back({Opcode::LoadImm, 1, 1, 0, LiteralKind::BigIntHandle});
  program.insns.push_back({Opcode::Halt, 0, 0, 0, LiteralKind::Int});

  auto vm = make_interpreter_vm();
  vm->load_program(program);

  [[maybe_unused]] const auto& state = vm->state();
  assert(state.bigints.size() == 1);
  assert(state.fractions.empty());

  [[maybe_unused]] auto res = vm->run_to_halt();
  assert(res.has_value());

  assert(state.contexts[0].registers[1] == 1);
  assert(state.contexts[0].register_tags[1] == ValueTag::BigIntHandle);
  assert(state.bigints[0].to_string() == "9223372036854775808");

  std::cout << "LOADI BigIntHandle deterministic extension passed." << std::endl;
}

void test_frac2i_bigint_overflow_preserves_integer_class() {
  std::cout << "Testing Frac2I BigInt overflow integer preservation..." << std::endl;

  t81::tisc::Program program;
  program.fraction_pool.emplace_back(
      t81::T81BigInt::from_i64(9223372036854775807LL) + t81::T81BigInt::one(),
      t81::T81BigInt::one());
  program.insns.push_back({Opcode::LoadImm, 1, 1, 0, LiteralKind::FractionHandle});
  program.insns.push_back({Opcode::Frac2I, 2, 1, 0, LiteralKind::Int});
  program.insns.push_back({Opcode::Halt, 0, 0, 0, LiteralKind::Int});

  auto vm = make_interpreter_vm();
  vm->load_program(program);

  [[maybe_unused]] auto res = vm->run_to_halt();
  assert(res.has_value());
  [[maybe_unused]] const auto& state = vm->state();
  assert(state.contexts[0].register_tags[2] == ValueTag::BigIntHandle);
  assert(state.contexts[0].registers[2] == 1);
  assert(state.bigints[0].to_string() == "9223372036854775808");

  std::cout << "Frac2I BigInt overflow integer preservation passed." << std::endl;
}

void test_bigint_integer_arithmetic_extension() {
  std::cout << "Testing BigInt integer arithmetic extension..." << std::endl;

  t81::tisc::Program program;
  program.bigint_pool.push_back(t81::T81BigInt::from_i64(9223372036854775807LL) + t81::T81BigInt::one());
  program.insns.push_back({Opcode::LoadImm, 1, 1, 0, LiteralKind::BigIntHandle});
  program.insns.push_back({Opcode::LoadImm, 2, 2, 0, LiteralKind::Int});
  program.insns.push_back({Opcode::Add, 3, 1, 2, LiteralKind::Int});
  program.insns.push_back({Opcode::Mul, 4, 3, 2, LiteralKind::Int});
  program.insns.push_back({Opcode::Halt, 0, 0, 0, LiteralKind::Int});

  auto vm = make_interpreter_vm();
  vm->load_program(program);

  [[maybe_unused]] auto res = vm->run_to_halt();
  assert(res.has_value());

  [[maybe_unused]] const auto& state = vm->state();
  assert(state.contexts[0].register_tags[3] == ValueTag::BigIntHandle);
  assert(state.contexts[0].register_tags[4] == ValueTag::BigIntHandle);
  assert(state.bigints[1].to_string() == "9223372036854775810");
  assert(state.bigints[2].to_string() == "18446744073709551620");

  std::cout << "BigInt integer arithmetic extension passed." << std::endl;
}

void test_int2bigint_materializes_handle() {
  std::cout << "Testing Int2BigInt materialization..." << std::endl;

  t81::tisc::Program program;
  program.insns.push_back({Opcode::LoadImm, 1, 42, 0, LiteralKind::Int});
  program.insns.push_back({Opcode::Int2BigInt, 2, 1, 0, LiteralKind::Int});
  program.insns.push_back({Opcode::Halt, 0, 0, 0, LiteralKind::Int});

  auto vm = make_interpreter_vm();
  vm->load_program(program);

  [[maybe_unused]] auto res = vm->run_to_halt();
  assert(res.has_value());

  [[maybe_unused]] const auto& state = vm->state();
  assert(state.bigints.size() == 1);
  assert(state.contexts[0].register_tags[2] == ValueTag::BigIntHandle);
  assert(state.contexts[0].registers[2] == 1);
  assert(state.bigints[0].to_string() == "42");

  std::cout << "Int2BigInt materialization passed." << std::endl;
}

void test_bigint_arithmetic_preserves_bigint_tag_for_small_results() {
  std::cout << "Testing BigInt arithmetic tag preservation for small results..." << std::endl;

  t81::tisc::Program program;
  program.bigint_pool.push_back(t81::T81BigInt::from_i64(1));
  program.bigint_pool.push_back(t81::T81BigInt::from_i64(-1));
  program.insns.push_back({Opcode::LoadImm, 1, 1, 0, LiteralKind::BigIntHandle});
  program.insns.push_back({Opcode::LoadImm, 2, 2, 0, LiteralKind::BigIntHandle});
  program.insns.push_back({Opcode::Add, 3, 1, 2, LiteralKind::Int});
  program.insns.push_back({Opcode::Halt, 0, 0, 0, LiteralKind::Int});

  auto vm = make_interpreter_vm();
  vm->load_program(program);

  [[maybe_unused]] auto res = vm->run_to_halt();
  assert(res.has_value());

  [[maybe_unused]] const auto& state = vm->state();
  assert(state.contexts[0].register_tags[3] == ValueTag::BigIntHandle);
  assert(state.bigints[2].to_string() == "0");

  std::cout << "BigInt arithmetic tag preservation passed." << std::endl;
}

int main() {
  test_i2f_deterministic_extension();
  test_i2frac_deterministic_extension();
  test_loadimm_bigint_handle_extension();
  test_frac2i_bigint_overflow_preserves_integer_class();
  test_bigint_integer_arithmetic_extension();
  test_int2bigint_materializes_handle();
  test_bigint_arithmetic_preserves_bigint_tag_for_small_results();
  return 0;
}
