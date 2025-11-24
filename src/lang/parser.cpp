#include "t81/lang/parser.hpp"

#include <cctype>
#include <limits>
#include <string>

namespace t81::lang {
namespace {
struct Lexer;
constexpr std::string_view kBase81ExtraDigits = "+@?!";

bool is_decimal_digit(char c) {
  return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

bool is_base81_digit(char c) {
  if (is_decimal_digit(c)) return true;
  if (std::isalpha(static_cast<unsigned char>(c))) return true;
  for (char extra : kBase81ExtraDigits) {
    if (c == extra) return true;
  }
  return false;
}

int base81_digit_value(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'Z') return 10 + (c - 'A');
  if (c >= 'a' && c <= 'z') return 36 + (c - 'a');
  if (c == '+') return 62;
  if (c == '@') return 63;
  if (c == '?') return 64;
  if (c == '!') return 65;
  return -1;
}

bool consume_exact(Lexer& lex, std::string_view text);
bool at_literal_suffix(std::string_view src, std::size_t pos) {
  return pos + 2 < src.size() && src[pos] == 't' && src[pos + 1] == '8' && src[pos + 2] == '1';
}

std::optional<std::int64_t> parse_decimal_value(std::string_view digits, bool negative) {
  if (digits.empty()) return std::nullopt;
  std::int64_t value = 0;
  for (char c : digits) {
    if (!is_decimal_digit(c)) return std::nullopt;
    int d = c - '0';
    if (!negative && (value > (std::numeric_limits<std::int64_t>::max() - d) / 10)) {
      return std::nullopt;
    }
    if (negative && (value < (std::numeric_limits<std::int64_t>::min() + d) / 10)) {
      return std::nullopt;
    }
    value = value * 10 + (negative ? -d : d);
  }
  return value;
}

std::optional<std::int64_t> parse_base81_value(std::string_view digits, bool negative) {
  if (digits.empty()) return std::nullopt;
  __int128 accum = 0;
  for (char c : digits) {
    int digit = base81_digit_value(c);
    if (digit < 0) return std::nullopt;
    accum = accum * 81 + digit;
  }
  if (negative) accum = -accum;
  if (accum > std::numeric_limits<std::int64_t>::max() ||
      accum < std::numeric_limits<std::int64_t>::min()) {
    return std::nullopt;
  }
  return static_cast<std::int64_t>(accum);
}

struct Lexer {
  std::string_view src;
  std::size_t pos{0};

  void skip_ws() {
    while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos]))) ++pos;
  }

  bool match(char c) {
    skip_ws();
    if (pos < src.size() && src[pos] == c) { ++pos; return true; }
    return false;
  }

  bool consume_keyword(std::string_view kw) {
    skip_ws();
    if (src.substr(pos, kw.size()) == kw) {
      std::size_t end = pos + kw.size();
      if (end < src.size() && std::isalnum(static_cast<unsigned char>(src[end]))) {
        return false;
      }
      pos = end;
      return true;
    }
    return false;
  }

  std::optional<std::string> identifier() {
    skip_ws();
    if (pos >= src.size()) return std::nullopt;
    if (!std::isalpha(static_cast<unsigned char>(src[pos])) && src[pos] != '_') return std::nullopt;
    std::size_t start = pos;
    while (pos < src.size() && (std::isalnum(static_cast<unsigned char>(src[pos])) || src[pos] == '_')) ++pos;
    return std::string(src.substr(start, pos - start));
  }

  bool eof() const { return pos >= src.size(); }
};

bool consume_exact(Lexer& lex, std::string_view text) {
  if (lex.src.substr(lex.pos, text.size()) == text) {
    lex.pos += text.size();
    return true;
  }
  return false;
}

