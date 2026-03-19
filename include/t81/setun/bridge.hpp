#pragma once

#include <string>
#include <string_view>
#include <utility>

#include "t81/isa/program.hpp"
#include "t81/support/expected.hpp"

using t81::expected;

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
  BridgeError error = BridgeError::EmptyInput;
  std::size_t line = 0;
  std::size_t column = 0;
  std::string message = "";
  std::string source_line = "";

  BridgeDiagnostic(BridgeError err, std::size_t l, std::size_t col, std::string msg,
                   std::string src_line)
      : error(err),
        line(l),
        column(col),
        message(std::move(msg)),
        source_line(std::move(src_line)) {}

  // Explicitly implement Rule of 5 to silence Clang Static Analyzer false positives
  // regarding uninitialized memory in implicit copy/move constructors.
  BridgeDiagnostic(const BridgeDiagnostic& other)
      : error(other.error),
        line(other.line),
        column(other.column),
        message(other.message),
        source_line(other.source_line) {}

  BridgeDiagnostic(BridgeDiagnostic&& other) noexcept
      : error(other.error),
        line(other.line),
        column(other.column),
        message(std::move(other.message)),
        source_line(std::move(other.source_line)) {}

  BridgeDiagnostic& operator=(const BridgeDiagnostic& other) {
    if (this != &other) {
      error = other.error;
      line = other.line;
      column = other.column;
      message = other.message;
      source_line = other.source_line;
    }
    return *this;
  }

  BridgeDiagnostic& operator=(BridgeDiagnostic&& other) noexcept {
    if (this != &other) {
      error = other.error;
      line = other.line;
      column = other.column;
      message = std::move(other.message);
      source_line = std::move(other.source_line);
    }
    return *this;
  }
};

[[nodiscard]] std::string_view bridge_error_message(BridgeError error);

// Translate a single Setun-compatible source line into one TISC instruction.
// Supported mnemonics: NOP, HALT, LOADI, MOV, ADD, SUB, LOAD, STORE, JMP,
// JZ, JNZ, JN, JP, TNOT_SWAR, TAND_SWAR, TOR_SWAR.
[[nodiscard]] t81::expected<t81::tisc::Insn, BridgeError> translate_line(std::string_view line);

// Translate a multi-line Setun-compatible program.
// Comments start with ';' or '#'.
[[nodiscard]] t81::expected<t81::tisc::Program, BridgeError> translate_program(
    std::string_view source);

// Detailed program translation with deterministic source location on failure.
[[nodiscard]] t81::expected<t81::tisc::Program, BridgeDiagnostic> translate_program_diagnostic(
    std::string_view source);

}  // namespace t81::setun
