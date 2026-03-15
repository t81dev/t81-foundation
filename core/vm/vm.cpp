#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <locale>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>

#include <string>
#include <string_view>
#include <utility>
#include "t81/fraction.hpp"
#include "t81/support/expected.hpp"
#include "t81/tensor.hpp"
#include "t81/tensor/llama.hpp"
#include "t81/tensor/matmul.hpp"

#include "internal/gc_helpers.hpp"
#include "internal/memory_segments.hpp"
#include "internal/policy_trace_bridge.hpp"
#include "internal/runtime_state_helpers.hpp"
#include "internal/tensor_helpers.hpp"
#include "internal/tier_limits.hpp"
#include "internal/value_ops.hpp"
#include "t81/axion/context.hpp"
#include "t81/axion/engine.hpp"
#include "t81/axion/nondeterminism_detector.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/axion/reasons.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/enum_meta.hpp"
#include "t81/jit/jit.hpp"
#include "t81/types/T81Float.hpp"
#include "t81/vm/vm.hpp"

namespace t81::vm {
namespace {
constexpr std::size_t kDefaultStackSize = 256;
constexpr std::size_t kDefaultHeapSize = 768;
constexpr std::size_t kDefaultTensorSpace = 256;
constexpr std::size_t kDefaultMetaSpace = 256;
constexpr std::size_t kHardRecursionCeiling = T81_HARD_RECURSION_CEILING;

enum class TierPromotionError {
  NotEligible,
  AxionDenied,
};

using TierPromotionResult = t81::expected<t81::cog::TierStatus, TierPromotionError>;

std::filesystem::path resolve_canonfs_root() {
  if (const char* raw = std::getenv("T81_CANONFS_ROOT"); raw != nullptr && raw[0] != '\0') {
    return std::filesystem::path(raw);
  }
  return std::filesystem::current_path() / ".t81_canonfs";
}

TierPromotionResult try_promote_tier(
    const t81::cog::TierStatus& status,
    const std::function<t81::axion::Verdict(const t81::axion::SyscallContext&)>& callback) {
  if (status.current == t81::cog::TierId::Tier5) {
    return TierPromotionResult(t81::unexpect, TierPromotionError::NotEligible);
  }

  t81::axion::SyscallContext syscall{{},      "system", "promote", "",
                                     nullptr, {},       0,         t81::tisc::Opcode::Nop};
  const auto verdict = callback(syscall);
  if (verdict.kind == t81::axion::VerdictKind::Deny) {
    return TierPromotionResult(t81::unexpect, TierPromotionError::AxionDenied);
  }

  t81::cog::TierStatus next = status;
  switch (status.current) {
    case t81::cog::TierId::Tier0:
      next.current = t81::cog::TierId::Tier1;
      next.label = "Tier1";
      break;
    case t81::cog::TierId::Tier1:
      next.current = t81::cog::TierId::Tier2;
      next.label = "Tier2";
      break;
    case t81::cog::TierId::Tier2:
      next.current = t81::cog::TierId::Tier3;
      next.label = "Tier3";
      break;
    case t81::cog::TierId::Tier3:
      next.current = t81::cog::TierId::Tier4;
      next.label = "Tier4";
      break;
    case t81::cog::TierId::Tier4:
      next.current = t81::cog::TierId::Tier5;
      next.label = "Tier5";
      break;
    default:
      return TierPromotionResult(t81::unexpect, TierPromotionError::NotEligible);
  }
  return next;
}

class DenyWithReasonEngine final : public t81::axion::Engine {
public:
  explicit DenyWithReasonEngine(std::string reason) : reason_(std::move(reason)) {}

  t81::axion::Verdict evaluate(const t81::axion::SyscallContext&) override {
    return {t81::axion::VerdictKind::Deny, reason_};
  }

private:
  std::string reason_;
};

std::unique_ptr<t81::axion::Engine> make_deny_with_reason_engine(std::string reason) {
  return std::make_unique<DenyWithReasonEngine>(std::move(reason));
}

using t81::vm::internal::max_branch_entropy_for_tier;
using t81::vm::internal::max_shape_complexity_for_tier;
using t81::vm::internal::max_symbolic_complexity_for_tier;
using t81::vm::internal::max_tensor_rank_for_tier;
using t81::vm::internal::recursion_limit_for_tier;
using t81::vm::internal::tier_from_rank;
using t81::vm::internal::tier_rank;

t81::T81Fraction fraction_from_double(double x) {
  if (x == 0.0) {
    return t81::T81Fraction{};
  }

  // Handle non-finite (NaN, Inf) or large values safely
  if (!std::isfinite(x) || std::abs(x) >= 9e18) {
    // Use intermediate float representation to convert large double/Inf/NaN to BigInt
    using TempFloat = t81::T81Float<72, 9>;
    auto f = TempFloat::from_double(x);
    t81::T81BigInt big = t81::T81BigInt::from_float(f);
    return t81::T81Fraction(std::move(big), t81::T81BigInt::one());
  }

  bool neg = x < 0.0;
  x = std::fabs(x);

  t81::T81BigInt integer(static_cast<std::int64_t>(x));
  double frac = x - static_cast<double>(integer.to_int64());

  t81::T81BigInt p0(1), q0(0);
  t81::T81BigInt p1(integer), q1(1);

  int iter = 0;
  // Limit iterations to prevent extremely large fractions from non-terminating expansions
  while (frac > 1e-15 && iter++ < 64) {
    const double r = 1.0 / frac;
    if (r > 9e18) break;  // Prevent overflow in int64 conversion
    const std::int64_t a = static_cast<std::int64_t>(r);

    t81::T81BigInt term_a(a);
    t81::T81BigInt next_p = term_a * p1 + p0;
    t81::T81BigInt next_q = term_a * q1 + q0;

    p0 = p1;
    q0 = q1;
    p1 = next_p;
    q1 = next_q;

    frac = r - static_cast<double>(a);
  }

  return t81::T81Fraction(neg ? -p1 : p1, q1);
}

}  // namespace
}  // namespace t81::vm

namespace t81::axion {

std::size_t DeterminismDetector::hash_event(const t81::vm::AxionEvent& ev, std::size_t seed) {
  const auto mix = [](std::size_t acc, std::size_t value) {
    acc ^= value + 0x9e3779b97f4a7c15ULL + (acc << 6U) + (acc >> 2U);
    return acc;
  };
  seed = mix(seed, static_cast<std::size_t>(ev.opcode));
  seed = mix(seed, static_cast<std::size_t>(ev.tag));
  seed = mix(seed, static_cast<std::size_t>(ev.value));
  seed = mix(seed, static_cast<std::size_t>(ev.verdict.kind));
  seed = mix(seed, std::hash<std::string>{}(ev.verdict.reason));
  seed = mix(seed, std::hash<std::string>{}(ev.structured.reason));
  seed = mix(seed, static_cast<std::size_t>(ev.structured.policy_id));
  seed = mix(seed, static_cast<std::size_t>(ev.structured.pc));
  seed = mix(seed, static_cast<std::size_t>(ev.structured.handle_id));
  seed = mix(seed, std::hash<std::string_view>{}(ev.structured.decision));
  seed = mix(seed, std::hash<std::string>{}(ev.structured.event_type));
  seed = mix(seed, std::hash<std::string>{}(ev.structured.reason_code));
  seed = mix(seed, std::hash<std::string>{}(ev.structured.storage_class));
  seed = mix(seed, std::hash<std::string>{}(ev.structured.numeric_class));
  seed = mix(seed, ev.structured.strict_core_eligible ? 1U : 0U);
  return seed;
}

void DeterminismDetector::record_run(const std::vector<t81::vm::AxionEvent>& log) {
  prev_hashes_ = std::move(curr_hashes_);
  curr_hashes_.clear();
  curr_hashes_.reserve(log.size());

  std::size_t rolling = 0;
  for (const auto& event : log) {
    rolling = hash_event(event, rolling);
    curr_hashes_.push_back(rolling);
  }
}

DivergenceReport DeterminismDetector::check_against_previous() const {
  DivergenceReport report;
  if (prev_hashes_.empty() || curr_hashes_.empty()) {
    return report;
  }

  const std::size_t shared = std::min(prev_hashes_.size(), curr_hashes_.size());
  for (std::size_t i = 0; i < shared; ++i) {
    if (prev_hashes_[i] != curr_hashes_[i]) {
      report.diverged = true;
      report.event_index = i;
      report.reason = "Nondeterministic divergence at event " + std::to_string(i);
      return report;
    }
  }

  if (prev_hashes_.size() != curr_hashes_.size()) {
    report.diverged = true;
    report.event_index = shared;
    report.reason = "Nondeterministic divergence in event count at event " + std::to_string(shared);
  }
  return report;
}

}  // namespace t81::axion

namespace t81::vm {
namespace {

class Interpreter : public IVirtualMachine {
public:
  explicit Interpreter(std::unique_ptr<t81::axion::Engine> engine)
      : axion_engine_(std::move(engine)) {
    if (!axion_engine_) {
      axion_engine_ = t81::axion::make_allow_all_engine();
    }
    // Only attach a CanonFS driver when T81_CANONFS_ROOT is explicitly set.
    // Without the env var the driver remains null and all canonfs_driver_ guards
    // correctly suppress audit events (AI-M4 contract).
    // Tests that need CanonFS should call set_canonfs_root() after construction.
    if (const char* raw = std::getenv("T81_CANONFS_ROOT"); raw != nullptr && raw[0] != '\0') {
      std::filesystem::path canon_root(raw);
      std::error_code ec;
      std::filesystem::create_directories(canon_root, ec);
      canonfs_driver_ = t81::canonfs::make_persistent_driver(canon_root);
    }
  }

  std::int64_t load_weights_tensor(std::string_view name) override {
    return intern_weights_tensor(name);
  }

  const t81::weights::NativeTensor* weights_tensor(std::int64_t handle) const override {
    if (handle <= 0) return nullptr;
    std::size_t idx = static_cast<std::size_t>(handle - 1);
    if (idx >= state_.weights_tensor_refs.size()) return nullptr;
    return state_.weights_tensor_refs[idx];
  }

  void load_program(const t81::tisc::Program& program) override {
    program_ = program;
    state_ = State{};
    state_.contexts.clear();
    state_.contexts.emplace_back();
    state_.current_context = 0;
    auto& ctx = state_.contexts.back();
    ctx.register_tags.fill(ValueTag::Int);

    auto& layout = state_.layout;
    layout.code.start = 0;
    layout.code.limit = program_.insns.size();
    layout.stack.start = layout.code.limit;
    layout.stack.limit = layout.stack.start + kDefaultStackSize;
    layout.heap.start = layout.stack.limit;
    layout.heap.limit = layout.heap.start + kDefaultHeapSize;
    layout.tensor.start = layout.heap.limit;
    layout.tensor.limit = layout.tensor.start + kDefaultTensorSpace;
    layout.meta.start = layout.tensor.limit;
    layout.meta.limit = layout.meta.start + kDefaultMetaSpace;
    state_.memory.resize(layout.total_size(), 0);
    state_.memory_tags.assign(state_.memory.size(), ValueTag::Int);

    ctx.stack_base = layout.stack.limit;
    ctx.stack_limit = layout.stack.start;
    ctx.sp = ctx.stack_base;

    state_.floats = program_.float_pool;
    state_.bigints = program_.bigint_pool;
    state_.fractions = program_.fraction_pool;
    state_.symbols = program_.symbol_pool;
    state_.string_vectors.clear();
    state_.tensors.clear();
    state_.tensors.reserve(program_.tensor_pool.size());
    for (const auto& t : program_.tensor_pool) {
      state_.tensors.push_back(t);
    }
    state_.free_tensor_indices.clear();
    state_.shapes = program_.shape_pool;
    state_.weights_model = program_.weights_model;
    state_.weights_tensor_refs.clear();
    state_.weights_tensor_handles.clear();
    tier_telemetry_.assign(state_.contexts.size(), TierTelemetry{});
    ctx.stack_frames.clear();
    ctx.call_depth = 0;
    state_.contradiction_events = 0;
    state_.heap_frames.clear();
    state_.heap_ptr = layout.heap.start;
    state_.meta_ptr = layout.meta.start;
    state_.options.clear();
    state_.results.clear();
    state_.enums.clear();
    state_.enum_metadata = program_.enum_metadata;
    state_.enum_metadata_index.clear();
    for (std::size_t i = 0; i < state_.enum_metadata.size(); ++i) {
      state_.enum_metadata_index[state_.enum_metadata[i].enum_id] = i;
    }
    t81::vm::internal::sync_system_registers(state_, program_, instruction_count_,
                                             state_.current_context);
    state_.policy.reset();
    state_.gc_cycles = 0;
    instructions_since_gc_ = 0;
    instruction_count_ = 0;
    for (const auto& t : state_.tensors) {
      if (!t.has_value()) continue;
      tier_telemetry_[0].max_shape_complexity = std::max(
          tier_telemetry_[0].max_shape_complexity, t81::vm::internal::tensor_shape_complexity(*t));
      tier_telemetry_[0].max_tensor_rank =
          std::max(tier_telemetry_[0].max_tensor_rank, static_cast<std::size_t>(t->rank()));
    }
    if (!program_.axion_policy_text.empty()) {
      auto policy = t81::axion::parse_policy(program_.axion_policy_text);
      if (policy.has_value()) {
        state_.policy = policy.value();
        axion_engine_ = t81::axion::make_policy_engine(state_.policy);
        for (const auto& loop : state_.policy->loops) {
          AxionEvent event;
          event.opcode = t81::tisc::Opcode::Nop;
          event.tag = loop.id;
          event.value = loop.depth;
          event.verdict.kind = t81::axion::VerdictKind::Allow;
          std::ostringstream reason;
          reason << "loop hint file=" << loop.file << " line=" << loop.line
                 << " column=" << loop.column << " bound=";
          if (loop.bound_infinite) {
            reason << "infinite";
          } else if (loop.bound_value) {
            reason << *loop.bound_value;
          } else {
            reason << "unknown";
          }
          event.verdict.reason = reason.str();
          state_.axion_log.push_back(event);
        }
      } else {
        const std::string parse_error = "Axion policy parse failed: " + policy.error();
        axion_engine_ = make_deny_with_reason_engine(parse_error);

        AxionEvent event;
        event.opcode = t81::tisc::Opcode::Nop;
        event.tag = 0;
        event.value = 0;
        event.verdict.kind = t81::axion::VerdictKind::Deny;
        event.verdict.reason = parse_error;
        state_.axion_log.push_back(event);
      }
    }
    if (!program_.match_metadata_text.empty()) {
      AxionEvent event;
      event.opcode = t81::tisc::Opcode::Nop;
      event.tag = 0;
      event.value = 0;
      event.verdict.kind = t81::axion::VerdictKind::Allow;
      event.verdict.reason = "match metadata: " + program_.match_metadata_text;
      state_.axion_log.push_back(event);
    }
  }

