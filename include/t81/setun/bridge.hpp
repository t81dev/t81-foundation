#pragma once

#include <string>
#include <string_view>
#include <utility>

#include "t81/support/expected.hpp"
#include "t81/tisc/program.hpp"

namespace t81::setun {

enum class BridgeError {
  EmptyInput = 0,
  UnsupportedMnemonic,
  InvalidOperand,
  InvalidRegister,
  InvalidImmediate,
  InvalidLabel,
  DuplicateLabel,
  UndefinedLabel,
};

struct BridgeDiagnostic {
  BridgeError error;
  std::size_t line;
  std::size_t column;
  std::string message;
  std::string source_line;

  BridgeDiagnostic(BridgeError error, std::size_t line, std::size_t column, std::string message,
                   std::string source_line)
      : error(error),
        line(line),
        column(column),
        message(std::move(message)),
        source_line(std::move(source_line)) {}
};

[[nodiscard]] std::string_view bridge_error_message(BridgeError error);

// Translate a single Setun-compatible source line into one TISC instruction.
// Supported mnemonics: NOP, HALT, LOADI, MOV, ADD, SUB, LOAD, STORE, JMP,
// JZ, JNZ, JN, JP.
[[nodiscard]] std::expected<t81::tisc::Insn, BridgeError> translate_line(std::string_view line);

// Translate a multi-line Setun-compatible program.
// Comments start with ';' or '#'.
[[nodiscard]] std::expected<t81::tisc::Program, BridgeError> translate_program(
    std::string_view source);

// Detailed program translation with deterministic source location on failure.
[[nodiscard]] std::expected<t81::tisc::Program, BridgeDiagnostic> translate_program_diagnostic(
    std::string_view source);

}  // namespace t81::setun