std::shared_ptr<Expr> parse_expr(Lexer& lex, ParseError& err);
std::vector<Statement> parse_block(Lexer& lex, ParseError& err);
std::shared_ptr<Expr> parse_numeric_literal(Lexer& lex, ParseError& err);
std::shared_ptr<Expr> parse_primary(Lexer& lex, ParseError& err);
std::shared_ptr<Expr> parse_term(Lexer& lex, ParseError& err);
std::shared_ptr<Expr> parse_additive(Lexer& lex, ParseError& err);
std::shared_ptr<Expr> parse_relational(Lexer& lex, ParseError& err);
std::shared_ptr<Expr> parse_equality(Lexer& lex, ParseError& err);
std::shared_ptr<Expr> parse_logical_and(Lexer& lex, ParseError& err);
std::shared_ptr<Expr> parse_logical_or(Lexer& lex, ParseError& err);
std::optional<MatchPattern> parse_match_pattern(Lexer& lex, ParseError& err);

std::optional<MatchPattern> parse_match_pattern(Lexer& lex, ParseError& err) {
  auto name = lex.identifier();
  if (!name) {
    err = ParseError::UnexpectedToken;
    return std::nullopt;
  }
  MatchPattern pattern;
  auto parse_binding = [&](MatchPattern& pat) -> bool {
    if (!lex.match('(')) { err = ParseError::UnexpectedToken; return false; }
    auto binding = lex.identifier();
    if (!binding) { err = ParseError::UnexpectedToken; return false; }
    if (*binding != "_") {
      pat.binding = *binding;
    }
    if (!lex.match(')')) { err = ParseError::Unterminated; return false; }
    return true;
  };
  if (*name == "Some") {
    pattern.kind = MatchPattern::Kind::OptionSome;
    if (!parse_binding(pattern)) return std::nullopt;
    return pattern;
  }
  if (*name == "None") {
    pattern.kind = MatchPattern::Kind::OptionNone;
    return pattern;
  }
  if (*name == "Ok") {
    pattern.kind = MatchPattern::Kind::ResultOk;
    if (!parse_binding(pattern)) return std::nullopt;
    return pattern;
  }
  if (*name == "Err") {
    pattern.kind = MatchPattern::Kind::ResultErr;
    if (!parse_binding(pattern)) return std::nullopt;
    return pattern;
  }
  err = ParseError::UnexpectedToken;
  return std::nullopt;
}

