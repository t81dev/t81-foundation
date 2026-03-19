#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include "t81/axion/engine.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/ffi/ffi.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/isa/ir.hpp"
#include "t81/vm/traps.hpp"
#include "t81/vm/vm.hpp"

namespace {

#ifndef T81_VM_FFI_TESTLIB_PATH
#define T81_VM_FFI_TESTLIB_PATH ""
#endif

std::string unavailable_library_name() {
#ifdef __APPLE__
  return "/usr/lib/libt81_vm_ffi_missing.dylib";
#elif defined(_WIN32)
  return "t81_vm_ffi_missing.dll";
#else
  return "/usr/lib/libt81_vm_ffi_missing.so";
#endif
}

std::string system_library_name() {
#ifdef __APPLE__
  return "/usr/lib/libSystem.B.dylib";
#elif defined(_WIN32)
  return "kernel32.dll";
#else
  return "libc.so.6";
#endif
}

std::string system_success_function_name() {
#ifdef _WIN32
  return "GetCurrentProcessId";
#else
  return "getpid";
#endif
}

std::string unary_system_function_name() {
#ifdef _WIN32
  return "GetCurrentProcessId";
#else
  return "llabs";
#endif
}

std::string system_string_function_name() {
#ifdef _WIN32
  return "lstrlenA";
#else
  return "strlen";
#endif
}

std::string system_double_function_name() {
#ifdef _WIN32
  return "";
#else
  return "fabs";
#endif
}

std::string binary_test_library_name() { return T81_VM_FFI_TESTLIB_PATH; }

std::string binary_test_function_name() { return "t81_ffi_add_i64"; }

std::string string_arg_test_function_name() { return "t81_ffi_strlen_bridge"; }

std::string string_result_test_function_name() { return "t81_ffi_hello_bridge"; }

std::string double_arg_test_function_name() { return "t81_ffi_half_double"; }

std::string double_result_test_function_name() { return "t81_ffi_fixed_double"; }

std::string bytes_arg_test_function_name() { return "t81_ffi_bytes_checksum"; }

std::string bytes_result_test_function_name() { return "t81_ffi_bytes_result_bridge"; }

std::string mixed_arg_test_function_name() { return "t81_ffi_mix_i64_strlen"; }

std::string string_list_result_test_function_name() { return "t81_ffi_pair_strings_bridge"; }

std::string string_list_arg_test_function_name() { return "t81_ffi_string_list_total_len"; }

std::string quarantined_test_function_name() { return "t81_ffi_quarantined_probe"; }

void register_bridge_target() {
  auto& registry = t81::ffi::FFILibraryRegistry::instance();
  auto result = registry.register_library(
      unavailable_library_name(),
      "bridge-test-v1",
      {t81::ffi::FFIFunction{
          .name = "bridge_target",
          .type = t81::ffi::FFIType::Deterministic,
          .library_name = unavailable_library_name(),
          .version_hash = "bridge-test-v1",
          .param_types = {},
          .return_type = "uint64_t",
          .is_variadic = false,
          .policy_reason = "VM bridge regression",
          .resource_quota = 64,
          .required_capabilities = {},
      }});
  if (!result && result.error().find("already registered") == std::string::npos) {
    std::cerr << "unexpected FFI registry error: " << result.error() << "\n";
    std::abort();
  }
}

void register_success_target() {
  auto& registry = t81::ffi::FFILibraryRegistry::instance();
  auto result = registry.register_library(
      system_library_name(),
      "bridge-success-v1",
      {
          t81::ffi::FFIFunction{
              .name = system_success_function_name(),
              .type = t81::ffi::FFIType::Deterministic,
              .library_name = system_library_name(),
              .version_hash = "bridge-success-v1",
              .param_types = {},
              .return_type = "uint64_t",
              .is_variadic = false,
              .policy_reason = "VM bridge success regression",
              .resource_quota = 64,
              .required_capabilities = {},
          },
#ifndef _WIN32
          t81::ffi::FFIFunction{
              .name = unary_system_function_name(),
              .type = t81::ffi::FFIType::Deterministic,
              .library_name = system_library_name(),
              .version_hash = "bridge-success-v1",
              .param_types = {"int64_t"},
              .return_type = "int64_t",
              .is_variadic = false,
              .policy_reason = "VM bridge unary success regression",
              .resource_quota = 64,
              .required_capabilities = {},
          },
          t81::ffi::FFIFunction{
              .name = system_string_function_name(),
              .type = t81::ffi::FFIType::Deterministic,
              .library_name = system_library_name(),
              .version_hash = "bridge-success-v1",
              .param_types = {"string"},
              .return_type = "int64_t",
              .is_variadic = false,
              .policy_reason = "VM bridge external string regression",
              .resource_quota = 64,
              .required_capabilities = {},
          },
          t81::ffi::FFIFunction{
              .name = system_double_function_name(),
              .type = t81::ffi::FFIType::Deterministic,
              .library_name = system_library_name(),
              .version_hash = "bridge-success-v1",
              .param_types = {"double"},
              .return_type = "double",
              .is_variadic = false,
              .policy_reason = "VM bridge external double regression",
              .resource_quota = 64,
              .required_capabilities = {},
          },
#endif
      });
  if (!result && result.error().find("already registered") == std::string::npos) {
    std::cerr << "unexpected FFI registry error: " << result.error() << "\n";
    std::abort();
  }
}

void register_unary_success_target() {
  register_success_target();
}

void register_binary_success_target() {
  auto& registry = t81::ffi::FFILibraryRegistry::instance();
  auto result = registry.register_library(
      binary_test_library_name(),
      "bridge-binary-v1",
      {t81::ffi::FFIFunction{
          .name = binary_test_function_name(),
          .type = t81::ffi::FFIType::Deterministic,
          .library_name = binary_test_library_name(),
          .version_hash = "bridge-binary-v1",
          .param_types = {"int64_t", "int64_t"},
          .return_type = "int64_t",
          .is_variadic = false,
          .policy_reason = "VM bridge binary success regression",
          .resource_quota = 64,
          .required_capabilities = {},
      },
       t81::ffi::FFIFunction{
          .name = string_arg_test_function_name(),
          .type = t81::ffi::FFIType::Deterministic,
          .library_name = binary_test_library_name(),
          .version_hash = "bridge-binary-v1",
          .param_types = {"string"},
          .return_type = "int64_t",
          .is_variadic = false,
          .policy_reason = "VM bridge string-arg regression",
          .resource_quota = 64,
          .required_capabilities = {},
      },
       t81::ffi::FFIFunction{
          .name = string_result_test_function_name(),
          .type = t81::ffi::FFIType::Deterministic,
          .library_name = binary_test_library_name(),
          .version_hash = "bridge-binary-v1",
          .param_types = {},
          .return_type = "string",
          .is_variadic = false,
          .policy_reason = "VM bridge string-result regression",
          .resource_quota = 64,
          .required_capabilities = {},
      },
       t81::ffi::FFIFunction{
          .name = double_arg_test_function_name(),
          .type = t81::ffi::FFIType::Deterministic,
          .library_name = binary_test_library_name(),
          .version_hash = "bridge-binary-v1",
          .param_types = {"double"},
          .return_type = "double",
          .is_variadic = false,
          .policy_reason = "VM bridge double-arg regression",
          .resource_quota = 64,
          .required_capabilities = {},
      },
       t81::ffi::FFIFunction{
          .name = double_result_test_function_name(),
          .type = t81::ffi::FFIType::Deterministic,
          .library_name = binary_test_library_name(),
          .version_hash = "bridge-binary-v1",
          .param_types = {},
          .return_type = "double",
          .is_variadic = false,
          .policy_reason = "VM bridge double-result regression",
          .resource_quota = 64,
          .required_capabilities = {},
      },
       t81::ffi::FFIFunction{
          .name = bytes_arg_test_function_name(),
          .type = t81::ffi::FFIType::Deterministic,
          .library_name = binary_test_library_name(),
          .version_hash = "bridge-binary-v1",
          .param_types = {"bytes"},
          .return_type = "int64_t",
          .is_variadic = false,
          .policy_reason = "VM bridge bytes-arg regression",
          .resource_quota = 64,
          .required_capabilities = {},
      },
       t81::ffi::FFIFunction{
          .name = bytes_result_test_function_name(),
          .type = t81::ffi::FFIType::Deterministic,
          .library_name = binary_test_library_name(),
          .version_hash = "bridge-binary-v1",
          .param_types = {},
          .return_type = "bytes",
          .is_variadic = false,
          .policy_reason = "VM bridge bytes-result regression",
          .resource_quota = 64,
          .required_capabilities = {},
      },
       t81::ffi::FFIFunction{
          .name = mixed_arg_test_function_name(),
          .type = t81::ffi::FFIType::Deterministic,
          .library_name = binary_test_library_name(),
          .version_hash = "bridge-binary-v1",
          .param_types = {"int64_t", "string"},
          .return_type = "int64_t",
          .is_variadic = false,
          .policy_reason = "VM bridge mixed-arg regression",
          .resource_quota = 64,
          .required_capabilities = {},
      },
       t81::ffi::FFIFunction{
          .name = string_list_result_test_function_name(),
          .type = t81::ffi::FFIType::Deterministic,
          .library_name = binary_test_library_name(),
          .version_hash = "bridge-binary-v1",
          .param_types = {},
          .return_type = "string[]",
          .is_variadic = false,
          .policy_reason = "VM bridge string-list-result regression",
          .resource_quota = 64,
          .required_capabilities = {},
      },
       t81::ffi::FFIFunction{
          .name = string_list_arg_test_function_name(),
          .type = t81::ffi::FFIType::Deterministic,
          .library_name = binary_test_library_name(),
          .version_hash = "bridge-binary-v1",
          .param_types = {"string[]"},
          .return_type = "int64_t",
          .is_variadic = false,
          .policy_reason = "VM bridge string-list-arg regression",
          .resource_quota = 64,
          .required_capabilities = {},
      },
       t81::ffi::FFIFunction{
          .name = quarantined_test_function_name(),
          .type = t81::ffi::FFIType::Quarantined,
          .library_name = binary_test_library_name(),
          .version_hash = "bridge-binary-v1",
          .param_types = {},
          .return_type = "int64_t",
          .is_variadic = false,
          .policy_reason = "VM bridge quarantine regression",
          .resource_quota = 64,
          .required_capabilities = {},
      }});
  if (!result && result.error().find("already registered") == std::string::npos) {
    std::cerr << "unexpected FFI registry error: " << result.error() << "\n";
    std::abort();
  }
}

void test_vm_ffi_call_uses_encoded_symbol() {
  register_bridge_target();

  t81::tisc::ir::IntermediateProgram ir_program;
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{1}, t81::tisc::ir::Immediate{0}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = "bridge_target";
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (result.has_value() || result.error() != t81::vm::Trap::FFILoadError) {
    std::cerr << "expected FFILoadError from registered bridge target\n";
    std::abort();
  }
}

void test_vm_ffi_call_success_writes_result_register() {
  register_success_target();

  t81::tisc::ir::IntermediateProgram ir_program;
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{3}, t81::tisc::ir::Immediate{0}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = system_success_function_name();
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful FFI call through VM bridge\n";
    std::abort();
  }

