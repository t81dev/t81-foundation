#include "t81/setun/bridge.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace t81::setun {
namespace {

struct ParsedLine {
  std::size_t line_no{0};
  std::string source_line;
  std::optional<std::string> label;
  std::vector<std::string> tokens;
};

std::string trim(std::string_view in) {
  const auto first =
      std::find_if_not(in.begin(), in.end(), [](unsigned char c) { return std::isspace(c) != 0; });
  if (first == in.end()) return {};
  const auto last = std::find_if_not(in.rbegin(), in.rend(), [](unsigned char c) {
                      return std::isspace(c) != 0;
                    }).base();
  return std::string(first, last);
}

std::string strip_comments(std::string_view raw) {
  std::string line(raw);
  const auto semicolon = line.find(';');
  if (semicolon != std::string::npos) line.resize(semicolon);
  const auto hash = line.find('#');
  if (hash != std::string::npos) line.resize(hash);
  return line;
}

std::string uppercase(std::string token) {
  std::transform(token.begin(), token.end(), token.begin(),
                 [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return token;
}

std::vector<std::string> tokenize(std::string line) {
  for (char& ch : line) {
    if (ch == ',') ch = ' ';
  }

  std::vector<std::string> out;
  std::string current;
  for (char ch : line) {
    if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
      if (!current.empty()) {
        out.push_back(std::move(current));
        current.clear();
      }
      continue;
    }
    current.push_back(ch);
  }
  if (!current.empty()) out.push_back(std::move(current));
  return out;
}

bool is_valid_label(std::string_view label) {
  if (label.empty()) return false;
  auto is_head = [](unsigned char c) { return std::isalpha(c) != 0 || c == '_'; };
  auto is_tail = [](unsigned char c) { return std::isalnum(c) != 0 || c == '_'; };
  if (!is_head(static_cast<unsigned char>(label.front()))) return false;
  for (std::size_t i = 1; i < label.size(); ++i) {
    if (!is_tail(static_cast<unsigned char>(label[i]))) return false;
  }
  return true;
}

std::size_t column_for_token(std::string_view source_line, std::string_view token) {
  if (!token.empty()) {
    const auto pos = source_line.find(token);
    if (pos != std::string::npos) return pos + 1;
  }
  const auto first = std::find_if_not(source_line.begin(), source_line.end(),
                                      [](unsigned char c) { return std::isspace(c) != 0; });
  if (first == source_line.end()) return 1;
  return static_cast<std::size_t>(std::distance(source_line.begin(), first)) + 1;
}

BridgeDiagnostic make_diag(BridgeError error, std::size_t line, std::string_view source_line,
                           std::string message, std::string_view token = {}) {
  return BridgeDiagnostic{error, line, column_for_token(source_line, token), std::move(message),
                          std::string(source_line)};
}

t81::expected<int, BridgeError> parse_register(std::string_view token) {
  if (token.size() < 2) return t81::make_unexpected(BridgeError::InvalidRegister);
  if (token.front() != 'R' && token.front() != 'r') {
    return t81::make_unexpected(BridgeError::InvalidRegister);
  }
  int index = -1;
  const char* begin = token.data() + 1;
  const char* end = token.data() + token.size();
  auto [ptr, ec] = std::from_chars(begin, end, index);
  if (ec != std::errc{} || ptr != end) {
    return t81::make_unexpected(BridgeError::InvalidRegister);
  }
  if (index < 0 || index > 242) {
    return t81::make_unexpected(BridgeError::InvalidRegister);
  }
  return index;
}

t81::expected<std::int64_t, BridgeError> parse_immediate(std::string_view token) {
  std::int64_t value = 0;
  const char* begin = token.data();
  const char* end = token.data() + token.size();
  auto [ptr, ec] = std::from_chars(begin, end, value);
  if (ec != std::errc{} || ptr != end) {
    return t81::make_unexpected(BridgeError::InvalidImmediate);
  }
  return value;
}

t81::expected<ParsedLine, BridgeDiagnostic> parse_source_line(std::string_view raw_line,
                                                              std::size_t line_no) {
  ParsedLine parsed;
  parsed.line_no = line_no;
  parsed.source_line = std::string(raw_line);

  std::string code = trim(strip_comments(raw_line));
  if (code.empty()) {
    return parsed;
  }

  const auto colon = code.find(':');
  if (colon != std::string::npos) {
    std::string label = trim(code.substr(0, colon));
    if (!is_valid_label(label)) {
      return t81::make_unexpected(make_diag(BridgeError::InvalidLabel, line_no, raw_line,
                                            "invalid label declaration", label));
    }
    parsed.label = std::move(label);
    code = trim(code.substr(colon + 1));
    if (code.empty()) {
      return parsed;
    }
  }

  parsed.tokens = tokenize(std::move(code));
  if (!parsed.tokens.empty()) {
    parsed.tokens[0] = uppercase(std::move(parsed.tokens[0]));
  }
  return parsed;
}

t81::expected<std::int64_t, BridgeDiagnostic> resolve_jump_target(
    std::string_view token, std::size_t line_no, std::string_view source_line,
    const std::unordered_map<std::string, std::int32_t>* labels) {
  auto immediate = parse_immediate(token);
  if (immediate.has_value()) {
    return immediate.value();
  }

  if (!labels) {
    return t81::make_unexpected(make_diag(BridgeError::InvalidImmediate, line_no, source_line,
                                          "expected immediate jump target", token));
  }

  if (!is_valid_label(token)) {
    return t81::make_unexpected(
        make_diag(BridgeError::InvalidLabel, line_no, source_line, "invalid jump label", token));
  }

  auto it = labels->find(std::string(token));
  if (it == labels->end()) {
    return t81::make_unexpected(
        make_diag(BridgeError::UndefinedLabel, line_no, source_line, "undefined label", token));
  }
  return static_cast<std::int64_t>(it->second);
}

t81::expected<t81::tisc::Insn, BridgeDiagnostic> encode_tokens(
    const std::vector<std::string>& tokens, std::size_t line_no, std::string_view source_line,
    const std::unordered_map<std::string, std::int32_t>* labels) {
  if (tokens.empty()) {
    return t81::make_unexpected(
        make_diag(BridgeError::EmptyInput, line_no, source_line, "empty input"));
  }

  const std::string& op = tokens[0];
  t81::tisc::Insn insn{};

  if (op == "NOP") {
    if (tokens.size() != 1) {
      return t81::make_unexpected(make_diag(BridgeError::InvalidOperand, line_no, source_line,
                                            "NOP takes no operands", tokens[0]));
    }
    insn.opcode = t81::tisc::Opcode::Nop;
    return insn;
  }

  if (op == "HALT") {
    if (tokens.size() != 1) {
      return t81::make_unexpected(make_diag(BridgeError::InvalidOperand, line_no, source_line,
                                            "HALT takes no operands", tokens[0]));
    }
    insn.opcode = t81::tisc::Opcode::Halt;
    return insn;
  }

  if (op == "LOADI") {
    if (tokens.size() != 3) {
      return t81::make_unexpected(make_diag(BridgeError::InvalidOperand, line_no, source_line,
                                            "LOADI expects: LOADI Rdst imm", tokens[0]));
    }
    auto dst = parse_register(tokens[1]);
    if (!dst.has_value()) {
      return t81::make_unexpected(
          make_diag(dst.error(), line_no, source_line, "invalid destination register", tokens[1]));
    }
    auto imm = parse_immediate(tokens[2]);
    if (!imm.has_value()) {
      return t81::make_unexpected(
          make_diag(imm.error(), line_no, source_line, "invalid immediate", tokens[2]));
    }
    insn.opcode = t81::tisc::Opcode::LoadImm;
    insn.a = dst.value();
    insn.b = imm.value();
    return insn;
  }

  if (op == "MOV") {
    if (tokens.size() != 3) {
      return t81::make_unexpected(make_diag(BridgeError::InvalidOperand, line_no, source_line,
                                            "MOV expects: MOV Rdst Rsrc", tokens[0]));
    }
    auto dst = parse_register(tokens[1]);
    auto src = parse_register(tokens[2]);
    if (!dst.has_value()) {
      return t81::make_unexpected(
          make_diag(dst.error(), line_no, source_line, "invalid destination register", tokens[1]));
    }
    if (!src.has_value()) {
      return t81::make_unexpected(
          make_diag(src.error(), line_no, source_line, "invalid source register", tokens[2]));
    }
    insn.opcode = t81::tisc::Opcode::Mov;
    insn.a = dst.value();
    insn.b = src.value();
    return insn;
  }

  if (op == "ADD" || op == "SUB") {
    if (tokens.size() != 3) {
      return t81::make_unexpected(make_diag(BridgeError::InvalidOperand, line_no, source_line,
                                            "ADD/SUB expect: ADD Rdst Rsrc", tokens[0]));
    }
    auto dst = parse_register(tokens[1]);
    auto src = parse_register(tokens[2]);
    if (!dst.has_value()) {
      return t81::make_unexpected(
          make_diag(dst.error(), line_no, source_line, "invalid destination register", tokens[1]));
    }
    if (!src.has_value()) {
      return t81::make_unexpected(
          make_diag(src.error(), line_no, source_line, "invalid source register", tokens[2]));
    }
    insn.opcode = (op == "ADD") ? t81::tisc::Opcode::Add : t81::tisc::Opcode::Sub;
    insn.a = dst.value();
    insn.b = dst.value();
    insn.c = src.value();
    return insn;
  }

  if (op == "LOAD") {
    if (tokens.size() != 3) {
      return t81::make_unexpected(make_diag(BridgeError::InvalidOperand, line_no, source_line,
                                            "LOAD expects: LOAD Rdst addr", tokens[0]));
    }
    auto dst = parse_register(tokens[1]);
    auto addr = parse_immediate(tokens[2]);
    if (!dst.has_value()) {
      return t81::make_unexpected(
          make_diag(dst.error(), line_no, source_line, "invalid destination register", tokens[1]));
    }
    if (!addr.has_value()) {
      return t81::make_unexpected(
          make_diag(addr.error(), line_no, source_line, "invalid load address", tokens[2]));
    }
    insn.opcode = t81::tisc::Opcode::Load;
    insn.a = dst.value();
    insn.b = addr.value();
    return insn;
  }

  if (op == "STORE") {
    if (tokens.size() != 3) {
      return t81::make_unexpected(make_diag(BridgeError::InvalidOperand, line_no, source_line,
                                            "STORE expects: STORE addr Rsrc", tokens[0]));
    }
    auto addr = parse_immediate(tokens[1]);
    auto src = parse_register(tokens[2]);
    if (!addr.has_value()) {
      return t81::make_unexpected(
          make_diag(addr.error(), line_no, source_line, "invalid store address", tokens[1]));
    }
    if (!src.has_value()) {
      return t81::make_unexpected(
          make_diag(src.error(), line_no, source_line, "invalid source register", tokens[2]));
    }
    insn.opcode = t81::tisc::Opcode::Store;
    insn.a = static_cast<std::int32_t>(addr.value());
    insn.b = src.value();
    return insn;
  }

  if (op == "JMP") {
    if (tokens.size() != 2) {
      return t81::make_unexpected(make_diag(BridgeError::InvalidOperand, line_no, source_line,
                                            "JMP expects: JMP target", tokens[0]));
    }
    auto target = resolve_jump_target(tokens[1], line_no, source_line, labels);
    if (!target.has_value()) {
      return t81::make_unexpected(target.error());
    }
    insn.opcode = t81::tisc::Opcode::Jump;
    insn.a = static_cast<std::int32_t>(target.value());
    return insn;
  }

  if (op == "JZ" || op == "JNZ") {
    if (tokens.size() != 3) {
      return t81::make_unexpected(make_diag(BridgeError::InvalidOperand, line_no, source_line,
                                            "JZ/JNZ expect: JZ Rcond target", tokens[0]));
    }
    auto cond = parse_register(tokens[1]);
    if (!cond.has_value()) {
      return t81::make_unexpected(
          make_diag(cond.error(), line_no, source_line, "invalid condition register", tokens[1]));
    }
    auto target = resolve_jump_target(tokens[2], line_no, source_line, labels);
    if (!target.has_value()) {
      return t81::make_unexpected(target.error());
    }
    insn.opcode = (op == "JZ") ? t81::tisc::Opcode::JumpIfZero : t81::tisc::Opcode::JumpIfNotZero;
    insn.a = static_cast<std::int32_t>(target.value());
    insn.b = cond.value();
    return insn;
  }

  if (op == "JN" || op == "JP") {
    if (tokens.size() != 2) {
      return t81::make_unexpected(make_diag(BridgeError::InvalidOperand, line_no, source_line,
                                            "JN/JP expect: JN target", tokens[0]));
    }
    auto target = resolve_jump_target(tokens[1], line_no, source_line, labels);
    if (!target.has_value()) {
      return t81::make_unexpected(target.error());
    }
    insn.opcode =
        (op == "JN") ? t81::tisc::Opcode::JumpIfNegative : t81::tisc::Opcode::JumpIfPositive;
    insn.a = static_cast<std::int32_t>(target.value());
    return insn;
  }

  if (op == "TNOT_SWAR") {
    if (tokens.size() != 3) {
      return t81::make_unexpected(make_diag(BridgeError::InvalidOperand, line_no, source_line,
                                            "TNOT_SWAR expects: TNOT_SWAR Rdst Rsrc", tokens[0]));
    }
    auto dst = parse_register(tokens[1]);
    auto src = parse_register(tokens[2]);
    if (!dst.has_value()) {
      return t81::make_unexpected(
          make_diag(dst.error(), line_no, source_line, "invalid destination register", tokens[1]));
    }
    if (!src.has_value()) {
      return t81::make_unexpected(
          make_diag(src.error(), line_no, source_line, "invalid source register", tokens[2]));
    }
    insn.opcode = t81::tisc::Opcode::TNOT_SWAR;
    insn.a = dst.value();
    insn.b = src.value();
    return insn;
  }

  if (op == "TAND_SWAR" || op == "TOR_SWAR") {
    if (tokens.size() != 4) {
      return t81::make_unexpected(
          make_diag(BridgeError::InvalidOperand, line_no, source_line,
                    "TAND_SWAR/TOR_SWAR expect: TAND_SWAR Rdst Rlhs Rrhs", tokens[0]));
    }
    auto dst = parse_register(tokens[1]);
    auto lhs = parse_register(tokens[2]);
    auto rhs = parse_register(tokens[3]);
    if (!dst.has_value()) {
      return t81::make_unexpected(
          make_diag(dst.error(), line_no, source_line, "invalid destination register", tokens[1]));
    }
    if (!lhs.has_value()) {
      return t81::make_unexpected(
          make_diag(lhs.error(), line_no, source_line, "invalid left source register", tokens[2]));
    }
    if (!rhs.has_value()) {
      return t81::make_unexpected(
          make_diag(rhs.error(), line_no, source_line, "invalid right source register", tokens[3]));
    }
    insn.opcode = (op == "TAND_SWAR") ? t81::tisc::Opcode::TAND_SWAR : t81::tisc::Opcode::TOR_SWAR;
    insn.a = dst.value();
    insn.b = lhs.value();
    insn.c = rhs.value();
    return insn;
  }

  return t81::make_unexpected(make_diag(BridgeError::UnsupportedMnemonic, line_no, source_line,
                                        "unsupported mnemonic", tokens[0]));
}

}  // namespace

std::string_view bridge_error_message(BridgeError error) {
  switch (error) {
    case BridgeError::EmptyInput:
      return "empty input";
    case BridgeError::UnsupportedMnemonic:
      return "unsupported mnemonic";
    case BridgeError::InvalidOperand:
      return "invalid operand";
    case BridgeError::InvalidRegister:
      return "invalid register";
    case BridgeError::InvalidImmediate:
      return "invalid immediate";
    case BridgeError::InvalidLabel:
      return "invalid label";
    case BridgeError::DuplicateLabel:
      return "duplicate label";
    case BridgeError::UndefinedLabel:
      return "undefined label";
  }
  return "unknown bridge error";
}

t81::expected<t81::tisc::Insn, BridgeError> translate_line(std::string_view raw_line) {
  auto parsed = parse_source_line(raw_line, 1);
  if (!parsed.has_value()) {
    return t81::make_unexpected(parsed.error().error);
  }
  if (parsed->tokens.empty()) {
    return t81::make_unexpected(BridgeError::EmptyInput);
  }

  auto insn = encode_tokens(parsed->tokens, 1, raw_line, nullptr);
  if (!insn.has_value()) {
    return t81::make_unexpected(insn.error().error);
  }
  return insn.value();
}

t81::expected<t81::tisc::Program, BridgeError> translate_program(std::string_view source) {
  auto detailed = translate_program_diagnostic(source);
  if (!detailed.has_value()) {
    return t81::make_unexpected(detailed.error().error);
  }
  return detailed.value();
}

t81::expected<t81::tisc::Program, BridgeDiagnostic> translate_program_diagnostic(
    std::string_view source) {
  t81::tisc::Program program{};
  std::vector<ParsedLine> lines;
  std::unordered_map<std::string, std::int32_t> labels;

  std::size_t start = 0;
  std::size_t line_no = 1;
  std::int32_t pc = 0;

  while (start <= source.size()) {
    const auto end = source.find('\n', start);
    const auto raw_line =
        source.substr(start, (end == std::string_view::npos) ? source.size() - start : end - start);

    auto parsed = parse_source_line(raw_line, line_no);
    if (!parsed.has_value()) {
      return t81::make_unexpected(parsed.error());
    }

    if (parsed->label.has_value()) {
      const auto [it, inserted] = labels.emplace(*parsed->label, pc);
      if (!inserted) {
        return t81::make_unexpected(make_diag(BridgeError::DuplicateLabel, line_no, raw_line,
                                              "duplicate label declaration", *parsed->label));
      }
    }

    if (!parsed->tokens.empty()) {
      lines.push_back(std::move(*parsed));
      ++pc;
    }

    if (end == std::string_view::npos) {
      break;
    }
    start = end + 1;
    ++line_no;
  }

  for (const auto& line : lines) {
    auto insn = encode_tokens(line.tokens, line.line_no, line.source_line, &labels);
    if (!insn.has_value()) {
      return t81::make_unexpected(insn.error());
    }
    program.insns.push_back(insn.value());
  }

  return program;
}

}  // namespace t81::setun