std::shared_ptr<Expr> parse_primary(Lexer& lex, ParseError& err) {
  lex.skip_ws();
  if (lex.eof()) { err = ParseError::UnexpectedToken; return {}; }
  if (lex.consume_keyword("minus")) {
    auto expr = std::make_shared<Expr>();
    ExprLiteral lit;
    lit.value.kind = LiteralValue::Kind::Int;
    lit.value.int_value = -1;
    lit.value.text = "minus";
    expr->node = lit;
    return expr;
  }
  if (lex.consume_keyword("zero")) {
    auto expr = std::make_shared<Expr>();
    ExprLiteral lit;
    lit.value.kind = LiteralValue::Kind::Int;
    lit.value.int_value = 0;
    lit.value.text = "zero";
    expr->node = lit;
    return expr;
  }
  if (lex.consume_keyword("plus")) {
    auto expr = std::make_shared<Expr>();
    ExprLiteral lit;
    lit.value.kind = LiteralValue::Kind::Int;
    lit.value.int_value = 1;
    lit.value.text = "plus";
    expr->node = lit;
    return expr;
  }
  if (lex.src[lex.pos] == ':') {
    ++lex.pos;
    std::size_t name_start = lex.pos;
    while (lex.pos < lex.src.size() &&
           (std::isalnum(static_cast<unsigned char>(lex.src[lex.pos])) ||
            lex.src[lex.pos] == '_')) {
      ++lex.pos;
    }
    if (lex.pos == name_start) { err = ParseError::InvalidLiteral; return {}; }
    LiteralValue value;
    value.kind = LiteralValue::Kind::Symbol;
    value.text = std::string(lex.src.substr(name_start, lex.pos - name_start));
    auto expr = std::make_shared<Expr>();
    ExprLiteral lit;
    lit.value = std::move(value);
    expr->node = lit;
    return expr;
  }
  if (std::isdigit(static_cast<unsigned char>(lex.src[lex.pos])) ||
      (lex.src[lex.pos] == '-' &&
       lex.pos + 1 < lex.src.size() &&
       std::isdigit(static_cast<unsigned char>(lex.src[lex.pos + 1])))) {
    auto literal = parse_numeric_literal(lex, err);
    if (err != ParseError::None) return {};
    if (literal) return literal;
  }
  if (lex.consume_keyword("match")) {
    if (!lex.match('(')) { err = ParseError::UnexpectedToken; return {}; }
    auto value = parse_expr(lex, err);
    if (err != ParseError::None) return {};
    if (!lex.match(')')) { err = ParseError::Unterminated; return {}; }
    if (!lex.match('{')) { err = ParseError::UnexpectedToken; return {}; }
    std::vector<MatchArm> arms;
    while (true) {
      lex.skip_ws();
      if (lex.eof()) { err = ParseError::Unterminated; return {}; }
      if (lex.match('}')) break;
      auto pattern = parse_match_pattern(lex, err);
      if (err != ParseError::None || !pattern.has_value()) return {};
      if (!lex.consume_keyword("=>")) { err = ParseError::UnexpectedToken; return {}; }
      auto branch_expr = parse_expr(lex, err);
      if (err != ParseError::None) return {};
      MatchArm arm;
      arm.pattern = pattern.value();
      arm.expr = branch_expr;
      arms.push_back(std::move(arm));
      lex.skip_ws();
      if (lex.match('}')) break;
      if (!lex.match(',')) { err = ParseError::UnexpectedToken; return {}; }
    }
    if (arms.empty()) { err = ParseError::UnexpectedToken; return {}; }
    auto expr = std::make_shared<Expr>();
    ExprMatch match_node;
    match_node.value = value;
    match_node.arms = std::move(arms);
    expr->node = std::move(match_node);
    return expr;
  }
  if (std::isalpha(static_cast<unsigned char>(lex.src[lex.pos]))) {
    std::size_t start = lex.pos;
    while (lex.pos < lex.src.size() &&
           (std::isalnum(static_cast<unsigned char>(lex.src[lex.pos])) ||
            lex.src[lex.pos] == '_')) {
      ++lex.pos;
    }
    std::string name(lex.src.substr(start, lex.pos - start));
    lex.skip_ws();
    if (lex.pos < lex.src.size() && lex.src[lex.pos] == '(') {
      ++lex.pos;
      std::vector<Expr> args;
      lex.skip_ws();
      if (!lex.match(')')) {
        while (true) {
          auto arg = parse_expr(lex, err);
          if (err != ParseError::None) return {};
          args.push_back(*arg);
          lex.skip_ws();
          if (lex.match(')')) break;
          if (!lex.match(',')) { err = ParseError::UnexpectedToken; return {}; }
        }
      }
      auto expr = std::make_shared<Expr>();
      ExprCall call;
      call.callee = std::move(name);
      call.args = std::move(args);
      expr->node = std::move(call);
      return expr;
    }
    auto expr = std::make_shared<Expr>();
    expr->node = ExprIdent{name};
    return expr;
  }
  if (lex.src[lex.pos] == '(') {
    ++lex.pos;
    auto e = parse_expr(lex, err);
    if (err != ParseError::None) return {};
    if (!lex.match(')')) { err = ParseError::Unterminated; return {}; }
    return e;
  }
  err = ParseError::UnexpectedToken;
  return {};
}