  const auto& state = vm->state();
  const auto& ctx = state.contexts[state.current_context];
  if (ctx.register_tags[3] != t81::vm::ValueTag::Int || ctx.registers[3] <= 0) {
    std::cerr << "expected positive Int result in destination register\n";
    std::abort();
  }
}

void test_vm_ffi_call_unary_argument_round_trip() {
  register_unary_success_target();

  t81::tisc::ir::IntermediateProgram ir_program;
#ifndef _WIN32
  ir_program.add_instruction(
      {t81::tisc::ir::Opcode::LOADI,
       {t81::tisc::ir::Register{1}, t81::tisc::ir::Immediate{-42}}});
  ir_program.add_instruction({t81::tisc::ir::Opcode::PUSH, {t81::tisc::ir::Register{1}}});
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{4}, t81::tisc::ir::Immediate{1}}};
#else
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{4}, t81::tisc::ir::Immediate{0}}};
#endif
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = unary_system_function_name();
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful unary FFI call through VM bridge, got trap "
              << t81::vm::to_string(result.error()) << "\n";
    std::abort();
  }

  const auto& state = vm->state();
  const auto& ctx = state.contexts[state.current_context];
  if (ctx.register_tags[4] != t81::vm::ValueTag::Int) {
    std::cerr << "expected Int result from unary FFI call\n";
    std::abort();
  }
