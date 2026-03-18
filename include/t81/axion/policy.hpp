#pragma once

#include <cctype>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "t81/support/expected.hpp"

namespace t81::axion {

// Binary policy tags for TLV encoding
enum class PolicyTag : uint8_t {
  Header = 0x01,
  Tier = 0x02,
  MaxStack = 0x03,
  MaxInstructions = 0x04,
  MaxRecursion = 0x05,
  MaxReflections = 0x0B,
  MaxMetaWrites = 0x0C,
  AllowedTensorHashes = 0x0D,
  LoopHint = 0x06,
  MatchGuard = 0x07,
  SegmentEvent = 0x08,
  AxionEvent = 0x09,
  Alignment = 0x0A,
  BytecodeHeader = 0x10,
  End = 0xFF
};

// Axion Policy Bytecode Opcodes
enum class AxionOp : uint8_t {
  CheckTier = 0x01,
  LimitInstructions = 0x02,
  LimitStack = 0x03,
  LimitRecursion = 0x04,
  LimitReflections = 0x0A,
  LimitMetaWrites = 0x0B,
  RequireLoop = 0x05,
  RequireMatchGuard = 0x06,
  RequireSegmentEvent = 0x07,
  RequireAxionEvent = 0x08,
  RequireAlignment = 0x09,
  Ret = 0xFF
};

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

  struct MatchGuardRequirement {
    std::string enum_name;
    std::string variant_name;
    std::optional<std::string> payload;
    std::string result{"pass"};
  };

  struct SegmentEventRequirement {
    std::string segment;
    std::string action;
    std::optional<int64_t> addr;
  };
  struct AxionEventRequirement {
    std::string reason;
  };
  struct AlignmentRequirement {
    std::string reason;
  };
  struct ActivationCeilingPolicy {
    std::optional<double> max_nonzero_fraction;
    std::vector<std::string> mode_mask;
    std::string scope{"thread"};
  };

  int tier{1};
  std::optional<int64_t> max_stack;
  std::optional<int64_t> max_instructions;
  std::optional<int64_t> max_recursion;
  std::optional<int64_t> max_reflections;
  std::optional<int64_t> max_meta_writes;
  std::optional<int64_t> max_tensors;
  std::optional<int64_t> max_tensor_elements;
  std::optional<int64_t> max_symbolic_nodes;
  std::optional<int64_t> max_symbolic_graphs;
  std::optional<int64_t> max_infinite_forms;
  std::vector<std::string> allowed_tensor_hashes;
  std::vector<std::string> allowed_ternary_model_hashes;
  bool ternary_weight_domain_check{true};
  // RFC-0034 §3.3 — activation-ceiling directive for TACT post-execute gate.
  // When set, the nonzero-trit fraction of TACT output is checked against this
  // threshold. Exceeding it triggers Quarantine (SecurityFault) or Deny
  // (ActivationFault) depending on the verdict returned by the Axion engine.
  std::optional<double> activation_ceiling_max_nonzero_fraction;  // range [0.0, 1.0]
  std::optional<ActivationCeilingPolicy> activation_ceiling_policy;
  std::vector<LoopHint> loops;
  std::vector<MatchGuardRequirement> match_guards;
  std::vector<SegmentEventRequirement> segment_requirements;
  std::vector<AxionEventRequirement> axion_event_requirements;
  std::vector<AlignmentRequirement> alignment_requirements;

  // New bytecode-related members
  std::vector<uint8_t> bytecode;
  std::vector<std::string> symbol_table;

  void serialize(std::ostream& os) const;
  void compile_to_bytecode();  // NEW: Emitter
  static t81::expected<Policy, std::string> deserialize(std::istream& is);
};

namespace detail {
struct PolicyToken {
  enum class Kind {
    LParen,
    RParen,
    LBracket,
    RBracket,
    Integer,
    Float,
    Symbol,
    String,
    End
  } kind{Kind::End};
  std::string text;
  int64_t value{0};
  double float_value{0.0};
};

class PolicyLexer {
public:
  explicit PolicyLexer(std::string_view src) : src_(src) {}