std::shared_ptr<Expr> parse_numeric_literal(Lexer& lex, ParseError& err) {
  std::size_t start = lex.pos;
  bool negative = false;
  if (lex.src[lex.pos] == '-') {
    negative = true;
    ++lex.pos;
  }
  if (lex.pos >= lex.src.size() || !std::isdigit(static_cast<unsigned char>(lex.src[lex.pos]))) {
    lex.pos = start;
    return {};
  }
  std::size_t digits_start = lex.pos;
  while (lex.pos < lex.src.size() &&
         !at_literal_suffix(lex.src, lex.pos) &&
         is_base81_digit(lex.src[lex.pos])) {
    ++lex.pos;
  }
  std::size_t digits_end = lex.pos;
  if (digits_end == digits_start) {
    lex.pos = start;
    err = ParseError::InvalidLiteral;
    return {};
  }
  auto make_expr = [&](LiteralValue value) -> std::shared_ptr<Expr> {
    auto expr = std::make_shared<Expr>();
    ExprLiteral lit;
    lit.value = std::move(value);
    expr->node = lit;
    return expr;
  };
  if (lex.pos < lex.src.size() && lex.src[lex.pos] == '/') {
    ++lex.pos;
    std::size_t denom_start = lex.pos;
    while (lex.pos < lex.src.size() &&
           !at_literal_suffix(lex.src, lex.pos) &&
           is_base81_digit(lex.src[lex.pos])) {
      ++lex.pos;
    }
    if (lex.pos == denom_start || !consume_exact(lex, "t81")) {
      lex.pos = start;
      err = ParseError::InvalidLiteral;
      return {};
    }
    LiteralValue value;
    value.kind = LiteralValue::Kind::Fraction;
    value.text = std::string(lex.src.substr(start, lex.pos - start));
    return make_expr(value);
  }
  if (lex.pos < lex.src.size() && lex.src[lex.pos] == '.') {
    ++lex.pos;
    std::size_t frac_start = lex.pos;
    while (lex.pos < lex.src.size() &&
           !at_literal_suffix(lex.src, lex.pos) &&
           is_base81_digit(lex.src[lex.pos])) {
      ++lex.pos;
    }
    std::size_t frac_end = lex.pos;
    if (frac_end == frac_start) {
      lex.pos = start;
      err = ParseError::InvalidLiteral;
      return {};
    }
    bool has_float_suffix = consume_exact(lex, "t81");
    if (!has_float_suffix) {
      lex.pos = start;
      err = ParseError::InvalidLiteral;
      return {};
    }
    if (lex.pos < lex.src.size() &&
        (lex.src[lex.pos] == 'f' || lex.src[lex.pos] == 'F')) {
      ++lex.pos;
    }
    if (lex.pos < lex.src.size() &&
        (lex.src[lex.pos] == 'e' || lex.src[lex.pos] == 'E')) {
      ++lex.pos;
      if (lex.pos < lex.src.size() &&
          (lex.src[lex.pos] == '+' || lex.src[lex.pos] == '-')) {
        ++lex.pos;
      }
      std::size_t exp_start = lex.pos;
      while (lex.pos < lex.src.size() && is_base81_digit(lex.src[lex.pos])) {
        ++lex.pos;
      }
      if (lex.pos == exp_start) {
        lex.pos = start;
        err = ParseError::InvalidLiteral;
        return {};
      }
    }
    LiteralValue value;
    value.kind = LiteralValue::Kind::Float;
    value.text = std::string(lex.src.substr(start, lex.pos - start));
    return make_expr(value);
  }
  bool has_base_suffix = consume_exact(lex, "t81");
  LiteralValue value;
  value.kind = LiteralValue::Kind::Int;
  value.text = std::string(lex.src.substr(start, lex.pos - start));
  std::string_view digits = lex.src.substr(digits_start, digits_end - digits_start);
  std::optional<std::int64_t> parsed = has_base_suffix
      ? parse_base81_value(digits, negative)
      : parse_decimal_value(digits, negative);
  if (!parsed) {
    lex.pos = start;
    err = ParseError::InvalidLiteral;
    return {};
  }
  value.int_value = *parsed;
  return make_expr(value);
}