#ifndef _WIN32
  if (ctx.registers[4] != 42) {
    std::cerr << "expected llabs(-42) to produce 42, got " << ctx.registers[4] << "\n";
    std::abort();
  }
#else
  if (ctx.registers[4] <= 0) {
    std::cerr << "expected positive process id from unary bridge fallback\n";
    std::abort();
  }
#endif
}

void test_vm_ffi_call_system_string_argument_round_trip() {
  register_success_target();

  t81::tisc::ir::IntermediateProgram ir_program;
  t81::tisc::ir::Instruction load_symbol{t81::tisc::ir::Opcode::LOADI,
                                         {t81::tisc::ir::Register{1}}};
  load_symbol.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  load_symbol.text_literal = "ffi-system";
  ir_program.add_instruction(load_symbol);
  ir_program.add_instruction({t81::tisc::ir::Opcode::PUSH, {t81::tisc::ir::Register{1}}});
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{17}, t81::tisc::ir::Immediate{1}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = system_string_function_name();
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful external-library string FFI call through VM bridge, got trap "
              << t81::vm::to_string(result.error()) << "\n";
    std::abort();
  }

  const auto& state = vm->state();
  const auto& ctx = state.contexts[state.current_context];
  if (ctx.register_tags[17] != t81::vm::ValueTag::Int || ctx.registers[17] != 10) {
    std::cerr << "expected system strlen/lstrlenA('ffi-system') to produce 10, got "
              << ctx.registers[17] << "\n";
    std::abort();
  }
}

