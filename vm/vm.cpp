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
#include "t81/tensor/lattice_crypto.hpp"
#include "t81/tensor/llama.hpp"
#include "t81/tensor/matmul.hpp"
#include "t81/tensor/ternary_native.hpp"

#include "internal/ffi_bridge.hpp"
#include "internal/gc_helpers.hpp"
#include "internal/memory_segments.hpp"
#include "internal/policy_trace_bridge.hpp"
#include "internal/runtime_state_helpers.hpp"
#include "internal/tensor_helpers.hpp"
#include "internal/tier_limits.hpp"
#include "internal/value_ops.hpp"
#include "internal/vm_support.hpp"
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

namespace t81::vm::csi {
#ifdef T81_CSI_EXPERIMENTAL
extern "C" Trap execute_csi_opcode_if_enabled(const t81::tisc::Insn& insn, ThreadContext& ctx);
#else
inline Trap execute_csi_opcode_if_enabled(const t81::tisc::Insn&, ThreadContext&) {
  return Trap::DecodeFault;
}
#endif
}  // namespace t81::vm::csi

namespace t81::vm::advanced_ai {
#if defined(T81_BUILD_ADVANCED_AI) || defined(T81_ADVANCED_AI_EXPERIMENTAL)
extern "C" Trap execute_advanced_ai_opcode_if_enabled(const t81::tisc::Insn& insn,
                                                      ThreadContext& ctx);
#else
inline Trap execute_advanced_ai_opcode_if_enabled(const t81::tisc::Insn&, ThreadContext&) {
  return Trap::DecodeFault;
}
#endif
}  // namespace t81::vm::advanced_ai