std::shared_ptr<Expr> parse_term(Lexer& lex, ParseError& err) {
  auto lhs = parse_primary(lex, err);
  if (err != ParseError::None) return {};
  while (true) {
    lex.skip_ws();
    if (lex.eof()) break;
    char c = lex.src[lex.pos];
    ExprBinary::Op op = ExprBinary::Op::Mul;
    if (c == '*') {
      op = ExprBinary::Op::Mul;
    } else if (c == '/') {
      op = ExprBinary::Op::Div;
    } else if (c == '%') {
      op = ExprBinary::Op::Mod;
    } else {
      break;
    }
    ++lex.pos;
    auto rhs = parse_primary(lex, err);
    if (err != ParseError::None) return {};
    auto expr = std::make_shared<Expr>();
    ExprBinary bin; bin.op = op; bin.lhs = lhs; bin.rhs = rhs;
    expr->node = bin;
    lhs = expr;
  }
  return lhs;
}

std::shared_ptr<Expr> parse_additive(Lexer& lex, ParseError& err) {
  auto lhs = parse_term(lex, err);
  if (err != ParseError::None) return {};
  while (true) {
    lex.skip_ws();
    if (lex.eof()) break;
    char c = lex.src[lex.pos];
    if (c != '+' && c != '-') break;
    ++lex.pos;
    auto rhs = parse_term(lex, err);
    if (err != ParseError::None) return {};
    auto expr = std::make_shared<Expr>();
    ExprBinary bin; bin.op = (c == '+') ? ExprBinary::Op::Add : ExprBinary::Op::Sub; bin.lhs = lhs; bin.rhs = rhs;
    expr->node = bin;
    lhs = expr;
  }
  return lhs;
}

std::shared_ptr<Expr> parse_relational(Lexer& lex, ParseError& err) {
  auto lhs = parse_additive(lex, err);
  if (err != ParseError::None) return {};
  while (true) {
    lex.skip_ws();
    if (lex.eof()) break;
    ExprBinary::Op op = ExprBinary::Op::Add;
    bool matched = false;
    if (consume_exact(lex, "<=")) {
      op = ExprBinary::Op::Le;
      matched = true;
    } else if (consume_exact(lex, ">=")) {
      op = ExprBinary::Op::Ge;
      matched = true;
    } else if (lex.pos < lex.src.size() && lex.src[lex.pos] == '<') {
      ++lex.pos;
      op = ExprBinary::Op::Lt;
      matched = true;
    } else if (lex.pos < lex.src.size() && lex.src[lex.pos] == '>') {
      ++lex.pos;
      op = ExprBinary::Op::Gt;
      matched = true;
    }
    if (!matched) break;
    auto rhs = parse_additive(lex, err);
    if (err != ParseError::None) return {};
    auto expr = std::make_shared<Expr>();
    ExprBinary bin; bin.op = op; bin.lhs = lhs; bin.rhs = rhs;
    expr->node = bin;
    lhs = expr;
  }
  return lhs;
}

std::shared_ptr<Expr> parse_equality(Lexer& lex, ParseError& err) {
  auto lhs = parse_relational(lex, err);
  if (err != ParseError::None) return {};
  while (true) {
    lex.skip_ws();
    if (lex.eof()) break;
    ExprBinary::Op op = ExprBinary::Op::Add;
    bool matched = false;
    if (consume_exact(lex, "==")) {
      op = ExprBinary::Op::Eq;
      matched = true;
    } else if (consume_exact(lex, "!=")) {
      op = ExprBinary::Op::Ne;
      matched = true;
    }
    if (!matched) break;
    auto rhs = parse_relational(lex, err);
    if (err != ParseError::None) return {};
    auto expr = std::make_shared<Expr>();
    ExprBinary bin; bin.op = op; bin.lhs = lhs; bin.rhs = rhs;
    expr->node = bin;
    lhs = expr;
  }
  return lhs;
}

std::shared_ptr<Expr> parse_logical_and(Lexer& lex, ParseError& err) {
  auto lhs = parse_equality(lex, err);
  if (err != ParseError::None) return {};
  while (true) {
    lex.skip_ws();
    if (lex.eof()) break;
    if (!consume_exact(lex, "&&")) break;
    auto rhs = parse_equality(lex, err);
    if (err != ParseError::None) return {};
    auto expr = std::make_shared<Expr>();
    ExprBinary bin; bin.op = ExprBinary::Op::Land; bin.lhs = lhs; bin.rhs = rhs;
    expr->node = bin;
    lhs = expr;
  }
  return lhs;
}