#ifndef _WIN32
void test_vm_ffi_call_system_double_argument_round_trip() {
  register_success_target();

  t81::tisc::ir::IntermediateProgram ir_program;
  t81::tisc::ir::Instruction load_float{t81::tisc::ir::Opcode::LOADI,
                                        {t81::tisc::ir::Register{1}}};
  load_float.literal_kind = t81::tisc::LiteralKind::FloatHandle;
  load_float.text_literal = "-6.5";
  ir_program.add_instruction(load_float);
  ir_program.add_instruction({t81::tisc::ir::Opcode::PUSH, {t81::tisc::ir::Register{1}}});
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{18}, t81::tisc::ir::Immediate{1}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = system_double_function_name();
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful external-library double FFI call through VM bridge, got trap "
              << t81::vm::to_string(result.error()) << "\n";
    std::abort();
  }

  const auto& state = vm->state();
  const auto& ctx = state.contexts[state.current_context];
  if (ctx.register_tags[18] != t81::vm::ValueTag::FloatHandle || ctx.registers[18] <= 0) {
    std::cerr << "expected FloatHandle result from external-library double FFI call\n";
    std::abort();
  }
  const auto handle = static_cast<std::size_t>(ctx.registers[18] - 1);
  if (handle >= state.floats.size() || std::fabs(state.floats[handle] - 6.5) > 1e-12) {
    std::cerr << "expected system fabs(-6.5) to produce 6.5\n";
    std::abort();
  }
}
#endif

void test_vm_ffi_register_reads_symbol_registers() {
  t81::tisc::Program program;
  program.symbol_pool = {"bridge.lib", "deadbeef"};

  program.insns.push_back({t81::tisc::Opcode::LoadImm, 1, 1, 0,
                           t81::tisc::LiteralKind::SymbolHandle});
  program.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 2, 0,
                           t81::tisc::LiteralKind::SymbolHandle});
  program.insns.push_back(
      {t81::tisc::Opcode::FFIRegister, 1, 2, 0, t81::tisc::LiteralKind::Int});
  program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0,
                           t81::tisc::LiteralKind::Int});

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  auto& registry = t81::ffi::FFILibraryRegistry::instance();
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "FFIRegister should complete successfully\n";
    std::abort();
  }

  auto duplicate = registry.register_library("bridge.lib", "deadbeef", {});
  assert(!duplicate.has_value());
  assert(duplicate.error().find("already registered") != std::string::npos);
}

