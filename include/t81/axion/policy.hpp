#pragma once

#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "t81/support/expected.hpp"

namespace t81::axion {
struct Policy {
  struct LoopHint {
    int id{0};
    std::string file;
    int line{0};
    int column{0};
    bool annotated{false};
    int depth{0};
    bool bound_infinite{false};
    std::optional<int64_t> bound_value;
  };

  int tier{1};
  std::optional<int64_t> max_stack;
  std::vector<LoopHint> loops;
};

namespace detail {
struct PolicyToken {
  enum class Kind { LParen, RParen, Integer, Symbol, End } kind{Kind::End};
  std::string text;
  int64_t value{0};
};

class PolicyLexer {
 public:
  explicit PolicyLexer(std::string_view src) : src_(src) {}

  PolicyToken next() {
    skip_ws_();
    if (pos_ >= src_.size()) return PolicyToken{};
    char c = src_[pos_];
    if (c == '(') { ++pos_; return PolicyToken{PolicyToken::Kind::LParen, {}}; }
    if (c == ')') { ++pos_; return PolicyToken{PolicyToken::Kind::RParen, {}}; }
    if (std::isdigit(static_cast<unsigned char>(c)) || c == '-' || c == '+') {
      std::size_t start = pos_;
      ++pos_;
      while (pos_ < src_.size() &&
             std::isdigit(static_cast<unsigned char>(src_[pos_]))) ++pos_;
      PolicyToken tok;
      tok.kind = PolicyToken::Kind::Integer;
      tok.text = std::string(src_.substr(start, pos_ - start));
      tok.value = std::stoll(tok.text);
      return tok;
    }
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
      std::size_t start = pos_;
      ++pos_;
      while (pos_ < src_.size()) {
        char ch = src_[pos_];
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '-' ||
              ch == '_')) {
          break;
        }
        ++pos_;
      }
      PolicyToken tok;
      tok.kind = PolicyToken::Kind::Symbol;
      tok.text = std::string(src_.substr(start, pos_ - start));
      return tok;
    }
    // Unknown char -> skip and treat as End to force parse failure.
    pos_ = src_.size();
    return PolicyToken{};
  }

 private:
  void skip_ws_() {
    while (pos_ < src_.size() &&
           std::isspace(static_cast<unsigned char>(src_[pos_]))) {
      ++pos_;
    }
  }

  std::string_view src_;
  std::size_t pos_{0};
};
}  // namespace detail

inline t81::expected<Policy, std::string> parse_policy(std::string_view text) {
  auto make_error = [](std::string msg) {
    return t81::expected<Policy, std::string>(std::move(msg));
  };
  detail::PolicyLexer lex(text);
  auto tok = lex.next();
  if (tok.kind != detail::PolicyToken::Kind::LParen) {
    return make_error("policy must start with '('");
  }
  tok = lex.next();
  if (tok.kind != detail::PolicyToken::Kind::Symbol || tok.text != "policy") {
    return make_error("root symbol must be 'policy'");
  }
  Policy policy;
  while (true) {
    tok = lex.next();
    if (tok.kind == detail::PolicyToken::Kind::RParen) break;
    if (tok.kind != detail::PolicyToken::Kind::LParen) {
      return make_error("expected '(' inside policy body");
    }
    auto key = lex.next();
    if (key.kind != detail::PolicyToken::Kind::Symbol) {
      return make_error("expected policy field symbol");
    }
    if (key.text == "tier") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Integer) {
        return make_error("tier requires integer");
      }
      policy.tier = static_cast<int>(val.value);
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "max-stack") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Integer) {
        return make_error("max-stack requires integer");
      }
      policy.max_stack = val.value;
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "loop") {
      Policy::LoopHint hint;
      while (true) {
        auto field_open = lex.next();
        if (field_open.kind == detail::PolicyToken::Kind::RParen) break;
        if (field_open.kind != detail::PolicyToken::Kind::LParen) {
          return make_error("expected '(' before loop field");
        }
        auto field = lex.next();
        if (field.kind != detail::PolicyToken::Kind::Symbol) {
          return make_error("expected loop field symbol");
        }
        auto val = lex.next();
        if (val.kind == detail::PolicyToken::Kind::End) {
          return make_error("unterminated loop clause");
        }
        if (field.text == "id") {
          if (val.kind != detail::PolicyToken::Kind::Integer) {
            return make_error("loop id requires integer");
          }
          hint.id = static_cast<int>(val.value);
        } else if (field.text == "file") {
          if (val.kind != detail::PolicyToken::Kind::Symbol) {
            return make_error("loop file requires symbol");
          }
          hint.file = val.text;
        } else if (field.text == "line") {
          if (val.kind != detail::PolicyToken::Kind::Integer) {
            return make_error("loop line requires integer");
          }
          hint.line = static_cast<int>(val.value);
        } else if (field.text == "column") {
          if (val.kind != detail::PolicyToken::Kind::Integer) {
            return make_error("loop column requires integer");
          }
          hint.column = static_cast<int>(val.value);
        } else if (field.text == "annotated") {
          if (val.kind != detail::PolicyToken::Kind::Symbol) {
            return make_error("loop annotated requires symbol");
          }
          hint.annotated = (val.text == "true");
        } else if (field.text == "depth") {
          if (val.kind != detail::PolicyToken::Kind::Integer) {
            return make_error("loop depth requires integer");
          }
          hint.depth = static_cast<int>(val.value);
        } else if (field.text == "bound") {
          if (val.kind == detail::PolicyToken::Kind::Symbol && val.text == "infinite") {
            hint.bound_infinite = true;
          } else if (val.kind == detail::PolicyToken::Kind::Integer) {
            hint.bound_value = val.value;
          } else {
            return make_error("loop bound must be 'infinite' or integer");
          }
        }
        auto field_close = lex.next();
        if (field_close.kind != detail::PolicyToken::Kind::RParen) {
          return make_error("expected ')' after loop field");
        }
      }
      policy.loops.push_back(hint);
      continue;
    }
    // Unknown clause -> skip forms deterministically.
    int depth = 1;
    while (depth > 0) {
      auto skip_tok = lex.next();
      if (skip_tok.kind == detail::PolicyToken::Kind::LParen) ++depth;
      else if (skip_tok.kind == detail::PolicyToken::Kind::RParen) --depth;
      else if (skip_tok.kind == detail::PolicyToken::Kind::End) {
        return make_error("unterminated policy clause");
      }
    }
  }
  return policy;
}
}  // namespace t81::axion