std::shared_ptr<Expr> parse_logical_or(Lexer& lex, ParseError& err) {
  auto lhs = parse_logical_and(lex, err);
  if (err != ParseError::None) return {};
  while (true) {
    lex.skip_ws();
    if (lex.eof()) break;
    if (!consume_exact(lex, "||")) break;
    auto rhs = parse_logical_and(lex, err);
    if (err != ParseError::None) return {};
    auto expr = std::make_shared<Expr>();
    ExprBinary bin; bin.op = ExprBinary::Op::Lor; bin.lhs = lhs; bin.rhs = rhs;
    expr->node = bin;
    lhs = expr;
  }
  return lhs;
}

std::shared_ptr<Expr> parse_expr(Lexer& lex, ParseError& err) {
  return parse_logical_or(lex, err);
}

std::optional<Type> parse_type_token(Lexer& lex, ParseError& err) {
  auto name = lex.identifier();
  if (!name) { err = ParseError::MissingType; return std::nullopt; }
  if (*name == "T81Int") return Type::primitive(Type::Kind::T81Int);
  if (*name == "T81Float") return Type::primitive(Type::Kind::T81Float);
  if (*name == "T81Fraction") return Type::primitive(Type::Kind::T81Fraction);
  if (*name == "Symbol") return Type::primitive(Type::Kind::Symbol);
  if (*name == "Option") {
    if (!lex.match('[')) { err = ParseError::MissingType; return std::nullopt; }
    auto inner = parse_type_token(lex, err);
    if (err != ParseError::None) return std::nullopt;
    if (!lex.match(']')) { err = ParseError::Unterminated; return std::nullopt; }
    return Type::option(*inner);
  }
  if (*name == "Result") {
    if (!lex.match('[')) { err = ParseError::MissingType; return std::nullopt; }
    auto ok_type = parse_type_token(lex, err);
    if (err != ParseError::None) return std::nullopt;
    if (!lex.match(',')) { err = ParseError::UnexpectedToken; return std::nullopt; }
    auto err_type = parse_type_token(lex, err);
    if (err != ParseError::None) return std::nullopt;
    if (!lex.match(']')) { err = ParseError::Unterminated; return std::nullopt; }
    return Type::result(*ok_type, *err_type);
  }
  err = ParseError::InvalidType;
  return std::nullopt;
}