void test_vm_ffi_call_binary_argument_round_trip() {
  if (binary_test_library_name().empty()) {
    std::cerr << "missing ffi test library path\n";
    std::abort();
  }
  register_binary_success_target();

  t81::tisc::ir::IntermediateProgram ir_program;
  ir_program.add_instruction(
      {t81::tisc::ir::Opcode::LOADI,
       {t81::tisc::ir::Register{1}, t81::tisc::ir::Immediate{7}}});
  ir_program.add_instruction(
      {t81::tisc::ir::Opcode::LOADI,
       {t81::tisc::ir::Register{2}, t81::tisc::ir::Immediate{35}}});
  ir_program.add_instruction({t81::tisc::ir::Opcode::PUSH, {t81::tisc::ir::Register{1}}});
  ir_program.add_instruction({t81::tisc::ir::Opcode::PUSH, {t81::tisc::ir::Register{2}}});
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{5}, t81::tisc::ir::Immediate{2}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = binary_test_function_name();
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful binary FFI call through VM bridge, got trap "
              << t81::vm::to_string(result.error()) << "\n";
    std::abort();
  }

  const auto& state = vm->state();
  const auto& ctx = state.contexts[state.current_context];
  if (ctx.register_tags[5] != t81::vm::ValueTag::Int || ctx.registers[5] != 42) {
    std::cerr << "expected t81_ffi_add_i64(7, 35) to produce 42, got "
              << ctx.registers[5] << "\n";
    std::abort();
  }
}

void test_vm_ffi_call_string_argument_round_trip() {
  if (binary_test_library_name().empty()) {
    std::cerr << "missing ffi test library path\n";
    std::abort();
  }
  register_binary_success_target();

  t81::tisc::ir::IntermediateProgram ir_program;
  t81::tisc::ir::Instruction load_symbol{t81::tisc::ir::Opcode::LOADI,
                                         {t81::tisc::ir::Register{1}}};
  load_symbol.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  load_symbol.text_literal = "hello bridge";
  ir_program.add_instruction(load_symbol);
  ir_program.add_instruction({t81::tisc::ir::Opcode::PUSH, {t81::tisc::ir::Register{1}}});
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{6}, t81::tisc::ir::Immediate{1}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = string_arg_test_function_name();
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful string-arg FFI call through VM bridge, got trap "
              << t81::vm::to_string(result.error()) << "\n";
    std::abort();
  }

  const auto& state = vm->state();
  const auto& ctx = state.contexts[state.current_context];
  if (ctx.register_tags[6] != t81::vm::ValueTag::Int || ctx.registers[6] != 12) {
    std::cerr << "expected strlen('hello bridge') to produce 12, got "
              << ctx.registers[6] << "\n";
    std::abort();
  }
}

void test_vm_ffi_call_string_result_round_trip() {
  if (binary_test_library_name().empty()) {
    std::cerr << "missing ffi test library path\n";
    std::abort();
  }
  register_binary_success_target();

  t81::tisc::ir::IntermediateProgram ir_program;
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{7}, t81::tisc::ir::Immediate{0}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = string_result_test_function_name();
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful string-result FFI call through VM bridge, got trap "
              << t81::vm::to_string(result.error()) << "\n";
    std::abort();
  }

  const auto& state = vm->state();
  const auto& ctx = state.contexts[state.current_context];
  if (ctx.register_tags[7] != t81::vm::ValueTag::SymbolHandle) {
    std::cerr << "expected SymbolHandle result from string-returning FFI call\n";
    std::abort();
  }
  const auto handle = static_cast<std::size_t>(ctx.registers[7] - 1);
  if (ctx.registers[7] <= 0 || handle >= state.symbols.size() ||
      state.symbols[handle] != "bridge-ok") {
    std::cerr << "expected string result 'bridge-ok'\n";
    std::abort();
  }
}