  PolicyToken next() {
    skip_ws_();
    if (pos_ >= src_.size()) return PolicyToken{};
    char c = src_[pos_];
    if (c == '(') {
      ++pos_;
      return PolicyToken{PolicyToken::Kind::LParen, {}};
    }
    if (c == ')') {
      ++pos_;
      return PolicyToken{PolicyToken::Kind::RParen, {}};
    }
    if (c == '[') {
      ++pos_;
      return PolicyToken{PolicyToken::Kind::LBracket, {}};
    }
    if (c == ']') {
      ++pos_;
      return PolicyToken{PolicyToken::Kind::RBracket, {}};
    }
    if (std::isdigit(static_cast<unsigned char>(c)) || c == '-' || c == '+') {
      std::size_t start = pos_;
      ++pos_;
      bool saw_dot = false;
      while (pos_ < src_.size()) {
        const char ch = src_[pos_];
        if (std::isdigit(static_cast<unsigned char>(ch))) {
          ++pos_;
          continue;
        }
        if (ch == '.' && !saw_dot) {
          saw_dot = true;
          ++pos_;
          continue;
        }
        break;
      }
      PolicyToken tok;
      tok.text = std::string(src_.substr(start, pos_ - start));
      if (saw_dot) {
        tok.kind = PolicyToken::Kind::Float;
        tok.float_value = std::stod(tok.text);
      } else {
        tok.kind = PolicyToken::Kind::Integer;
        tok.value = std::stoll(tok.text);
      }
      return tok;
    }
    if (c == '"') {
      ++pos_;
      std::size_t start = pos_;
      while (pos_ < src_.size() && src_[pos_] != '"') {
        ++pos_;
      }
      PolicyToken tok;
      tok.kind = PolicyToken::Kind::String;
      tok.text = std::string(src_.substr(start, pos_ - start));
      if (pos_ < src_.size() && src_[pos_] == '"') ++pos_;
      return tok;
    }
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
      std::size_t start = pos_;
      ++pos_;
      while (pos_ < src_.size()) {
        char ch = src_[pos_];
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '-' || ch == '_')) {
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
    while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) {
      ++pos_;
    }
  }

  std::string_view src_;
  std::size_t pos_{0};
};
}  // namespace detail

