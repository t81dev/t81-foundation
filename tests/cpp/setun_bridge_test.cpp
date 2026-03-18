#include "t81/setun/bridge.hpp"

#include <cassert>
#include <iostream>

using t81::setun::BridgeError;
using t81::setun::translate_line;
using t81::setun::translate_program;
using t81::setun::translate_program_diagnostic;
using t81::tisc::Opcode;

static void test_translate_add_two_address() {
  [[maybe_unused]] auto insn = translate_line("ADD R7, R9");
  assert(insn.has_value());
  assert(insn->opcode == Opcode::Add);
  assert(insn->a == 7);
  assert(insn->b == 7);
  assert(insn->c == 9);
}

static void test_translate_program_with_comments() {
  constexpr const char* source = R"(
; Setun compatibility subset
LOADI R1, 10
LOADI R2, 32
ADD R1, R2
JMP 9
HALT # inline comment
)";
  auto program = translate_program(source);
  assert(program.has_value());
  assert(program->insns.size() == 5);
  assert(program->insns[0].opcode == Opcode::LoadImm);
  assert(program->insns[3].opcode == Opcode::Jump);
  assert(program->insns[4].opcode == Opcode::Halt);
}

static void test_translate_program_with_labels_and_branches() {
  constexpr const char* source = R"(
start:
LOADI R1, 2
LOAD R2, 40
ADD R1, R2
JNZ R1, start
JN done
JP done
done: STORE 41, R1
HALT
)";
  auto program = translate_program(source);
  assert(program.has_value());
  assert(program->insns.size() == 8);
  assert(program->insns[1].opcode == Opcode::Load);
  assert(program->insns[3].opcode == Opcode::JumpIfNotZero);
  assert(program->insns[3].a == 0);
  assert(program->insns[5].opcode == Opcode::JumpIfPositive);
  assert(program->insns[6].opcode == Opcode::Store);
}

static void test_unsupported_mnemonic_fails_deterministically() {
  [[maybe_unused]] auto insn = translate_line("MUL R1, R2");
  assert(!insn.has_value());
  assert(insn.error() == BridgeError::UnsupportedMnemonic);
}

static void test_translate_swar_mnemonics() {
  [[maybe_unused]] auto tnot = translate_line("TNOT_SWAR R2, R7");
  assert(tnot.has_value());
  assert(tnot->opcode == Opcode::TNOT_SWAR);
  assert(tnot->a == 2);
  assert(tnot->b == 7);
  assert(tnot->c == 0);

  [[maybe_unused]] auto tand = translate_line("TAND_SWAR R3, R4, R5");
  assert(tand.has_value());
  assert(tand->opcode == Opcode::TAND_SWAR);
  assert(tand->a == 3);
  assert(tand->b == 4);
  assert(tand->c == 5);

  [[maybe_unused]] auto tor = translate_line("TOR_SWAR R9, R10, R11");
  assert(tor.has_value());
  assert(tor->opcode == Opcode::TOR_SWAR);
  assert(tor->a == 9);
  assert(tor->b == 10);
  assert(tor->c == 11);
}

static void test_diagnostic_reports_line_and_column() {
  constexpr const char* source = R"(
LOADI R1, 1
JMP missing_label
HALT
)";
  auto program = translate_program_diagnostic(source);
  assert(!program.has_value());
  assert(program.error().error == BridgeError::UndefinedLabel);
  assert(program.error().line == 3);
  assert(program.error().column >= 1);
  assert(!program.error().message.empty());
}

int main() {
  test_translate_add_two_address();
  test_translate_program_with_comments();
  test_translate_program_with_labels_and_branches();
  test_unsupported_mnemonic_fails_deterministically();
  test_translate_swar_mnemonics();
  test_diagnostic_reports_line_and_column();
  std::cout << "setun bridge tests passed!\n";
  return 0;
}