void test_vm_ffi_call_double_argument_round_trip() {
  if (binary_test_library_name().empty()) {
    std::cerr << "missing ffi test library path\n";
    std::abort();
  }
  register_binary_success_target();

  t81::tisc::ir::IntermediateProgram ir_program;
  t81::tisc::ir::Instruction load_float{t81::tisc::ir::Opcode::LOADI,
                                        {t81::tisc::ir::Register{1}}};
  load_float.literal_kind = t81::tisc::LiteralKind::FloatHandle;
  load_float.text_literal = "6.5";
  ir_program.add_instruction(load_float);
  ir_program.add_instruction({t81::tisc::ir::Opcode::PUSH, {t81::tisc::ir::Register{1}}});
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{8}, t81::tisc::ir::Immediate{1}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = double_arg_test_function_name();
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful double-arg FFI call through VM bridge, got trap "
              << t81::vm::to_string(result.error()) << "\n";
    std::abort();
  }

  const auto& state = vm->state();
  const auto& ctx = state.contexts[state.current_context];
  if (ctx.register_tags[8] != t81::vm::ValueTag::FloatHandle || ctx.registers[8] <= 0) {
    std::cerr << "expected FloatHandle result from double-arg FFI call\n";
    std::abort();
  }
  const auto handle = static_cast<std::size_t>(ctx.registers[8] - 1);
  if (handle >= state.floats.size() || std::fabs(state.floats[handle] - 3.25) > 1e-12) {
    std::cerr << "expected half_double(6.5) to produce 3.25\n";
    std::abort();
  }
}

void test_vm_ffi_call_double_result_round_trip() {
  if (binary_test_library_name().empty()) {
    std::cerr << "missing ffi test library path\n";
    std::abort();
  }
  register_binary_success_target();

  t81::tisc::ir::IntermediateProgram ir_program;
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{9}, t81::tisc::ir::Immediate{0}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = double_result_test_function_name();
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful double-result FFI call through VM bridge, got trap "
              << t81::vm::to_string(result.error()) << "\n";
    std::abort();
  }

  const auto& state = vm->state();
  const auto& ctx = state.contexts[state.current_context];
  if (ctx.register_tags[9] != t81::vm::ValueTag::FloatHandle || ctx.registers[9] <= 0) {
    std::cerr << "expected FloatHandle result from double-returning FFI call\n";
    std::abort();
  }
  const auto handle = static_cast<std::size_t>(ctx.registers[9] - 1);
  if (handle >= state.floats.size() || std::fabs(state.floats[handle] - 1.5) > 1e-12) {
    std::cerr << "expected fixed_double() to produce 1.5\n";
    std::abort();
  }
}

void test_vm_ffi_call_bytes_argument_round_trip() {
  if (binary_test_library_name().empty()) {
    std::cerr << "missing ffi test library path\n";
    std::abort();
  }
  register_binary_success_target();

  t81::tisc::Program program;
  program.symbol_pool.emplace_back(std::string("A\0B\x7f", 4));
  program.symbol_pool.emplace_back(bytes_arg_test_function_name());
  program.insns.push_back({t81::tisc::Opcode::LoadImm, 1, 1, 0,
                           t81::tisc::LiteralKind::SymbolHandle});
  program.insns.push_back({t81::tisc::Opcode::Push, 1, 0, 0,
                           t81::tisc::LiteralKind::Int});
  program.insns.push_back({t81::tisc::Opcode::FFICall, 10, 1, 2,
                           t81::tisc::LiteralKind::SymbolHandle});
  program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0,
                           t81::tisc::LiteralKind::Int});

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful bytes-arg FFI call through VM bridge, got trap "
              << t81::vm::to_string(result.error()) << "\n";
    std::abort();
  }

  const auto& state = vm->state();
  const auto& ctx = state.contexts[state.current_context];
  if (ctx.register_tags[10] != t81::vm::ValueTag::Int || ctx.registers[10] != 258) {
    std::cerr << "expected bytes checksum 258, got " << ctx.registers[10] << "\n";
    std::abort();
  }
}

void test_vm_ffi_call_bytes_result_round_trip() {
  if (binary_test_library_name().empty()) {
    std::cerr << "missing ffi test library path\n";
    std::abort();
  }
  register_binary_success_target();

  t81::tisc::ir::IntermediateProgram ir_program;
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{11}, t81::tisc::ir::Immediate{0}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = bytes_result_test_function_name();
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful bytes-result FFI call through VM bridge, got trap "
              << t81::vm::to_string(result.error()) << "\n";
    std::abort();
  }

  const auto& state = vm->state();
  const auto& ctx = state.contexts[state.current_context];
  if (ctx.register_tags[11] != t81::vm::ValueTag::SymbolHandle || ctx.registers[11] <= 0) {
    std::cerr << "expected SymbolHandle result from bytes-returning FFI call\n";
    std::abort();
  }
  const auto handle = static_cast<std::size_t>(ctx.registers[11] - 1);
  const std::string expected("A\0B\x7f", 4);
  if (handle >= state.symbols.size() || state.symbols[handle] != expected) {
    std::cerr << "expected exact bytes result payload round trip\n";
    std::abort();
  }
}