inline t81::expected<Policy, std::string> parse_policy(std::string_view text) {
  auto make_error = [](std::string msg) {
    return t81::expected<Policy, std::string>(t81::unexpect, std::move(msg));
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
    if (key.text == "max-reflections") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Integer) {
        return make_error("max-reflections requires integer");
      }
      policy.max_reflections = val.value;
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "max-meta-writes") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Integer) {
        return make_error("max-meta-writes requires integer");
      }
      policy.max_meta_writes = val.value;
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "max-tensors") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Integer) {
        return make_error("max-tensors requires integer");
      }
      policy.max_tensors = val.value;
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "max-tensor-elements") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Integer) {
        return make_error("max-tensor-elements requires integer");
      }
      policy.max_tensor_elements = val.value;
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "max-symbolic-graphs") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Integer) {
        return make_error("max-symbolic-graphs requires integer");
      }
      policy.max_symbolic_graphs = val.value;
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "max-symbolic-nodes") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Integer) {
        return make_error("max-symbolic-nodes requires integer");
      }
      policy.max_symbolic_nodes = val.value;
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "max-infinite-forms") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Integer) {
        return make_error("max-infinite-forms requires integer");
      }
      policy.max_infinite_forms = val.value;
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "max-instructions") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Integer) {
        return make_error("max-instructions requires integer");
      }
      policy.max_instructions = val.value;
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "activation-ceiling") {
      Policy::ActivationCeilingPolicy ceiling;
      auto val = lex.next();
      if (val.kind == detail::PolicyToken::Kind::Integer || val.kind == detail::PolicyToken::Kind::Float) {
        const double frac =
            val.kind == detail::PolicyToken::Kind::Float ? val.float_value : static_cast<double>(val.value);
        ceiling.max_nonzero_fraction = frac;
        policy.activation_ceiling_max_nonzero_fraction = frac;
        policy.activation_ceiling_policy = ceiling;
        tok = lex.next();
        if (tok.kind != detail::PolicyToken::Kind::RParen) {
          return make_error("expected ')'");
        }
        continue;
      }
      if (val.kind != detail::PolicyToken::Kind::LParen) {
        return make_error("activation-ceiling requires numeric threshold or clause list");
      }
      while (true) {
        auto field = lex.next();
        if (field.kind != detail::PolicyToken::Kind::Symbol) {
          return make_error("expected activation-ceiling field symbol");
        }
        if (field.text == "max-nonzero-fraction") {
          auto frac = lex.next();
          if (frac.kind != detail::PolicyToken::Kind::Integer &&
              frac.kind != detail::PolicyToken::Kind::Float) {
            return make_error("activation-ceiling max-nonzero-fraction requires numeric value");
          }
          ceiling.max_nonzero_fraction =
              frac.kind == detail::PolicyToken::Kind::Float ? frac.float_value
                                                            : static_cast<double>(frac.value);
        } else if (field.text == "scope") {
          auto scope = lex.next();
          if (scope.kind != detail::PolicyToken::Kind::Symbol &&
              scope.kind != detail::PolicyToken::Kind::String) {
            return make_error("activation-ceiling scope requires symbol or string");
          }
          ceiling.scope = scope.text;
        } else if (field.text == "mode-mask") {
          auto open = lex.next();
          if (open.kind != detail::PolicyToken::Kind::LBracket) {
            return make_error("activation-ceiling mode-mask requires '['");
          }
          while (true) {
            auto mode = lex.next();
            if (mode.kind == detail::PolicyToken::Kind::RBracket) break;
            if (mode.kind != detail::PolicyToken::Kind::Symbol &&
                mode.kind != detail::PolicyToken::Kind::String) {
              return make_error("activation-ceiling mode-mask entries require symbol or string");
            }
            ceiling.mode_mask.push_back(mode.text);
          }
        } else {
          return make_error("unknown activation-ceiling field");
        }
        auto field_close = lex.next();
        if (field_close.kind != detail::PolicyToken::Kind::RParen) {
          return make_error("expected ')' after activation-ceiling field");
        }
        auto next = lex.next();
        if (next.kind == detail::PolicyToken::Kind::RParen) {
          break;
        }
        if (next.kind != detail::PolicyToken::Kind::LParen) {
          return make_error("expected '(' before activation-ceiling field");
        }
      }
      if (ceiling.max_nonzero_fraction.has_value()) {
        policy.activation_ceiling_max_nonzero_fraction = *ceiling.max_nonzero_fraction;
      }
      policy.activation_ceiling_policy = std::move(ceiling);
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "max-recursion") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Integer) {
        return make_error("max-recursion requires integer");
      }
      policy.max_recursion = val.value;
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
    if (key.text == "max-tensors") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Integer) {
        return make_error("max-tensors requires integer");
      }
      policy.max_tensors = val.value;
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "max-tensor-elements") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Integer) {
        return make_error("max-tensor-elements requires integer");
      }
      policy.max_tensor_elements = val.value;
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "max-symbolic-nodes") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Integer) {
        return make_error("max-symbolic-nodes requires integer");
      }
      policy.max_symbolic_nodes = val.value;
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "allowed-tensor-hashes") {
      auto bracket = lex.next();
      if (bracket.kind != detail::PolicyToken::Kind::LBracket) {
        return make_error("allowed-tensor-hashes requires '['");
      }
      while (true) {
        auto val = lex.next();
        if (val.kind == detail::PolicyToken::Kind::RBracket) break;
        if (val.kind == detail::PolicyToken::Kind::End) {
          return make_error("unterminated allowed-tensor-hashes list");
        }
        if (val.kind != detail::PolicyToken::Kind::String) {
          return make_error("allowed-tensor-hashes requires string literals");
        }
        policy.allowed_tensor_hashes.push_back(val.text);
      }
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "allowed-ternary-model-hashes") {
      auto bracket = lex.next();
      if (bracket.kind != detail::PolicyToken::Kind::LBracket) {
        return make_error("allowed-ternary-model-hashes requires '['");
      }
      while (true) {
        auto val = lex.next();
        if (val.kind == detail::PolicyToken::Kind::RBracket) break;
        if (val.kind == detail::PolicyToken::Kind::End) {
          return make_error("unterminated allowed-ternary-model-hashes list");
        }
        if (val.kind != detail::PolicyToken::Kind::String) {
          return make_error("allowed-ternary-model-hashes requires string literals");
        }
        policy.allowed_ternary_model_hashes.push_back(val.text);
      }
      tok = lex.next();
      if (tok.kind != detail::PolicyToken::Kind::RParen) {
        return make_error("expected ')'");
      }
      continue;
    }
    if (key.text == "ternary-weight-domain-check") {
      auto val = lex.next();
      if (val.kind != detail::PolicyToken::Kind::Symbol &&
          val.kind != detail::PolicyToken::Kind::String) {
        return make_error("ternary-weight-domain-check requires true/false");
      }
      if (val.text == "true") {
        policy.ternary_weight_domain_check = true;
      } else if (val.text == "false") {
        policy.ternary_weight_domain_check = false;
      } else {
        return make_error("ternary-weight-domain-check requires true or false");
      }
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
          if (val.kind != detail::PolicyToken::Kind::Symbol &&
              val.kind != detail::PolicyToken::Kind::String) {
            return make_error("loop file requires symbol or string");
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
          if (val.kind != detail::PolicyToken::Kind::Symbol &&
              val.kind != detail::PolicyToken::Kind::String) {
            return make_error("loop annotated requires symbol or string");
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
    if (key.text == "require-match-guard") {
      Policy::MatchGuardRequirement req;
      while (true) {
        auto field_open = lex.next();
        if (field_open.kind == detail::PolicyToken::Kind::RParen) break;
        if (field_open.kind != detail::PolicyToken::Kind::LParen) {
          return make_error("expected '(' before match guard field");
        }
        auto field = lex.next();
        if (field.kind != detail::PolicyToken::Kind::Symbol) {
          return make_error("expected match guard field symbol");
        }
        auto val = lex.next();
        if (val.kind == detail::PolicyToken::Kind::End) {
          return make_error("unterminated match guard clause");
        }
        if (field.text == "enum") {
          if (val.kind != detail::PolicyToken::Kind::Symbol &&
              val.kind != detail::PolicyToken::Kind::String) {
            return make_error("match guard enum requires symbol or string");
          }
          req.enum_name = val.text;
        } else if (field.text == "variant") {
          if (val.kind != detail::PolicyToken::Kind::Symbol &&
              val.kind != detail::PolicyToken::Kind::String) {
            return make_error("match guard variant requires symbol or string");
          }
          req.variant_name = val.text;
        } else if (field.text == "payload") {
          if (val.kind != detail::PolicyToken::Kind::Symbol &&
              val.kind != detail::PolicyToken::Kind::String) {
            return make_error("match guard payload requires symbol or string");
          }
          req.payload = val.text;
        } else if (field.text == "result") {
          if (val.kind != detail::PolicyToken::Kind::Symbol &&
              val.kind != detail::PolicyToken::Kind::String) {
            return make_error("match guard result requires symbol or string");
          }
          req.result = val.text;
        } else {
          return make_error("unknown match guard field");
        }
        auto field_close = lex.next();
        if (field_close.kind != detail::PolicyToken::Kind::RParen) {
          return make_error("expected ')' after match guard field");
        }
      }
      if (req.enum_name.empty() || req.variant_name.empty()) {
        return make_error("match guard requires enum and variant names");
      }
      policy.match_guards.push_back(std::move(req));
      continue;
    }
    if (key.text == "require-segment-event") {
      Policy::SegmentEventRequirement req;
      while (true) {
        auto field_open = lex.next();
        if (field_open.kind == detail::PolicyToken::Kind::RParen) break;
        if (field_open.kind != detail::PolicyToken::Kind::LParen) {
          return make_error("expected '(' before segment field");
        }
        auto field = lex.next();
        if (field.kind != detail::PolicyToken::Kind::Symbol) {
          return make_error("expected segment field symbol");
        }
        auto val = lex.next();
        if (val.kind == detail::PolicyToken::Kind::End) {
          return make_error("unterminated segment clause");
        }
        if (field.text == "segment") {
          if (val.kind != detail::PolicyToken::Kind::Symbol &&
              val.kind != detail::PolicyToken::Kind::String) {
            return make_error("segment requires symbol or string");
          }
          req.segment = val.text;
        } else if (field.text == "action") {
          if (val.kind != detail::PolicyToken::Kind::Symbol &&
              val.kind != detail::PolicyToken::Kind::String) {
            return make_error("action requires symbol or string");
          }
          req.action = val.text;
        } else if (field.text == "addr") {
          if (val.kind != detail::PolicyToken::Kind::Integer) {
            return make_error("segment addr requires integer");
          }
          req.addr = val.value;
        } else {
          return make_error("unknown segment field");
        }
        auto field_close = lex.next();
        if (field_close.kind != detail::PolicyToken::Kind::RParen) {
          return make_error("expected ')' after segment field");
        }
      }
      if (req.segment.empty() || req.action.empty()) {
        return make_error("segment event requires segment and action");
      }
      policy.segment_requirements.push_back(std::move(req));
      continue;
    }
    if (key.text == "require-axion-event") {
      Policy::AxionEventRequirement req;
      while (true) {
        auto field_open = lex.next();
        if (field_open.kind == detail::PolicyToken::Kind::RParen) break;
        if (field_open.kind != detail::PolicyToken::Kind::LParen) {
          return make_error("expected '(' before axion event field");
        }
        auto field = lex.next();
        if (field.kind != detail::PolicyToken::Kind::Symbol) {
          return make_error("expected axion event field symbol");
        }
        auto val = lex.next();
        if (val.kind == detail::PolicyToken::Kind::End) {
          return make_error("unterminated axion event clause");
        }
        if (field.text == "reason") {
          if (val.kind != detail::PolicyToken::Kind::String &&
              val.kind != detail::PolicyToken::Kind::Symbol) {
            return make_error("axion event reason requires symbol or string");
          }
          req.reason = val.text;
        } else {
          return make_error("unknown axion event field");
        }
        auto field_close = lex.next();
        if (field_close.kind != detail::PolicyToken::Kind::RParen) {
          return make_error("expected ')' after axion event field");
        }
      }
      if (req.reason.empty()) {
        return make_error("axion event requires reason");
      }
      policy.axion_event_requirements.push_back(std::move(req));
      continue;
    }
    if (key.text == "require-alignment") {
      Policy::AlignmentRequirement req;
      while (true) {
        auto field_open = lex.next();
        if (field_open.kind == detail::PolicyToken::Kind::RParen) break;
        if (field_open.kind != detail::PolicyToken::Kind::LParen) {
          return make_error("expected '(' before alignment field");
        }
        auto field = lex.next();
        if (field.kind != detail::PolicyToken::Kind::Symbol) {
          return make_error("expected alignment field symbol");
        }
        auto val = lex.next();
        if (val.kind == detail::PolicyToken::Kind::End) {
          return make_error("unterminated alignment clause");
        }
        if (field.text == "reason") {
          if (val.kind != detail::PolicyToken::Kind::String &&
              val.kind != detail::PolicyToken::Kind::Symbol) {
            return make_error("alignment reason requires symbol or string");
          }
          req.reason = val.text;
        } else {
          return make_error("unknown alignment field");
        }
        auto field_close = lex.next();
        if (field_close.kind != detail::PolicyToken::Kind::RParen) {
          return make_error("expected ')' after alignment field");
        }
      }
      if (req.reason.empty()) {
        return make_error("alignment requires reason");
      }
      policy.alignment_requirements.push_back(std::move(req));
      continue;
    }
    return make_error("unknown policy clause: " + key.text);
  }
  return policy;
}
}  // namespace t81::axion