Statement parse_statement(Lexer& lex, ParseError& err) {
  lex.skip_ws();
  if (lex.consume_keyword("let")) {
    auto name = lex.identifier();
    if (!name) { err = ParseError::UnexpectedToken; return {}; }
    if (!lex.match(':')) { err = ParseError::MissingType; return {}; }
    auto ty = parse_type_token(lex, err);
    if (err != ParseError::None) return {};
    if (!lex.match('=')) { err = ParseError::UnexpectedToken; return {}; }
    auto e = parse_expr(lex, err);
    if (err != ParseError::None) return {};
    if (!lex.match(';')) { err = ParseError::Unterminated; return {}; }
    StatementLet let_stmt;
    let_stmt.name = *name;
    let_stmt.declared_type = ty;
    let_stmt.expr = *e;
    Statement s;
    s.node = let_stmt;
    return s;
  }
  if (lex.consume_keyword("loop")) {
    auto body = parse_block(lex, err);
    if (err != ParseError::None) return {};
    StatementLoop loop_stmt;
    loop_stmt.body = std::move(body);
    Statement s;
    s.node = loop_stmt;
    return s;
  }
  if (lex.consume_keyword("if")) {
    if (!lex.match('(')) { err = ParseError::UnexpectedToken; return {}; }
    auto cond = parse_expr(lex, err);
    if (err != ParseError::None) return {};
    if (!lex.match(')')) { err = ParseError::Unterminated; return {}; }
    auto then_body = parse_block(lex, err);
    if (err != ParseError::None) return {};
    std::vector<Statement> else_body;
    if (lex.consume_keyword("else")) {
      else_body = parse_block(lex, err);
      if (err != ParseError::None) return {};
    }
    Statement s; s.node = StatementIf{*cond, std::move(then_body), std::move(else_body)}; return s;
  }
  if (lex.consume_keyword("return")) {
    auto e = parse_expr(lex, err);
    if (err != ParseError::None) return {};
    if (!lex.match(';')) { err = ParseError::Unterminated; return {}; }
    Statement s; s.node = StatementReturn{*e}; return s;
  }
  std::size_t stmt_start = lex.pos;
  auto name = lex.identifier();
  if (name) {
    lex.skip_ws();
    if (lex.pos < lex.src.size() && lex.src[lex.pos] == '=') {
      ++lex.pos;
      auto e = parse_expr(lex, err);
      if (err != ParseError::None) return {};
      if (!lex.match(';')) { err = ParseError::Unterminated; return {}; }
      StatementAssign assign_stmt;
      assign_stmt.name = *name;
      assign_stmt.expr = *e;
      Statement s;
      s.node = assign_stmt;
      return s;
    }
    lex.pos = stmt_start;
  }
  auto e = parse_expr(lex, err);
  if (err != ParseError::None) return {};
  if (!lex.match(';')) { err = ParseError::Unterminated; return {}; }
  StatementExpr expr_stmt;
  expr_stmt.expr = *e;
  Statement s;
  s.node = expr_stmt;
  return s;
}

std::vector<Statement> parse_block(Lexer& lex, ParseError& err) {
  std::vector<Statement> out;
  if (!lex.match('{')) { err = ParseError::UnexpectedToken; return out; }
  while (true) {
    lex.skip_ws();
    if (lex.eof()) { err = ParseError::Unterminated; return out; }
    if (lex.match('}')) break;
    auto stmt = parse_statement(lex, err);
    if (err != ParseError::None) return out;
    out.push_back(std::move(stmt));
  }
  return out;
}

}  // namespace

t81::expected<Module, ParseError> parse_module(std::string_view source) {
  ParseError err = ParseError::None;
  Lexer lex{source};
  std::vector<Function> functions;

  while (true) {
    lex.skip_ws();
    if (lex.eof()) break;
    if (!lex.consume_keyword("fn")) {
      err = ParseError::UnexpectedToken;
      return err;
    }
    auto name = lex.identifier();
    if (!name) { err = ParseError::UnexpectedToken; return err; }
    if (!lex.match('(')) { err = ParseError::UnexpectedToken; return err; }

    std::vector<Parameter> params;
    lex.skip_ws();
    if (!lex.match(')')) {
      while (true) {
        auto pname = lex.identifier();
        if (!pname) { err = ParseError::UnexpectedToken; return err; }
        if (!lex.match(':')) { err = ParseError::MissingType; return err; }
        auto ptype = parse_type_token(lex, err);
        if (err != ParseError::None) return err;
        params.push_back(Parameter{*pname, *ptype});
        lex.skip_ws();
        if (lex.match(')')) break;
        if (!lex.match(',')) { err = ParseError::UnexpectedToken; return err; }
      }
    }

    if (!lex.consume_keyword("->")) {
      err = ParseError::UnexpectedToken;
      return err;
    }
    auto ret_type = parse_type_token(lex, err);
    if (err != ParseError::None) return err;

    auto body = parse_block(lex, err);
    if (err != ParseError::None) return err;

    Function fn;
    fn.name = *name;
    fn.return_type = *ret_type;
    fn.params = std::move(params);
    fn.body = std::move(body);
    functions.push_back(std::move(fn));
  }

  if (functions.empty()) return ParseError::MissingFunction;
  Module mod;
  mod.functions = std::move(functions);
  return mod;
}

}  // namespace t81::lang
