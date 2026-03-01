#include "internal/policy_trace_bridge.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include "t81/axion/reasons.hpp"

namespace t81::vm::internal {

t81::axion::SyscallContext make_syscall_context(const State& state, std::size_t current_context,
                                                std::string_view caller, std::string_view syscall,
                                                std::string_view payload, std::size_t pc,
                                                t81::tisc::Opcode opcode,
                                                std::size_t instruction_count,
                                                std::optional<std::size_t> instruction_override) {
  t81::axion::SyscallContext sys_ctx;
  sys_ctx.caller.assign(caller);
  sys_ctx.syscall.assign(syscall);
  sys_ctx.payload.assign(payload);
  sys_ctx.pc = pc;
  sys_ctx.next_opcode = opcode;
  sys_ctx.instruction_count = instruction_override.value_or(instruction_count);

  if (!state.contexts.empty() && current_context < state.contexts.size()) {
    const auto& tctx = state.contexts[current_context];
    sys_ctx.recursion_depth = std::max(tctx.stack_frames.size(), tctx.call_depth);
    sys_ctx.stack_usage = tctx.stack_base - tctx.sp;
    sys_ctx.current_tier = static_cast<int>(tctx.tier_status.current);
  } else {
    sys_ctx.recursion_depth = 0;
    sys_ctx.stack_usage = 0;
    sys_ctx.current_tier = 0;
  }

  sys_ctx.reflection_count = state.reflection_count;
  sys_ctx.meta_write_count = state.meta_write_count;
  sys_ctx.policy = state.policy ? &*state.policy : nullptr;
  sys_ctx.trace_reasons.reserve(state.axion_log.size());
  for (const auto& entry : state.axion_log) {
    sys_ctx.trace_reasons.push_back(entry.verdict.reason);
  }
  return sys_ctx;
}

std::string format_memory_access_reason(MemorySegmentKind kind, std::size_t addr, std::size_t size,
                                        std::string_view action) {
  std::ostringstream reason;
  reason << action << " " << to_string(kind) << " addr=" << addr;
  if (kind == MemorySegmentKind::Stack || action.find("allocated") != std::string_view::npos ||
      action.find("freed") != std::string_view::npos) {
    reason << " size=" << size;
  } else if (size > 1) {
    reason << " size=" << size;
  }
  return reason.str();
}

std::string format_bounds_fault_reason(MemorySegmentKind kind, int addr, std::string_view action) {
  return t81::axion::reasons::canonical_bounds_fault_reason(to_string(kind), addr, action);
}

std::string append_segment_reason(std::string_view action, MemorySegmentKind kind, std::size_t addr,
                                  std::string_view base_reason) {
  std::ostringstream reason;
  reason << action << " segment=" << to_string(kind) << " addr=" << addr;
  if (!base_reason.empty()) {
    reason << " " << base_reason;
  }
  return reason.str();
}

void apply_segment_reason(t81::axion::Verdict& verdict, std::string_view action,
                          MemorySegmentKind kind, std::size_t addr) {
  verdict.reason = append_segment_reason(action, kind, addr, verdict.reason);
}

void log_memory_segment_access(State& state, std::size_t current_context, t81::tisc::Opcode opcode,
                               MemorySegmentKind kind, std::size_t addr, std::size_t size,
                               std::string_view action) {
  t81::axion::Verdict verdict;
  verdict.kind = t81::axion::VerdictKind::Allow;
  verdict.reason = format_memory_access_reason(kind, addr, size, action);
  record_axion_event(state, current_context, opcode, static_cast<std::int32_t>(kind),
                     static_cast<std::int64_t>(addr), verdict);
}

void log_bounds_fault(State& state, std::size_t current_context, t81::tisc::Opcode opcode,
                      MemorySegmentKind kind, int addr, std::string_view action) {
  t81::axion::Verdict verdict;
  verdict.kind = t81::axion::VerdictKind::Allow;
  verdict.reason = format_bounds_fault_reason(kind, addr, action);
  record_axion_event(state, current_context, opcode, static_cast<std::int32_t>(kind),
                     static_cast<std::int64_t>(addr), verdict);
}

namespace {

void push_axion_event(State& state, const AxionEvent& event) {
  static const bool log_to_stderr = []() {
    if (const char* v = std::getenv("T81_VM_AXION_EVENT_STDERR")) {
      return std::strcmp(v, "0") != 0;
    }
    return false;
  }();
  if (log_to_stderr) {
    std::cerr << "[VM] push_axion_event: opcode=" << static_cast<int>(event.opcode) << " reason=\""
              << event.verdict.reason << "\"\n";
  }
  state.axion_log.push_back(event);
}

void log_meta_slot(State& state, const char* /*label*/) {
  if (!state.layout.meta.contains(state.meta_ptr)) {
    return;
  }
  AxionEvent meta_event;
  meta_event.opcode = t81::tisc::Opcode::Nop;
  meta_event.tag = static_cast<std::int32_t>(MemorySegmentKind::Meta);
  meta_event.value = static_cast<std::int64_t>(state.meta_ptr);
  meta_event.verdict.kind = t81::axion::VerdictKind::Allow;
  meta_event.verdict.reason = t81::axion::reasons::canonical_meta_slot_reason(state.meta_ptr);
  push_axion_event(state, meta_event);
  ++state.meta_ptr;
}

}  // namespace

void record_axion_event(State& state, std::size_t current_context, t81::tisc::Opcode opcode,
                        std::int32_t tag_val, std::int64_t val_data,
                        const t81::axion::Verdict& verdict) {
  log_meta_slot(state, t81::axion::reasons::kMetaSlotAxionEvent.data());
  AxionEvent event;
  event.opcode = opcode;
  event.tag = tag_val;
  event.value = val_data;
  event.verdict = verdict;
  event.structured.reason = verdict.reason;
  if (!state.contexts.empty() && current_context < state.contexts.size()) {
    event.structured.pc = state.contexts[current_context].pc;
  } else {
    event.structured.pc = 0;
  }
  event.structured.handle_id = val_data;
  if (verdict.kind == t81::axion::VerdictKind::Allow) {
    event.structured.decision = "allow";
  } else if (verdict.kind == t81::axion::VerdictKind::Warn) {
    event.structured.decision = "warn";
  } else if (verdict.kind == t81::axion::VerdictKind::Defer) {
    event.structured.decision = "defer";
  } else {
    event.structured.decision = "deny";
  }
  push_axion_event(state, event);
}

}  // namespace t81::vm::internal