  std::expected<void, Trap> step() override {
    if (state_.halted) {
      return {};
    }
    if (state_.contexts.empty()) {
      state_.halted = true;
      return {};
    }

    // Round-Robin Scheduling: Find next active context
    size_t checked_count = 0;
    while (state_.contexts[state_.current_context].halted) {
      state_.current_context = (state_.current_context + 1) % state_.contexts.size();
      checked_count++;
      if (checked_count >= state_.contexts.size()) {
        state_.halted = true;
        return {};
      }
    }

    auto& ctx = state_.contexts[state_.current_context];
    if (tier_telemetry_.size() < state_.contexts.size()) {
      tier_telemetry_.resize(state_.contexts.size());
    }
    auto& telemetry = tier_telemetry_[state_.current_context];

    // Deterministic Fault Injection Check
    for (auto it = state_.pending_faults.begin(); it != state_.pending_faults.end();) {
      if (instruction_count_ == it->instruction_count) {
        Trap t = it->trap;
        // Log the fault injection as an Axion event
        t81::axion::Verdict verdict;
        verdict.kind = t81::axion::VerdictKind::Deny;
        std::ostringstream reason;
        reason << "FaultInjection instruction_count=" << instruction_count_
               << " trap=" << to_string(t);
        verdict.reason = reason.str();
        record_axion_event(t81::tisc::Opcode::Trap, 0, 0, verdict);

        state_.pending_faults.erase(it);
        return std::expected<void, Trap>(t81::unexpect, t);
      } else {
        ++it;
      }
    }

    // Check if we have a compiled trace for the current PC.
    auto trace_it = compiled_traces_.find(ctx.pc);
    if (trace_it != compiled_traces_.end()) {
      const std::size_t trace_pc = ctx.pc;
      const auto first_opcode = trace_pc < program_.insns.size() ? program_.insns[trace_pc].opcode
                                                                 : t81::tisc::Opcode::Halt;

      auto enter = eval_axion_call(t81::axion::reasons::kJitTraceEnter, trace_pc, first_opcode);
      if (enter.kind == t81::axion::VerdictKind::Deny) {
        return std::expected<void, Trap>(t81::unexpect, Trap::SecurityFault);
      }

      t81::axion::Verdict enter_event{t81::axion::VerdictKind::Allow, ""};
      {
        std::ostringstream reason;
        reason << t81::axion::reasons::kJitTraceEnter << " pc=" << trace_pc
               << " trace-len=" << trace_it->second->size();
        enter_event.reason = reason.str();
      }
      record_axion_event(t81::tisc::Opcode::Nop,
                         static_cast<std::int32_t>(trace_it->second->size()),
                         static_cast<std::int64_t>(trace_pc), enter_event);

      const auto base_instruction_count = instruction_count_;
      const auto exec_result = trace_it->second->execute(
          state_,
          [this, base_instruction_count](std::size_t pc, const t81::tisc::Insn& insn,
                                         std::size_t executed_so_far) -> bool {
            const auto verdict = eval_axion_call(t81::axion::reasons::kStep, pc, insn.opcode, {},
                                                 base_instruction_count + executed_so_far + 1);
            if (verdict.kind == t81::axion::VerdictKind::Warn) {
              record_axion_event(insn.opcode, 0, 0, verdict);
            }
            return verdict.kind != t81::axion::VerdictKind::Deny;
          });
      instruction_count_ += exec_result.instructions_executed;

      t81::axion::Verdict exit_event{t81::axion::VerdictKind::Allow, ""};
      {
        std::ostringstream reason;
        const bool deopt = exec_result.exit_kind == JitTrace::ExitKind::GuardDeopt;
        reason << (deopt ? t81::axion::reasons::kJitTraceDeopt : t81::axion::reasons::kJitTraceExit)
               << " pc=" << ctx.pc << " executed=" << exec_result.instructions_executed
               << " exit-kind=";
        switch (exec_result.exit_kind) {
          case JitTrace::ExitKind::Completed:
            reason << "completed";
            break;
          case JitTrace::ExitKind::Branch:
            reason << "branch";
            break;
          case JitTrace::ExitKind::GuardDeopt:
            reason << "guard-deopt";
            break;
          case JitTrace::ExitKind::PolicyDeny:
            reason << "policy-deny";
            break;
        }
        exit_event.reason = reason.str();
      }
      record_axion_event(t81::tisc::Opcode::Nop,
                         static_cast<std::int32_t>(exec_result.instructions_executed),
                         static_cast<std::int64_t>(ctx.pc), exit_event);

      const auto exit_reason = exec_result.exit_kind == JitTrace::ExitKind::GuardDeopt
                                   ? t81::axion::reasons::kJitTraceDeopt
                                   : t81::axion::reasons::kJitTraceExit;
      auto exit = eval_axion_call(exit_reason, ctx.pc, first_opcode);
      if (exit.kind == t81::axion::VerdictKind::Deny) {
        return std::expected<void, Trap>(t81::unexpect, Trap::SecurityFault);
      }

      if (exec_result.exit_kind == JitTrace::ExitKind::PolicyDeny) {
        return std::expected<void, Trap>(t81::unexpect, Trap::SecurityFault);
      }

      if (exec_result.exit_kind != JitTrace::ExitKind::GuardDeopt) {
        return {};
      }

      // Guard deopt: invalidate the trace at this entry and continue with
      // interpreter execution from the resumed PC in this same step.
      compiled_traces_.erase(trace_pc);
    }

    if (ctx.pc >= program_.insns.size()) {
      auto verdict = eval_axion_call(t81::axion::reasons::kStep, ctx.pc, t81::tisc::Opcode::Halt);
      if (verdict.kind == t81::axion::VerdictKind::Deny) {
        return std::expected<void, Trap>(t81::unexpect, Trap::SecurityFault);
      }
      ctx.halted = true;
      state_.halted = true;  // Single thread behavior
      return {};
    }

    const std::size_t current_pc = ctx.pc;
    const auto& insn = program_.insns[current_pc];
    ctx.pc += 1;
    instruction_count_++;

    // Hot-spot detection and JIT compilation.
    if (!jit_compiler_.is_tracing()) {
      hot_spots_[current_pc]++;
      if (hot_spots_[current_pc] >= kHotSpotThreshold) {
        jit_compiler_.start_tracing(current_pc);
      }
    }

    if (jit_compiler_.is_tracing()) {
      jit_compiler_.record_instruction(insn);
      if (!jit_compiler_.is_tracing()) {
        const auto trace_start_pc = jit_compiler_.trace_start_pc();
        auto trace = jit_compiler_.compile();
        if (trace) {
          compiled_traces_[trace_start_pc] = std::move(trace);
        }
      }
    }

    // Evaluate Axion policy before every instruction.
    auto verdict = eval_axion_call(t81::axion::reasons::kStep, current_pc, insn.opcode);
    if (verdict.kind == t81::axion::VerdictKind::Deny) {
      // Preserve deny reason visibility for pre-dispatch policy failures.
      record_axion_event(insn.opcode, 0, 0, verdict);
      return std::expected<void, Trap>(t81::unexpect, Trap::SecurityFault);
    }
    if (verdict.kind == t81::axion::VerdictKind::Warn) {
      // Log the warning event
      record_axion_event(insn.opcode, 0, 0, verdict);
    }

    auto reg_ok = [&ctx](int r) {
      return r >= 0 && static_cast<std::size_t>(r) < ctx.registers.size();
    };
    auto mem_ok = [this](int addr, bool code = false) {
      return t81::vm::internal::mem_ok(state_, addr, code);
    };
    auto check_mem = [this, &mem_ok](t81::tisc::Opcode opcode, int addr, std::string_view action,
                                     bool code = false) -> bool {
      if (mem_ok(addr, code)) return true;
      this->log_bounds_fault(opcode, addr, action);
      return false;
    };
    auto log_trace = [this, current_pc](t81::tisc::Opcode op, Trap trap = Trap::None) {
      TraceEntry t{current_pc, op, std::nullopt};
      if (trap != Trap::None) t.trap = trap;
      state_.trace.push_back(t);
    };
    auto literal_kind_to_tag = [](t81::tisc::LiteralKind kind) -> ValueTag {
      switch (kind) {
        case t81::tisc::LiteralKind::Bool:
          return ValueTag::Bool;
        case t81::tisc::LiteralKind::FloatHandle:
          return ValueTag::FloatHandle;
        case t81::tisc::LiteralKind::FractionHandle:
          return ValueTag::FractionHandle;
        case t81::tisc::LiteralKind::SymbolHandle:
          return ValueTag::SymbolHandle;
        case t81::tisc::LiteralKind::TensorHandle:
          return ValueTag::TensorHandle;
        case t81::tisc::LiteralKind::ShapeHandle:
          return ValueTag::ShapeHandle;
        case t81::tisc::LiteralKind::BigIntHandle:
          return ValueTag::BigIntHandle;
        case t81::tisc::LiteralKind::Int:
        default:
          return ValueTag::Int;
      }
    };
    auto set_reg = [&ctx](int reg, std::int64_t val_data, ValueTag tag) {
      if (reg == 0 || (reg >= 75 && reg <= 80)) return;
      ctx.registers[reg] = val_data;
      ctx.register_tags[reg] = tag;
    };
    auto copy_reg = [&ctx](int dst, int src) {
      if (dst == 0 || (dst >= 75 && dst <= 80)) return;
      ctx.registers[dst] = ctx.registers[src];
      ctx.register_tags[dst] = ctx.register_tags[src];
    };
    auto update_flags = [&ctx](std::int64_t v) {
      ctx.flags.zero = (v == 0);
      ctx.flags.negative = (v < 0);
      ctx.flags.positive = (v > 0);
    };
    auto push_stack = [&ctx, this](std::int64_t val_data,
                                   ValueTag tag) -> std::optional<std::size_t> {
      return t81::vm::internal::push_stack_word(state_, ctx, val_data, tag);
    };
    auto pop_stack = [&ctx, this](std::int64_t& value,
                                  ValueTag& tag) -> std::optional<std::size_t> {
      return t81::vm::internal::pop_stack_word(state_, ctx, value, tag);
    };
    auto tensor_ptr = [this](std::int64_t handle) -> t81::T729DynamicTensor* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.tensors.size()) return nullptr;
      if (!state_.tensors[idx].has_value()) return nullptr;
      return &state_.tensors[idx].value();
    };
    auto alloc_tensor = [this, current_pc, &telemetry](
                            t81::T729DynamicTensor tensor) -> std::expected<std::int64_t, Trap> {
      const std::size_t tensor_elements = tensor.size();
      const std::size_t tensor_shape_complexity =
          t81::vm::internal::tensor_shape_complexity(tensor);
      const std::size_t tensor_rank = static_cast<std::size_t>(tensor.rank());
      const auto tensor_policy =
          t81::vm::internal::evaluate_tensor_alloc_policy(state_, tensor_elements);
      if (tensor_policy == t81::vm::internal::TensorAllocPolicyResult::MaxTensorsExceeded) {
        record_axion_event(program_.insns[current_pc].opcode, 0, 0,
                           {t81::axion::VerdictKind::Deny, "Policy: max-tensors limit exceeded"});
        return t81::unexpected(Trap::SecurityFault);
      }
      if (tensor_policy == t81::vm::internal::TensorAllocPolicyResult::MaxTensorElementsExceeded) {
        record_axion_event(
            program_.insns[current_pc].opcode, 0, 0,
            {t81::axion::VerdictKind::Deny, "Policy: max-tensor-elements limit exceeded"});
        return t81::unexpected(Trap::SecurityFault);
      }

      t81::vm::internal::account_tensor_allocation(state_, tensor_elements);
      const std::size_t idx_handle =
          t81::vm::internal::store_tensor_slot(state_, std::move(tensor));
      telemetry.max_shape_complexity =
          std::max(telemetry.max_shape_complexity, tensor_shape_complexity);
      telemetry.max_tensor_rank = std::max(telemetry.max_tensor_rank, tensor_rank);

      log_memory_segment_access(program_.insns[current_pc].opcode, MemorySegmentKind::Tensor,
                                idx_handle, 1, t81::axion::reasons::kTensorAlloc);
      if (const auto& stored = state_.tensors[idx_handle - 1]; stored.has_value()) {
        t81::vm::internal::log_tensor_provenance(state_, state_.current_context,
                                                 program_.insns[current_pc].opcode, idx_handle,
                                                 stored.value(), "alloc");
      }
      return static_cast<std::int64_t>(idx_handle);
    };
    auto promote_to_tensor = [&](int reg) -> std::expected<void, Trap> {
      if (reg < 0 || static_cast<std::size_t>(reg) >= ctx.registers.size()) {
        return std::expected<void, Trap>(t81::unexpect, Trap::DecodeFault);
      }
      if (ctx.register_tags[reg] == ValueTag::WeightsTensorHandle) {
        auto handle = ctx.registers[reg];
        const auto* native = weights_tensor(handle);
        if (!native) return std::expected<void, Trap>(t81::unexpect, Trap::DecodeFault);
        auto promoted = t81::vm::internal::decode_native_tensor(
            *native, t81::vm::internal::TensorDecodeMode::StrictCanonical);
        if (!promoted.has_value()) {
          return std::expected<void, Trap>(t81::unexpect, Trap::DecodeFault);
        }
        auto result = alloc_tensor(std::move(*promoted));
        if (!result) {
          return std::expected<void, Trap>(t81::unexpect, result.error());
        }
        ctx.registers[reg] = *result;
        ctx.register_tags[reg] = ValueTag::TensorHandle;
      }
      return {};
    };
    auto float_ptr = [this](std::int64_t handle) -> double* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.floats.size()) return nullptr;
      return &state_.floats[idx];
    };
    auto alloc_float = [this, current_pc](double value) -> std::int64_t {
      state_.floats.push_back(value);
      auto idx = state_.floats.size();
      log_memory_segment_access(program_.insns[current_pc].opcode, MemorySegmentKind::Heap, idx, 1,
                                t81::axion::reasons::kHeapAlloc);
      return static_cast<std::int64_t>(idx);
    };
    auto fraction_ptr = [this](std::int64_t handle) -> t81::T81Fraction* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.fractions.size()) return nullptr;
      return &state_.fractions[idx];
    };
    auto bigint_ptr = [this](std::int64_t handle) -> t81::T81BigInt* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.bigints.size()) return nullptr;
      return &state_.bigints[idx];
    };
    auto symbol_ptr = [this](std::int64_t handle) -> const std::string* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.symbols.size()) return nullptr;
      return &state_.symbols[idx];
    };
    auto runtime_token_text = [](ValueTag tag) -> std::optional<std::string_view> {
      switch (tag) {
        case ValueTag::ProofHandle:
          return std::string_view{"std.sys.proof"};
        case ValueTag::IoStreamHandle:
          return std::string_view{"std.io.stream"};
        case ValueTag::IoNetHandle:
          return std::string_view{"std.io.net"};
        case ValueTag::AsyncThreadHandle:
          return std::string_view{"std.async.thread"};
        case ValueTag::AsyncPromiseHandle:
          return std::string_view{"std.async.promise"};
        default:
          break;
      }
      return std::nullopt;
    };
    auto symbol_like_text = [&](ValueTag tag,
                                std::int64_t value) -> std::optional<std::string_view> {
      if (tag == ValueTag::SymbolHandle) {
        auto* symbol = symbol_ptr(value);
        if (symbol == nullptr) return std::nullopt;
        return std::string_view{*symbol};
      }
      return runtime_token_text(tag);
    };
    auto runtime_token_tag_from_symbol_handle =
        [&](std::int64_t symbol_handle) -> std::optional<ValueTag> {
      auto symbol = symbol_ptr(symbol_handle);
      if (symbol == nullptr) return std::nullopt;
      if (*symbol == "std.sys.proof") return ValueTag::ProofHandle;
      if (*symbol == "std.io.stream") return ValueTag::IoStreamHandle;
      if (*symbol == "std.io.net") return ValueTag::IoNetHandle;
      if (*symbol == "std.async.thread") return ValueTag::AsyncThreadHandle;
      if (*symbol == "std.async.promise") return ValueTag::AsyncPromiseHandle;
      return std::nullopt;
    };
    auto string_vector_ptr = [this](std::int64_t handle) -> const std::vector<std::string>* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.string_vectors.size()) return nullptr;
      return &state_.string_vectors[idx];
    };
    auto intern_symbol = [this](std::string text) -> std::int64_t {
      for (std::size_t i = 0; i < state_.symbols.size(); ++i) {
        if (state_.symbols[i] == text) {
          return static_cast<std::int64_t>(i + 1);
        }
      }
      state_.symbols.push_back(std::move(text));
      return static_cast<std::int64_t>(state_.symbols.size());
    };
    auto string_vector_mut = [this](std::int64_t handle) -> std::vector<std::string>* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.string_vectors.size()) return nullptr;
      return &state_.string_vectors[idx];
    };
    auto alloc_string_vector = [this]() -> std::int64_t {
      state_.string_vectors.emplace_back();
      return static_cast<std::int64_t>(state_.string_vectors.size());
    };
    auto alloc_fraction = [this, current_pc](t81::T81Fraction frac) -> std::int64_t {
      state_.fractions.push_back(std::move(frac));
      auto idx = state_.fractions.size();
      log_memory_segment_access(program_.insns[current_pc].opcode, MemorySegmentKind::Heap, idx, 1,
                                t81::axion::reasons::kHeapAlloc);
      return static_cast<std::int64_t>(idx);
    };
    auto alloc_bigint = [this, current_pc](t81::T81BigInt value) -> std::int64_t {
      state_.bigints.push_back(std::move(value));
      auto idx = state_.bigints.size();
      log_memory_segment_access(program_.insns[current_pc].opcode, MemorySegmentKind::Heap, idx, 1,
                                t81::axion::reasons::kHeapAlloc);
      return static_cast<std::int64_t>(idx);
    };
    auto shape_ptr = [this](std::int64_t handle) -> const std::vector<int>* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.shapes.size()) return nullptr;
      return &state_.shapes[idx];
    };
    auto option_ptr = [this](std::int64_t handle) -> OptionValue* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.options.size()) return nullptr;
      return &state_.options[idx];
    };
    auto result_ptr = [this](std::int64_t handle) -> ResultValue* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.results.size()) return nullptr;
      return &state_.results[idx];
    };
    auto enum_ptr = [this](std::int64_t handle) -> EnumValue* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.enums.size()) return nullptr;
      return &state_.enums[idx];
    };
    auto complex_ptr = [this](std::int64_t handle) -> ComplexValue* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.complexes.size()) return nullptr;
      return &state_.complexes[idx];
    };
    auto symbolic_graph_ptr = [this](std::int64_t handle) -> t81::cog::v1::SymbolicGraph* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.symbolic_graphs.size()) return nullptr;
      return &state_.symbolic_graphs[idx];
    };
    auto tier2_frame_ptr = [&ctx](std::int64_t handle) -> t81::cog::v2::ReflectiveFrame* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= ctx.tier2_frames.size()) return nullptr;
      return &ctx.tier2_frames[idx];
    };
    auto infinite_form_ptr = [this](std::int64_t handle) -> t81::cog::v5::InfiniteCanonicalForm* {
      if (handle <= 0) return nullptr;
      std::size_t idx = static_cast<std::size_t>(handle - 1);
      if (idx >= state_.infinite_forms.size()) return nullptr;
      if (!state_.infinite_forms[idx].has_value()) return nullptr;
      return &state_.infinite_forms[idx].value();
    };
    auto alloc_symbolic_graph =
        [this, current_pc](t81::cog::v1::SymbolicGraph graph) -> std::expected<std::int64_t, Trap> {
      size_t nodes = graph.nodes.size();
      if (state_.policy) {
        if (state_.policy->max_symbolic_graphs &&
            state_.metrics.total_symbolic_graphs + 1 >
                static_cast<std::size_t>(*state_.policy->max_symbolic_graphs)) {
          record_axion_event(
              program_.insns[current_pc].opcode, 0, 0,
              {t81::axion::VerdictKind::Deny, "Policy: max-symbolic-graphs limit exceeded"});
          return t81::unexpected(Trap::SecurityFault);
        }
        if (state_.policy->max_symbolic_nodes) {
          if (state_.total_symbolic_nodes + nodes >
              static_cast<std::size_t>(*state_.policy->max_symbolic_nodes)) {
            record_axion_event(
                program_.insns[current_pc].opcode, 0, 0,
                {t81::axion::VerdictKind::Deny, "Policy: max-symbolic-nodes limit exceeded"});
            return t81::unexpected(Trap::SecurityFault);
          }
        }
      }
      state_.total_symbolic_nodes += nodes;

      state_.symbolic_graphs.push_back(std::move(graph));
      auto idx = state_.symbolic_graphs.size();

      state_.metrics.total_symbolic_graphs++;
      state_.metrics.total_symbolic_nodes += nodes;

      log_memory_segment_access(program_.insns[current_pc].opcode, MemorySegmentKind::Heap, idx, 1,
                                "graph alloc");
      return static_cast<std::int64_t>(idx);
    };
    auto alloc_infinite_form =
        [this, current_pc](
            t81::cog::v5::InfiniteCanonicalForm form) -> std::expected<std::int64_t, Trap> {
      if (state_.policy) {
        if (state_.policy->max_infinite_forms &&
            state_.metrics.total_infinite_forms + 1 >
                static_cast<std::size_t>(*state_.policy->max_infinite_forms)) {
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Deny,
                                      "Max infinite forms limit exceeded"};
          record_axion_event(program_.insns[current_pc].opcode, 0, 0, verdict);
          return t81::unexpected(Trap::SecurityFault);
        }
      }

      std::size_t idx_handle;
      if (!state_.free_infinite_indices.empty()) {
        auto raw_idx = state_.free_infinite_indices.back();
        state_.free_infinite_indices.pop_back();
        state_.infinite_forms[raw_idx] = std::move(form);
        idx_handle = raw_idx + 1;
      } else {
        state_.infinite_forms.push_back(std::move(form));
        idx_handle = state_.infinite_forms.size();
      }

      state_.metrics.total_infinite_forms++;

      log_memory_segment_access(program_.insns[current_pc].opcode, MemorySegmentKind::Heap,
                                idx_handle, 1, "InfAlloc");
      return static_cast<std::int64_t>(idx_handle);
    };
    auto intern_option = [this](bool has_value, ValueTag payload_tag,
                                std::int64_t payload) -> std::int64_t {
      for (std::size_t i = 0; i < state_.options.size(); ++i) {
        const auto& existing = state_.options[i];
        if (existing.has_value != has_value) continue;
        if (!has_value) return static_cast<std::int64_t>(i + 1);
        if (existing.payload_tag == payload_tag && existing.payload == payload) {
          return static_cast<std::int64_t>(i + 1);
        }
      }
      OptionValue val;
      val.has_value = has_value;
      val.payload_tag = payload_tag;
      val.payload = payload;
      state_.options.push_back(val);
      return static_cast<std::int64_t>(state_.options.size());
    };
    auto intern_result = [this](bool is_ok, ValueTag payload_tag,
                                std::int64_t payload) -> std::int64_t {
      for (std::size_t i = 0; i < state_.results.size(); ++i) {
        const auto& existing = state_.results[i];
        if (existing.is_ok != is_ok) continue;
        if (existing.payload_tag == payload_tag && existing.payload == payload) {
          return static_cast<std::int64_t>(i + 1);
        }
      }
      ResultValue val;
      val.is_ok = is_ok;
      val.payload_tag = payload_tag;
      val.payload = payload;
      state_.results.push_back(val);
      return static_cast<std::int64_t>(state_.results.size());
    };
    auto intern_enum = [this](int global_variant_id, bool has_payload, ValueTag payload_tag,
                              std::int64_t payload) -> std::int64_t {
      if (global_variant_id < 0) return 0;
      int enum_id = t81::enum_meta::decode_enum_id(global_variant_id);
      for (std::size_t i = 0; i < state_.enums.size(); ++i) {
        const auto& existing = state_.enums[i];
        if (existing.variant_id != global_variant_id) continue;
        if (existing.enum_id != enum_id) continue;
        if (existing.has_payload != has_payload) continue;
        if (!has_payload) {
          return static_cast<std::int64_t>(i + 1);
        }
        if (existing.payload_tag == payload_tag && existing.payload == payload) {
          return static_cast<std::int64_t>(i + 1);
        }
      }
      EnumValue val;
      val.variant_id = global_variant_id;
      val.enum_id = enum_id;
      val.has_payload = has_payload;
      val.payload_tag = payload_tag;
      val.payload = payload;
      state_.enums.push_back(val);
      return static_cast<std::int64_t>(state_.enums.size());
    };
    auto intern_complex = [this](std::int64_t real, std::int64_t imag) -> std::int64_t {
      for (std::size_t i = 0; i < state_.complexes.size(); ++i) {
        const auto& existing = state_.complexes[i];
        if (existing.real == real && existing.imag == imag) {
          return static_cast<std::int64_t>(i + 1);
        }
      }
      state_.complexes.push_back(ComplexValue{real, imag});
      return static_cast<std::int64_t>(state_.complexes.size());
    };
    auto clamp_trit = [](std::int64_t v) -> int {
      if (v > 0) return 1;
      if (v < 0) return -1;
      return 0;
    };
    auto ensure_min_tier = [&](t81::cog::TierId required_tier, std::string_view cause) -> bool {
      while (tier_rank(ctx.tier_status.current) < tier_rank(required_tier)) {
        auto res = try_promote_tier(ctx.tier_status, [&](const t81::axion::SyscallContext& pctx) {
          return axion_engine_->evaluate(pctx);
        });
        if (!res) {
          t81::axion::Verdict verdict;
          verdict.kind = t81::axion::VerdictKind::Deny;
          std::ostringstream reason;
          reason << "TierFault promotion denied cause=" << cause
                 << " current=" << static_cast<int>(ctx.tier_status.current)
                 << " required>=" << static_cast<int>(required_tier);
          verdict.reason = reason.str();
          record_axion_event(insn.opcode, static_cast<std::int32_t>(required_tier),
                             static_cast<std::int64_t>(ctx.tier_status.current), verdict);
          return false;
        }
        ctx.tier_status = *res;
        t81::axion::Verdict verdict;
        verdict.kind = t81::axion::VerdictKind::Allow;
        verdict.reason = "Cognitive Tier Promotion: " + ctx.tier_status.label;
        record_axion_event(insn.opcode, static_cast<std::int32_t>(required_tier),
                           static_cast<std::int64_t>(ctx.tier_status.current), verdict);
      }
      return true;
    };
    auto record_tier_fault = [&](std::string_view code, std::string_view detail,
                                 std::int64_t value = 0) {
      t81::axion::Verdict verdict;
      verdict.kind = t81::axion::VerdictKind::Deny;
      std::ostringstream reason;
      reason << "TierFault code=" << code << " tier=" << static_cast<int>(ctx.tier_status.current)
             << " call_depth=" << ctx.call_depth
             << " recurse_depth=" << ctx.tier3_recursor.current_depth << " pc=" << current_pc
             << " value=" << value;
      if (!state_.trace.empty()) {
        const auto& last = state_.trace.back();
        reason << " recent_last_pc=" << last.pc
               << " recent_last_op=" << t81::tisc::opcode_name(last.opcode);
      }
      if (!detail.empty()) {
        reason << " detail=" << detail;
      }
      verdict.reason = reason.str();
      record_axion_event(insn.opcode, static_cast<std::int32_t>(tier_rank(ctx.tier_status.current)),
                         value, verdict);
    };
    auto record_branch_decision = [&](bool taken) {
      telemetry.branch_events += 1;
      if (taken) {
        telemetry.branch_taken += 1;
      }
    };
    auto branch_entropy_bits = [&]() -> double {
      if (telemetry.branch_events == 0) {
        return 0.0;
      }
      const double p_taken = static_cast<double>(telemetry.branch_taken) /
                             static_cast<double>(telemetry.branch_events);
      const double p_not_taken = 1.0 - p_taken;
      double shannon = 0.0;
      if (p_taken > 0.0) {
        shannon -= p_taken * std::log2(p_taken);
      }
      if (p_not_taken > 0.0) {
        shannon -= p_not_taken * std::log2(p_not_taken);
      }
      return shannon * static_cast<double>(telemetry.branch_events);
    };
    auto update_flags_for_integer_value = [&](ValueTag tag, std::int64_t value) {
      if (tag == ValueTag::BigIntHandle) {
        auto* bigint = bigint_ptr(value);
        if (bigint == nullptr) {
          ctx.flags.zero = false;
          ctx.flags.negative = false;
          ctx.flags.positive = false;
          return;
        }
        ctx.flags.zero = bigint->is_zero();
        ctx.flags.negative = bigint->is_negative();
        ctx.flags.positive = !bigint->is_zero() && !bigint->is_negative();
        return;
      }
      update_flags(value);
    };
    auto is_integer_like_tag = [](ValueTag tag) {
      return tag == ValueTag::Int || tag == ValueTag::Bool || tag == ValueTag::BigIntHandle;
    };
    auto bigint_from_integer_like = [&](ValueTag tag, std::int64_t value) -> std::optional<t81::T81BigInt> {
      switch (tag) {
        case ValueTag::Int:
        case ValueTag::Bool:
          return t81::T81BigInt(value);
        case ValueTag::BigIntHandle: {
          auto* bigint = bigint_ptr(value);
          if (bigint == nullptr) return std::nullopt;
          return *bigint;
        }
        default:
          return std::nullopt;
      }
    };
    auto store_integer_result =
        [&](int reg, t81::T81BigInt value, bool force_bigint = false) -> std::optional<Trap> {
      if (!force_bigint) {
        if (auto narrowed = value.maybe_int64(); narrowed.has_value()) {
          const auto small = *narrowed;
          set_reg(reg, small, ValueTag::Int);
          update_flags_for_integer_value(ValueTag::Int, small);
          return std::nullopt;
        }
      }
      const auto handle = alloc_bigint(std::move(value));
      set_reg(reg, handle, ValueTag::BigIntHandle);
      update_flags_for_integer_value(ValueTag::BigIntHandle, handle);
      return std::nullopt;
    };
    auto store_bigint_materialized =
        [&](int reg, t81::T81BigInt value) -> std::optional<Trap> {
      const auto handle = alloc_bigint(std::move(value));
      set_reg(reg, handle, ValueTag::BigIntHandle);
      update_flags_for_integer_value(ValueTag::BigIntHandle, handle);
      return std::nullopt;
    };
    auto infer_required_tier_for_recursion = [&]() -> int {
      const std::size_t depth = std::max<std::size_t>(
          ctx.call_depth, static_cast<std::size_t>(ctx.tier3_recursor.current_depth));
      if (depth > recursion_limit_for_tier(t81::cog::TierId::Tier4)) return 5;
      if (depth > recursion_limit_for_tier(t81::cog::TierId::Tier3)) return 4;
      if (depth > recursion_limit_for_tier(t81::cog::TierId::Tier2)) return 3;
      if (depth > recursion_limit_for_tier(t81::cog::TierId::Tier1)) return 2;
      return 1;
    };

    std::function<std::optional<int>(ValueTag, std::int64_t, std::int64_t)> compare_value =
        [&](ValueTag tag, std::int64_t lhs_val, std::int64_t rhs_val) -> std::optional<int> {
      if (auto lhs = symbol_like_text(tag, lhs_val); lhs.has_value()) {
        auto rhs = symbol_like_text(tag, rhs_val);
        if (!rhs.has_value()) return std::nullopt;
        if (*lhs == *rhs) return 0;
        return (*lhs < *rhs) ? -1 : 1;
      }
      switch (tag) {
        case ValueTag::Int:
          if (lhs_val == rhs_val) return 0;
          return (lhs_val < rhs_val) ? -1 : 1;
        case ValueTag::Bool:
          if (lhs_val == rhs_val) return 0;
          return (lhs_val < rhs_val) ? -1 : 1;
        case ValueTag::BigIntHandle: {
          auto* lhs = bigint_ptr(lhs_val);
          auto* rhs = bigint_ptr(rhs_val);
          if (lhs == nullptr || rhs == nullptr) return std::nullopt;
          if (*lhs == *rhs) return 0;
          return (*lhs < *rhs) ? -1 : 1;
        }
        case ValueTag::FloatHandle: {
          auto* lhs = float_ptr(lhs_val);
          auto* rhs = float_ptr(rhs_val);
          if (lhs == nullptr || rhs == nullptr) return std::nullopt;
          if (*lhs == *rhs) return 0;
          return (*lhs < *rhs) ? -1 : 1;
        }
        case ValueTag::FractionHandle: {
          auto* lhs = fraction_ptr(lhs_val);
          auto* rhs = fraction_ptr(rhs_val);
          if (lhs == nullptr || rhs == nullptr) return std::nullopt;
          return t81::T81Fraction::cmp(*lhs, *rhs);
        }
        case ValueTag::SymbolHandle:
          return std::nullopt;
        case ValueTag::StringVectorHandle:
        case ValueTag::SymbolicGraphHandle:
        case ValueTag::Tier2FrameHandle:
        case ValueTag::InfiniteHandle:
          if (lhs_val == rhs_val) return 0;
          return (lhs_val < rhs_val) ? -1 : 1;
        case ValueTag::TensorHandle: {
          if (lhs_val == rhs_val) return 0;
          auto* lhs_t = tensor_ptr(lhs_val);
          auto* rhs_t = tensor_ptr(rhs_val);
          if (!lhs_t || !rhs_t) return std::nullopt;
          if (lhs_t->shape() != rhs_t->shape()) {
            return (lhs_t->shape() < rhs_t->shape()) ? -1 : 1;
          }
          const auto& ld = lhs_t->data();
          const auto& rd = rhs_t->data();
          for (std::size_t i = 0; i < ld.size(); ++i) {
            if (ld[i] < rd[i]) return -1;
            if (ld[i] > rd[i]) return 1;
          }
          return 0;
        }
        case ValueTag::ShapeHandle:
        case ValueTag::WeightsTensorHandle:
        case ValueTag::ReflectionHandle:
        case ValueTag::ProofHandle:
        case ValueTag::IoStreamHandle:
        case ValueTag::IoNetHandle:
        case ValueTag::AsyncThreadHandle:
        case ValueTag::AsyncPromiseHandle:
          if (lhs_val == rhs_val) return 0;
          return (lhs_val < rhs_val) ? -1 : 1;
        case ValueTag::ComplexHandle: {
          auto lhs = complex_ptr(lhs_val);
          auto rhs = complex_ptr(rhs_val);
          if (lhs == nullptr || rhs == nullptr) return std::nullopt;
          if (lhs->real == rhs->real && lhs->imag == rhs->imag) return 0;
          if (lhs->real == rhs->real) {
            return (lhs->imag < rhs->imag) ? -1 : 1;
          }
          return (lhs->real < rhs->real) ? -1 : 1;
        }
        case ValueTag::OptionHandle: {
          auto lhs = option_ptr(lhs_val);
          auto rhs = option_ptr(rhs_val);
          if (lhs == nullptr || rhs == nullptr) return std::nullopt;
          if (lhs->has_value != rhs->has_value) {
            return lhs->has_value ? 1 : -1;
          }
          if (!lhs->has_value) return 0;
          if (lhs->payload_tag != rhs->payload_tag) return std::nullopt;
          return compare_value(lhs->payload_tag, lhs->payload, rhs->payload);
        }
        case ValueTag::ResultHandle: {
          auto lhs = result_ptr(lhs_val);
          auto rhs = result_ptr(rhs_val);
          if (lhs == nullptr || rhs == nullptr) return std::nullopt;
          if (lhs->is_ok != rhs->is_ok) {
            return lhs->is_ok ? 1 : -1;
          }
          if (lhs->payload_tag != rhs->payload_tag) return std::nullopt;
          return compare_value(lhs->payload_tag, lhs->payload, rhs->payload);
        }
        case ValueTag::EnumHandle:
          return std::nullopt;
      }
      return std::nullopt;
    };

    std::function<std::optional<std::string>(ValueTag, std::int64_t, int)> format_value =
        [&](ValueTag tag, std::int64_t val_data, int depth) -> std::optional<std::string> {
      if (depth > 8) return std::nullopt;
      if (auto symbol = symbol_like_text(tag, val_data); symbol.has_value()) {
        return std::string(*symbol);
      }
      switch (tag) {
        case ValueTag::Int:
          return std::to_string(val_data);
        case ValueTag::Bool:
          return val_data != 0 ? "true" : "false";
        case ValueTag::BigIntHandle: {
          auto* bigint = bigint_ptr(val_data);
          if (!bigint) return std::nullopt;
          return bigint->to_string();
        }
        case ValueTag::FloatHandle: {
          auto* ptr_val = float_ptr(val_data);
          if (!ptr_val) return std::nullopt;
          double canonical = (*ptr_val == 0.0) ? 0.0 : *ptr_val;
          std::ostringstream out;
          out.imbue(std::locale::classic());
          out.precision(std::numeric_limits<double>::max_digits10);
          out << canonical << "t81";
          return out.str();
        }
        case ValueTag::FractionHandle: {
          auto* frac = fraction_ptr(val_data);
          if (!frac) return std::nullopt;
          return frac->num.to_string() + "/" + frac->den.to_string() + "t81";
        }
        case ValueTag::SymbolHandle:
          return std::nullopt;
        case ValueTag::StringVectorHandle: {
          auto* ptr_val = string_vector_ptr(val_data);
          if (!ptr_val) return std::nullopt;
          return "<strvec#" + std::to_string(val_data) + ">";
        }
        case ValueTag::SymbolicGraphHandle: {
          auto* graph = symbolic_graph_ptr(val_data);
          if (!graph) return std::nullopt;
          return graph->serialize_canonical();
        }
        case ValueTag::Tier2FrameHandle:
          return "<tier2_frame#" + std::to_string(val_data) + ">";
        case ValueTag::InfiniteHandle:
          return "<infinite#" + std::to_string(val_data) + ">";
        case ValueTag::TensorHandle:
          return "<tensor#" + std::to_string(val_data) + ">";
        case ValueTag::ShapeHandle:
          return "<shape#" + std::to_string(val_data) + ">";
        case ValueTag::WeightsTensorHandle:
          return "<weights#" + std::to_string(val_data) + ">";
        case ValueTag::ReflectionHandle:
          return "<reflection#" + std::to_string(val_data) + ">";
        case ValueTag::ProofHandle:
        case ValueTag::IoStreamHandle:
        case ValueTag::IoNetHandle:
        case ValueTag::AsyncThreadHandle:
        case ValueTag::AsyncPromiseHandle:
          return std::nullopt;
        case ValueTag::ComplexHandle: {
          auto* complex = complex_ptr(val_data);
          if (!complex) return std::nullopt;
          return "<complex(" + std::to_string(complex->real) + "," + std::to_string(complex->imag) +
                 ")>";
        }
        case ValueTag::OptionHandle: {
          auto* opt = option_ptr(val_data);
          if (!opt) return std::nullopt;
          if (!opt->has_value) return std::string{"None"};
          auto payload = format_value(opt->payload_tag, opt->payload, depth + 1);
          if (!payload) return std::nullopt;
          return "Some(" + *payload + ")";
        }
        case ValueTag::ResultHandle: {
          auto* result = result_ptr(val_data);
          if (!result) return std::nullopt;
          auto payload = format_value(result->payload_tag, result->payload, depth + 1);
          if (!payload) return std::nullopt;
          return result->is_ok ? "Ok(" + *payload + ")" : "Err(" + *payload + ")";
        }
        case ValueTag::EnumHandle: {
          auto* enum_value = enum_ptr(val_data);
          if (!enum_value) return std::nullopt;
          if (!enum_value->has_payload) {
            return "<enum#" + std::to_string(enum_value->variant_id) + ">";
          }
          auto payload = format_value(enum_value->payload_tag, enum_value->payload, depth + 1);
          if (!payload) return std::nullopt;
          return "<enum#" + std::to_string(enum_value->variant_id) + "(" + *payload + ")>";
        }
      }
      return std::nullopt;
    };

    auto handle_blocked_neural_opcode = [&](bool require_b_operand) -> std::optional<Trap> {
      if (!reg_ok(insn.a) || (require_b_operand && !reg_ok(insn.b))) {
        return Trap::DecodeFault;
      }
      t81::axion::Verdict verdict{t81::axion::VerdictKind::Deny,
                                  "Blocked: unimplemented neural opcode"};
      record_axion_event(insn.opcode, 0, 0, verdict);
      return Trap::SecurityFault;
    };

    auto handle_bitwise_binary = [&]() -> std::optional<Trap> {
      if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
        return Trap::DecodeFault;
      }
      
      // Check if either operand is BigInt - if so, use BigInt arithmetic
      if (auto lhs = bigint_from_integer_like(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          lhs.has_value()) {
        if (auto rhs = bigint_from_integer_like(ctx.register_tags[insn.c], ctx.registers[insn.c]);
            rhs.has_value()) {
          const bool preserve_bigint = ctx.register_tags[insn.b] == ValueTag::BigIntHandle ||
                                       ctx.register_tags[insn.c] == ValueTag::BigIntHandle;
          
          // For BigInt bitwise operations, we need to implement two's complement semantics
          // Since T81BigInt doesn't have native bitwise ops, we convert through int64_t
          // but we need to be careful about the range and canonical form
          
          // Convert BigInt to int64_t for the operation
          std::int64_t lhs_int = lhs->to_int64();
          std::int64_t rhs_int = rhs->to_int64();
          
          std::int64_t result_int = 0;
          if (insn.opcode == t81::tisc::Opcode::BitAnd) {
            result_int = lhs_int & rhs_int;
          } else if (insn.opcode == t81::tisc::Opcode::BitOr) {
            result_int = lhs_int | rhs_int;
          } else {
            result_int = lhs_int ^ rhs_int;
          }
          
          // Convert result back to BigInt to preserve type consistency
          t81::T81BigInt result(result_int);
          if (auto op_trap = store_integer_result(insn.a, std::move(result), preserve_bigint);
              op_trap.has_value()) {
            return *op_trap;
          }
          return std::nullopt;
        }
      }
      
      // Fallback to int64_t bitwise operations for pure Int operands
      std::int64_t v = 0;
      if (insn.opcode == t81::tisc::Opcode::BitAnd) {
        v = ctx.registers[insn.b] & ctx.registers[insn.c];
      } else if (insn.opcode == t81::tisc::Opcode::BitOr) {
        v = ctx.registers[insn.b] | ctx.registers[insn.c];
      } else {
        v = ctx.registers[insn.b] ^ ctx.registers[insn.c];
      }
      
      ctx.registers[insn.a] = v;
      ctx.register_tags[insn.a] = ValueTag::Int;
      update_flags(v);
      return std::nullopt;
    };

    auto handle_bitwise_not = [&]() -> std::optional<Trap> {
      if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
        return Trap::DecodeFault;
      }
      
      // Check if operand is BigInt - if so, use BigInt arithmetic
      if (auto operand = bigint_from_integer_like(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          operand.has_value()) {
        const bool preserve_bigint = ctx.register_tags[insn.b] == ValueTag::BigIntHandle;
        
        // Convert BigInt to int64_t for the operation
        std::int64_t operand_int = operand->to_int64();
        std::int64_t result_int = ~operand_int;
        
        // Convert result back to BigInt to preserve type consistency
        t81::T81BigInt result(result_int);
        if (auto op_trap = store_integer_result(insn.a, std::move(result), preserve_bigint);
            op_trap.has_value()) {
          return *op_trap;
        }
        return std::nullopt;
      }
      
      // Fallback to int64_t bitwise NOT for pure Int operands
      std::int64_t v = ~ctx.registers[insn.b];
      ctx.registers[insn.a] = v;
      ctx.register_tags[insn.a] = ValueTag::Int;
      update_flags(v);
      return std::nullopt;
    };

    auto handle_bitwise_shift = [&]() -> std::optional<Trap> {
      if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
        return Trap::DecodeFault;
      }
      
      // Check if value operand is BigInt - if so, use BigInt arithmetic
      if (auto val = bigint_from_integer_like(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          val.has_value()) {
        if (auto amt = bigint_from_integer_like(ctx.register_tags[insn.c], ctx.registers[insn.c]);
            amt.has_value()) {
          const bool preserve_bigint = ctx.register_tags[insn.b] == ValueTag::BigIntHandle ||
                                       ctx.register_tags[insn.c] == ValueTag::BigIntHandle;
          
          // Convert BigInt to int64_t for the operation
          std::int64_t val_int = val->to_int64();
          std::int64_t amt_int = amt->to_int64();
          
          // Apply shift amount masking (0x3F = 63)
          std::int64_t masked_amt = amt_int & 0x3F;
          
          std::int64_t result_int = 0;
          if (insn.opcode == t81::tisc::Opcode::BitShl) {
            result_int = val_int << masked_amt;
          } else if (insn.opcode == t81::tisc::Opcode::BitShr) {
            result_int = val_int >> masked_amt;
          } else {
            // BitUShr - logical right shift (zero-fill)
            result_int = static_cast<std::int64_t>(static_cast<uint64_t>(val_int) >> masked_amt);
          }
          
          // Convert result back to BigInt to preserve type consistency
          t81::T81BigInt result(result_int);
          if (auto op_trap = store_integer_result(insn.a, std::move(result), preserve_bigint);
              op_trap.has_value()) {
            return *op_trap;
          }
          return std::nullopt;
        }
      }
      
      // Fallback to int64_t bitwise shift for pure Int operands
      std::int64_t val = ctx.registers[insn.b];
      std::int64_t amt = ctx.registers[insn.c] & 0x3F;
      std::int64_t res = 0;
      if (insn.opcode == t81::tisc::Opcode::BitShl) {
        res = val << amt;
      } else if (insn.opcode == t81::tisc::Opcode::BitShr) {
        res = val >> amt;
      } else {
        res = static_cast<std::int64_t>(static_cast<uint64_t>(val) >> amt);
      }
      
      ctx.registers[insn.a] = res;
      ctx.register_tags[insn.a] = ValueTag::Int;
      update_flags(res);
      return std::nullopt;
    };

    auto decode_ai_packed_reg_pair = [&](std::int32_t packed) -> std::optional<std::pair<int, int>> {
      const auto raw = static_cast<std::uint32_t>(packed);
      if ((raw & 0xFFFF0000U) != 0U) {
        return std::nullopt;
      }
      const int first = static_cast<int>(raw & 0xFFU);
      const int second = static_cast<int>((raw >> 8) & 0xFFU);
      if (!reg_ok(first) || !reg_ok(second)) {
        return std::nullopt;
      }
      return std::pair<int, int>{first, second};
    };

    Trap trap = Trap::None;
    if (auto dispatched_trap =
            dispatch_axion_opcode_from_step(insn, ctx, current_pc, symbol_like_text);
        dispatched_trap.has_value()) {
      trap = *dispatched_trap;
    } else {
      switch (insn.opcode) {
      case t81::tisc::Opcode::Nop: {
        if (insn.literal_kind == t81::tisc::LiteralKind::SymbolHandle && insn.b > 0) {
          auto idx = static_cast<std::size_t>(insn.b);
          if (idx <= state_.symbols.size()) {
            AxionEvent event;
            event.opcode = insn.opcode;
            event.tag = static_cast<std::int32_t>(insn.b);
            event.value = 0;
            event.verdict.kind = t81::axion::VerdictKind::Allow;
            event.verdict.reason = state_.symbols[idx - 1];
            state_.axion_log.push_back(event);
          }
        }
        break;
      }
      case t81::tisc::Opcode::Halt:
        state_.halted = true;
        break;
      case t81::tisc::Opcode::LoadImm: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (insn.literal_kind == t81::tisc::LiteralKind::BigIntHandle) {
          if (insn.b <= 0 ||
              static_cast<std::size_t>(insn.b) > program_.bigint_pool.size()) {
            trap = Trap::DecodeFault;
            break;
          }
          set_reg(insn.a, insn.b, ValueTag::BigIntHandle);
          update_flags_for_integer_value(ValueTag::BigIntHandle, ctx.registers[insn.a]);
          break;
        }
        auto tag = literal_kind_to_tag(insn.literal_kind);
        std::int64_t value = insn.b;
        if (tag == ValueTag::SymbolHandle) {
          if (auto runtime_tag = runtime_token_tag_from_symbol_handle(insn.b);
              runtime_tag.has_value()) {
            tag = *runtime_tag;
            value = 1;
          }
        }
        set_reg(insn.a, value, tag);
        update_flags_for_integer_value(tag, ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::F2Frac: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::FloatHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* f = float_ptr(ctx.registers[insn.b]);
        if (!f) {
          trap = Trap::DecodeFault;
          break;
        }
        t81::T81Fraction result = fraction_from_double(*f);
        ctx.registers[insn.a] = alloc_fraction(std::move(result));
        ctx.register_tags[insn.a] = ValueTag::FractionHandle;
        break;
      }
      case t81::tisc::Opcode::Frac2F: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::FractionHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* frac = fraction_ptr(ctx.registers[insn.b]);
        if (!frac) {
          trap = Trap::DecodeFault;
          break;
        }
        double result;
        try {
          // Use exact integer arithmetic for values that fit in int64_t.
          const std::int64_t n_i = frac->num.to_int64();
          const std::int64_t d_i = frac->den.to_int64();
          result = static_cast<double>(n_i) / static_cast<double>(d_i);
        } catch (...) {
          // Fallback for BigInt values that exceed int64 range.
          const double n = frac->num.to_float<72, 9>().to_double();
          const double d = frac->den.to_float<72, 9>().to_double();
          result = n / d;
        }
        ctx.registers[insn.a] = alloc_float(result);
        ctx.register_tags[insn.a] = ValueTag::FloatHandle;
        break;
      }
      case t81::tisc::Opcode::Mov:
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        copy_reg(insn.a, insn.b);
        update_flags(ctx.registers[insn.a]);
        break;
      case t81::tisc::Opcode::Inc:
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!is_integer_like_tag(ctx.register_tags[insn.a])) {
          trap = Trap::TypeFault;
          break;
        }
        if (auto lhs = bigint_from_integer_like(ctx.register_tags[insn.a], ctx.registers[insn.a]);
            lhs.has_value()) {
          const bool preserve_bigint = ctx.register_tags[insn.a] == ValueTag::BigIntHandle;
          if (auto op_trap =
                  store_integer_result(insn.a, *lhs + t81::T81BigInt(1), preserve_bigint);
              op_trap.has_value()) {
            trap = *op_trap;
          }
        } else {
          trap = Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Dec:
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!is_integer_like_tag(ctx.register_tags[insn.a])) {
          trap = Trap::TypeFault;
          break;
        }
        if (auto lhs = bigint_from_integer_like(ctx.register_tags[insn.a], ctx.registers[insn.a]);
            lhs.has_value()) {
          const bool preserve_bigint = ctx.register_tags[insn.a] == ValueTag::BigIntHandle;
          if (auto op_trap =
                  store_integer_result(insn.a, *lhs - t81::T81BigInt(1), preserve_bigint);
              op_trap.has_value()) {
            trap = *op_trap;
          }
        } else {
          trap = Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Add:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!is_integer_like_tag(ctx.register_tags[insn.b]) ||
            !is_integer_like_tag(ctx.register_tags[insn.c])) {
          trap = Trap::TypeFault;
          break;
        }
        if (auto lhs = bigint_from_integer_like(ctx.register_tags[insn.b], ctx.registers[insn.b]);
            lhs.has_value()) {
          auto rhs = bigint_from_integer_like(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!rhs.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          const bool preserve_bigint = ctx.register_tags[insn.b] == ValueTag::BigIntHandle ||
                                       ctx.register_tags[insn.c] == ValueTag::BigIntHandle;
          if (auto op_trap = store_integer_result(insn.a, *lhs + *rhs, preserve_bigint);
              op_trap.has_value()) {
            trap = *op_trap;
          }
        } else {
          trap = Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Sub:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!is_integer_like_tag(ctx.register_tags[insn.b]) ||
            !is_integer_like_tag(ctx.register_tags[insn.c])) {
          trap = Trap::TypeFault;
          break;
        }
        if (auto lhs = bigint_from_integer_like(ctx.register_tags[insn.b], ctx.registers[insn.b]);
            lhs.has_value()) {
          auto rhs = bigint_from_integer_like(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!rhs.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          const bool preserve_bigint = ctx.register_tags[insn.b] == ValueTag::BigIntHandle ||
                                       ctx.register_tags[insn.c] == ValueTag::BigIntHandle;
          if (auto op_trap = store_integer_result(insn.a, *lhs - *rhs, preserve_bigint);
              op_trap.has_value()) {
            trap = *op_trap;
          }
        } else {
          trap = Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Load: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!check_mem(insn.opcode, insn.b, "memory load")) {
          trap = Trap::BoundsFault;
          break;
        }
        std::size_t addr = static_cast<std::size_t>(insn.b);
        ctx.registers[insn.a] = state_.memory[addr];
        ctx.register_tags[insn.a] = state_.memory_tags[addr];
        log_memory_segment_access(insn.opcode, t81::vm::internal::segment_for_address(state_, addr),
                                  addr, 1, t81::axion::reasons::kMemLoad);
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::WeightsLoad: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (insn.b <= 0 || static_cast<std::size_t>(insn.b) > state_.symbols.size()) {
          trap = Trap::DecodeFault;
          break;
        }
        const std::string& name = state_.symbols[static_cast<std::size_t>(insn.b - 1)];
        auto handle = intern_weights_tensor(name);
        ctx.registers[insn.a] = handle;
        ctx.register_tags[insn.a] = ValueTag::WeightsTensorHandle;
        {
          t81::axion::Verdict verdict;
          verdict.kind = t81::axion::VerdictKind::Allow;
          verdict.reason = "weights.load \"" + name + "\"";
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), handle, verdict);
        }
        break;
      }
      case t81::tisc::Opcode::TLoadHash: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto symbol = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        if (!symbol.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        std::string hash_str = std::string(*symbol);

        auto verdict =
            eval_axion_call(t81::axion::reasons::kStep, current_pc, insn.opcode, hash_str);
        if (verdict.kind == t81::axion::VerdictKind::Deny) {
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), 0, verdict);
          trap = Trap::SecurityFault;
          break;
        }

        if (!canonfs_driver_) {
          // No CanonFS driver: validate hash format first, then treat as a miss.
          if (!t81::vm::internal::parse_canon_tensor_ref(hash_str).has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict miss_verdict;
          miss_verdict.kind = t81::axion::VerdictKind::Allow;
          miss_verdict.reason = "TLOADHASH canonfs_miss hash=" + hash_str;
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), 0, miss_verdict);
          trap = Trap::BoundsFault;
          break;
        }
        auto load_res = t81::vm::internal::load_canon_tensor_by_hash(*canonfs_driver_, hash_str);
        if (load_res.status == t81::vm::internal::TensorLoadHashStatus::InvalidHash) {
          trap = Trap::DecodeFault;
          break;
        }
        if (load_res.status == t81::vm::internal::TensorLoadHashStatus::CanonFsMiss) {
          t81::axion::Verdict miss_verdict;
          miss_verdict.kind = t81::axion::VerdictKind::Allow;
          miss_verdict.reason = "TLOADHASH canonfs_miss hash=" + hash_str;
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), 0, miss_verdict);
          trap = Trap::BoundsFault;
          break;
        }
        if (load_res.status != t81::vm::internal::TensorLoadHashStatus::Ok ||
            !load_res.tensor.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        auto res = alloc_tensor(std::move(*load_res.tensor));
        if (!res) {
          trap = res.error();
          break;
        }
        ctx.registers[insn.a] = *res;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;

        t81::axion::Verdict success_verdict;
        success_verdict.kind = t81::axion::VerdictKind::Allow;
        success_verdict.reason = "TLOADHASH success hash=" + hash_str +
                                 " handle=" + std::to_string(ctx.registers[insn.a]);
        record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.a],
                           success_verdict);
        break;
      }
      case t81::tisc::Opcode::TExp: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(ctx.registers[insn.b]);
        if (tensor == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        auto res_handle = alloc_tensor(t81::vm::internal::tensor_unary_exp(*tensor));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::MetaRead: {
        if (!reg_ok(insn.a) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        MemorySegmentKind segment = static_cast<MemorySegmentKind>(insn.b);
        std::int64_t addr = ctx.registers[insn.c];
        auto verdict = eval_axion_call(t81::axion::reasons::kMetaRead, current_pc, insn.opcode);
        if (verdict.kind == t81::axion::VerdictKind::Deny) {
          trap = Trap::SecurityFault;
          break;
        }
        if (segment == MemorySegmentKind::Registers) {
          if (!reg_ok(static_cast<int>(addr))) {
            trap = Trap::BoundsFault;
            break;
          }
          ctx.registers[insn.a] = ctx.registers[addr];
          ctx.register_tags[insn.a] = ctx.register_tags[addr];
        } else if (segment == MemorySegmentKind::Code) {
          if (addr < 0 || static_cast<size_t>(addr) >= program_.insns.size()) {
            trap = Trap::BoundsFault;
            break;
          }
          ctx.registers[insn.a] = static_cast<std::int64_t>(program_.insns[addr].opcode);
          ctx.register_tags[insn.a] = ValueTag::Int;
        } else {
          std::size_t physical_addr = 0;
          bool ok = false;
          const auto& layout = state_.layout;
          switch (segment) {
            case MemorySegmentKind::Stack:
              if (layout.stack.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            case MemorySegmentKind::Heap:
              if (layout.heap.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            case MemorySegmentKind::Tensor:
              if (layout.tensor.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            case MemorySegmentKind::Meta:
              if (layout.meta.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            default:
              break;
          }
          if (!ok) {
            trap = Trap::BoundsFault;
            break;
          }
          ctx.registers[insn.a] = state_.memory[physical_addr];
          ctx.register_tags[insn.a] = state_.memory_tags[physical_addr];
        }
        update_flags(ctx.registers[insn.a]);
        t81::vm::internal::apply_segment_reason(verdict, "MetaRead reflection", segment,
                                                static_cast<size_t>(addr));
        record_axion_event(insn.opcode, static_cast<int32_t>(segment), addr, verdict);
        break;
      }
      case t81::tisc::Opcode::MetaWrite: {
        if (!reg_ok(insn.a) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        MemorySegmentKind segment = static_cast<MemorySegmentKind>(insn.b);
        std::int64_t addr = ctx.registers[insn.c];
        std::int64_t val = ctx.registers[insn.a];
        ValueTag tag = ctx.register_tags[insn.a];
        auto verdict = eval_axion_call(t81::axion::reasons::kMetaWrite, current_pc, insn.opcode);
        if (verdict.kind == t81::axion::VerdictKind::Deny) {
          trap = Trap::SecurityFault;
          break;
        }
        if (segment == MemorySegmentKind::Registers) {
          if (!reg_ok(static_cast<int>(addr))) {
            trap = Trap::BoundsFault;
            break;
          }
          ctx.registers[addr] = val;
          ctx.register_tags[addr] = tag;
        } else if (segment == MemorySegmentKind::Code) {
          if (addr < 0 || static_cast<size_t>(addr) >= program_.insns.size()) {
            trap = Trap::BoundsFault;
            break;
          }
          // Code segment is protected: Writable only via privileged loader
          t81::axion::Verdict code_verdict;
          code_verdict.kind = t81::axion::VerdictKind::Deny;
          code_verdict.reason = "MetaWrite to Code segment denied (protected)";
          record_axion_event(insn.opcode, static_cast<int32_t>(segment), addr, code_verdict);
          trap = Trap::SecurityFault;
          break;
        } else {
          std::size_t physical_addr = 0;
          bool ok = false;
          const auto& layout = state_.layout;
          switch (segment) {
            case MemorySegmentKind::Stack:
              if (layout.stack.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            case MemorySegmentKind::Heap:
              if (layout.heap.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            case MemorySegmentKind::Tensor:
              if (layout.tensor.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            case MemorySegmentKind::Meta:
              if (layout.meta.contains(addr)) {
                physical_addr = addr;
                ok = true;
              }
              break;
            default:
              break;
          }
          if (!ok) {
            trap = Trap::BoundsFault;
            break;
          }
          state_.memory[physical_addr] = val;
          state_.memory_tags[physical_addr] = tag;
        }
        t81::vm::internal::apply_segment_reason(verdict, "MetaWrite reflection", segment,
                                                static_cast<size_t>(addr));
        record_axion_event(insn.opcode, static_cast<int32_t>(segment), addr, verdict);
        break;
      }
      case t81::tisc::Opcode::MetaReflect: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.reflection_count >= kMaxReflectionsPerEpoch) {
          trap = Trap::SecurityFault;
          break;
        }

        auto verdict = eval_axion_call(t81::axion::reasons::kMetaReflect, current_pc, insn.opcode);
        if (verdict.kind == t81::axion::VerdictKind::Deny) {
          trap = Trap::SecurityFault;
          break;
        }

        ReflectionSnapshot snapshot;
        snapshot.pc = current_pc;
        snapshot.registers = ctx.registers;
        snapshot.register_tags = ctx.register_tags;
        snapshot.flags = ctx.flags;

        // Capture recent trace (up to 81 entries)
        std::size_t trace_start = (state_.trace.size() > 81) ? (state_.trace.size() - 81) : 0;
        for (std::size_t i = trace_start; i < state_.trace.size(); ++i) {
          snapshot.recent_trace.push_back(state_.trace[i]);
        }

        // Improved hash of code segment including operands
        uint64_t h = 0;
        for (const auto& pi : program_.insns) {
          auto combine = [&](uint64_t v) { h ^= v + 0x9e3779b9 + (h << 6) + (h >> 2); };
          combine(static_cast<uint64_t>(pi.opcode));
          combine(static_cast<uint64_t>(pi.a));
          combine(static_cast<uint64_t>(pi.b));
          combine(static_cast<uint64_t>(pi.c));
        }
        snapshot.code_hash = h;

        state_.reflection_snapshots.push_back(std::move(snapshot));
        state_.reflection_count++;

        std::int64_t handle = static_cast<std::int64_t>(state_.reflection_snapshots.size());
        set_reg(insn.a, handle, ValueTag::ReflectionHandle);
        update_flags(handle);
        record_axion_event(insn.opcode, insn.b, handle, verdict);
        break;
      }
      case t81::tisc::Opcode::MetaRefine: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        // RS1 (insn.b) = memory address of commands
        // RS2 (insn.c) = number of commands

        auto verdict = eval_axion_call(t81::axion::reasons::kMetaRefine, current_pc, insn.opcode);
        if (verdict.kind == t81::axion::VerdictKind::Deny) {
          trap = Trap::SecurityFault;
          break;
        }

        std::int64_t cmd_addr = ctx.registers[insn.b];
        std::int64_t cmd_count = ctx.registers[insn.c];

        if (cmd_count < 0 || cmd_count > static_cast<int64_t>(kMaxMetaWritesPerEpoch)) {
          trap = Trap::BoundsFault;
          break;
        }

        // Read commands into a temporary list for all-or-nothing atomicity
        std::vector<RefinementCommand> commands;
        bool read_ok = true;
        for (int i = 0; i < cmd_count; ++i) {
          std::size_t base = static_cast<std::size_t>(cmd_addr + i * 4);
          if (!mem_ok(base) || !mem_ok(base + 3)) {
            read_ok = false;
            break;
          }
          RefinementCommand cmd;
          cmd.op = static_cast<RefinementCommand::Op>(state_.memory[base]);
          cmd.target = state_.memory[base + 1];
          cmd.value = state_.memory[base + 2];
          cmd.tag = static_cast<ValueTag>(state_.memory[base + 3]);
          commands.push_back(cmd);
        }

        if (!read_ok) {
          trap = Trap::BoundsFault;
          break;
        }

        // VALIDATION PASS (Atomicity check)
        std::size_t future_meta_write_count = state_.meta_write_count;
        for (const auto& cmd : commands) {
          switch (cmd.op) {
            case RefinementCommand::Op::WriteCode:
              // Code segment is protected: Writable only via privileged loader
              {
                t81::axion::Verdict code_verdict;
                code_verdict.kind = t81::axion::VerdictKind::Deny;
                code_verdict.reason = "MetaRefine WriteCode denied (protected)";
                record_axion_event(insn.opcode, insn.b, 0, code_verdict);
              }
              trap = Trap::SecurityFault;
              break;
              if (future_meta_write_count >= kMaxMetaWritesPerEpoch) {
                trap = Trap::SecurityFault;
                break;
              }
              if (cmd.target < 0 || static_cast<std::size_t>(cmd.target) >= program_.insns.size()) {
                trap = Trap::BoundsFault;
                break;
              }
              future_meta_write_count++;
              break;
            case RefinementCommand::Op::WriteReg:
              if (!reg_ok(static_cast<int>(cmd.target))) {
                trap = Trap::BoundsFault;
                break;
              }
              break;
            case RefinementCommand::Op::WriteMem:
              if (!mem_ok(static_cast<std::size_t>(cmd.target))) {
                trap = Trap::BoundsFault;
                break;
              }
              break;
            case RefinementCommand::Op::Noop:
              break;
          }
          if (trap != Trap::None) {
            break;
          }
        }

        if (trap != Trap::None) {
          break;
        }

        // APPLICATION PASS
        for (const auto& cmd : commands) {
          switch (cmd.op) {
            case RefinementCommand::Op::WriteCode:
              // Unreachable if validation works, but kept for safety or reverted?
              // Reverting to original logic for consistency if we ever allow it via policy
              program_.insns[cmd.target].opcode = static_cast<t81::tisc::Opcode>(cmd.value);
              state_.meta_write_count++;
              compiled_traces_.clear();  // Invalidate JIT cache
              break;
            case RefinementCommand::Op::WriteReg:
              ctx.registers[cmd.target] = cmd.value;
              ctx.register_tags[cmd.target] = cmd.tag;
              break;
            case RefinementCommand::Op::WriteMem:
              state_.memory[cmd.target] = cmd.value;
              state_.memory_tags[cmd.target] = cmd.tag;
              break;
            case RefinementCommand::Op::Noop:
              break;
          }
        }

        set_reg(insn.a, 1, ValueTag::Int);  // Success
        update_flags(1);
        record_axion_event(insn.opcode, insn.b, 1, verdict);
        break;
      }
      case t81::tisc::Opcode::TSqrt: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(ctx.registers[insn.b]);
        if (tensor == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        auto res_handle = alloc_tensor(t81::vm::internal::tensor_unary_sqrt(*tensor));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TSiLU: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(ctx.registers[insn.b]);
        if (tensor == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TSiLU kernel execution"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), ctx.registers[insn.b],
                           verdict);
        auto res_handle = alloc_tensor(t81::vm::internal::tensor_unary_silu(*tensor));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TSoftmax: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(ctx.registers[insn.b]);
        if (tensor == nullptr || !t81::vm::internal::tensor_softmax_compatible(*tensor)) {
          trap = Trap::DecodeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TSoftmax kernel execution"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), ctx.registers[insn.b],
                           verdict);
        auto res_handle = alloc_tensor(t81::vm::internal::tensor_unary_softmax(*tensor));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TRMSNorm: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        if (auto res = promote_to_tensor(insn.c); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(ctx.registers[insn.b]);
        auto* w = tensor_ptr(ctx.registers[insn.c]);
        if (tensor == nullptr || w == nullptr ||
            !t81::vm::internal::tensor_rmsnorm_compatible(*tensor, *w)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, 0, "TRMSNorm shape mismatch");
          trap = Trap::ShapeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TRMSNorm kernel execution"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), ctx.registers[insn.b],
                           verdict);
        auto res_handle = alloc_tensor(t81::vm::internal::tensor_rmsnorm(*tensor, *w));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TRoPE: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(ctx.registers[insn.b]);
        if (tensor == nullptr || !t81::vm::internal::tensor_rope_compatible(*tensor)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, 0, "TRoPE shape mismatch");
          trap = Trap::ShapeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TRoPE kernel execution"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), ctx.registers[insn.b],
                           verdict);
        int pos = static_cast<int>(ctx.registers[insn.c]);
        auto res_handle = alloc_tensor(t81::vm::internal::tensor_rope(*tensor, pos));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::Store: {
        if (!reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!check_mem(insn.opcode, insn.a, "memory store")) {
          trap = Trap::BoundsFault;
          break;
        }
        std::size_t addr = static_cast<std::size_t>(insn.a);
        state_.memory[addr] = ctx.registers[insn.b];
        state_.memory_tags[addr] = ctx.register_tags[insn.b];
        log_memory_segment_access(insn.opcode, t81::vm::internal::segment_for_address(state_, addr),
                                  addr, 1, t81::axion::reasons::kMemStore);
        break;
      }
      case t81::tisc::Opcode::Mul:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!is_integer_like_tag(ctx.register_tags[insn.b]) ||
            !is_integer_like_tag(ctx.register_tags[insn.c])) {
          trap = Trap::TypeFault;
          break;
        }
        if (auto lhs = bigint_from_integer_like(ctx.register_tags[insn.b], ctx.registers[insn.b]);
            lhs.has_value()) {
          auto rhs = bigint_from_integer_like(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!rhs.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          const bool preserve_bigint = ctx.register_tags[insn.b] == ValueTag::BigIntHandle ||
                                       ctx.register_tags[insn.c] == ValueTag::BigIntHandle;
          if (auto op_trap = store_integer_result(insn.a, *lhs * *rhs, preserve_bigint);
              op_trap.has_value()) {
            trap = *op_trap;
          }
        } else {
          trap = Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Div:
      case t81::tisc::Opcode::Mod: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!is_integer_like_tag(ctx.register_tags[insn.b]) ||
            !is_integer_like_tag(ctx.register_tags[insn.c])) {
          trap = Trap::TypeFault;
          break;
        }
        auto lhs = bigint_from_integer_like(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        auto rhs = bigint_from_integer_like(ctx.register_tags[insn.c], ctx.registers[insn.c]);
        if (!lhs.has_value() || !rhs.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        if (rhs->is_zero()) {
          trap = Trap::DivisionFault;
          break;
        }

        t81::T81BigInt result;
        if (insn.opcode == t81::tisc::Opcode::Div) {
          // Explicit truncation towards zero independent of host
          auto q = lhs->abs() / rhs->abs();
          if ((lhs->is_negative() && !rhs->is_negative()) || (!lhs->is_negative() && rhs->is_negative())) {
            result = -q;
          } else {
            result = q;
          }
        } else {
          // MOD definition: a = (a/b)*b + r
          auto q = lhs->abs() / rhs->abs();
          t81::T81BigInt trunc_div;
          if ((lhs->is_negative() && !rhs->is_negative()) || (!lhs->is_negative() && rhs->is_negative())) {
            trunc_div = -q;
          } else {
            trunc_div = q;
          }
          result = *lhs - (trunc_div * *rhs);
        }
        const bool preserve_bigint = ctx.register_tags[insn.b] == ValueTag::BigIntHandle ||
                                     ctx.register_tags[insn.c] == ValueTag::BigIntHandle;
        if (auto op_trap = store_integer_result(insn.a, std::move(result), preserve_bigint);
            op_trap.has_value()) {
          trap = *op_trap;
        }
        break;
      }
      case t81::tisc::Opcode::Jump:
        if (!check_mem(insn.opcode, insn.a, "jump", true)) {
          trap = Trap::DecodeFault;
          break;
        }
        ctx.pc = static_cast<std::size_t>(insn.a);
        break;
      case t81::tisc::Opcode::JumpIfZero:
        if (!reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        {
          const bool taken = ctx.registers[insn.b] == 0;
          record_branch_decision(taken);
          if (taken) {
            if (!check_mem(insn.opcode, insn.a, "jump if zero", true)) {
              trap = Trap::DecodeFault;
              break;
            }
            ctx.pc = static_cast<std::size_t>(insn.a);
          }
        }
        break;
      case t81::tisc::Opcode::JumpIfNotZero:
        if (!reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        {
          const bool taken = ctx.registers[insn.b] != 0;
          record_branch_decision(taken);
          if (taken) {
            if (!check_mem(insn.opcode, insn.a, "jump if not zero", true)) {
              trap = Trap::DecodeFault;
              break;
            }
            ctx.pc = static_cast<std::size_t>(insn.a);
          }
        }
        break;
      case t81::tisc::Opcode::Neg:
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!is_integer_like_tag(ctx.register_tags[insn.b])) {
          trap = Trap::TypeFault;
          break;
        }
        if (auto value = bigint_from_integer_like(ctx.register_tags[insn.b], ctx.registers[insn.b]);
            value.has_value()) {
          const bool preserve_bigint = ctx.register_tags[insn.b] == ValueTag::BigIntHandle;
          if (auto op_trap = store_integer_result(insn.a, -*value, preserve_bigint);
              op_trap.has_value()) {
            trap = *op_trap;
          }
        } else {
          trap = Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::JumpIfNegative: {
        const bool taken = ctx.flags.negative;
        record_branch_decision(taken);
        if (taken) {
          if (!check_mem(insn.opcode, insn.a, "jump if negative", true)) {
            trap = Trap::DecodeFault;
            break;
          }
          ctx.pc = static_cast<std::size_t>(insn.a);
        }
        break;
      }
      case t81::tisc::Opcode::JumpIfPositive: {
        const bool taken = ctx.flags.positive;
        record_branch_decision(taken);
        if (taken) {
          if (!check_mem(insn.opcode, insn.a, "jump if positive", true)) {
            trap = Trap::DecodeFault;
            break;
          }
          ctx.pc = static_cast<std::size_t>(insn.a);
        }
        break;
      }
      case t81::tisc::Opcode::Less:
      case t81::tisc::Opcode::LessEqual:
      case t81::tisc::Opcode::Greater:
      case t81::tisc::Opcode::GreaterEqual:
      case t81::tisc::Opcode::Equal:
      case t81::tisc::Opcode::NotEqual: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto tag_b = ctx.register_tags[insn.b];
        auto tag_c = ctx.register_tags[insn.c];
        // Allow Int/Bool mixed comparisons — both store 0/1 values.
        auto normalize_tag = [](ValueTag t) {
          if (t == ValueTag::Bool) return ValueTag::Int;
          return t;
        };
        const auto normalized_b = normalize_tag(tag_b);
        const auto normalized_c = normalize_tag(tag_c);
        const bool integer_mixed = is_integer_like_tag(normalized_b) && is_integer_like_tag(normalized_c);
        if (!integer_mixed && normalized_b != normalized_c) {
          trap = Trap::TypeFault;
          break;
        }
        std::optional<int> relation_opt;
        if (integer_mixed) {
          auto lhs = bigint_from_integer_like(normalized_b, ctx.registers[insn.b]);
          auto rhs = bigint_from_integer_like(normalized_c, ctx.registers[insn.c]);
          if (!lhs.has_value() || !rhs.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          if (*lhs == *rhs) {
            relation_opt = 0;
          } else {
            relation_opt = (*lhs < *rhs) ? -1 : 1;
          }
        } else {
          relation_opt = compare_value(normalized_b, ctx.registers[insn.b], ctx.registers[insn.c]);
        }
        if (!relation_opt.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        int relation = relation_opt.value();
        bool result = false;
        switch (insn.opcode) {
          case t81::tisc::Opcode::Less:
            result = relation < 0;
            break;
          case t81::tisc::Opcode::LessEqual:
            result = relation <= 0;
            break;
          case t81::tisc::Opcode::Greater:
            result = relation > 0;
            break;
          case t81::tisc::Opcode::GreaterEqual:
            result = relation >= 0;
            break;
          case t81::tisc::Opcode::Equal:
            result = relation == 0;
            break;
          case t81::tisc::Opcode::NotEqual:
            result = relation != 0;
            break;
          default:
            break;
        }
        ctx.registers[insn.a] = result ? 1 : 0;
        ctx.register_tags[insn.a] = ValueTag::Int;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::Cmp: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto tag_a = ctx.register_tags[insn.a];
        auto tag_b = ctx.register_tags[insn.b];
        const bool integer_mixed = is_integer_like_tag(tag_a) && is_integer_like_tag(tag_b);
        if (!integer_mixed && tag_a != tag_b) {
          trap = Trap::TypeFault;
          break;
        }
        std::optional<int> relation_opt;
        if (integer_mixed) {
          auto lhs = bigint_from_integer_like(tag_a, ctx.registers[insn.a]);
          auto rhs = bigint_from_integer_like(tag_b, ctx.registers[insn.b]);
          if (!lhs.has_value() || !rhs.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          if (*lhs == *rhs) {
            relation_opt = 0;
          } else {
            relation_opt = (*lhs < *rhs) ? -1 : 1;
          }
        } else {
          relation_opt = compare_value(tag_a, ctx.registers[insn.a], ctx.registers[insn.b]);
        }
        if (!relation_opt.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        int relation = relation_opt.value();
        ctx.flags.zero = (relation == 0);
        ctx.flags.negative = (relation < 0);
        ctx.flags.positive = (relation > 0);
        break;
      }
      case t81::tisc::Opcode::SetF: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        std::int64_t flag_value = 0;
        if (ctx.flags.negative) {
          flag_value = -1;
        } else if (!ctx.flags.zero) {
          flag_value = 1;
        }
        set_reg(insn.a, flag_value, ValueTag::Int);
        update_flags(flag_value);
        break;
      }
      case t81::tisc::Opcode::Push: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto addr_opt = push_stack(ctx.registers[insn.a], ctx.register_tags[insn.a]);
        if (!addr_opt.has_value()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(ctx.sp),
                           "stack push");
          trap = Trap::StackFault;
          break;
        }
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Stack, *addr_opt, 1,
                                  t81::axion::reasons::kMemStore);
        break;
      }
      case t81::tisc::Opcode::Pop: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        ValueTag tag = ValueTag::Int;
        auto addr_opt = pop_stack(ctx.registers[insn.a], tag);
        if (!addr_opt.has_value()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(ctx.sp),
                           "stack pop");
          trap = Trap::StackFault;
          break;
        }
        ctx.register_tags[insn.a] = tag;
        update_flags(ctx.registers[insn.a]);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Stack, *addr_opt, 1,
                                  t81::axion::reasons::kMemLoad);
        break;
      }
      case t81::tisc::Opcode::StackAlloc: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (insn.b < 0) {
          trap = Trap::DecodeFault;
          break;
        }
        const auto& stack = state_.layout.stack;
        if (!stack.valid()) {
          trap = Trap::DecodeFault;
          break;
        }
        auto size = t81::vm::internal::align_block81(static_cast<std::size_t>(insn.b));
        std::size_t available = ctx.sp - stack.start;
        if (size > available) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(ctx.sp),
                           "stack frame allocate");
          trap = Trap::StackFault;
          break;
        }
        std::size_t new_sp = ctx.sp - size;
        if (new_sp < stack.start) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(new_sp),
                           "stack frame allocate");
          trap = Trap::StackFault;
          break;
        }
        if (state_.policy && state_.policy->max_stack &&
            static_cast<std::int64_t>(stack.limit - new_sp) > *state_.policy->max_stack) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(new_sp),
                           "stack frame allocate");
          trap = Trap::StackFault;
          break;
        }
        std::int64_t addr = static_cast<std::int64_t>(new_sp);
        ctx.stack_frames.emplace_back(addr, static_cast<std::int64_t>(size));
        ctx.sp = new_sp;
        set_reg(insn.a, addr, ValueTag::Int);
        update_flags(addr);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Stack,
                                  static_cast<std::size_t>(addr), size,
                                  t81::axion::reasons::kStackAlloc);
        break;
      }
      case t81::tisc::Opcode::StackFree: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (insn.b < 0) {
          trap = Trap::DecodeFault;
          break;
        }
        const auto& stack = state_.layout.stack;
        auto size = t81::vm::internal::align_block81(static_cast<std::size_t>(insn.b));
        if (!stack.valid()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, 0, "stack frame free");
          trap = Trap::StackFault;
          break;
        }
        if (ctx.stack_frames.empty()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(ctx.sp),
                           "stack frame free");
          trap = Trap::StackFault;
          break;
        }
        std::int64_t ptr = ctx.registers[insn.a];
        if (!stack.contains(static_cast<std::size_t>(ptr))) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(ptr),
                           "stack frame free");
          trap = Trap::DecodeFault;
          break;
        }
        auto [expected_addr, expected_size] = ctx.stack_frames.back();
        if (expected_addr != ptr || expected_size != static_cast<std::int64_t>(size)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(ptr),
                           "stack frame free");
          trap = Trap::StackFault;
          break;
        }
        ctx.stack_frames.pop_back();
        ctx.sp = static_cast<std::size_t>(ptr + size);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Stack,
                                  static_cast<std::size_t>(ptr), size,
                                  t81::axion::reasons::kStackFree);
        break;
      }
      case t81::tisc::Opcode::HeapAlloc: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        const auto& heap = state_.layout.heap;
        if (!heap.valid()) {
          trap = Trap::DecodeFault;
          break;
        }
        if (insn.b < 0) {
          trap = Trap::DecodeFault;
          break;
        }
        auto size = t81::vm::internal::align_block81(static_cast<std::size_t>(insn.b));
        if (size > heap.size()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap, static_cast<int>(heap.limit),
                           "heap block allocate");
          trap = Trap::BoundsFault;
          break;
        }
        std::size_t addr = state_.heap_ptr;
        if (addr < heap.start || addr + size > heap.limit) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap, static_cast<int>(addr),
                           "heap block allocate");
          trap = Trap::BoundsFault;
          break;
        }
        if (ctx.registers[insn.a] != 0) {
          trap = Trap::DecodeFault;
          break;
        }
        state_.heap_frames.emplace_back(static_cast<std::int64_t>(addr),
                                        static_cast<std::int64_t>(size));
        state_.heap_ptr = addr + size;
        set_reg(insn.a, static_cast<std::int64_t>(addr), ValueTag::Int);
        update_flags(ctx.registers[insn.a]);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Heap, addr, size,
                                  t81::axion::reasons::kHeapAlloc);
        break;
      }
      case t81::tisc::Opcode::HeapFree: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        const auto& heap = state_.layout.heap;
        if (!heap.valid()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap, 0, "heap block free");
          trap = Trap::BoundsFault;
          break;
        }
        if (insn.b < 0) {
          trap = Trap::DecodeFault;
          break;
        }
        if (state_.heap_frames.empty()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap, static_cast<int>(state_.heap_ptr),
                           "heap block free");
          trap = Trap::BoundsFault;
          break;
        }
        auto size = t81::vm::internal::align_block81(static_cast<std::size_t>(insn.b));
        std::int64_t ptr = ctx.registers[insn.a];
        if (!heap.contains(static_cast<std::size_t>(ptr))) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap, static_cast<int>(ptr),
                           "heap block free");
          trap = Trap::DecodeFault;
          break;
        }
        auto [expected_addr, expected_size] = state_.heap_frames.back();
        if (expected_addr != ptr || expected_size != static_cast<std::int64_t>(size)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap, static_cast<int>(ptr),
                           "heap block free");
          trap = Trap::DecodeFault;
          break;
        }
        state_.heap_frames.pop_back();
        state_.heap_ptr = static_cast<std::size_t>(ptr);
        log_memory_segment_access(insn.opcode, MemorySegmentKind::Heap,
                                  static_cast<std::size_t>(ptr), size,
                                  t81::axion::reasons::kHeapFree);
        break;
      }
      case t81::tisc::Opcode::TNot:
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        {
          int t = clamp_trit(ctx.registers[insn.b]);
          ctx.registers[insn.a] = -t;
          ctx.register_tags[insn.a] = ValueTag::Int;
          update_flags(ctx.registers[insn.a]);
        }
        break;
      case t81::tisc::Opcode::TAnd:
      case t81::tisc::Opcode::TOr:
      case t81::tisc::Opcode::TXor:
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        {
          int lhs = clamp_trit(ctx.registers[insn.b]);
          int rhs = clamp_trit(ctx.registers[insn.c]);
          int result = 0;
          if (insn.opcode == t81::tisc::Opcode::TAnd) {
            result = (lhs < rhs) ? lhs : rhs;
          } else if (insn.opcode == t81::tisc::Opcode::TOr) {
            result = (lhs > rhs) ? lhs : rhs;
          } else {
            result = lhs - rhs;
            if (result > 1) {
              result = -1;
            }
            if (result < -1) {
              result = 1;
            }
          }
          ctx.registers[insn.a] = result;
          ctx.register_tags[insn.a] = ValueTag::Int;
          update_flags(ctx.registers[insn.a]);
        }
        break;
      case t81::tisc::Opcode::Call: {
        if (!reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        std::size_t next_depth = ctx.call_depth + 1;
        while (next_depth > recursion_limit_for_tier(ctx.tier_status.current) &&
               ctx.tier_status.current != t81::cog::TierId::Tier5) {
          if (!ensure_min_tier(
                  static_cast<t81::cog::TierId>(tier_rank(ctx.tier_status.current) + 1),
                  "call-depth")) {
            trap = Trap::TierFault;
            break;
          }
        }
        if (trap != Trap::None) {
          break;
        }
        if (next_depth > recursion_limit_for_tier(ctx.tier_status.current)) {
          std::ostringstream reason;
          reason << "TierFault recursion depth exceeded depth=" << next_depth
                 << " tier=" << static_cast<int>(ctx.tier_status.current)
                 << " limit=" << recursion_limit_for_tier(ctx.tier_status.current);
          record_tier_fault("recursion-limit", reason.str(), static_cast<std::int64_t>(next_depth));
          trap = Trap::TierFault;
          break;
        }

        if (ctx.call_depth >= kHardRecursionCeiling) {
          ++state_.contradiction_events;
          t81::axion::Verdict recursion_verdict;
          recursion_verdict.kind = t81::axion::VerdictKind::Deny;
          std::ostringstream reason;
          reason << t81::axion::reasons::kRecursionCeiling << " depth=" << ctx.call_depth
                 << " limit=" << kHardRecursionCeiling;
          recursion_verdict.reason = reason.str();
          record_axion_event(insn.opcode, insn.b, static_cast<std::int64_t>(ctx.call_depth),
                             recursion_verdict);
          trap = Trap::SecurityFault;
          break;
        }
        auto target = ctx.registers[insn.b];
        if (!check_mem(insn.opcode, static_cast<int>(target), "call", true)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!push_stack(static_cast<std::int64_t>(ctx.pc), ValueTag::Int)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(ctx.sp),
                           "stack call");
          trap = Trap::StackFault;
          break;
        }
        // NOTE: register save/restore for recursive isolation is deferred.
        // The calling convention mixes register and stack return channels;
        // proper caller-save discipline requires IRGen cooperation.
        // See: docs/architecture/OVERVIEW.md — "Recursive Register Isolation" work item.
        ++ctx.call_depth;
        ctx.pc = static_cast<std::size_t>(target);
        break;
      }
      case t81::tisc::Opcode::Ret: {
        std::int64_t addr = 0;
        ValueTag tag = ValueTag::Int;
        if (!pop_stack(addr, tag)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(ctx.sp),
                           "stack return");
          trap = Trap::StackFault;
          break;
        }
        if (tag != ValueTag::Int) {
          trap = Trap::TypeFault;
          break;
        }
        if (!check_mem(insn.opcode, static_cast<int>(addr), "return", true)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.call_depth > 0) {
          --ctx.call_depth;
        } else {
          ++state_.contradiction_events;
          t81::axion::Verdict contradiction_verdict;
          contradiction_verdict.kind = t81::axion::VerdictKind::Allow;
          contradiction_verdict.reason =
              std::string(t81::axion::reasons::kContradictionDetected) + " return-without-call";
          record_axion_event(insn.opcode, insn.a, addr, contradiction_verdict);
        }
        ctx.pc = static_cast<std::size_t>(addr);
        break;
      }
      case t81::tisc::Opcode::Trap:
        trap = Trap::TrapInstruction;
        break;
      case t81::tisc::Opcode::Print: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto rendered = format_value(ctx.register_tags[insn.a], ctx.registers[insn.a], 0);
        if (!rendered.has_value()) {
          trap = Trap::TypeFault;
          break;
        }
        state_.printed_output.push_back(*rendered);
        break;
      }
      case t81::tisc::Opcode::StrLen: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto symbol = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        if (!symbol.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        ctx.registers[insn.a] = static_cast<std::int64_t>(symbol->size());
        ctx.register_tags[insn.a] = ValueTag::Int;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrEmpty: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto symbol = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        if (!symbol.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        ctx.registers[insn.a] = symbol->empty() ? 1 : 0;
        ctx.register_tags[insn.a] = ValueTag::Bool;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::VecLen: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        std::int64_t length = 0;
        if (ctx.register_tags[insn.b] == ValueTag::StringVectorHandle) {
          auto* values = string_vector_ptr(ctx.registers[insn.b]);
          if (values == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          length = static_cast<std::int64_t>(values->size());
        } else if (ctx.register_tags[insn.b] == ValueTag::TensorHandle) {
          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (tensor->rank() != 1 || tensor->shape().empty()) {
            trap = Trap::TypeFault;
            break;
          }
          length = static_cast<std::int64_t>(tensor->shape().front());
        } else {
          trap = Trap::TypeFault;
          break;
        }
        ctx.registers[insn.a] = length;
        ctx.register_tags[insn.a] = ValueTag::Int;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::VecEmpty: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        bool is_empty = false;
        if (ctx.register_tags[insn.b] == ValueTag::StringVectorHandle) {
          auto* values = string_vector_ptr(ctx.registers[insn.b]);
          if (values == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          is_empty = values->empty();
        } else if (ctx.register_tags[insn.b] == ValueTag::TensorHandle) {
          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (tensor->rank() != 1 || tensor->shape().empty()) {
            trap = Trap::TypeFault;
            break;
          }
          is_empty = tensor->shape().front() == 0;
        } else {
          trap = Trap::TypeFault;
          break;
        }
        ctx.registers[insn.a] = is_empty ? 1 : 0;
        ctx.register_tags[insn.a] = ValueTag::Bool;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::VecFirst: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] == ValueTag::StringVectorHandle) {
          auto* values = string_vector_ptr(ctx.registers[insn.b]);
          if (values == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (values->empty()) {
            trap = Trap::TypeFault;
            break;
          }
          ctx.registers[insn.a] = intern_symbol(values->front());
          ctx.register_tags[insn.a] = ValueTag::SymbolHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        if (ctx.register_tags[insn.b] == ValueTag::TensorHandle) {
          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (tensor->rank() != 1 || tensor->shape().empty() || tensor->shape().front() <= 0) {
            trap = Trap::TypeFault;
            break;
          }
          const auto& data = tensor->data();
          if (data.empty()) {
            trap = Trap::TypeFault;
            break;
          }
          ctx.registers[insn.a] = static_cast<std::int64_t>(data.front());
          ctx.register_tags[insn.a] = ValueTag::Int;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        trap = Trap::TypeFault;
        break;
      }
      case t81::tisc::Opcode::VecLast: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] == ValueTag::StringVectorHandle) {
          auto* values = string_vector_ptr(ctx.registers[insn.b]);
          if (values == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (values->empty()) {
            trap = Trap::TypeFault;
            break;
          }
          ctx.registers[insn.a] = intern_symbol(values->back());
          ctx.register_tags[insn.a] = ValueTag::SymbolHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        if (ctx.register_tags[insn.b] == ValueTag::TensorHandle) {
          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (tensor->rank() != 1 || tensor->shape().empty() || tensor->shape().front() <= 0) {
            trap = Trap::TypeFault;
            break;
          }
          const auto& data = tensor->data();
          if (data.empty()) {
            trap = Trap::TypeFault;
            break;
          }
          ctx.registers[insn.a] = static_cast<std::int64_t>(data.back());
          ctx.register_tags[insn.a] = ValueTag::Int;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        trap = Trap::TypeFault;
        break;
      }
      case t81::tisc::Opcode::VecPush: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] == ValueTag::StringVectorHandle) {
          if (ctx.register_tags[insn.c] != ValueTag::SymbolHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* values = string_vector_ptr(ctx.registers[insn.b]);
          auto* value = symbol_ptr(ctx.registers[insn.c]);
          if (values == nullptr || value == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          std::vector<std::string> pushed = *values;
          pushed.push_back(*value);
          state_.string_vectors.push_back(std::move(pushed));
          ctx.registers[insn.a] = static_cast<std::int64_t>(state_.string_vectors.size());
          ctx.register_tags[insn.a] = ValueTag::StringVectorHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        if (ctx.register_tags[insn.b] == ValueTag::TensorHandle) {
          if (ctx.register_tags[insn.c] != ValueTag::Int) {
            trap = Trap::TypeFault;
            break;
          }
          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (tensor->rank() != 1 || tensor->shape().empty()) {
            trap = Trap::TypeFault;
            break;
          }
          const auto old_len = tensor->shape().front();
          if (old_len < 0) {
            trap = Trap::TypeFault;
            break;
          }
          auto data = tensor->data();
          data.push_back(static_cast<float>(ctx.registers[insn.c]));
          auto res_handle = alloc_tensor(T729DynamicTensor({old_len + 1}, std::move(data)));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        trap = Trap::TypeFault;
        break;
      }
      case t81::tisc::Opcode::VecPop: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] == ValueTag::StringVectorHandle) {
          auto* values = string_vector_ptr(ctx.registers[insn.b]);
          if (values == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (values->empty()) {
            trap = Trap::TypeFault;
            break;
          }
          std::vector<std::string> popped = *values;
          popped.pop_back();
          state_.string_vectors.push_back(std::move(popped));
          ctx.registers[insn.a] = static_cast<std::int64_t>(state_.string_vectors.size());
          ctx.register_tags[insn.a] = ValueTag::StringVectorHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        if (ctx.register_tags[insn.b] == ValueTag::TensorHandle) {
          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (tensor->rank() != 1 || tensor->shape().empty() || tensor->shape().front() <= 0) {
            trap = Trap::TypeFault;
            break;
          }
          auto data = tensor->data();
          if (data.empty()) {
            trap = Trap::TypeFault;
            break;
          }
          data.pop_back();
          auto res_handle =
              alloc_tensor(T729DynamicTensor({tensor->shape().front() - 1}, std::move(data)));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        trap = Trap::TypeFault;
        break;
      }
      case t81::tisc::Opcode::StrConcat: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto lhs = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        auto rhs = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
        if (!lhs.has_value() || !rhs.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        std::string combined(*lhs);
        combined += std::string(*rhs);
        ctx.registers[insn.a] = intern_symbol(std::move(combined));
        ctx.register_tags[insn.a] = ValueTag::SymbolHandle;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrStartsWith: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        auto prefix = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
        if (!value.has_value() || !prefix.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        const bool match =
            value->size() >= prefix->size() && value->compare(0, prefix->size(), *prefix) == 0;
        ctx.registers[insn.a] = match ? 1 : 0;
        ctx.register_tags[insn.a] = ValueTag::Bool;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrEndsWith: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        auto suffix = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
        if (!value.has_value() || !suffix.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        const bool match =
            value->size() >= suffix->size() &&
            value->compare(value->size() - suffix->size(), suffix->size(), *suffix) == 0;
        ctx.registers[insn.a] = match ? 1 : 0;
        ctx.register_tags[insn.a] = ValueTag::Bool;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrContains: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        auto needle = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
        if (!value.has_value() || !needle.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        const bool contains = value->find(*needle) != std::string::npos;
        ctx.registers[insn.a] = contains ? 1 : 0;
        ctx.register_tags[insn.a] = ValueTag::Bool;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrIndexOf: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        auto needle = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
        if (!value.has_value() || !needle.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        const std::size_t pos = value->find(*needle);
        ctx.registers[insn.a] = pos == std::string::npos ? -1 : static_cast<std::int64_t>(pos);
        ctx.register_tags[insn.a] = ValueTag::Int;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrReplace: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto source = symbol_like_text(ctx.register_tags[insn.a], ctx.registers[insn.a]);
        auto needle = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        auto replacement = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
        if (!source.has_value() || !needle.has_value() || !replacement.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        if (needle->empty()) {
          ctx.register_tags[insn.a] = ValueTag::SymbolHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }

        std::string replaced;
        replaced.reserve(source->size());
        std::size_t search_from = 0;
        while (true) {
          std::size_t pos = source->find(*needle, search_from);
          if (pos == std::string::npos) {
            replaced.append(*source, search_from, std::string::npos);
            break;
          }
          replaced.append(*source, search_from, pos - search_from);
          replaced.append(*replacement);
          search_from = pos + needle->size();
        }

        ctx.registers[insn.a] = intern_symbol(std::move(replaced));
        ctx.register_tags[insn.a] = ValueTag::SymbolHandle;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrVecNew: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        ctx.registers[insn.a] = alloc_string_vector();
        ctx.register_tags[insn.a] = ValueTag::StringVectorHandle;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrVecPush: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.a] != ValueTag::StringVectorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* values = string_vector_mut(ctx.registers[insn.a]);
        auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        if (values == nullptr || !value.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        values->push_back(std::string(*value));
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrSplit: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        auto sep = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
        if (!value.has_value() || !sep.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        if (sep->empty()) {
          trap = Trap::TypeFault;
          break;
        }
        std::vector<std::string> parts;
        std::size_t start = 0;
        while (true) {
          std::size_t pos = value->find(*sep, start);
          if (pos == std::string::npos) {
            parts.push_back(std::string(value->substr(start)));
            break;
          }
          parts.push_back(std::string(value->substr(start, pos - start)));
          start = pos + sep->size();
        }
        state_.string_vectors.push_back(std::move(parts));
        ctx.registers[insn.a] = static_cast<std::int64_t>(state_.string_vectors.size());
        ctx.register_tags[insn.a] = ValueTag::StringVectorHandle;
        {
          t81::axion::Verdict verdict;
          verdict.kind = t81::axion::VerdictKind::Allow;
          std::ostringstream reason;
          reason << t81::axion::reasons::kStringSplit << " input_len=" << value->size()
                 << " sep_len=" << sep->size() << " parts=" << state_.string_vectors.back().size();
          verdict.reason = reason.str();
          record_axion_event(insn.opcode, static_cast<std::int32_t>(state_.string_vectors.size()),
                             static_cast<std::int64_t>(state_.string_vectors.back().size()),
                             verdict);
        }
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::StrJoin: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* parts = string_vector_ptr(ctx.registers[insn.b]);
        auto sep = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
        if (parts == nullptr || !sep.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        std::string joined;
        if (!parts->empty()) {
          joined = parts->front();
          for (std::size_t i = 1; i < parts->size(); ++i) {
            joined += *sep;
            joined += parts->at(i);
          }
        }
        ctx.registers[insn.a] = intern_symbol(std::move(joined));
        ctx.register_tags[insn.a] = ValueTag::SymbolHandle;
        {
          t81::axion::Verdict verdict;
          verdict.kind = t81::axion::VerdictKind::Allow;
          std::ostringstream reason;
          reason << t81::axion::reasons::kStringJoin << " parts=" << parts->size()
                 << " sep_len=" << sep->size();
          verdict.reason = reason.str();
          record_axion_event(insn.opcode, static_cast<std::int32_t>(parts->size()),
                             ctx.registers[insn.a], verdict);
        }
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::I2F: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!is_integer_like_tag(ctx.register_tags[insn.b])) {
          trap = Trap::TypeFault;
          break;
        }
        double value = 0.0;
        if (ctx.register_tags[insn.b] == ValueTag::BigIntHandle) {
          auto* bigint = bigint_ptr(ctx.registers[insn.b]);
          if (!bigint) {
            trap = Trap::DecodeFault;
            break;
          }
          value = bigint->to_float<72, 9>().to_double();
        } else {
          value = static_cast<double>(ctx.registers[insn.b]);
        }
        ctx.registers[insn.a] = alloc_float(value);
        ctx.register_tags[insn.a] = ValueTag::FloatHandle;
        break;
      }
      case t81::tisc::Opcode::F2I: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::FloatHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* ptr_val = float_ptr(ctx.registers[insn.b]);
        if (!ptr_val) {
          trap = Trap::DecodeFault;
          break;
        }
        ctx.registers[insn.a] = static_cast<std::int64_t>(*ptr_val);
        ctx.register_tags[insn.a] = ValueTag::Int;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::I2Frac: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!is_integer_like_tag(ctx.register_tags[insn.b])) {
          trap = Trap::TypeFault;
          break;
        }
        t81::T81Fraction frac;
        if (ctx.register_tags[insn.b] == ValueTag::BigIntHandle) {
          auto* bigint = bigint_ptr(ctx.registers[insn.b]);
          if (!bigint) {
            trap = Trap::DecodeFault;
            break;
          }
          frac = t81::T81Fraction(*bigint, t81::T81BigInt::one());
        } else {
          frac = t81::T81Fraction::from_int(ctx.registers[insn.b]);
        }
        ctx.registers[insn.a] = alloc_fraction(std::move(frac));
        ctx.register_tags[insn.a] = ValueTag::FractionHandle;
        break;
      }
      case t81::tisc::Opcode::Int2BigInt: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (!is_integer_like_tag(ctx.register_tags[insn.b])) {
          trap = Trap::TypeFault;
          break;
        }
        if (auto value = bigint_from_integer_like(ctx.register_tags[insn.b], ctx.registers[insn.b]);
            value.has_value()) {
          if (auto op_trap = store_bigint_materialized(insn.a, std::move(*value)); op_trap.has_value()) {
            trap = *op_trap;
          }
        } else {
          trap = Trap::DecodeFault;
        }
        break;
      }
      case t81::tisc::Opcode::Frac2I: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::FractionHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* ptr_val = fraction_ptr(ctx.registers[insn.b]);
        if (!ptr_val || !t81::T81BigInt::is_one(ptr_val->den)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto op_trap = store_integer_result(insn.a, ptr_val->num); op_trap.has_value()) {
          trap = *op_trap;
        }
        break;
      }
      case t81::tisc::Opcode::FAdd:
      case t81::tisc::Opcode::FSub:
      case t81::tisc::Opcode::FMul:
      case t81::tisc::Opcode::FDiv: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::FloatHandle ||
            ctx.register_tags[insn.c] != ValueTag::FloatHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* lhs = float_ptr(ctx.registers[insn.b]);
        auto* rhs = float_ptr(ctx.registers[insn.c]);
        if (lhs == nullptr || rhs == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        double result = 0.0;
        switch (insn.opcode) {
          case t81::tisc::Opcode::FAdd:
            result = *lhs + *rhs;
            break;
          case t81::tisc::Opcode::FSub:
            result = *lhs - *rhs;
            break;
          case t81::tisc::Opcode::FMul:
            result = *lhs * *rhs;
            break;
          case t81::tisc::Opcode::FDiv:
            if (*rhs == 0.0) {
              trap = Trap::DivisionFault;
              break;
            }
            result = *lhs / *rhs;
            break;
          default:
            break;
        }
        if (trap != Trap::None) {
          break;
        }
        ctx.registers[insn.a] = alloc_float(result);
        ctx.register_tags[insn.a] = ValueTag::FloatHandle;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::FSin:
      case t81::tisc::Opcode::FCos:
      case t81::tisc::Opcode::FTan:
      case t81::tisc::Opcode::FAsin:
      case t81::tisc::Opcode::FAcos:
      case t81::tisc::Opcode::FAtan:
      case t81::tisc::Opcode::FSinh:
      case t81::tisc::Opcode::FCosh:
      case t81::tisc::Opcode::FTanh:
      case t81::tisc::Opcode::FSqrt:
      case t81::tisc::Opcode::FExp:
      case t81::tisc::Opcode::FLog:
      case t81::tisc::Opcode::FPow: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
            (insn.opcode == t81::tisc::Opcode::FPow && !reg_ok(insn.c))) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::FloatHandle) {
          trap = Trap::TypeFault;
          break;
        }
        if (insn.opcode == t81::tisc::Opcode::FPow &&
            ctx.register_tags[insn.c] != ValueTag::FloatHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* ptr_val = float_ptr(ctx.registers[insn.b]);
        if (!ptr_val) {
          trap = Trap::DecodeFault;
          break;
        }
        using VMFloat = t81::T81Float<72, 9>;
        const VMFloat input = VMFloat::from_double(*ptr_val);
        VMFloat out = VMFloat::zero();
        if (insn.opcode == t81::tisc::Opcode::FSin) {
          out = input.sin();
        } else if (insn.opcode == t81::tisc::Opcode::FCos) {
          out = input.cos();
        } else if (insn.opcode == t81::tisc::Opcode::FTan) {
          out = input.tan();
        } else if (insn.opcode == t81::tisc::Opcode::FAsin) {
          out = input.asin();
        } else if (insn.opcode == t81::tisc::Opcode::FAcos) {
          out = input.acos();
        } else if (insn.opcode == t81::tisc::Opcode::FAtan) {
          out = input.atan();
        } else if (insn.opcode == t81::tisc::Opcode::FSinh) {
          out = input.sinh();
        } else if (insn.opcode == t81::tisc::Opcode::FCosh) {
          out = input.cosh();
        } else if (insn.opcode == t81::tisc::Opcode::FTanh) {
          out = input.tanh();
        } else if (insn.opcode == t81::tisc::Opcode::FSqrt) {
          out = input.sqrt();
        } else if (insn.opcode == t81::tisc::Opcode::FExp) {
          out = input.exp();
        } else if (insn.opcode == t81::tisc::Opcode::FLog) {
          out = input.log();
        } else {
          auto* exponent = float_ptr(ctx.registers[insn.c]);
          if (!exponent) {
            trap = Trap::DecodeFault;
            break;
          }
          out = input.pow(VMFloat::from_double(*exponent));
        }
        ctx.registers[insn.a] = alloc_float(out.to_double());
        ctx.register_tags[insn.a] = ValueTag::FloatHandle;
        break;
      }
      case t81::tisc::Opcode::FracAdd:
      case t81::tisc::Opcode::FracSub:
      case t81::tisc::Opcode::FracMul:
      case t81::tisc::Opcode::FracDiv: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::FractionHandle ||
            ctx.register_tags[insn.c] != ValueTag::FractionHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* lhs = fraction_ptr(ctx.registers[insn.b]);
        auto* rhs = fraction_ptr(ctx.registers[insn.c]);
        if (lhs == nullptr || rhs == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        try {
          t81::T81Fraction result;
          switch (insn.opcode) {
            case t81::tisc::Opcode::FracAdd:
              result = t81::T81Fraction::add(*lhs, *rhs);
              break;
            case t81::tisc::Opcode::FracSub:
              result = t81::T81Fraction::sub(*lhs, *rhs);
              break;
            case t81::tisc::Opcode::FracMul:
              result = t81::T81Fraction::mul(*lhs, *rhs);
              break;
            case t81::tisc::Opcode::FracDiv:
              if (t81::T81BigInt::is_zero(rhs->num)) {
                trap = Trap::DivisionFault;
                break;
              }
              result = t81::T81Fraction::div(*lhs, *rhs);
              break;
            default:
              break;
          }
          if (trap != Trap::None) {
            break;
          }
          ctx.registers[insn.a] = alloc_fraction(std::move(result));
          ctx.register_tags[insn.a] = ValueTag::FractionHandle;
          update_flags(ctx.registers[insn.a]);
        } catch (...) {
          trap = Trap::DecodeFault;
        }
        break;
      }
      case t81::tisc::Opcode::ChkShape: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::TensorHandle ||
            ctx.register_tags[insn.c] != ValueTag::ShapeHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* tensor = tensor_ptr(ctx.registers[insn.b]);
        const auto* expected = shape_ptr(ctx.registers[insn.c]);
        if (tensor == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(ctx.registers[insn.b]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }
        if (expected == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        bool match = tensor->shape() == *expected;
        set_reg(insn.a, match ? 1 : 0, ValueTag::Int);
        update_flags(ctx.registers[insn.a]);
        break;
      }

      // -----------------------------------------------------------------
      // RFC-0005 v0.4 vector helpers
      // -----------------------------------------------------------------
      case t81::tisc::Opcode::ReadIsaVersion: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        // TISC version 0.4 constant.
        set_reg(insn.a, 4, ValueTag::Int);
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::VAdd: {
        // VAdd RD, RS1, RS2 — elementwise add on tensor handles.
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) { trap = res.error(); break; }
        if (auto res = promote_to_tensor(insn.c); !res) { trap = res.error(); break; }
        auto* t1 = tensor_ptr(ctx.registers[insn.b]);
        auto* t2 = tensor_ptr(ctx.registers[insn.c]);
        if (!t1 || !t2) { trap = Trap::DecodeFault; break; }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "VAdd kernel execution"};
        record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b], verdict);
        auto computed = t81::vm::internal::tensor_vec_binary_checked(*t1, *t2, /*multiply=*/false);
        if (!computed) { trap = computed.error(); break; }
        auto rh = alloc_tensor(std::move(*computed));
        if (!rh) { trap = rh.error(); break; }
        ctx.registers[insn.a] = *rh;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::VFma: {
        // VFma RD, RS1, RS2 — RD = RS1 * RS2 + RD (fused multiply-accumulate).
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.a] != ValueTag::TensorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) { trap = res.error(); break; }
        if (auto res = promote_to_tensor(insn.c); !res) { trap = res.error(); break; }
        auto* accum = tensor_ptr(ctx.registers[insn.a]);
        auto* t1    = tensor_ptr(ctx.registers[insn.b]);
        auto* t2    = tensor_ptr(ctx.registers[insn.c]);
        if (!accum || !t1 || !t2) { trap = Trap::DecodeFault; break; }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "VFma kernel execution"};
        record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b], verdict);
        auto computed = t81::vm::internal::tensor_vfma_checked(*accum, *t1, *t2);
        if (!computed) { trap = computed.error(); break; }
        auto rh = alloc_tensor(std::move(*computed));
        if (!rh) { trap = rh.error(); break; }
        ctx.registers[insn.a] = *rh;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::VLoad: {
        // VLoad RD, RS_SRC, RS_SHAPE — reshape RS_SRC tensor to the shape in RS_SHAPE.
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.c] != ValueTag::ShapeHandle) {
          trap = Trap::TypeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) { trap = res.error(); break; }
        auto* src = tensor_ptr(ctx.registers[insn.b]);
        const auto* new_shape = shape_ptr(ctx.registers[insn.c]);
        if (!src || !new_shape) { trap = Trap::DecodeFault; break; }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "VLoad kernel execution"};
        record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b], verdict);
        auto computed = t81::vm::internal::tensor_vload_checked(*src, *new_shape);
        if (!computed) { trap = computed.error(); break; }
        auto rh = alloc_tensor(std::move(*computed));
        if (!rh) { trap = rh.error(); break; }
        ctx.registers[insn.a] = *rh;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::VStore: {
        // VStore RD, RS_SRC, RS_SHAPE — shape-validated copy of RS_SRC.
        // Faults if RS_SRC shape != RS_SHAPE; stores validated copy handle in RD.
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.c] != ValueTag::ShapeHandle) {
          trap = Trap::TypeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) { trap = res.error(); break; }
        auto* src = tensor_ptr(ctx.registers[insn.b]);
        const auto* expected_shape = shape_ptr(ctx.registers[insn.c]);
        if (!src || !expected_shape) { trap = Trap::DecodeFault; break; }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "VStore kernel execution"};
        record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b], verdict);
        auto computed = t81::vm::internal::tensor_vstore_checked(*src, *expected_shape);
        if (!computed) { trap = computed.error(); break; }
        auto rh = alloc_tensor(std::move(*computed));
        if (!rh) { trap = rh.error(); break; }
        ctx.registers[insn.a] = *rh;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }

      case t81::tisc::Opcode::MakeOptionSome: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto handle = intern_option(true, ctx.register_tags[insn.b], ctx.registers[insn.b]);
        ctx.registers[insn.a] = handle;
        ctx.register_tags[insn.a] = ValueTag::OptionHandle;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeOptionNone: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto handle = intern_option(false, ValueTag::Int, 0);
        ctx.registers[insn.a] = handle;
        ctx.register_tags[insn.a] = ValueTag::OptionHandle;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeResultOk: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto handle = intern_result(true, ctx.register_tags[insn.b], ctx.registers[insn.b]);
        ctx.registers[insn.a] = handle;
        ctx.register_tags[insn.a] = ValueTag::ResultHandle;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeResultErr: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto handle = intern_result(false, ctx.register_tags[insn.b], ctx.registers[insn.b]);
        ctx.registers[insn.a] = handle;
        ctx.register_tags[insn.a] = ValueTag::ResultHandle;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeEnumVariant: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto handle = intern_enum(static_cast<int>(insn.b), false, ValueTag::Int, 0);
        ctx.registers[insn.a] = handle;
        ctx.register_tags[insn.a] = ValueTag::EnumHandle;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeEnumVariantPayload: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (insn.c < 0) {
          trap = Trap::DecodeFault;
          break;
        }
        auto handle = intern_enum(static_cast<int>(insn.c), true, ctx.register_tags[insn.b],
                                  ctx.registers[insn.b]);
        ctx.registers[insn.a] = handle;
        ctx.register_tags[insn.a] = ValueTag::EnumHandle;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MakeComplex: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::Int ||
            ctx.register_tags[insn.c] != ValueTag::Int) {
          trap = Trap::TypeFault;
          break;
        }
        auto handle = intern_complex(ctx.registers[insn.b], ctx.registers[insn.c]);
        ctx.registers[insn.a] = handle;
        ctx.register_tags[insn.a] = ValueTag::ComplexHandle;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::OptionIsSome: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::OptionHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* opt = option_ptr(ctx.registers[insn.b]);
        if (opt == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        set_reg(insn.a, opt->has_value ? 1 : 0, ValueTag::Int);
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::OptionUnwrap: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::OptionHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* opt = option_ptr(ctx.registers[insn.b]);
        if (opt == nullptr || !opt->has_value) {
          trap = Trap::DecodeFault;
          break;
        }
        set_reg(insn.a, opt->payload, opt->payload_tag);
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::ResultIsOk: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::ResultHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* res = result_ptr(ctx.registers[insn.b]);
        if (res == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        set_reg(insn.a, res->is_ok ? 1 : 0, ValueTag::Int);
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::ResultUnwrapOk: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::ResultHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* res = result_ptr(ctx.registers[insn.b]);
        if (res == nullptr || !res->is_ok) {
          trap = Trap::DecodeFault;
          break;
        }
        set_reg(insn.a, res->payload, res->payload_tag);
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::ResultUnwrapErr: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::ResultHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* res = result_ptr(ctx.registers[insn.b]);
        if (res == nullptr || res->is_ok) {
          trap = Trap::DecodeFault;
          break;
        }
        set_reg(insn.a, res->payload, res->payload_tag);
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::EnumIsVariant: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::EnumHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* val = enum_ptr(ctx.registers[insn.b]);
        if (val == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        bool matches = (val->variant_id == insn.c);
        set_reg(insn.a, matches ? 1 : 0, ValueTag::Int);
        update_flags(ctx.registers[insn.a]);
        {
          t81::axion::Verdict verdict;
          verdict.kind = t81::axion::VerdictKind::Allow;
          std::ostringstream reason;
          const int guard_variant_id = static_cast<int>(insn.c);  // explicit cast
          const int guard_enum_id = t81::enum_meta::decode_enum_id(guard_variant_id);
          const int guard_local_variant = t81::enum_meta::decode_variant_id(guard_variant_id);
          const auto* meta = enum_metadata_for(guard_enum_id);
          const auto* variant_meta = variant_metadata(meta, guard_local_variant);
          reason << "enum guard";
          if (meta != nullptr) {
            reason << " enum=" << meta->name;
          }
          if (variant_meta != nullptr) {
            reason << " variant=" << variant_meta->name;
            if (variant_meta->payload.has_value()) {
              reason << " payload=" << *variant_meta->payload;
            }
          }
          reason << " match=" << (matches ? "pass" : "fail");
          verdict.reason = reason.str();
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.c), matches ? 1 : 0,
                             verdict);
        }
        break;
      }
      case t81::tisc::Opcode::EnumUnwrapPayload: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::EnumHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* val = enum_ptr(ctx.registers[insn.b]);
        if (val == nullptr || !val->has_payload) {
          trap = Trap::DecodeFault;
          break;
        }
        set_reg(insn.a, val->payload, val->payload_tag);
        update_flags(ctx.registers[insn.a]);
        {
          t81::axion::Verdict verdict;
          verdict.kind = t81::axion::VerdictKind::Allow;
          std::ostringstream reason;
          const int global_variant_id = val->variant_id;
          const int enum_id = t81::enum_meta::decode_enum_id(global_variant_id);
          const int local_variant = t81::enum_meta::decode_variant_id(global_variant_id);
          const auto* meta = enum_metadata_for(enum_id);
          const auto* variant_meta = variant_metadata(meta, local_variant);
          reason << "enum payload";
          if (meta != nullptr) {
            reason << " enum=" << meta->name;
          }
          if (variant_meta != nullptr) {
            reason << " variant=" << variant_meta->name;
            if (variant_meta->payload.has_value()) {
              reason << " payload=" << *variant_meta->payload;
            }
          }
          verdict.reason = reason.str();
          record_axion_event(insn.opcode, static_cast<std::int32_t>(global_variant_id),
                             val->payload, verdict);
        }
        break;
      }
      case t81::tisc::Opcode::TVecAdd:
      case t81::tisc::Opcode::TVecMul: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        if (auto res = promote_to_tensor(insn.c); !res) {
          trap = res.error();
          break;
        }
        auto* tensor_a = tensor_ptr(ctx.registers[insn.b]);
        if (tensor_a == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(ctx.registers[insn.b]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }
        auto* tensor_b = tensor_ptr(ctx.registers[insn.c]);
        if (tensor_b == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(ctx.registers[insn.c]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                    insn.opcode == t81::tisc::Opcode::TVecAdd
                                        ? "TVecAdd kernel execution"
                                        : "TVecMul kernel execution"};
        record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                           verdict);

        const bool multiply = insn.opcode == t81::tisc::Opcode::TVecMul;
        auto computed =
            t81::vm::internal::tensor_vec_binary_checked(*tensor_a, *tensor_b, multiply);
        if (!computed.has_value()) {
          trap = computed.error();
          break;
        }
        auto res_handle = alloc_tensor(std::move(*computed));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TTranspose: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(ctx.registers[insn.b]);
        if (tensor == nullptr) {
          trap = Trap::ShapeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TTranspose kernel execution"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), ctx.registers[insn.b],
                           verdict);
        auto computed = t81::vm::internal::tensor_transpose_checked(*tensor);
        if (!computed.has_value()) {
          trap = computed.error();
          break;
        }
        auto res_handle = alloc_tensor(std::move(*computed));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TMatMul: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        if (auto res = promote_to_tensor(insn.c); !res) {
          trap = res.error();
          break;
        }
        auto* tensor_a = tensor_ptr(ctx.registers[insn.b]);
        if (tensor_a == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(ctx.registers[insn.b]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }
        auto* tensor_b = tensor_ptr(ctx.registers[insn.c]);
        if (tensor_b == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(ctx.registers[insn.c]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TMatMul kernel execution"};
        record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                           verdict);
        auto computed = t81::vm::internal::tensor_matmul_checked(*tensor_a, *tensor_b);
        if (!computed.has_value()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, 0, "TMatMul shape mismatch");
          trap = computed.error();
          break;
        }
        auto res_handle = alloc_tensor(std::move(*computed));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TTenDot: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        if (auto res = promote_to_tensor(insn.c); !res) {
          trap = res.error();
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::TensorHandle ||
            ctx.register_tags[insn.c] != ValueTag::TensorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* tensor_a = tensor_ptr(ctx.registers[insn.b]);
        if (tensor_a == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(ctx.registers[insn.b]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }
        auto* tensor_b = tensor_ptr(ctx.registers[insn.c]);
        if (tensor_b == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(ctx.registers[insn.c]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }
        auto computed = t81::vm::internal::tensor_contract_dot_checked(*tensor_a, *tensor_b);
        if (!computed.has_value()) {
          trap = computed.error();
          break;
        }
        auto res_handle = alloc_tensor(std::move(*computed));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::TGet: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }

        auto* tensor = tensor_ptr(ctx.registers[insn.b]);
        if (tensor == nullptr) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                           static_cast<int>(ctx.registers[insn.b]), "tensor handle access");
          trap = Trap::DecodeFault;
          break;
        }

        if (ctx.register_tags[insn.c] != ValueTag::Int) {
          trap = Trap::TypeFault;
          break;
        }
        std::int64_t index = ctx.registers[insn.c];
        auto value = t81::vm::internal::tensor_get_checked(*tensor, index);
        if (!value.has_value()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, static_cast<int>(index),
                           "tensor index out of bounds");
          trap = value.error();
          break;
        }
        ctx.registers[insn.a] = alloc_float(static_cast<double>(*value));
        ctx.register_tags[insn.a] = ValueTag::FloatHandle;

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TGet kernel execution"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), ctx.registers[insn.b],
                           verdict);
        break;
      }
      case t81::tisc::Opcode::TNew: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::Int) {
          trap = Trap::TypeFault;
          break;
        }
        std::int64_t size = ctx.registers[insn.b];
        auto tensor = t81::vm::internal::tensor_new_1d(size);
        if (!tensor.has_value()) {
          trap = Trap::BoundsFault;
          break;
        }
        auto res_handle = alloc_tensor(std::move(*tensor));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TNew"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(size), ctx.registers[insn.a],
                           verdict);
        break;
      }
      case t81::tisc::Opcode::TSet: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.a] != ValueTag::TensorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* tensor = tensor_ptr(ctx.registers[insn.a]);
        if (tensor == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }

        if (ctx.register_tags[insn.b] != ValueTag::Int) {
          trap = Trap::TypeFault;
          break;
        }
        std::int64_t idx = ctx.registers[insn.b];

        float val = 0.0F;
        auto val_tag = ctx.register_tags[insn.c];
        if (val_tag == ValueTag::FloatHandle) {
          auto* ptr_val = float_ptr(ctx.registers[insn.c]);
          if (ptr_val) val = static_cast<float>(*ptr_val);
        } else if (val_tag == ValueTag::Int) {
          val = static_cast<float>(ctx.registers[insn.c]);
        } else {
          trap = Trap::TypeFault;
          break;
        }

        const auto write_kind = val_tag == ValueTag::FloatHandle
                                    ? t81::tensor_mutation::ScalarWriteKind::FloatValue
                                    : t81::tensor_mutation::ScalarWriteKind::IntValue;
        auto set_res = t81::vm::internal::tensor_set_checked(*tensor, idx, val, write_kind);
        if (!set_res.has_value()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, static_cast<int>(idx),
                           "TSet OOB");
          trap = set_res.error();
          break;
        }
        t81::vm::internal::log_tensor_provenance(state_, state_.current_context, insn.opcode,
                                                 static_cast<std::size_t>(ctx.registers[insn.a]),
                                                 *tensor, "set");

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TSet"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), 0, verdict);
        break;
      }
      case t81::tisc::Opcode::TID: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(ctx.registers[insn.b]);
        if (tensor == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        auto res_handle = alloc_tensor(t81::vm::internal::tensor_identity_copy(*tensor));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TID (Identity/Copy)"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), ctx.registers[insn.a],
                           verdict);
        break;
      }
      case t81::tisc::Opcode::NSend: {
        if (!reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Deny,
                                    "Blocked: unimplemented async/network opcode"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), 0, verdict);
        trap = Trap::SecurityFault;
        break;
      }
      case t81::tisc::Opcode::NRecv: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Deny,
                                    "Blocked: unimplemented async/network opcode"};
        record_axion_event(insn.opcode, 0, 0, verdict);
        trap = Trap::SecurityFault;
        break;
      }
      case t81::tisc::Opcode::VWait: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Deny,
                                    "Blocked: unimplemented async/network opcode"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), 0, verdict);
        trap = Trap::SecurityFault;
        break;
      }
      case t81::tisc::Opcode::VYield: {
        if (!reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Deny,
                                    "Blocked: unimplemented async/network opcode"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), 0, verdict);
        trap = Trap::SecurityFault;
        break;
      }
      case t81::tisc::Opcode::TNorm: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        int t = clamp_trit(ctx.registers[insn.b]);
        set_reg(insn.a, t, ValueTag::Int);
        update_flags(t);
        break;
      }
      case t81::tisc::Opcode::Canon: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        // Canonicalize memory/register (placeholder logic)
        // In full implementation, this would enforce canonical representation
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "Canon"};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.a), ctx.registers[insn.a],
                           verdict);
        break;
      }
      case t81::tisc::Opcode::MemZero: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        std::size_t addr = static_cast<std::size_t>(ctx.registers[insn.a]);
        std::size_t size = static_cast<std::size_t>(ctx.registers[insn.b]);
        for (std::size_t i = 0; i < size; ++i) {
          if (!mem_ok(static_cast<int>(addr + i))) {
            trap = Trap::BoundsFault;
            break;
          }
          state_.memory[addr + i] = 0;
          state_.memory_tags[addr + i] = ValueTag::Int;
        }
        if (trap == Trap::None) {
          log_memory_segment_access(insn.opcode,
                                    t81::vm::internal::segment_for_address(state_, addr), addr,
                                    size, "MemZero");
        }
        break;
      }
      case t81::tisc::Opcode::Copy: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        std::size_t dst = static_cast<std::size_t>(ctx.registers[insn.a]);
        std::size_t src = static_cast<std::size_t>(ctx.registers[insn.b]);
        std::size_t size = static_cast<std::size_t>(ctx.registers[insn.c]);
        for (std::size_t i = 0; i < size; ++i) {
          if (!mem_ok(static_cast<int>(src + i)) || !mem_ok(static_cast<int>(dst + i))) {
            trap = Trap::BoundsFault;
            break;
          }
          // Check for overlap if needed, but TISC spec says COPY is guaranteed non-overlapping
          // usually, or we handle it safely.
        }
        if (trap == Trap::None) {
          // Perform copy
          for (std::size_t i = 0; i < size; ++i) {
            state_.memory[dst + i] = state_.memory[src + i];
            state_.memory_tags[dst + i] = state_.memory_tags[src + i];
          }
          log_memory_segment_access(
              insn.opcode, t81::vm::internal::segment_for_address(state_, dst), dst, size, "Copy");
        }
        break;
      }
      case t81::tisc::Opcode::Assert: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.registers[insn.a] == 0) {
          t81::axion::Verdict verdict;
          verdict.kind = t81::axion::VerdictKind::Deny;
          verdict.reason = "ASSERT failed";
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.a), 0, verdict);
          trap = Trap::AssertionFailed;
        }
        break;
      }
      // Cognitive Tier Stubs
      case t81::tisc::Opcode::SymLoad: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        t81::cog::v1::SymbolicGraph graph;
        if (ctx.register_tags[insn.b] == ValueTag::SymbolHandle) {
          auto* sym = symbol_ptr(ctx.registers[insn.b]);
          if (sym) {
            graph.add_node(t81::cog::v1::SymbolicAtom::create(*sym));
          }
        }
        auto res_handle = alloc_symbolic_graph(std::move(graph));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::SymbolicGraphHandle;
        {
          telemetry.symbolic_rewrites += 1;
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "SymLoad"};
          record_axion_event(insn.opcode, 0, ctx.registers[insn.a], verdict);
        }
        break;
      }
      case t81::tisc::Opcode::SymRewrite: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.a] != ValueTag::SymbolicGraphHandle ||
            ctx.register_tags[insn.b] != ValueTag::SymbolHandle ||
            ctx.register_tags[insn.c] != ValueTag::SymbolHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* graph = symbolic_graph_ptr(ctx.registers[insn.a]);
        if (!graph) {
          trap = Trap::BoundsFault;
          break;
        }
        auto* match_str = symbol_ptr(ctx.registers[insn.b]);
        auto* replace_str = symbol_ptr(ctx.registers[insn.c]);

        if (match_str && replace_str) {
          t81::cog::v1::RewriteRule rule;
          rule.match_node = t81::T81Symbol::intern(*match_str);
          rule.replace_node = t81::T81Symbol::intern(*replace_str);
          std::size_t old_nodes = graph->nodes.size();
          graph->apply_rewrite(rule);
          std::size_t new_nodes = graph->nodes.size();
          if (new_nodes >= old_nodes) {
            state_.metrics.total_symbolic_nodes += (new_nodes - old_nodes);
          } else {
            state_.metrics.total_symbolic_nodes -= (old_nodes - new_nodes);
          }
        }
        {
          telemetry.symbolic_rewrites += 3;
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "SymRewrite"};
          record_axion_event(insn.opcode, 0, 0, verdict);
        }
        break;
      }
      case t81::tisc::Opcode::SymConfluence: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::SymbolicGraphHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* graph = symbolic_graph_ptr(ctx.registers[insn.b]);
        bool conf = graph ? graph->is_confluent() : false;
        set_reg(insn.a, conf ? 1 : 0, ValueTag::Bool);
        {
          telemetry.symbolic_rewrites += 1;
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "SymConfluence"};
          record_axion_event(insn.opcode, 0, conf ? 1 : 0, verdict);
        }
        break;
      }
      case t81::tisc::Opcode::SymCanon: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.a] != ValueTag::SymbolicGraphHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* graph = symbolic_graph_ptr(ctx.registers[insn.a]);
        if (graph) graph->canonicalize();
        {
          telemetry.symbolic_rewrites += 2;
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "SymCanon"};
          record_axion_event(insn.opcode, 0, 0, verdict);
        }
        break;
      }
      case t81::tisc::Opcode::SymBind: {
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "SymBind"};
        record_axion_event(insn.opcode, 0, 0, verdict);
        break;
      }
      case t81::tisc::Opcode::ReflCap: {
        if (!ensure_min_tier(t81::cog::TierId::Tier2, "reflective-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        std::string description;
        if (ctx.register_tags[insn.b] == ValueTag::SymbolHandle) {
          auto* sym = symbol_ptr(ctx.registers[insn.b]);
          if (sym) description = *sym;
        }

        t81::cog::v2::ReflectiveFrame frame;
        // Make a copy of registers. std::array to std::vector.
        std::vector<std::int64_t> regs(ctx.registers.begin(), ctx.registers.end());
        frame.capture_state(current_pc, regs, description);

        ctx.tier2_frames.push_back(std::move(frame));
        ctx.registers[insn.a] = static_cast<std::int64_t>(ctx.tier2_frames.size());
        ctx.register_tags[insn.a] = ValueTag::Tier2FrameHandle;

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                    frame.axion_trace_event("capture")};
        record_axion_event(insn.opcode, static_cast<std::int32_t>(ctx.tier2_frames.size()), 0,
                           verdict);
        break;
      }
      case t81::tisc::Opcode::ReflJustify: {
        if (!ensure_min_tier(t81::cog::TierId::Tier2, "reflective-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.a] != ValueTag::Tier2FrameHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* frame = tier2_frame_ptr(ctx.registers[insn.a]);
        if (!frame) {
          trap = Trap::BoundsFault;
          break;
        }
        std::string text;
        if (auto s = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
            s.has_value()) {
          text = *s;
        }
        std::vector<std::int64_t> regs_snapshot(ctx.registers.begin(), ctx.registers.end());
        frame->add_justification_step(text, current_pc, regs_snapshot);

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                    frame->axion_trace_event("justify")};
        record_axion_event(insn.opcode, 0, 0, verdict);
        break;
      }
      case t81::tisc::Opcode::ReflCheck: {
        if (!ensure_min_tier(t81::cog::TierId::Tier2, "reflective-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::Tier2FrameHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* frame = tier2_frame_ptr(ctx.registers[insn.b]);
        if (!frame) {
          trap = Trap::BoundsFault;
          break;
        }
        std::string criteria;
        if (auto s = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
            s.has_value()) {
          criteria = *s;
        } else {
          trap = Trap::TypeFault;
          break;
        }

        bool result = frame->check(criteria);
        set_reg(insn.a, result ? 1 : 0, ValueTag::Bool);
        update_flags(ctx.registers[insn.a]);

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "ReflCheck"};
        record_axion_event(insn.opcode, 0, result, verdict);
        break;
      }
      case t81::tisc::Opcode::ReflTrace: {
        if (!ensure_min_tier(t81::cog::TierId::Tier2, "reflective-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.a] != ValueTag::Tier2FrameHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* frame = tier2_frame_ptr(ctx.registers[insn.a]);
        if (!frame) {
          trap = Trap::BoundsFault;
          break;
        }
        // Capture current registers
        std::vector<int64_t> current_regs(ctx.registers.begin(), ctx.registers.end());
        frame->trace(current_pc, current_regs);

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                    frame->axion_trace_event("trace")};
        record_axion_event(insn.opcode, 0, 0, verdict);
        break;
      }
      case t81::tisc::Opcode::ReflSeal: {
        if (!ensure_min_tier(t81::cog::TierId::Tier2, "reflective-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.a] != ValueTag::Tier2FrameHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* frame = tier2_frame_ptr(ctx.registers[insn.a]);
        if (!frame) {
          trap = Trap::BoundsFault;
          break;
        }
        frame->seal();

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                    frame->axion_trace_event("seal")};
        record_axion_event(insn.opcode, 0, static_cast<int64_t>(frame->hash), verdict);
        break;
      }
      case t81::tisc::Opcode::Recurse: {
        if (!ensure_min_tier(t81::cog::TierId::Tier3, "recurse-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        ctx.tier3_recursor.max_depth =
            static_cast<int>(recursion_limit_for_tier(ctx.tier_status.current));
        if (!ctx.tier3_recursor.can_recurse()) {
          record_tier_fault("tier3-recursor-limit", "Tier 3 recursion limit exceeded",
                            static_cast<std::int64_t>(ctx.tier3_recursor.current_depth));
          trap = Trap::TierFault;
          break;
        }
        const double seed_entropy =
            static_cast<double>((state_.layout.stack.limit - ctx.sp) +
                                (state_.heap_ptr - state_.layout.heap.start) + ctx.call_depth);
        t81::cog::v3::ContractionProof proof{true, seed_entropy, seed_entropy};
        if (!ctx.tier3_recursor.push_frame(proof)) {
          record_tier_fault("tier3-depth-proof", "Tier 3 depth proof rejected",
                            static_cast<std::int64_t>(ctx.tier3_recursor.current_depth));
          trap = Trap::TierFault;
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "Recurse: depth increased"};
        record_axion_event(insn.opcode, 0,
                           static_cast<std::int64_t>(ctx.tier3_recursor.current_depth), verdict);
        break;
      }
      case t81::tisc::Opcode::Contract: {
        if (!ensure_min_tier(t81::cog::TierId::Tier3, "contract-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        // Read entropy value (assumed integer or float handle?)
        double current_entropy = 0.0;
        if (ctx.register_tags[insn.b] == ValueTag::Int) {
          current_entropy = static_cast<double>(ctx.registers[insn.b]);
        } else if (ctx.register_tags[insn.b] == ValueTag::FloatHandle) {
          auto* ptr = float_ptr(ctx.registers[insn.b]);
          if (ptr) current_entropy = *ptr;
        }

        if (!ctx.tier3_recursor.contract_top(current_entropy)) {
          record_tier_fault("tier3-contraction", "Contract: non-contractive entropy update",
                            static_cast<std::int64_t>(current_entropy));
          trap = Trap::TierFault;
          break;
        }

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "Contract: entropy contracted"};
        record_axion_event(insn.opcode, 0, static_cast<std::int64_t>(current_entropy), verdict);
        break;
      }
      case t81::tisc::Opcode::Entropy: {
        if (!ensure_min_tier(t81::cog::TierId::Tier3, "entropy-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        // Calculate system entropy (stack usage + heap usage + depth)
        std::int64_t entropy = (state_.layout.stack.limit - ctx.sp) +
                               (state_.heap_ptr - state_.layout.heap.start) + ctx.call_depth;
        set_reg(insn.a, entropy, ValueTag::Int);
        update_flags(entropy);
        break;
      }
      case t81::tisc::Opcode::Depth: {
        if (!ensure_min_tier(t81::cog::TierId::Tier3, "depth-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        std::int64_t depth = static_cast<std::int64_t>(ctx.tier3_recursor.current_depth);
        set_reg(insn.a, depth, ValueTag::Int);
        update_flags(depth);
        break;
      }
      case t81::tisc::Opcode::Terminate: {
        if (!ensure_min_tier(t81::cog::TierId::Tier3, "terminate-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        ctx.tier3_recursor.pop_frame();
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "Terminate: depth decreased"};
        record_axion_event(insn.opcode, 0,
                           static_cast<std::int64_t>(ctx.tier3_recursor.current_depth), verdict);
        break;
      }
      case t81::tisc::Opcode::InfSeed: {
        if (!ensure_min_tier(t81::cog::TierId::Tier5, "infinite-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }

        t81::T81Fraction start_val;
        if (ctx.register_tags[insn.b] == ValueTag::Int) {
          start_val = t81::T81Fraction::from_int(ctx.registers[insn.b]);
        } else if (ctx.register_tags[insn.b] == ValueTag::FractionHandle) {
          auto* f = fraction_ptr(ctx.registers[insn.b]);
          if (f) start_val = *f;
        } else {
          trap = Trap::TypeFault;
          break;
        }

        t81::cog::v5::InfiniteCanonicalForm form;
        form.first_term = start_val;
        form.type = t81::cog::v5::SeriesType::Geometric;
        form.seed_lazy_prefix();

        auto res = alloc_infinite_form(std::move(form));
        if (!res) {
          trap = res.error();
          break;
        }
        ctx.registers[insn.a] = *res;
        ctx.register_tags[insn.a] = ValueTag::InfiniteHandle;

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "InfSeed"};
        record_axion_event(insn.opcode, 0, ctx.registers[insn.a], verdict);
        break;
      }
      case t81::tisc::Opcode::InfExpand: {
        if (!ensure_min_tier(t81::cog::TierId::Tier5, "infinite-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.a] != ValueTag::InfiniteHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* form = infinite_form_ptr(ctx.registers[insn.a]);
        if (!form) {
          trap = Trap::BoundsFault;
          break;
        }

        t81::T81Fraction ratio_val;
        if (ctx.register_tags[insn.b] == ValueTag::Int) {
          ratio_val = t81::T81Fraction::from_int(ctx.registers[insn.b]);
        } else if (ctx.register_tags[insn.b] == ValueTag::FractionHandle) {
          auto* f = fraction_ptr(ctx.registers[insn.b]);
          if (f) ratio_val = *f;
        } else {
          trap = Trap::TypeFault;
          break;
        }

        form->ratio = ratio_val;
        form->expand_lazy(1);

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "InfExpand"};
        record_axion_event(insn.opcode, 0, ctx.registers[insn.a], verdict);
        break;
      }
      case t81::tisc::Opcode::InfCollapse: {
        if (!ensure_min_tier(t81::cog::TierId::Tier5, "infinite-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.a] != ValueTag::InfiniteHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* form = infinite_form_ptr(ctx.registers[insn.a]);
        if (!form) {
          trap = Trap::BoundsFault;
          break;
        }

        form->collapse();

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "InfCollapse"};
        record_axion_event(insn.opcode, 0, ctx.registers[insn.a], verdict);
        break;
      }
      case t81::tisc::Opcode::InfConverge: {
        if (!ensure_min_tier(t81::cog::TierId::Tier5, "infinite-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::InfiniteHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* form = infinite_form_ptr(ctx.registers[insn.b]);
        if (!form) {
          trap = Trap::BoundsFault;
          break;
        }

        set_reg(insn.a, form->is_convergent ? 1 : 0, ValueTag::Bool);
        update_flags(ctx.registers[insn.a]);

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "InfConverge"};
        record_axion_event(insn.opcode, 0, form->is_convergent, verdict);
        break;
      }
      case t81::tisc::Opcode::InfSignature: {
        if (!ensure_min_tier(t81::cog::TierId::Tier5, "infinite-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::InfiniteHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* form = infinite_form_ptr(ctx.registers[insn.b]);
        if (!form) {
          trap = Trap::BoundsFault;
          break;
        }

        auto sig = t81::cog::v5::CollapseSignature::generate(*form);
        ctx.registers[insn.a] = intern_symbol(sig.hash);
        ctx.register_tags[insn.a] = ValueTag::SymbolHandle;

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "InfSignature"};
        record_axion_event(insn.opcode, 0, ctx.registers[insn.a], verdict);
        break;
      }
      case t81::tisc::Opcode::Gossip: {
        if (!ensure_min_tier(t81::cog::TierId::Tier4, "distributed-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        std::int64_t val = ctx.registers[insn.b];
        std::int32_t tag = static_cast<std::int32_t>(ctx.register_tags[insn.b]);
        state_.tier4_state.gossip(val, tag, instruction_count_);
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "Gossip: broadcast state"};
        record_axion_event(insn.opcode, tag, val, verdict);
        break;
      }
      case t81::tisc::Opcode::Merge: {
        if (!ensure_min_tier(t81::cog::TierId::Tier4, "distributed-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto msg = state_.tier4_state.merge();
        std::int64_t handle = 0;
        if (msg) {
          handle = intern_option(true, static_cast<ValueTag>(msg->tag), msg->payload);
        } else {
          handle = intern_option(false, ValueTag::Int, 0);
        }
        set_reg(insn.a, handle, ValueTag::OptionHandle);
        update_flags(handle);

        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "Merge: process inbox"};
        record_axion_event(insn.opcode, msg ? 1 : 0, handle, verdict);
        break;
      }
      case t81::tisc::Opcode::TickSync: {
        if (!ensure_min_tier(t81::cog::TierId::Tier4, "distributed-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        // Operand A contains remote tick
        uint64_t remote = static_cast<uint64_t>(ctx.registers[insn.a]);
        state_.tier4_state.sync_tick(remote);
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TickSync"};
        record_axion_event(insn.opcode, 0,
                           static_cast<int64_t>(state_.tier4_state.vector.global_tick), verdict);
        break;
      }
      case t81::tisc::Opcode::Coherence: {
        if (!ensure_min_tier(t81::cog::TierId::Tier4, "distributed-opcode")) {
          trap = Trap::TierFault;
          break;
        }
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        // Returns coherence status (e.g., drift from global tick)
        int64_t drift = static_cast<int64_t>(state_.tier4_state.vector.global_tick) -
                        static_cast<int64_t>(instruction_count_);
        set_reg(insn.a, drift, ValueTag::Int);
        update_flags(drift);
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "Coherence check"};
        record_axion_event(insn.opcode, 0, drift, verdict);
        break;
      }
      case t81::tisc::Opcode::DistSeal: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        // "Seal" the distributed state (e.g. generate a hash of outbox/inbox state)
        // For simulation, we just sum up outbox payloads
        int64_t seal = 0;
        for (const auto& m : state_.tier4_state.outbox) {
          seal ^= m.payload;
        }
        set_reg(insn.a, seal, ValueTag::Int);
        update_flags(seal);
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "DistSeal"};
        record_axion_event(insn.opcode, 0, seal, verdict);
        break;
      }
      case t81::tisc::Opcode::TNeuralFwd: {
        if (auto neural_trap = handle_blocked_neural_opcode(true); neural_trap.has_value()) {
          trap = *neural_trap;
        }
        break;
      }
      case t81::tisc::Opcode::TNeuralBwd: {
        if (auto neural_trap = handle_blocked_neural_opcode(false); neural_trap.has_value()) {
          trap = *neural_trap;
        }
        break;
      }
      case t81::tisc::Opcode::ATTN: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto regs = decode_ai_packed_reg_pair(insn.c);
        if (!regs.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        const int q_reg = insn.b;
        const int k_reg = regs->first;
        const int v_reg = regs->second;
        if (auto res = promote_to_tensor(q_reg); !res) {
          trap = res.error();
          break;
        }
        if (auto res = promote_to_tensor(k_reg); !res) {
          trap = res.error();
          break;
        }
        if (auto res = promote_to_tensor(v_reg); !res) {
          trap = res.error();
          break;
        }
        auto* tensor_q = tensor_ptr(ctx.registers[q_reg]);
        auto* tensor_k = tensor_ptr(ctx.registers[k_reg]);
        auto* tensor_v = tensor_ptr(ctx.registers[v_reg]);
        if (tensor_v == nullptr || tensor_q == nullptr || tensor_k == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        auto computed = t81::vm::internal::tensor_attention_checked(*tensor_q, *tensor_k, *tensor_v);
        if (!computed.has_value()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, 0, "ATTN shape mismatch");
          trap = computed.error();
          break;
        }
        t81::axion::Verdict verdict{
            t81::axion::VerdictKind::Allow,
            "ATTN kernel execution (phase1 packed operand encoding)"};
        record_axion_event(insn.opcode, static_cast<int32_t>(q_reg), ctx.registers[q_reg], verdict);
        auto res_handle = alloc_tensor(std::move(*computed));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::QMATMUL: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto regs = decode_ai_packed_reg_pair(insn.c);
        if (!regs.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        const int act_reg = insn.b;
        const int wt_reg = regs->first;
        const int scale_reg = regs->second;
        if (auto res = promote_to_tensor(act_reg); !res) {
          trap = res.error();
          break;
        }
        if (auto res = promote_to_tensor(wt_reg); !res) {
          trap = res.error();
          break;
        }
        auto* tensor_act = tensor_ptr(ctx.registers[act_reg]);
        auto* tensor_wt = tensor_ptr(ctx.registers[wt_reg]);
        if (tensor_act == nullptr || tensor_wt == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        t81::core::detail::DFixed fixed_scale = t81::core::detail::DFixed::zero();
        float scale = 0.0F;
        if (ctx.register_tags[scale_reg] == ValueTag::Int) {
          scale = static_cast<float>(ctx.registers[scale_reg]);
          fixed_scale = t81::core::detail::DFixed(static_cast<int>(ctx.registers[scale_reg]));
        } else if (ctx.register_tags[scale_reg] == ValueTag::FloatHandle) {
          auto* scale_ptr = float_ptr(ctx.registers[scale_reg]);
          if (scale_ptr == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          if (!std::isfinite(*scale_ptr)) {
            trap = Trap::TypeFault;
            break;
          }
          scale = static_cast<float>(*scale_ptr);
          using VMFloat = t81::T81Float<72, 9>;
          fixed_scale = t81::core::detail::DFixed::from_float(VMFloat::from_double(*scale_ptr));
        } else {
          trap = Trap::TypeFault;
          break;
        }
        t81::T729DynamicTensor computed;
        try {
          if (tensor_act->has_canonical_fixed_data() && tensor_wt->has_canonical_fixed_data() &&
              tensor_act->strict_core_eligible() && tensor_wt->strict_core_eligible()) {
            computed = t81::ops::qmatmul(*tensor_act, *tensor_wt, fixed_scale);
          } else {
            computed = t81::ops::qmatmul(*tensor_act, *tensor_wt, scale);
          }
        } catch (...) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, 0, "QMATMUL shape mismatch");
          trap = Trap::ShapeFault;
          break;
        }
        t81::axion::Verdict verdict{
            t81::axion::VerdictKind::Allow,
            "QMATMUL kernel execution (phase1 packed operand encoding)"};
        record_axion_event(insn.opcode, static_cast<int32_t>(act_reg), ctx.registers[act_reg], verdict);
        auto res_handle = alloc_tensor(std::move(computed));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::EMBED: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        if (ctx.register_tags[insn.c] != ValueTag::Int) {
          trap = Trap::TypeFault;
          break;
        }
        auto* table = tensor_ptr(ctx.registers[insn.b]);
        if (table == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        const std::int64_t index = ctx.registers[insn.c];
        auto computed = t81::vm::internal::tensor_embed_checked(*table, index);
        if (!computed.has_value()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, static_cast<int>(index),
                           "EMBED index/table mismatch");
          trap = computed.error();
          break;
        }
        t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "EMBED kernel execution"};
        record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b], verdict);
        auto res_handle = alloc_tensor(std::move(*computed));
        if (!res_handle) {
          trap = res_handle.error();
          break;
        }
        ctx.registers[insn.a] = *res_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::WLOAD: {
        // WLOAD RD, R_SRC, R_POLICY — AI-M4: CanonFS audit gate active.
        // R_POLICY register is reserved for phase-2 ambient policy dispatch.
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* wload_src = tensor_ptr(ctx.registers[insn.b]);
        if (wload_src == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        auto wload_computed = t81::vm::internal::tensor_wload_checked(*wload_src);
        if (!wload_computed.has_value()) {
          trap = wload_computed.error();
          break;
        }
        auto wload_handle = alloc_tensor(std::move(*wload_computed));
        if (!wload_handle) {
          trap = wload_handle.error();
          break;
        }
        // AI-M4: CanonFS audit — "meta slot axion event segment=meta addr=<n> action=WeightLoad"
        if (canonfs_driver_) {
          t81::vm::internal::log_canonfs_operation(state_, state_.current_context, insn.opcode,
                                                   t81::axion::reasons::kWeightLoad);
        }
        // Tensor provenance record for the materialized weight handle.
        const auto wload_out_h = static_cast<std::size_t>(*wload_handle);
        if (const auto& wload_stored = state_.tensors[wload_out_h - 1]; wload_stored.has_value()) {
          t81::vm::internal::log_tensor_provenance(state_, state_.current_context, insn.opcode,
                                                   wload_out_h, wload_stored.value(), "wload");
        }
        t81::axion::Verdict wload_verdict{t81::axion::VerdictKind::Allow,
                                          "WLOAD weight materialization"};
        record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                           wload_verdict);
        ctx.registers[insn.a] = *wload_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::GATHER: {
        // GATHER RD, R_SRC, PACK(R_IDX, R_AXIS) — AI-M5: axis register now active.
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto gather_regs = decode_ai_packed_reg_pair(insn.c);
        if (!gather_regs.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        const int gather_idx_reg = gather_regs->first;
        const int gather_axis_reg = gather_regs->second;
        if (ctx.register_tags[gather_idx_reg] != ValueTag::Int ||
            ctx.register_tags[gather_axis_reg] != ValueTag::Int) {
          trap = Trap::TypeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* gather_src = tensor_ptr(ctx.registers[insn.b]);
        if (gather_src == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        const std::int64_t gather_index = ctx.registers[gather_idx_reg];
        const int gather_axis = static_cast<int>(ctx.registers[gather_axis_reg]);
        auto gather_computed =
            t81::vm::internal::tensor_gather_checked(*gather_src, gather_index, gather_axis);
        if (!gather_computed.has_value()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, static_cast<int>(gather_index),
                           "GATHER index/axis mismatch");
          trap = gather_computed.error();
          break;
        }
        t81::axion::Verdict gather_verdict{t81::axion::VerdictKind::Allow,
                                           "GATHER kernel execution"};
        record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                           gather_verdict);
        auto gather_handle = alloc_tensor(std::move(*gather_computed));
        if (!gather_handle) {
          trap = gather_handle.error();
          break;
        }
        ctx.registers[insn.a] = *gather_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::SCATTER: {
        // SCATTER RD, R_DST, PACK(R_IDX, R_SRC) — AI-M5: aliasing detection active.
        // Phase-1 packed encoding convention: axis is fixed to 0.
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        auto scatter_regs = decode_ai_packed_reg_pair(insn.c);
        if (!scatter_regs.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        const int scatter_idx_reg = scatter_regs->first;
        const int scatter_src_reg = scatter_regs->second;
        if (ctx.register_tags[scatter_idx_reg] != ValueTag::Int) {
          trap = Trap::TypeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        if (auto res = promote_to_tensor(scatter_src_reg); !res) {
          trap = res.error();
          break;
        }
        auto* scatter_dst = tensor_ptr(ctx.registers[insn.b]);
        auto* scatter_src_ptr = tensor_ptr(ctx.registers[scatter_src_reg]);
        if (scatter_dst == nullptr || scatter_src_ptr == nullptr) {
          trap = Trap::DecodeFault;
          break;
        }
        const std::int64_t scatter_dst_handle = ctx.registers[insn.b];
        const std::int64_t scatter_index = ctx.registers[scatter_idx_reg];
        constexpr int scatter_axis = 0;  // Phase-1 packed encoding convention.

        // AI-M5: aliasing detection — RFC §5.15.6 MUST enforcement.
        const auto alias_key = std::make_tuple(scatter_dst_handle, scatter_axis, scatter_index);
        if (ctx.scatter_used.count(alias_key) != 0) {
          t81::axion::Verdict alias_verdict{
              t81::axion::VerdictKind::Deny,
              "SCATTER aliasing violation: same (dst, axis, index) reused in execution frame"};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), scatter_dst_handle,
                             alias_verdict);
          trap = Trap::SecurityFault;
          break;
        }
        ctx.scatter_used.insert(alias_key);

        auto scatter_computed = t81::vm::internal::tensor_scatter_checked(
            *scatter_dst, scatter_index, *scatter_src_ptr, scatter_axis);
        if (!scatter_computed.has_value()) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, static_cast<int>(scatter_index),
                           "SCATTER index/shape mismatch");
          trap = scatter_computed.error();
          break;
        }
        t81::axion::Verdict scatter_verdict{t81::axion::VerdictKind::Allow,
                                            "SCATTER kernel execution"};
        record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                           scatter_verdict);
        auto scatter_handle = alloc_tensor(std::move(*scatter_computed));
        if (!scatter_handle) {
          trap = scatter_handle.error();
          break;
        }
        ctx.registers[insn.a] = *scatter_handle;
        ctx.register_tags[insn.a] = ValueTag::TensorHandle;
        break;
      }
      case t81::tisc::Opcode::BitAnd:
      case t81::tisc::Opcode::BitOr:
      case t81::tisc::Opcode::BitXor: {
        if (auto bit_trap = handle_bitwise_binary(); bit_trap.has_value()) {
          trap = *bit_trap;
        }
        break;
      }
      case t81::tisc::Opcode::BitNot: {
        if (auto bit_trap = handle_bitwise_not(); bit_trap.has_value()) {
          trap = *bit_trap;
        }
        break;
      }
      case t81::tisc::Opcode::BitShl:
      case t81::tisc::Opcode::BitShr:
      case t81::tisc::Opcode::BitUShr: {
        if (auto bit_trap = handle_bitwise_shift(); bit_trap.has_value()) {
          trap = *bit_trap;
        }
        break;
      }
      case t81::tisc::Opcode::MapNew:
      case t81::tisc::Opcode::SetNew: {
        if (!reg_ok(insn.a)) {
          trap = Trap::DecodeFault;
          break;
        }
        ctx.registers[insn.a] = alloc_string_vector();
        ctx.register_tags[insn.a] = ValueTag::StringVectorHandle;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MapPut: {
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        // map=a (in/out), key=b, value=c.
        // NOTE: T81 TISC usually uses A as destination. Here A is also the map handle source?
        // Wait, typical TISC is dst, src1, src2.
        // So: dest_reg = MapPut(map_reg, key_reg, val_reg) ??
        // The IR generator call I'm planning to write will likely emit:
        // MapPut dest, map, key ?? No, TISC is 3 operands max.
        // But Put needs Map, Key, Value.
        // Let's assume: A=Map, B=Key, C=Value. And it updates Map in place (since handles are
        // refs). Return value? Map handle.
        if (ctx.register_tags[insn.a] != ValueTag::StringVectorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* vec = string_vector_mut(ctx.registers[insn.a]);
        auto key = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        auto val = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
        if (!vec || !key.has_value() || !val.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        bool found = false;
        for (size_t i = 0; i + 1 < vec->size(); i += 2) {
          if ((*vec)[i] == *key) {
            (*vec)[i + 1] = *val;
            found = true;
            break;
          }
        }
        if (!found) {
          vec->push_back(std::string(*key));
          vec->push_back(std::string(*val));
        }
        // Result is the map handle (A)
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MapGet: {
        // A=Dest, B=Map, C=Key
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* vec = string_vector_ptr(ctx.registers[insn.b]);
        auto key = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
        if (!vec || !key.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        std::optional<std::string> val;
        for (size_t i = 0; i + 1 < vec->size(); i += 2) {
          if ((*vec)[i] == *key) {
            val = (*vec)[i + 1];
            break;
          }
        }
        if (val) {
          std::int64_t handle = intern_symbol(*val);
          std::int64_t opt_handle = intern_option(true, ValueTag::SymbolHandle, handle);
          ctx.registers[insn.a] = opt_handle;
          ctx.register_tags[insn.a] = ValueTag::OptionHandle;
        } else {
          std::int64_t opt_handle = intern_option(false, ValueTag::Int, 0);
          ctx.registers[insn.a] = opt_handle;
          ctx.register_tags[insn.a] = ValueTag::OptionHandle;
        }
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MapHas: {
        // A=Dest, B=Map, C=Key
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* vec = string_vector_ptr(ctx.registers[insn.b]);
        auto key = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
        if (!vec || !key.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        bool found = false;
        for (size_t i = 0; i + 1 < vec->size(); i += 2) {
          if ((*vec)[i] == *key) {
            found = true;
            break;
          }
        }
        ctx.registers[insn.a] = found ? 1 : 0;
        ctx.register_tags[insn.a] = ValueTag::Bool;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MapRemove: {
        // A=Dest(Map), B=Map, C=Key -- Assume in-place update of B, returning B in A.
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* vec = string_vector_mut(ctx.registers[insn.b]);
        auto key = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
        if (!vec || !key.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        for (size_t i = 0; i + 1 < vec->size(); i += 2) {
          if ((*vec)[i] == *key) {
            // Remove key and value
            vec->erase(vec->begin() + i, vec->begin() + i + 2);
            break;
          }
        }
        ctx.registers[insn.a] = ctx.registers[insn.b];
        ctx.register_tags[insn.a] = ValueTag::StringVectorHandle;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MapKeys: {
        // A=Dest(Vec), B=Map
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* vec = string_vector_ptr(ctx.registers[insn.b]);
        if (!vec) {
          trap = Trap::DecodeFault;
          break;
        }
        std::vector<std::string> keys;
        for (size_t i = 0; i + 1 < vec->size(); i += 2) {
          keys.push_back((*vec)[i]);
        }
        state_.string_vectors.push_back(std::move(keys));
        ctx.registers[insn.a] = static_cast<std::int64_t>(state_.string_vectors.size());
        ctx.register_tags[insn.a] = ValueTag::StringVectorHandle;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::MapSize: {
        // A=Dest(Int), B=Map
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* vec = string_vector_ptr(ctx.registers[insn.b]);
        if (!vec) {
          trap = Trap::DecodeFault;
          break;
        }
        ctx.registers[insn.a] = static_cast<std::int64_t>(vec->size() / 2);
        ctx.register_tags[insn.a] = ValueTag::Int;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::SetAdd: {
        // A=Set(in/out), B=Key
        // TISC instructions usually have A as dest. We will use A as Set handle.
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.a] != ValueTag::StringVectorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* vec = string_vector_mut(ctx.registers[insn.a]);
        auto key = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        if (!vec || !key.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        bool found = false;
        for (const auto& item : *vec) {
          if (item == *key) {
            found = true;
            break;
          }
        }
        if (!found) {
          vec->push_back(std::string(*key));
        }
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::SetRemove: {
        // A=Set(in/out), B=Key
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.a] != ValueTag::StringVectorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* vec = string_vector_mut(ctx.registers[insn.a]);
        auto key = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
        if (!vec || !key.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        for (auto it = vec->begin(); it != vec->end(); ++it) {
          if (*it == *key) {
            vec->erase(it);
            break;
          }
        }
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::SetHas: {
        // A=Dest(Bool), B=Set, C=Key
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* vec = string_vector_ptr(ctx.registers[insn.b]);
        auto key = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
        if (!vec || !key.has_value()) {
          trap = Trap::DecodeFault;
          break;
        }
        bool found = false;
        for (const auto& item : *vec) {
          if (item == *key) {
            found = true;
            break;
          }
        }
        ctx.registers[insn.a] = found ? 1 : 0;
        ctx.register_tags[insn.a] = ValueTag::Bool;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::SetSize: {
        // A=Dest(Int), B=Set
        if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
          trap = Trap::TypeFault;
          break;
        }
        auto* vec = string_vector_ptr(ctx.registers[insn.b]);
        if (!vec) {
          trap = Trap::DecodeFault;
          break;
        }
        ctx.registers[insn.a] = static_cast<std::int64_t>(vec->size());
        ctx.register_tags[insn.a] = ValueTag::Int;
        update_flags(ctx.registers[insn.a]);
        break;
      }
      case t81::tisc::Opcode::TShape: {
        // TShape A, B, C — A = shape[R[C]] of tensor R[B]
        if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
          trap = Trap::DecodeFault;
          break;
        }
        if (auto res = promote_to_tensor(insn.b); !res) {
          trap = res.error();
          break;
        }
        auto* tensor = tensor_ptr(ctx.registers[insn.b]);
        if (!tensor) {
          trap = Trap::DecodeFault;
          break;
        }
        if (ctx.register_tags[insn.c] != ValueTag::Int) {
          trap = Trap::TypeFault;
          break;
        }
        std::int64_t dim_idx = ctx.registers[insn.c];
        const auto& shape = tensor->shape();
        if (dim_idx < 0 || static_cast<std::size_t>(dim_idx) >= shape.size()) {
          trap = Trap::BoundsFault;
          break;
        }
        ctx.registers[insn.a] = static_cast<std::int64_t>(shape[static_cast<std::size_t>(dim_idx)]);
        ctx.register_tags[insn.a] = ValueTag::Int;
        update_flags(ctx.registers[insn.a]);
        break;
      }
        default:
          trap = Trap::DecodeFault;
          break;
      }
    }
    if (trap == Trap::None) {
      telemetry.epoch_steps += 1;
      const double branch_entropy = branch_entropy_bits();
      const std::size_t symbolic_complexity = state_.total_symbolic_nodes +
                                              (state_.symbolic_graphs.size() * 2) +
                                              (telemetry.symbolic_rewrites * 3);

      std::size_t observed_shape_complexity = telemetry.max_shape_complexity;
      std::size_t observed_tensor_rank = telemetry.max_tensor_rank;
      for (const auto& tensor : state_.tensors) {
        if (!tensor.has_value()) continue;
        observed_shape_complexity = std::max(observed_shape_complexity,
                                             t81::vm::internal::tensor_shape_complexity(*tensor));
        observed_tensor_rank =
            std::max(observed_tensor_rank, static_cast<std::size_t>(tensor->rank()));
      }

      auto promote_to_fit_tier_limit = [&](bool exceeded, std::string_view cause) -> bool {
        while (exceeded && ctx.tier_status.current != t81::cog::TierId::Tier5) {
          if (!ensure_min_tier(tier_from_rank(tier_rank(ctx.tier_status.current) + 1), cause)) {
            trap = Trap::TierFault;
            return false;
          }
          exceeded = false;
          if (cause == std::string_view("branching-entropy")) {
            exceeded = branch_entropy > max_branch_entropy_for_tier(ctx.tier_status.current);
          } else if (cause == std::string_view("symbolic-complexity")) {
            exceeded =
                symbolic_complexity > max_symbolic_complexity_for_tier(ctx.tier_status.current);
          } else if (cause == std::string_view("shape-complexity")) {
            exceeded =
                observed_shape_complexity > max_shape_complexity_for_tier(ctx.tier_status.current);
          } else if (cause == std::string_view("tensor-rank")) {
            exceeded = observed_tensor_rank >
                       static_cast<std::size_t>(max_tensor_rank_for_tier(ctx.tier_status.current));
          }
        }
        return true;
      };

      bool exceeded_branch_entropy =
          branch_entropy > max_branch_entropy_for_tier(ctx.tier_status.current);
      if (exceeded_branch_entropy &&
          !promote_to_fit_tier_limit(exceeded_branch_entropy, "branching-entropy")) {
        record_tier_fault("branching-entropy",
                          "Branching entropy exceeded policy after promotion attempts",
                          static_cast<std::int64_t>(branch_entropy));
      }
      if (trap == Trap::None &&
          branch_entropy > max_branch_entropy_for_tier(ctx.tier_status.current)) {
        record_tier_fault("branching-entropy", "Branching entropy exceeded tier ceiling",
                          static_cast<std::int64_t>(branch_entropy));
        trap = Trap::TierFault;
      }

      bool exceeded_symbolic =
          symbolic_complexity > max_symbolic_complexity_for_tier(ctx.tier_status.current);
      if (trap == Trap::None && exceeded_symbolic &&
          !promote_to_fit_tier_limit(exceeded_symbolic, "symbolic-complexity")) {
        record_tier_fault("symbolic-complexity",
                          "Symbolic complexity exceeded policy after promotion attempts",
                          static_cast<std::int64_t>(symbolic_complexity));
      }
      if (trap == Trap::None &&
          symbolic_complexity > max_symbolic_complexity_for_tier(ctx.tier_status.current)) {
        record_tier_fault("symbolic-complexity", "Symbolic complexity exceeded tier ceiling",
                          static_cast<std::int64_t>(symbolic_complexity));
        trap = Trap::TierFault;
      }

      bool exceeded_shape =
          observed_shape_complexity > max_shape_complexity_for_tier(ctx.tier_status.current);
      if (trap == Trap::None && exceeded_shape &&
          !promote_to_fit_tier_limit(exceeded_shape, "shape-complexity")) {
        record_tier_fault("shape-complexity",
                          "Shape complexity exceeded policy after promotion attempts",
                          static_cast<std::int64_t>(observed_shape_complexity));
      }
      if (trap == Trap::None &&
          observed_shape_complexity > max_shape_complexity_for_tier(ctx.tier_status.current)) {
        record_tier_fault("shape-complexity", "Tensor shape complexity exceeded tier ceiling",
                          static_cast<std::int64_t>(observed_shape_complexity));
        trap = Trap::TierFault;
      }

      bool exceeded_rank =
          observed_tensor_rank >
          static_cast<std::size_t>(max_tensor_rank_for_tier(ctx.tier_status.current));
      if (trap == Trap::None && exceeded_rank &&
          !promote_to_fit_tier_limit(exceeded_rank, "tensor-rank")) {
        record_tier_fault("tensor-rank", "Tensor rank exceeded policy after promotion attempts",
                          static_cast<std::int64_t>(observed_tensor_rank));
      }
      if (trap == Trap::None &&
          observed_tensor_rank >
              static_cast<std::size_t>(max_tensor_rank_for_tier(ctx.tier_status.current))) {
        record_tier_fault("tensor-rank", "Tensor rank exceeded tier ceiling",
                          static_cast<std::int64_t>(observed_tensor_rank));
        trap = Trap::TierFault;
      }

      if (trap == Trap::None) {
        int required_rank = 1;
        required_rank = std::max(required_rank, infer_required_tier_for_recursion());
        if (branch_entropy > max_branch_entropy_for_tier(t81::cog::TierId::Tier1))
          required_rank = 2;
        if (branch_entropy > max_branch_entropy_for_tier(t81::cog::TierId::Tier2))
          required_rank = 3;
        if (branch_entropy > max_branch_entropy_for_tier(t81::cog::TierId::Tier3))
          required_rank = 4;
        if (branch_entropy > max_branch_entropy_for_tier(t81::cog::TierId::Tier4))
          required_rank = 5;
        if (symbolic_complexity > max_symbolic_complexity_for_tier(t81::cog::TierId::Tier1))
          required_rank = std::max(required_rank, 3);
        if (observed_tensor_rank >
                static_cast<std::size_t>(max_tensor_rank_for_tier(t81::cog::TierId::Tier1)) ||
            observed_shape_complexity > max_shape_complexity_for_tier(t81::cog::TierId::Tier1)) {
          required_rank = std::max(required_rank, 2);
        }
        if (observed_tensor_rank >
                static_cast<std::size_t>(max_tensor_rank_for_tier(t81::cog::TierId::Tier2)) ||
            observed_shape_complexity > max_shape_complexity_for_tier(t81::cog::TierId::Tier2)) {
          required_rank = std::max(required_rank, 3);
        }
        if (!state_.tier4_state.inbox.empty() || !state_.tier4_state.outbox.empty()) {
          required_rank = std::max(required_rank, 4);
        }
        bool has_infinite_state = false;
        for (const auto& inf : state_.infinite_forms) {
          if (inf.has_value()) {
            has_infinite_state = true;
            break;
          }
        }
        if (has_infinite_state) {
          required_rank = std::max(required_rank, 5);
        }

        if (required_rank < tier_rank(ctx.tier_status.current)) {
          telemetry.stable_simple_steps += 1;
          if (telemetry.stable_simple_steps >= 81) {
            const int candidate_rank = tier_rank(ctx.tier_status.current) - 1;
            const auto candidate_tier = tier_from_rank(candidate_rank);
            bool converged = true;
            converged = converged && ctx.call_depth <= recursion_limit_for_tier(candidate_tier) &&
                        static_cast<std::size_t>(ctx.tier3_recursor.current_depth) <=
                            recursion_limit_for_tier(candidate_tier);
            if (candidate_rank < 5) {
              converged = converged && !has_infinite_state;
            }
            if (candidate_rank < 4) {
              converged = converged && state_.tier4_state.inbox.empty() &&
                          state_.tier4_state.outbox.empty();
            }
            if (candidate_rank < 3) {
              converged = converged && state_.symbolic_graphs.empty();
            }
            if (converged && candidate_rank >= required_rank) {
              ctx.tier_status.current = candidate_tier;
              ctx.tier_status.label = "tier-" + std::to_string(candidate_rank) + "-demoted";
              t81::axion::Verdict demotion_verdict;
              demotion_verdict.kind = t81::axion::VerdictKind::Allow;
              demotion_verdict.reason = "Cognitive Tier Demotion: convergence conditions met";
              record_axion_event(insn.opcode, candidate_rank,
                                 static_cast<std::int64_t>(ctx.tier_status.current),
                                 demotion_verdict);
              telemetry.stable_simple_steps = 0;
            }
          }
        } else {
          telemetry.stable_simple_steps = 0;
        }
      }

      if (telemetry.epoch_steps >= 243) {
        telemetry.epoch_steps = 0;
        telemetry.branch_events = 0;
        telemetry.branch_taken = 0;
        telemetry.symbolic_rewrites = 0;
        telemetry.max_shape_complexity = observed_shape_complexity;
        telemetry.max_tensor_rank = observed_tensor_rank;
      }
    }
    ++instructions_since_gc_;
    if (instructions_since_gc_ >= kGcInterval) {
      run_gc_cycle_("interval");
    }

    t81::vm::internal::sync_system_registers(state_, program_, instruction_count_,
                                             state_.current_context);
    log_trace(insn.opcode, trap);
    if (trap != Trap::None) {
      return t81::unexpected(trap);
    }

    // Schedule next context for the next step
    state_.current_context = (state_.current_context + 1) % state_.contexts.size();

    return {};
  }

  std::expected<void, Trap> run_to_halt(std::size_t max_steps) override {
    for (std::size_t i = 0; i < max_steps && !state_.halted; ++i) {
      auto result = step();
      if (!result.has_value()) {
        return result;
      }
    }
    // AX-M5: when a determinism detector is registered, record this run's hash
    // chain and compare against the previous run.  A divergence is reported as
    // a SecurityFault so callers see a hard error rather than silent mismatch.
    if (determinism_detector_ && state_.halted) {
      determinism_detector_->record_run(state_.axion_log);
      auto report = determinism_detector_->check_against_previous();
      if (report.diverged) {
        t81::axion::Verdict v;
        v.kind = t81::axion::VerdictKind::Deny;
        v.reason = report.reason;
        record_axion_event(t81::tisc::Opcode::Halt, 0, 0, v);
        return t81::unexpected(Trap::SecurityFault);
      }
    }
    return {};
  }

  void set_canonfs_root(const std::filesystem::path& root) override {
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    canonfs_driver_ = t81::canonfs::make_persistent_driver(root);
  }

  void set_determinism_detector(t81::axion::DeterminismDetector* detector) override {
    determinism_detector_ = detector;
  }

  const State& state() const override { return state_; }

  void set_register(int idx, std::int64_t val_data, ValueTag tag) override {
    if (state_.contexts.empty()) return;
    auto& ctx = state_.contexts[state_.current_context];
    if (idx < 0 || static_cast<std::size_t>(idx) >= ctx.registers.size()) {
      return;
    }
    if (idx == 0 || (idx >= 75 && idx <= 80)) {
      return;
    }
    ctx.registers[idx] = val_data;
    ctx.register_tags[idx] = tag;

    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason_stream;
    reason_stream << "register mutation R" << idx << " value=" << val_data;
    verdict.reason = reason_stream.str();
    record_axion_event(t81::tisc::Opcode::Nop, idx, val_data, verdict);
  }

  void set_memory_word(std::size_t word_index, std::int64_t value) noexcept override {
    if (word_index >= state_.memory.size()) return;
    state_.memory[word_index]      = value;
    state_.memory_tags[word_index] = ValueTag::Int;
  }

  void set_fault_injections(std::vector<FaultInjection> faults) override {
    state_.pending_faults = std::move(faults);
  }

private:
  std::int64_t intern_weights_tensor(std::string_view name) {
    if (name.empty() || !state_.weights_model) {
      return 0;
    }
    auto key = std::string(name);
    auto iter = state_.weights_tensor_handles.find(key);
    if (iter != state_.weights_tensor_handles.end()) {
      return iter->second;
    }
    auto native_iter = state_.weights_model->native.find(key);
    if (native_iter == state_.weights_model->native.end()) {
      return 0;
    }
    state_.weights_tensor_refs.push_back(&native_iter->second);
    auto handle = static_cast<std::int64_t>(state_.weights_tensor_refs.size());
    state_.weights_tensor_handles.emplace(std::move(key), handle);
    return handle;
  }

  t81::axion::Verdict eval_axion_call(
      std::string_view syscall, std::size_t prog_counter, t81::tisc::Opcode opcode,
      std::string_view payload = {},
      std::optional<std::size_t> instruction_count_override = std::nullopt) {
    if (syscall == t81::axion::reasons::kMetaRead) {
      // Internal MetaRead check could go here
    }
    auto sys_ctx = t81::vm::internal::make_syscall_context(
        state_, state_.current_context, "t81vm", syscall, payload, prog_counter, opcode,
        instruction_count_, instruction_count_override);
    return axion_engine_->evaluate(sys_ctx);
  }

  const t81::tisc::EnumMetadata* enum_metadata_for(int enum_id) const {
    auto iter = state_.enum_metadata_index.find(enum_id);
    if (iter == state_.enum_metadata_index.end()) {
      return nullptr;
    }
    return &state_.enum_metadata[iter->second];
  }

  static const t81::tisc::EnumVariantMetadata* variant_metadata(const t81::tisc::EnumMetadata* meta,
                                                                int variant_id) {
    if (meta == nullptr) {
      return nullptr;
    }
    for (const auto& variant : meta->variants) {
      if (variant.variant_id == variant_id) {
        return &variant;
      }
    }
    return nullptr;
  }

  void log_memory_segment_access(t81::tisc::Opcode opcode, MemorySegmentKind kind, std::size_t addr,
                                 std::size_t size, std::string_view action) {
    t81::vm::internal::log_memory_segment_access(state_, state_.current_context, opcode, kind, addr,
                                                 size, action);
  }

  void log_bounds_fault(t81::tisc::Opcode opcode, MemorySegmentKind kind, int addr,
                        std::string_view action) {
    t81::vm::internal::log_bounds_fault(state_, state_.current_context, opcode, kind, addr, action);
  }

  void log_bounds_fault(t81::tisc::Opcode opcode, int addr, std::string_view action) {
    MemorySegmentKind kind = MemorySegmentKind::Unknown;
    if (addr >= 0) {
      kind = t81::vm::internal::segment_for_address(state_, static_cast<std::size_t>(addr));
    }
    log_bounds_fault(opcode, kind, addr, action);
  }

  void record_axion_event(t81::tisc::Opcode opcode, std::int32_t tag_val, std::int64_t val_data,
                          const t81::axion::Verdict& verdict) {
    t81::vm::internal::record_axion_event(state_, state_.current_context, opcode, tag_val, val_data,
                                          verdict);
  }

  Trap blocked_privileged_axion_opcode(t81::tisc::Opcode opcode) {
    t81::axion::Verdict verdict{t81::axion::VerdictKind::Deny,
                                "Blocked: unimplemented privileged Axion opcode"};
    record_axion_event(opcode, 0, 0, verdict);
    return Trap::SecurityFault;
  }

  std::optional<Trap> handle_axread(const t81::tisc::Insn& insn, ThreadContext& ctx,
                                    std::size_t current_pc) {
    const auto reg_ok = [&ctx](int r) {
      return r >= 0 && static_cast<std::size_t>(r) < ctx.registers.size();
    };
    if (!reg_ok(insn.a)) {
      return Trap::DecodeFault;
    }
    auto verdict = eval_axion_call(t81::axion::reasons::kAxRead, current_pc, insn.opcode);
    auto guard_addr = static_cast<std::size_t>(insn.b);
    auto guard_kind = t81::vm::internal::segment_for_address(state_, guard_addr);
    t81::vm::internal::apply_segment_reason(verdict, "AxRead guard", guard_kind, guard_addr);
    if (verdict.kind == t81::axion::VerdictKind::Deny) {
      record_axion_event(insn.opcode, insn.b, 0, verdict);
      return Trap::SecurityFault;
    }
    ctx.registers[insn.a] = insn.b;
    ctx.register_tags[insn.a] = ValueTag::Int;
    ctx.flags.zero = (ctx.registers[insn.a] == 0);
    ctx.flags.negative = (ctx.registers[insn.a] < 0);
    ctx.flags.positive = (ctx.registers[insn.a] > 0);
    record_axion_event(insn.opcode, insn.b, ctx.registers[insn.a], verdict);
    return std::nullopt;
  }

  std::optional<Trap> handle_axset(const t81::tisc::Insn& insn, ThreadContext& ctx,
                                   std::size_t current_pc) {
    const auto reg_ok = [&ctx](int r) {
      return r >= 0 && static_cast<std::size_t>(r) < ctx.registers.size();
    };
    if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
      return Trap::DecodeFault;
    }
    auto value = ctx.registers[insn.b];
    auto verdict = eval_axion_call(t81::axion::reasons::kAxSet, current_pc, insn.opcode);
    std::size_t guard_addr = 0;
    MemorySegmentKind guard_kind = MemorySegmentKind::Unknown;
    if (ctx.registers[insn.a] >= 0) {
      guard_addr = static_cast<std::size_t>(ctx.registers[insn.a]);
      guard_kind = t81::vm::internal::segment_for_address(state_, guard_addr);
    }
    t81::vm::internal::apply_segment_reason(verdict, "AxSet guard", guard_kind, guard_addr);
    record_axion_event(insn.opcode, insn.a, value, verdict);
    if (verdict.kind == t81::axion::VerdictKind::Deny) {
      return Trap::SecurityFault;
    }
    if (canonfs_driver_) {
      // Emit CanonFS write audit event before any backing store write.
      t81::vm::internal::log_canonfs_operation(state_, state_.current_context, insn.opcode,
                                               "Write");
    }
    return std::nullopt;
  }

  std::optional<Trap> handle_axverify(const t81::tisc::Insn& insn, ThreadContext& ctx,
                                      std::size_t current_pc) {
    const auto reg_ok = [&ctx](int r) {
      return r >= 0 && static_cast<std::size_t>(r) < ctx.registers.size();
    };
    if (!reg_ok(insn.a)) {
      return Trap::DecodeFault;
    }
    auto verdict = eval_axion_call(t81::axion::reasons::kAxVerify, current_pc, insn.opcode);
    if (verdict.kind == t81::axion::VerdictKind::Deny) {
      record_axion_event(insn.opcode, insn.b, 0, verdict);
      return Trap::SecurityFault;
    }
    ctx.registers[insn.a] = (verdict.kind == t81::axion::VerdictKind::Defer) ? 1 : 0;
    ctx.register_tags[insn.a] = ValueTag::Int;
    ctx.flags.zero = (ctx.registers[insn.a] == 0);
    ctx.flags.negative = (ctx.registers[insn.a] < 0);
    ctx.flags.positive = (ctx.registers[insn.a] > 0);
    record_axion_event(insn.opcode, insn.b, ctx.registers[insn.a], verdict);
    return std::nullopt;
  }

  std::optional<Trap> handle_axhalt(const t81::tisc::Insn& insn) {
    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Deny;
    // RFC-0000 §4: AXHALT carries an ethics violation reason in operand A.
    // A == 0 → generic halt; A == 1 → EthicsViolation (Θ-overlay breach);
    // A == 2 → CapabilityDenied (capability grant absent or revoked).
    const bool ethics_halt = (insn.a == 1);
    const bool cap_denied  = (insn.a == 2);
    verdict.reason = ethics_halt ? "AXHALT: EthicsViolation (Theta-overlay breach)"
                   : cap_denied  ? "AXHALT: CapabilityDenied (grant absent or revoked)"
                                 : "AXHALT instruction";
    record_axion_event(insn.opcode, 0, 0, verdict);
    state_.halted = true;
    if (ethics_halt) return Trap::EthicsViolation;
    if (cap_denied)  return Trap::CapabilityDenied;
    return std::nullopt;
  }

  std::optional<Trap> handle_ax_memory_opcode(const t81::tisc::Insn& insn, ThreadContext& ctx,
                                              std::size_t current_pc) {
    switch (insn.opcode) {
      case t81::tisc::Opcode::AxRead:
        return handle_axread(insn, ctx, current_pc);
      case t81::tisc::Opcode::AxSet:
        return handle_axset(insn, ctx, current_pc);
      case t81::tisc::Opcode::AxVerify:
        return handle_axverify(insn, ctx, current_pc);
      default:
        return Trap::DecodeFault;
    }
  }

  std::optional<Trap> handle_axion_opcode(
      const t81::tisc::Insn& insn, ThreadContext& ctx, std::size_t current_pc,
      const std::function<std::optional<std::string_view>(ValueTag, std::int64_t)>&
          symbol_like_text) {
    switch (insn.opcode) {
      case t81::tisc::Opcode::AxRead:
      case t81::tisc::Opcode::AxSet:
      case t81::tisc::Opcode::AxVerify:
        return handle_ax_memory_opcode(insn, ctx, current_pc);
      case t81::tisc::Opcode::AxCheck:
        return handle_axcheck(insn, ctx, current_pc, symbol_like_text);
      case t81::tisc::Opcode::AxReport:
        return handle_axreport(insn, ctx, current_pc, symbol_like_text);
      case t81::tisc::Opcode::AxSign:
      case t81::tisc::Opcode::AxLineage:
      case t81::tisc::Opcode::AxCanon:
        return blocked_privileged_axion_opcode(insn.opcode);
      case t81::tisc::Opcode::AxHalt:
        return handle_axhalt(insn);
      default:
        return Trap::DecodeFault;
    }
  }

  // Returns nullopt when opcode is not Axion, Trap::None when handled without fault,
  // and a non-None trap when Axion dispatch handled but faulted.
  std::optional<Trap> dispatch_axion_opcode_from_step(
      const t81::tisc::Insn& insn, ThreadContext& ctx, std::size_t current_pc,
      const std::function<std::optional<std::string_view>(ValueTag, std::int64_t)>&
          symbol_like_text) {
    switch (insn.opcode) {
      case t81::tisc::Opcode::AxRead:
      case t81::tisc::Opcode::AxSet:
      case t81::tisc::Opcode::AxVerify:
      case t81::tisc::Opcode::AxCheck:
      case t81::tisc::Opcode::AxReport:
      case t81::tisc::Opcode::AxSign:
      case t81::tisc::Opcode::AxLineage:
      case t81::tisc::Opcode::AxCanon:
      case t81::tisc::Opcode::AxHalt: {
        if (auto trap = handle_axion_opcode(insn, ctx, current_pc, symbol_like_text);
            trap.has_value()) {
          return *trap;
        }
        return Trap::None;
      }
      default:
        return std::nullopt;
    }
  }

  std::optional<Trap> handle_axcheck(
      const t81::tisc::Insn& insn, ThreadContext& ctx, std::size_t current_pc,
      const std::function<std::optional<std::string_view>(ValueTag, std::int64_t)>&
          symbol_like_text) {
    const auto reg_ok = [&ctx](int r) {
      return r >= 0 && static_cast<std::size_t>(r) < ctx.registers.size();
    };
    if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
      return Trap::DecodeFault;
    }

    const bool ok = ctx.registers[insn.a] != 0;
    auto msg = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
    std::string text = msg.has_value() ? std::string(*msg) : "Check";

    t81::axion::Verdict verdict;
    if (!ok) {
      verdict.kind = t81::axion::VerdictKind::Deny;
      verdict.reason = "AxCheck: " + text;
    } else {
      verdict = eval_axion_call(t81::axion::reasons::kAxCheck, current_pc, insn.opcode);
      if (verdict.kind == t81::axion::VerdictKind::Allow ||
          verdict.kind == t81::axion::VerdictKind::Warn) {
        verdict.reason = "AxCheck: " + text;
      }
    }

    record_axion_event(insn.opcode, 0, ok ? 1 : 0, verdict);
    if (verdict.kind == t81::axion::VerdictKind::Deny) {
      return Trap::SecurityFault;
    }
    return std::nullopt;
  }

  std::optional<Trap> handle_axreport(
      const t81::tisc::Insn& insn, ThreadContext& ctx, std::size_t current_pc,
      const std::function<std::optional<std::string_view>(ValueTag, std::int64_t)>&
          symbol_like_text) {
    const auto reg_ok = [&ctx](int r) {
      return r >= 0 && static_cast<std::size_t>(r) < ctx.registers.size();
    };
    if (!reg_ok(insn.a)) {
      return Trap::DecodeFault;
    }

    auto msg = symbol_like_text(ctx.register_tags[insn.a], ctx.registers[insn.a]);
    std::string text = msg.has_value() ? std::string(*msg) : "Report";

    t81::axion::Verdict verdict =
        eval_axion_call(t81::axion::reasons::kAxReport, current_pc, insn.opcode);
    if (verdict.kind == t81::axion::VerdictKind::Allow ||
        verdict.kind == t81::axion::VerdictKind::Warn) {
      verdict.reason = "AxReport: " + text;
    }

    record_axion_event(insn.opcode, 0, 0, verdict);
    if (verdict.kind == t81::axion::VerdictKind::Deny) {
      return Trap::SecurityFault;
    }
    return std::nullopt;
  }

  void run_gc_cycle_(const char* reason) {
    instructions_since_gc_ = 0;
    state_.gc_cycles++;
    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason_stream;
    // Format: 'GC cycle reason=[reason]'
    reason_stream << t81::axion::reasons::kGcCycle << " reason=" << reason;
    verdict.reason = reason_stream.str();
    record_axion_event(t81::tisc::Opcode::Trap, static_cast<std::int32_t>(state_.gc_cycles),
                       static_cast<std::int64_t>(state_.gc_cycles), verdict);

    auto reclaimed = t81::vm::internal::mark_and_sweep(state_);
    if (reclaimed.tensors > 0 || reclaimed.infinite_forms > 0) {
      t81::axion::Verdict reclaimed_verdict;
      reclaimed_verdict.kind = t81::axion::VerdictKind::Allow;
      std::ostringstream reclaimed_reason;
      reclaimed_reason << "GC reclaimed tensors=" << reclaimed.tensors
                       << " infinite_forms=" << reclaimed.infinite_forms;
      reclaimed_verdict.reason = reclaimed_reason.str();
      record_axion_event(t81::tisc::Opcode::Trap, 0,
                         static_cast<std::int64_t>(reclaimed.tensors + reclaimed.infinite_forms),
                         reclaimed_verdict);
    }

    log_heap_compaction(state_.heap_ptr, state_.heap_frames.size());
    log_heap_relocation(state_.heap_ptr, state_.layout.heap.start, state_.heap_frames.size());
    t81::vm::internal::compact_heap(state_, state_.layout.heap.start);
  }

  void log_heap_compaction(std::size_t heap_ptr, std::size_t heap_frames) {
    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason_stream;
    reason_stream << t81::axion::reasons::kHeapCompaction << " heap_frames=" << heap_frames
                  << " heap_ptr=" << heap_ptr;
    verdict.reason = reason_stream.str();
    record_axion_event(t81::tisc::Opcode::Trap, static_cast<std::int32_t>(MemorySegmentKind::Heap),
                       static_cast<std::int64_t>(heap_ptr), verdict);
  }

  void log_heap_relocation(std::size_t addr_from, std::size_t addr_to, std::size_t size) {
    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason_stream;
    reason_stream << t81::axion::reasons::kHeapRelocation << " from=" << addr_from
                  << " to=" << addr_to << " size=" << size;
    verdict.reason = reason_stream.str();
    record_axion_event(t81::tisc::Opcode::Trap, static_cast<std::int32_t>(MemorySegmentKind::Heap),
                       static_cast<std::int64_t>(addr_to), verdict);
  }

  struct TierTelemetry {
    std::size_t epoch_steps{0};
    std::size_t branch_events{0};
    std::size_t branch_taken{0};
    std::size_t symbolic_rewrites{0};
    std::size_t max_shape_complexity{0};
    std::size_t max_tensor_rank{0};
    std::size_t stable_simple_steps{0};
  };

  State state_{};
  t81::tisc::Program program_{};
  std::unique_ptr<t81::axion::Engine> axion_engine_;
  std::unique_ptr<t81::canonfs::Driver> canonfs_driver_;
  t81::axion::DeterminismDetector* determinism_detector_{nullptr};
  static constexpr std::size_t kGcInterval = 64;
  std::size_t instructions_since_gc_{0};
  std::size_t instruction_count_{0};

  // JIT components
  JitCompiler jit_compiler_;
  std::unordered_map<std::size_t, std::size_t> hot_spots_;
  std::unordered_map<std::size_t, std::unique_ptr<JitTrace>> compiled_traces_;
  std::vector<TierTelemetry> tier_telemetry_;
  static constexpr std::size_t kHotSpotThreshold = 50;
};
}  // namespace

std::unique_ptr<IVirtualMachine> make_interpreter_vm(std::unique_ptr<t81::axion::Engine> engine) {
  return std::make_unique<Interpreter>(std::move(engine));
}
}  // namespace t81::vm