void test_vm_ffi_call_mixed_argument_round_trip() {
  if (binary_test_library_name().empty()) {
    std::cerr << "missing ffi test library path\n";
    std::abort();
  }
  register_binary_success_target();

  t81::tisc::ir::IntermediateProgram ir_program;
  ir_program.add_instruction(
      {t81::tisc::ir::Opcode::LOADI,
       {t81::tisc::ir::Register{1}, t81::tisc::ir::Immediate{30}}});
  t81::tisc::ir::Instruction load_symbol{t81::tisc::ir::Opcode::LOADI,
                                         {t81::tisc::ir::Register{2}}};
  load_symbol.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  load_symbol.text_literal = "bridge";
  ir_program.add_instruction(load_symbol);
  ir_program.add_instruction({t81::tisc::ir::Opcode::PUSH, {t81::tisc::ir::Register{1}}});
  ir_program.add_instruction({t81::tisc::ir::Opcode::PUSH, {t81::tisc::ir::Register{2}}});
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{12}, t81::tisc::ir::Immediate{2}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = mixed_arg_test_function_name();
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful mixed-arg FFI call through VM bridge, got trap "
              << t81::vm::to_string(result.error()) << "\n";
    std::abort();
  }

  const auto& state = vm->state();
  const auto& ctx = state.contexts[state.current_context];
  if (ctx.register_tags[12] != t81::vm::ValueTag::Int || ctx.registers[12] != 36) {
    std::cerr << "expected mixed-arg result 36, got " << ctx.registers[12] << "\n";
    std::abort();
  }
}

void test_vm_ffi_call_string_list_result_round_trip() {
  if (binary_test_library_name().empty()) {
    std::cerr << "missing ffi test library path\n";
    std::abort();
  }
  register_binary_success_target();

  t81::tisc::ir::IntermediateProgram ir_program;
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{13}, t81::tisc::ir::Immediate{0}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = string_list_result_test_function_name();
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful string-list-result FFI call through VM bridge, got trap "
              << t81::vm::to_string(result.error()) << "\n";
    std::abort();
  }

  const auto& state = vm->state();
  const auto& ctx = state.contexts[state.current_context];
  if (ctx.register_tags[13] != t81::vm::ValueTag::StringVectorHandle ||
      ctx.registers[13] <= 0) {
    std::cerr << "expected StringVectorHandle result from structured FFI call\n";
    std::abort();
  }
  const auto handle = static_cast<std::size_t>(ctx.registers[13] - 1);
  if (handle >= state.string_vectors.size() || state.string_vectors[handle].size() != 2 ||
      state.string_vectors[handle][0] != "alpha" || state.string_vectors[handle][1] != "beta") {
    std::cerr << "expected structured string-vector result ['alpha', 'beta']\n";
    std::abort();
  }
}

void test_vm_ffi_call_string_list_argument_round_trip() {
  if (binary_test_library_name().empty()) {
    std::cerr << "missing ffi test library path\n";
    std::abort();
  }
  register_binary_success_target();

  t81::tisc::Program program;
  program.symbol_pool = {"alpha", "beta", string_list_arg_test_function_name()};
  program.insns.push_back({t81::tisc::Opcode::StrVecNew, 1, 0, 0,
                           t81::tisc::LiteralKind::Int});
  program.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 1, 0,
                           t81::tisc::LiteralKind::SymbolHandle});
  program.insns.push_back({t81::tisc::Opcode::StrVecPush, 1, 2, 0,
                           t81::tisc::LiteralKind::Int});
  program.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 2, 0,
                           t81::tisc::LiteralKind::SymbolHandle});
  program.insns.push_back({t81::tisc::Opcode::StrVecPush, 1, 3, 0,
                           t81::tisc::LiteralKind::Int});
  program.insns.push_back({t81::tisc::Opcode::Push, 1, 0, 0,
                           t81::tisc::LiteralKind::Int});
  program.insns.push_back({t81::tisc::Opcode::FFICall, 14, 1, 3,
                           t81::tisc::LiteralKind::SymbolHandle});
  program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0,
                           t81::tisc::LiteralKind::Int});

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful string-list-arg FFI call through VM bridge, got trap "
              << t81::vm::to_string(result.error()) << "\n";
    std::abort();
  }

  const auto& state = vm->state();
  const auto& ctx = state.contexts[state.current_context];
  if (ctx.register_tags[14] != t81::vm::ValueTag::Int || ctx.registers[14] != 9) {
    std::cerr << "expected string-list total length 9, got " << ctx.registers[14] << "\n";
    std::abort();
  }
}