namespace t81::vm {
namespace {
constexpr std::size_t kDefaultStackSize = 256;
constexpr std::size_t kDefaultHeapSize = 768;
constexpr std::size_t kDefaultTensorSpace = 256;
constexpr std::size_t kDefaultMetaSpace = 256;
constexpr std::size_t kHardRecursionCeiling = T81_HARD_RECURSION_CEILING;

using t81::vm::internal::fraction_from_double;
using t81::vm::internal::hash_matches_policy_entry;
using t81::vm::internal::is_exact_ternary_tensor;
using t81::vm::internal::make_deny_with_reason_engine;
using t81::vm::internal::max_branch_entropy_for_tier;
using t81::vm::internal::max_shape_complexity_for_tier;
using t81::vm::internal::max_symbolic_complexity_for_tier;
using t81::vm::internal::max_tensor_rank_for_tier;
using t81::vm::internal::recursion_limit_for_tier;
using t81::vm::internal::tier_from_rank;
using t81::vm::internal::tier_rank;
using t81::vm::internal::TierPromotionError;
using t81::vm::internal::TierPromotionResult;
using t81::vm::internal::try_promote_tier;

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
    tensor_canon_hashes_.clear();
    state_.tensors.reserve(program_.tensor_pool.size());
    for (const auto& t : program_.tensor_pool) {
      state_.tensors.push_back(t);
    }
    state_.free_tensor_indices.clear();
    state_.shapes = program_.shape_pool;
    state_.weights_model = program_.weights_model;
    state_.weights_tensor_refs.clear();
    state_.weights_tensor_handles.clear();
    state_.weights_tensor_trits.clear();
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
        event.structured.reason = parse_error;
        event.structured.decision = "deny";
        event.structured.event_type = "policy_parse_failure";
        event.structured.reason_code = "AXION_POLICY_PARSE_FAILED";
        event.structured.pc = 0;
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
            std::string_view payload;
            if (insn.opcode == t81::tisc::Opcode::WeightsLoad && insn.b > 0 &&
                static_cast<std::size_t>(insn.b) <= state_.symbols.size()) {
              payload = state_.symbols[static_cast<std::size_t>(insn.b - 1)];
              if (payload.empty() && state_.policy.has_value()) {
                t81::axion::Verdict denied{t81::axion::VerdictKind::Deny,
                                           "WeightsLoad policy_violation name="};
                record_axion_event(insn.opcode, insn.b, 0, denied);
                return false;
              }
            }
            const auto verdict =
                eval_axion_call(t81::axion::reasons::kStep, pc, insn.opcode, payload,
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
          case JitTrace::ExitKind::AxionBoundary:
            // RFC-0028 §5: Axion-gated opcode (AxRead/AxSet/AxVerify/AxReport).
            // The trace exits cleanly; the interpreter resumes and evaluates
            // the Axion boundary natively via policy_engine.cpp.
            reason << "axion-boundary";
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

      // RFC-0028 §5: AxionBoundary — the trace stopped before an Axion opcode.
      // Resume interpreter at the current PC (already set by the OSR bailout);
      // the Axion opcode will be evaluated natively on the next step() iteration.
      // Do NOT invalidate the trace — it is still valid for re-entry after the
      // Axion boundary is resolved.
      if (exec_result.exit_kind == JitTrace::ExitKind::AxionBoundary) {
        return {};
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
    auto step_verdict = eval_axion_call(t81::axion::reasons::kStep, current_pc, insn.opcode);
    if (step_verdict.kind == t81::axion::VerdictKind::Deny) {
      // Preserve deny reason visibility for pre-dispatch policy failures.
      record_axion_event(insn.opcode, 0, 0, step_verdict);
      return std::expected<void, Trap>(t81::unexpect, Trap::SecurityFault);
    }
    if (step_verdict.kind == t81::axion::VerdictKind::Warn) {
      // Log the warning event
      record_axion_event(insn.opcode, 0, 0, step_verdict);
    }

    auto reg_ok = [&ctx](std::int64_t r) {
      return r >= 0 && static_cast<std::size_t>(r) < ctx.registers.size();
    };
    auto mem_ok = [this](std::int64_t addr, bool code = false) {
      return t81::vm::internal::mem_ok(state_, addr, code);
    };
    auto check_mem = [this, &mem_ok](t81::tisc::Opcode opcode, std::int64_t addr,
                                     std::string_view action, bool code = false) -> bool {
      if (mem_ok(addr, code)) return true;
      this->log_bounds_fault(opcode, static_cast<int>(addr), action);
      return false;
    };
    bool restore_pc_on_trap = false;
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
    auto authorize_weights_tensor =
        [&](int source_reg,
            std::int64_t source_handle) -> std::expected<const t81::weights::NativeTensor*, Trap> {
      const auto* source_native = weights_tensor(source_handle);
      if (!source_native) {
        return std::expected<const t81::weights::NativeTensor*, Trap>(t81::unexpect,
                                                                      Trap::DecodeFault);
      }
      if (state_.policy.has_value()) {
        if (!state_.policy->allowed_ternary_model_hashes.empty()) {
          const bool hash_allowed = state_.weights_model != nullptr &&
                                    !state_.weights_model->checksum.empty() &&
                                    std::any_of(state_.policy->allowed_ternary_model_hashes.begin(),
                                                state_.policy->allowed_ternary_model_hashes.end(),
                                                [&](const std::string& allowed) {
                                                  return hash_matches_policy_entry(
                                                      allowed, state_.weights_model->checksum);
                                                });
          if (!hash_allowed) {
            const std::string checksum =
                state_.weights_model != nullptr ? state_.weights_model->checksum : "";
            t81::axion::Verdict verdict{
                t81::axion::VerdictKind::Deny,
                checksum.empty()
                    ? "WLOAD denied (allowed-ternary-model-hashes: missing model checksum)"
                    : "WLOAD denied (allowed-ternary-model-hashes mismatch checksum=sha3-512:" +
                          checksum + ")"};
            record_axion_event(program_.insns[current_pc].opcode, static_cast<int32_t>(source_reg),
                               source_handle, verdict);
            return std::expected<const t81::weights::NativeTensor*, Trap>(t81::unexpect,
                                                                          Trap::SecurityFault);
          }
        }
        if (state_.policy->ternary_weight_domain_check) {
          auto decoded = t81::vm::internal::decode_native_tensor(
              *source_native, t81::vm::internal::TensorDecodeMode::StrictCanonical);
          if (!decoded.has_value() || !is_exact_ternary_tensor(*decoded)) {
            t81::axion::Verdict verdict{t81::axion::VerdictKind::Deny,
                                        "WLOAD denied (ternary-weight-domain-check failed)"};
            record_axion_event(program_.insns[current_pc].opcode, static_cast<int32_t>(source_reg),
                               source_handle, verdict);
            return std::expected<const t81::weights::NativeTensor*, Trap>(t81::unexpect,
                                                                          Trap::SecurityFault);
          }
        }
      }
      return source_native;
    };
    auto promote_to_tensor = [&](int reg) -> std::expected<void, Trap> {
      if (reg < 0 || static_cast<std::size_t>(reg) >= ctx.registers.size()) {
        return std::expected<void, Trap>(t81::unexpect, Trap::DecodeFault);
      }
      if (ctx.register_tags[reg] == ValueTag::WeightsTensorHandle) {
        auto handle = ctx.registers[reg];
        auto authorized = authorize_weights_tensor(reg, handle);
        if (!authorized.has_value()) {
          return std::expected<void, Trap>(t81::unexpect, authorized.error());
        }
        auto promoted = t81::vm::internal::decode_native_tensor(
            **authorized, t81::vm::internal::TensorDecodeMode::StrictCanonical);
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
    auto bigint_from_integer_like = [&](ValueTag tag,
                                        std::int64_t value) -> std::optional<t81::T81BigInt> {
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
    auto store_integer_result = [&](int reg, t81::T81BigInt value,
                                    bool force_bigint = false) -> std::optional<Trap> {
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
    auto store_bigint_materialized = [&](int reg, t81::T81BigInt value) -> std::optional<Trap> {
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
        case ValueTag::IntVectorHandle:
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
        case ValueTag::IntVectorHandle:
          return "<intvec#" + std::to_string(val_data) + ">";
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

    auto decode_ai_packed_reg_pair =
        [&](std::int32_t packed) -> std::optional<std::pair<int, int>> {
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
            if (insn.b <= 0 || static_cast<std::size_t>(insn.b) > program_.bigint_pool.size()) {
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
          log_memory_segment_access(insn.opcode,
                                    t81::vm::internal::segment_for_address(state_, addr), addr, 1,
                                    t81::axion::reasons::kMemLoad);
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
          if (state_.policy.has_value() && name.empty()) {
            t81::axion::Verdict denied{t81::axion::VerdictKind::Deny,
                                       "WeightsLoad policy_violation name="};
            record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), 0, denied);
            trap = Trap::SecurityFault;
            break;
          }
          auto verdict = eval_axion_call(t81::axion::reasons::kStep, current_pc, insn.opcode, name);
          if (verdict.kind == t81::axion::VerdictKind::Deny) {
            record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), 0, verdict);
            trap = Trap::SecurityFault;
            break;
          }
          auto handle = intern_weights_tensor(name);
          ctx.registers[insn.a] = handle;
          ctx.register_tags[insn.a] = ValueTag::WeightsTensorHandle;
          verdict.kind = t81::axion::VerdictKind::Allow;
          verdict.reason = "weights.load \"" + name + "\"";
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), handle, verdict);
          break;
        }
        // Tensor hash loading, metadata reflection, and LLM math opcodes (TLoadHash..TRoPE)
#include "internal/vm_llm_math_opcodes.inc"
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
          log_memory_segment_access(insn.opcode,
                                    t81::vm::internal::segment_for_address(state_, addr), addr, 1,
                                    t81::axion::reasons::kMemStore);
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
            if ((lhs->is_negative() && !rhs->is_negative()) ||
                (!lhs->is_negative() && rhs->is_negative())) {
              result = -q;
            } else {
              result = q;
            }
          } else {
            // MOD definition: a = (a/b)*b + r
            auto q = lhs->abs() / rhs->abs();
            t81::T81BigInt trunc_div;
            if ((lhs->is_negative() && !rhs->is_negative()) ||
                (!lhs->is_negative() && rhs->is_negative())) {
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
          if (auto value =
                  bigint_from_integer_like(ctx.register_tags[insn.b], ctx.registers[insn.b]);
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
          const bool integer_mixed =
              is_integer_like_tag(normalized_b) && is_integer_like_tag(normalized_c);
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
            relation_opt =
                compare_value(normalized_b, ctx.registers[insn.b], ctx.registers[insn.c]);
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
          heap_bytes_since_gc_ += size;  // RFC-0006 §2.3: byte-threshold accounting
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
            log_bounds_fault(insn.opcode, MemorySegmentKind::Heap,
                             static_cast<int>(state_.heap_ptr), "heap block free");
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
            record_tier_fault("recursion-limit", reason.str(),
                              static_cast<std::int64_t>(next_depth));
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
        // String and vector intrinsics (StrLen..StrJoin)
#include "internal/vm_string_vec_opcodes.inc"
        // Numeric conversion, floating-point, transcendental, and exact fraction opcodes
        // (I2F..FracDiv)
#include "internal/vm_numeric_opcodes.inc"
        // Vector, runtime, ADT (Option/Result/Enum/Complex), SWAR, and core tensor opcodes
        // (ChkShape..Assert)
#include "internal/vm_tensor_adt_opcodes.inc"
        // Cognitive Tier 1-5 symbolic, reflection, infinite, and distributed opcodes
        // (SymLoad..DistSeal)
#include "internal/vm_cognitive_opcodes.inc"
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
          auto computed =
              t81::vm::internal::tensor_attention_checked(*tensor_q, *tensor_k, *tensor_v);
          if (!computed.has_value()) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, 0, "ATTN shape mismatch");
            trap = computed.error();
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                      "ATTN kernel execution (phase1 packed operand encoding)"};
          record_axion_event(insn.opcode, static_cast<int32_t>(q_reg), ctx.registers[q_reg],
                             verdict);
          auto res_handle = alloc_tensor(std::move(*computed));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        // RFC-0026 / RFC-0034 AI, quantization, and ternary-weight opcodes
#include "internal/vm_ai_opcodes.inc"
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
    if (instructions_since_gc_ >= kGcInterval || heap_bytes_since_gc_ >= kGcThreshold) {
      // RFC-0006 §2.3: fire on instruction-count interval OR byte threshold.
      if (auto gc_trap =
              run_gc_cycle_(heap_bytes_since_gc_ >= kGcThreshold ? "byte-threshold" : "interval");
          gc_trap.has_value()) {
        return t81::unexpected(gc_trap.value());
      }
    }

    t81::vm::internal::sync_system_registers(state_, program_, instruction_count_,
                                             state_.current_context);
    if (trap != Trap::None && restore_pc_on_trap) {
      ctx.pc = current_pc;
    }
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
    canonfs_driver_ =
        std::shared_ptr<t81::canonfs::Driver>(t81::canonfs::make_persistent_driver(root).release());
  }

  void set_canonfs_driver(std::shared_ptr<t81::canonfs::Driver> driver) override {
    canonfs_driver_ = std::move(driver);
  }

  void initialize_ffi_subsystem(t81::axion::Engine& policy_engine) override {
    t81::vm::initialize_ffi_subsystem(policy_engine);
  }

  ffi::FFIDispatcher* get_ffi_dispatcher() const override { return t81::vm::get_ffi_dispatcher(); }

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
    state_.memory[word_index] = value;
    state_.memory_tags[word_index] = ValueTag::Int;
  }

