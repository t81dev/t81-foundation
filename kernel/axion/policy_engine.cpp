#include "t81/axion/policy_engine.hpp"

#include <sstream>
#include "t81/axion/ethics.hpp"
#include "t81/axion/policy_validator.hpp"
#include "t81/isa/opcodes.hpp"

namespace t81::axion {

PolicyEngine::PolicyEngine(std::optional<Policy> policy) : policy_(std::move(policy)) {
  if (policy_ && !policy_->loops.empty()) {
    loop_reqs_.reserve(policy_->loops.size());
    for (const auto& hint : policy_->loops) {
      InternalLoopReq req;
      req.hint = &hint;
      std::ostringstream expect;
      expect << "loop hint file=" << hint.file << " line=" << hint.line << " column=" << hint.column
             << " bound=";
      if (hint.bound_infinite) {
        expect << "infinite";
      } else if (hint.bound_value) {
        expect << *hint.bound_value;
      } else {
        expect << "unknown";
      }
      req.expected_reason = expect.str();
      req.satisfied = false;
      loop_reqs_.push_back(std::move(req));
    }
  }
}

Verdict PolicyEngine::execute_bytecode(const SyscallContext& ctx) {
  if (!policy_ || policy_->bytecode.empty()) {
    return Verdict{VerdictKind::Allow, "Axion policy engine (no bytecode)"};
  }

  const uint8_t* pc = policy_->bytecode.data();
  const uint8_t* end = pc + policy_->bytecode.size();

  auto read_u32 = [&]() {
    uint32_t v = 0;
    v |= *pc++;
    v |= (static_cast<uint32_t>(*pc++) << 8);
    v |= (static_cast<uint32_t>(*pc++) << 16);
    v |= (static_cast<uint32_t>(*pc++) << 24);
    return v;
  };
  auto read_u64 = [&]() {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= (static_cast<uint64_t>(*pc++) << (i * 8));
    return v;
  };
  auto get_sym = [&](uint32_t idx) -> std::string_view {
    if (idx >= policy_->symbol_table.size()) return "";
    return policy_->symbol_table[idx];
  };

  const bool final_instruction = ctx.next_opcode == t81::tisc::Opcode::Halt;

  while (pc < end) {
    AxionOp op = static_cast<AxionOp>(*pc++);
    switch (op) {
      case AxionOp::CheckTier: {
        uint32_t required = read_u32();
        if (static_cast<uint32_t>(ctx.current_tier) < required) {
          std::ostringstream ss;
          ss << "Tier check failed: current=" << ctx.current_tier << " required=" << required;
          return Verdict{VerdictKind::Deny, ss.str()};
        }
        break;
      }
      case AxionOp::LimitInstructions: {
        uint64_t limit = read_u64();
        if (ctx.instruction_count > limit) {
          std::ostringstream ss;
          ss << "Instruction count limit exceeded: count=" << ctx.instruction_count
             << " limit=" << limit;
          return Verdict{VerdictKind::Deny, ss.str()};
        }
        break;
      }
      case AxionOp::LimitReflections: {
        uint64_t limit = read_u64();
        if (ctx.reflection_count > limit) {
          std::ostringstream ss;
          ss << "Reflection count limit exceeded: count=" << ctx.reflection_count
             << " limit=" << limit;
          return Verdict{VerdictKind::Deny, ss.str()};
        }
        break;
      }
      case AxionOp::LimitMetaWrites: {
        uint64_t limit = read_u64();
        if (ctx.meta_write_count > limit) {
          std::ostringstream ss;
          ss << "Meta write count limit exceeded: count=" << ctx.meta_write_count
             << " limit=" << limit;
          return Verdict{VerdictKind::Deny, ss.str()};
        }
        break;
      }
      case AxionOp::LimitStack: {
        uint64_t limit = read_u64();
        if (ctx.stack_usage > limit) {
          std::ostringstream ss;
          ss << "Stack usage limit exceeded: usage=" << ctx.stack_usage << " limit=" << limit;
          return Verdict{VerdictKind::Deny, ss.str()};
        }
        break;
      }
      case AxionOp::LimitRecursion: {
        uint64_t limit = read_u64();
        if (ctx.recursion_depth > limit) {
          std::ostringstream ss;
          ss << "Recursion depth limit exceeded: depth=" << ctx.recursion_depth
             << " limit=" << limit;
          return Verdict{VerdictKind::Deny, ss.str()};
        }
        break;
      }
      case AxionOp::RequireLoop: {
        uint32_t id = read_u32();
        std::string_view file = get_sym(read_u32());
        uint32_t line = read_u32();
        uint32_t col = read_u32();
        uint8_t infinite = *pc++;
        uint64_t bound = read_u64();

        std::ostringstream expect;
        expect << "loop hint file=" << file << " line=" << line << " column=" << col << " bound=";
        if (infinite)
          expect << "infinite";
        else
          expect << bound;

        bool satisfied = false;
        for (const auto& entry : ctx.trace_reasons) {
          if (entry.find(expect.str()) != std::string::npos) {
            satisfied = true;
            break;
          }
        }
        if (!satisfied) {
          return Verdict{VerdictKind::Deny, "Missing loop hint trace: " + expect.str()};
        }
        (void)id;
        break;
      }
      case AxionOp::RequireMatchGuard: {
        std::string_view enum_name = get_sym(read_u32());
        std::string_view variant_name = get_sym(read_u32());
        uint32_t payload_idx = read_u32();
        std::string_view result = get_sym(read_u32());

        if (final_instruction) {
          Policy::MatchGuardRequirement req;
          req.enum_name = enum_name;
          req.variant_name = variant_name;
          if (payload_idx != 0xFFFFFFFF) req.payload = get_sym(payload_idx);
          req.result = result;
          if (!match_guard_satisfied(ctx, req)) {
            return Verdict{VerdictKind::Deny, "Missing match guard event"};
          }
        }
        break;
      }
      case AxionOp::RequireSegmentEvent: {
        std::string_view segment = get_sym(read_u32());
        std::string_view action = get_sym(read_u32());
        uint8_t has_addr = *pc++;
        uint64_t addr = read_u64();

        if (final_instruction) {
          Policy::SegmentEventRequirement req;
          req.segment = segment;
          req.action = action;
          if (has_addr) req.addr = static_cast<int64_t>(addr);
          if (!segment_event_satisfied(ctx, req)) {
            return Verdict{VerdictKind::Deny, "Missing segment event"};
          }
        }
        break;
      }
      case AxionOp::RequireAxionEvent: {
        std::string_view reason = get_sym(read_u32());
        if (final_instruction) {
          Policy::AxionEventRequirement req{std::string(reason)};
          if (!axion_event_satisfied(ctx, req)) {
            return Verdict{VerdictKind::Deny,
                           "Missing Axion event reason containing \"" + std::string(reason) + "\""};
          }
        }
        break;
      }
      case AxionOp::RequireAlignment: {
        std::string_view reason = get_sym(read_u32());
        if (final_instruction) {
          Policy::AlignmentRequirement req{std::string(reason)};
          if (!alignment_event_satisfied(ctx, req)) {
            return Verdict{VerdictKind::Deny,
                           "Missing alignment event: reason=\"" + std::string(reason) + "\""};
          }
        }
        break;
      }
      case AxionOp::Ret:
        return Verdict{VerdictKind::Allow, "Axion bytecode executed successfully"};
      default:
        return Verdict{VerdictKind::Deny, "Unknown Axion opcode"};
    }
  }
  return Verdict{VerdictKind::Allow, "Axion bytecode end reached"};
}

Verdict PolicyEngine::evaluate(const SyscallContext& ctx) {
  Verdict warning = {VerdictKind::Allow, ""};

  // 1. Immutable Ethics Check (Theta-1 - Theta-9)
  // These principles take precedence over user policy.
  for (int i = 1; i <= kEthicsPrincipleCount; ++i) {
    auto principle = static_cast<EthicsPrinciple>(i);
    Verdict v = check_ethics(principle, ctx);
    if (v.kind == VerdictKind::Deny) {
      return v;
    }
    if (v.kind == VerdictKind::Warn && warning.kind == VerdictKind::Allow) {
      warning = v;
    }
  }

  Verdict v = evaluate_internal(ctx);
  if (v.kind == VerdictKind::Allow && warning.kind == VerdictKind::Warn) {
    return warning;
  }
  return v;
}

Verdict PolicyEngine::evaluate_internal(const SyscallContext& ctx) {
  if (!policy_) {
    if (ctx.next_opcode == t81::tisc::Opcode::TLoadHash) {
      return Verdict{VerdictKind::Deny, "TLOADHASH requires active policy"};
    }
    return Verdict{VerdictKind::Allow, "Axion policy engine (no policy)"};
  }
  if (ctx.next_opcode == t81::tisc::Opcode::TLoadHash) {
    if (policy_->allowed_tensor_hashes.empty()) {
      return Verdict{VerdictKind::Deny, "TLOADHASH denied (allowed-tensor-hashes empty)"};
    }
    if (!ctx.payload.empty()) {
      bool found = false;
      for (const auto& h : policy_->allowed_tensor_hashes) {
        if (h == ctx.payload) {
          found = true;
          break;
        }
      }
      if (!found) {
        std::ostringstream ss;
        ss << "TLOADHASH policy_violation hash=" << ctx.payload;
        return Verdict{VerdictKind::Deny, ss.str()};
      }
    }
    // Allow logic continues below...
  }

  if (!policy_->bytecode.empty()) {
    return execute_bytecode(ctx);
  }
  if (policy_->max_instructions &&
      ctx.instruction_count > static_cast<std::size_t>(*policy_->max_instructions)) {
    std::ostringstream reason;
    reason << "Instruction count limit exceeded: count=" << ctx.instruction_count
           << " limit=" << *policy_->max_instructions;
    return Verdict{VerdictKind::Deny, reason.str()};
  }
  if (policy_->max_recursion &&
      ctx.recursion_depth > static_cast<std::size_t>(*policy_->max_recursion)) {
    std::ostringstream reason;
    reason << "Recursion depth limit exceeded: depth=" << ctx.recursion_depth
           << " limit=" << *policy_->max_recursion;
    return Verdict{VerdictKind::Deny, reason.str()};
  }
  if (policy_->max_stack && ctx.stack_usage > static_cast<std::size_t>(*policy_->max_stack)) {
    std::ostringstream reason;
    reason << "Stack usage limit exceeded: usage=" << ctx.stack_usage
           << " limit=" << *policy_->max_stack;
    return Verdict{VerdictKind::Deny, reason.str()};
  }
  if (policy_->max_reflections &&
      ctx.reflection_count > static_cast<std::size_t>(*policy_->max_reflections)) {
    std::ostringstream reason;
    reason << "Reflection count limit exceeded: count=" << ctx.reflection_count
           << " limit=" << *policy_->max_reflections;
    return Verdict{VerdictKind::Deny, reason.str()};
  }
  if (policy_->max_meta_writes &&
      ctx.meta_write_count > static_cast<std::size_t>(*policy_->max_meta_writes)) {
    std::ostringstream reason;
    reason << "Meta write count limit exceeded: count=" << ctx.meta_write_count
           << " limit=" << *policy_->max_meta_writes;
    return Verdict{VerdictKind::Deny, reason.str()};
  }
  for (size_t i = 0; i < loop_reqs_.size(); ++i) {
    if (!loop_hint_satisfied(ctx, i)) {
      std::ostringstream reason;
      reason << "Missing loop hint trace: " << loop_reqs_[i].expected_reason;
      return Verdict{VerdictKind::Deny, reason.str()};
    }
  }
  const bool final_instruction = ctx.next_opcode == t81::tisc::Opcode::Halt;
  if (final_instruction) {
    for (const auto& req : policy_->match_guards) {
      if (!match_guard_satisfied(ctx, req)) {
        std::ostringstream reason;
        reason << "Missing match guard event: enum=" << req.enum_name
               << " variant=" << req.variant_name;
        if (req.payload) reason << " payload=" << *req.payload;
        reason << " result=" << req.result;
        return Verdict{VerdictKind::Deny, reason.str()};
      }
    }
    for (const auto& req : policy_->segment_requirements) {
      if (!segment_event_satisfied(ctx, req)) {
        std::ostringstream reason;
        reason << "Missing segment event: action=\"" << req.action << "\""
               << " segment=" << req.segment;
        if (req.addr) reason << " addr=" << *req.addr;
        return Verdict{VerdictKind::Deny, reason.str()};
      }
    }
    for (const auto& req : policy_->axion_event_requirements) {
      if (!axion_event_satisfied(ctx, req)) {
        std::ostringstream reason;
        reason << "Missing Axion event reason containing \"" << req.reason << "\"";
        return Verdict{VerdictKind::Deny, reason.str()};
      }
    }
    for (const auto& req : policy_->alignment_requirements) {
      if (!alignment_event_satisfied(ctx, req)) {
        std::ostringstream reason;
        reason << "Missing alignment event: reason=\"" << req.reason << "\"";
        return Verdict{VerdictKind::Deny, reason.str()};
      }
    }
  }
  return Verdict{VerdictKind::Allow, "Axion policy engine (loop hints satisfied)"};
}

bool PolicyEngine::loop_hint_satisfied(const SyscallContext& ctx, size_t requirement_idx) const {
  auto& req = loop_reqs_[requirement_idx];
  if (req.satisfied) return true;

  for (const auto& entry : ctx.trace_reasons) {
    if (entry.find(req.expected_reason) != std::string_view::npos) {
      req.satisfied = true;
      return true;
    }
  }
  return false;
}

bool PolicyEngine::alignment_event_satisfied(const SyscallContext& ctx,
                                             const Policy::AlignmentRequirement& req) const {
  for (const auto& entry : ctx.trace_reasons) {
    if (entry.find("alignment") != std::string_view::npos &&
        entry.find(req.reason) != std::string_view::npos) {
      return true;
    }
  }
  return false;
}

bool PolicyEngine::match_guard_satisfied(const SyscallContext& ctx,
                                         const Policy::MatchGuardRequirement& req) const {
  const std::string enum_token = "enum=" + req.enum_name;
  const std::string variant_token = "variant=" + req.variant_name;
  const std::string match_token = "match=" + req.result;
  const std::string payload_token = req.payload ? "payload=" + *req.payload : std::string();
  for (const auto& entry : ctx.trace_reasons) {
    if (entry.find("enum guard") == std::string_view::npos) continue;
    if (!req.enum_name.empty() && entry.find(enum_token) == std::string_view::npos) continue;
    if (!req.variant_name.empty() && entry.find(variant_token) == std::string_view::npos) continue;
    if (req.payload && entry.find(payload_token) == std::string_view::npos) continue;
    if (entry.find(match_token) == std::string_view::npos) continue;
    return true;
  }
  return false;
}

bool PolicyEngine::segment_event_satisfied(const SyscallContext& ctx,
                                           const Policy::SegmentEventRequirement& req) const {
  const std::string segment_eq = "segment=" + req.segment;
  const std::string segment_spaced = " " + req.segment + " ";
  const std::string addr_token = req.addr ? ("addr=" + std::to_string(*req.addr)) : std::string();
  for (const auto& entry : ctx.trace_reasons) {
    if (entry.find(req.action) == std::string_view::npos) continue;
    bool segment_ok = req.segment.empty();
    if (!segment_ok) {
      segment_ok = entry.find(segment_eq) != std::string_view::npos ||
                   entry.find(segment_spaced) != std::string_view::npos;
    }
    if (!segment_ok) continue;
    if (!addr_token.empty() && entry.find(addr_token) == std::string_view::npos) continue;
    return true;
  }
  return false;
}

bool PolicyEngine::axion_event_satisfied(const SyscallContext& ctx,
                                         const Policy::AxionEventRequirement& req) const {
  for (const auto& entry : ctx.trace_reasons) {
    if (entry.find(req.reason) != std::string_view::npos) {
      return true;
    }
  }
  return false;
}

std::optional<PolicyViolation> PolicyEngine::validate_policy(const std::string& policy_text) {
  return PolicyValidator::validate_policy(policy_text);
}

std::optional<PolicyViolation> PolicyEngine::validate_tensor_hash(const std::string& model_hash,
                                                                  const std::string& policy_text) {
  return PolicyValidator::validate_tensor_hash(model_hash, policy_text);
}

std::string PolicyEngine::generate_policy_report(const PolicyViolation& violation) {
  return PolicyValidator::generate_policy_report(violation);
}

std::unique_ptr<Engine> make_policy_engine(std::optional<Policy> policy) {
  return std::make_unique<PolicyEngine>(std::move(policy));
}

}  // namespace t81::axion
