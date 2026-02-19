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
  BridgeDiagnostic() : error(BridgeError::EmptyInput), line(0), column(0) {}

  BridgeDiagnostic(BridgeError err, std::size_t line_no, std::size_t column_no, std::string msg,
                   std::string source)
      : error(err),
        line(line_no),
        column(column_no),
        message(std::move(msg)),
        source_line(std::move(source)) {}

  BridgeDiagnostic(const BridgeDiagnostic&) = default;
  BridgeDiagnostic& operator=(const BridgeDiagnostic&) = default;
  BridgeDiagnostic(BridgeDiagnostic&&) noexcept = default;
  BridgeDiagnostic& operator=(BridgeDiagnostic&&) noexcept = default;

  ~BridgeDiagnostic() = default;

  BridgeError error{BridgeError::EmptyInput};
  std::size_t line{0};    // 1-based line index
  std::size_t column{0};  // 1-based column index
  std::string message;
  std::string source_line;
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