  void set_memory_word_tagged(std::size_t word_index, std::int64_t value,
                              ValueTag tag) noexcept override {
    if (word_index >= state_.memory.size()) return;
    state_.memory[word_index] = value;
    state_.memory_tags[word_index] = tag;
  }

  std::int64_t intern_float(double value) noexcept override {
    state_.floats.push_back(value);
    return static_cast<std::int64_t>(state_.floats.size());  // 1-based handle
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
    if (auto decoded = t81::vm::internal::decode_balanced_ternary_trits(native_iter->second);
        decoded.has_value()) {
      state_.weights_tensor_trits.push_back(std::move(*decoded));
    } else {
      state_.weights_tensor_trits.push_back(std::nullopt);
    }
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
    const auto reg_ok = [&ctx](std::int64_t r) {
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
    const auto reg_ok = [&ctx](std::int64_t r) {
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
    const auto reg_ok = [&ctx](std::int64_t r) {
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
    const bool cap_denied = (insn.a == 2);
    verdict.reason = ethics_halt  ? "AXHALT: EthicsViolation (Theta-overlay breach)"
                     : cap_denied ? "AXHALT: CapabilityDenied (grant absent or revoked)"
                                  : "AXHALT instruction";
    record_axion_event(insn.opcode, 0, 0, verdict);
    state_.halted = true;
    if (ethics_halt) return Trap::EthicsViolation;
    if (cap_denied) return Trap::CapabilityDenied;
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
    const auto reg_ok = [&ctx](std::int64_t r) {
      return r >= 0 && static_cast<std::size_t>(r) < ctx.registers.size();
    };
    if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
      return Trap::DecodeFault;
    }

    const bool ok = ctx.registers[static_cast<std::size_t>(insn.a)] != 0;
    auto msg = symbol_like_text(ctx.register_tags[static_cast<std::size_t>(insn.b)],
                                ctx.registers[static_cast<std::size_t>(insn.b)]);
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
    const auto reg_ok = [&ctx](std::int64_t r) {
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

  // RFC-0006 §2.3+§2.4 — Deterministic GC cycle.
  // Returns a Trap if Axion policy vetoes the cycle (§2.4 policy veto path).
  std::optional<Trap> run_gc_cycle_(const char* reason) {
    const std::size_t heap_bytes_snapshot = heap_bytes_since_gc_;
    instructions_since_gc_ = 0;
    heap_bytes_since_gc_ = 0;

    // §2.4 Axion policy veto: evaluate before running the cycle.
    // A Deny verdict halts execution with SecurityFault (GcFault path).
    {
      std::ostringstream payload;
      payload << "reason=" << reason << " heap_bytes=" << heap_bytes_snapshot
              << " gc_cycle=" << (state_.gc_cycles + 1);
      auto veto =
          eval_axion_call(t81::axion::reasons::kGcCycle, static_cast<std::size_t>(state_.gc_cycles),
                          t81::tisc::Opcode::GcSafepoint, payload.str());
      if (veto.kind == t81::axion::VerdictKind::Deny) {
        return Trap::SecurityFault;
      }
    }

    state_.gc_cycles++;
    t81::axion::Verdict verdict;
    verdict.kind = t81::axion::VerdictKind::Allow;
    std::ostringstream reason_stream;
    reason_stream << t81::axion::reasons::kGcCycle << " reason=" << reason;
    verdict.reason = reason_stream.str();
    record_axion_event(t81::tisc::Opcode::GcSafepoint, static_cast<std::int32_t>(state_.gc_cycles),
                       static_cast<std::int64_t>(state_.gc_cycles), verdict);

    auto reclaimed = t81::vm::internal::mark_and_sweep(state_);
    if (reclaimed.tensors > 0) {
      for (std::size_t free_idx : state_.free_tensor_indices) {
        tensor_canon_hashes_.erase(static_cast<std::int64_t>(free_idx + 1));
      }
    }
    if (reclaimed.tensors > 0 || reclaimed.infinite_forms > 0) {
      t81::axion::Verdict reclaimed_verdict;
      reclaimed_verdict.kind = t81::axion::VerdictKind::Allow;
      std::ostringstream reclaimed_reason;
      reclaimed_reason << "GC reclaimed tensors=" << reclaimed.tensors
                       << " infinite_forms=" << reclaimed.infinite_forms;
      reclaimed_verdict.reason = reclaimed_reason.str();
      record_axion_event(t81::tisc::Opcode::GcSafepoint, 0,
                         static_cast<std::int64_t>(reclaimed.tensors + reclaimed.infinite_forms),
                         reclaimed_verdict);
    }

    log_heap_compaction(state_.heap_ptr, state_.heap_frames.size());
    log_heap_relocation(state_.heap_ptr, state_.layout.heap.start, state_.heap_frames.size());
    t81::vm::internal::compact_heap(state_, state_.layout.heap.start);
    return std::nullopt;
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
  std::shared_ptr<t81::canonfs::Driver> canonfs_driver_;
  std::unordered_map<std::int64_t, std::string> tensor_canon_hashes_;
  t81::axion::DeterminismDetector* determinism_detector_{nullptr};
  static constexpr std::size_t kGcInterval = 64;
  /// RFC-0006 §2.3: canonical byte threshold = 3^12 = 531441 bytes.
  static constexpr std::size_t kGcThreshold = 531441;
  std::size_t instructions_since_gc_{0};
  std::size_t heap_bytes_since_gc_{0};
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