void test_vm_ffi_call_emits_audit_trail() {
  register_binary_success_target();

  t81::tisc::ir::IntermediateProgram ir_program;
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{15}, t81::tisc::ir::Immediate{0}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = string_result_test_function_name();
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "expected successful audited FFI call, got trap "
              << t81::vm::to_string(result.error()) << "\n";
    std::abort();
  }

  auto* dispatcher = vm->get_ffi_dispatcher();
  if (dispatcher == nullptr) {
    std::cerr << "expected initialized FFI dispatcher\n";
    std::abort();
  }
  const auto trail = dispatcher->get_audit_trail();
  if (trail.empty()) {
    std::cerr << "expected FFI audit trail entry\n";
    std::abort();
  }
  const auto& last = trail.back();
  if (last.status != t81::ffi::FFIResult::Success || last.audit_events.empty() ||
      last.provenance_hash.empty()) {
    std::cerr << "expected successful audited FFI trail entry\n";
    std::abort();
  }
}

void test_vm_ffi_quarantine_fails_closed() {
  register_binary_success_target();

  t81::tisc::ir::IntermediateProgram ir_program;
  t81::tisc::ir::Instruction ffi_call{
      t81::tisc::ir::Opcode::FFI_CALL,
      {t81::tisc::ir::Register{16}, t81::tisc::ir::Immediate{0}}};
  ffi_call.literal_kind = t81::tisc::LiteralKind::SymbolHandle;
  ffi_call.text_literal = quarantined_test_function_name();
  ir_program.add_instruction(ffi_call);
  ir_program.add_instruction({t81::tisc::ir::Opcode::HALT, {}});

  t81::tisc::BinaryEmitter emitter;
  auto program = emitter.emit(ir_program);

  auto vm = t81::vm::make_interpreter_vm();
  t81::axion::PolicyEngine policy_engine(std::nullopt);
  vm->initialize_ffi_subsystem(policy_engine);
  vm->load_program(program);

  const auto result = vm->run_to_halt();
  if (result.has_value() || result.error() != t81::vm::Trap::FFIPolicyDenied) {
    std::cerr << "expected quarantined FFI call to fail closed with FFIPolicyDenied\n";
    std::abort();
  }
}

}  // namespace

int main() {
  std::cout << "=== VM FFI bridge tests ===\n";
  test_vm_ffi_call_uses_encoded_symbol();
  test_vm_ffi_call_success_writes_result_register();
  test_vm_ffi_call_unary_argument_round_trip();
  test_vm_ffi_call_system_string_argument_round_trip();
#ifndef _WIN32
  test_vm_ffi_call_system_double_argument_round_trip();
#endif
  test_vm_ffi_call_binary_argument_round_trip();
  test_vm_ffi_call_string_argument_round_trip();
  test_vm_ffi_call_string_result_round_trip();
  test_vm_ffi_call_double_argument_round_trip();
  test_vm_ffi_call_double_result_round_trip();
  test_vm_ffi_call_bytes_argument_round_trip();
  test_vm_ffi_call_bytes_result_round_trip();
  test_vm_ffi_call_mixed_argument_round_trip();
  test_vm_ffi_call_string_list_result_round_trip();
  test_vm_ffi_call_string_list_argument_round_trip();
  test_vm_ffi_call_emits_audit_trail();
  test_vm_ffi_quarantine_fails_closed();
  test_vm_ffi_register_reads_symbol_registers();
  std::cout << "VM FFI bridge tests passed.\n";
  return 0;
}
