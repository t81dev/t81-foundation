#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "t81/axion/context.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/vm/state.hpp"

namespace t81::vm::internal {

t81::axion::SyscallContext make_syscall_context(const State& state, std::size_t current_context,
                                                std::string_view caller, std::string_view syscall,
                                                std::string_view payload, std::size_t pc,
                                                t81::tisc::Opcode opcode,
                                                std::size_t instruction_count,
                                                std::optional<std::size_t> instruction_override);

std::string format_memory_access_reason(MemorySegmentKind kind, std::size_t addr, std::size_t size,
                                        std::string_view action);

std::string format_bounds_fault_reason(MemorySegmentKind kind, int addr, std::string_view action);

std::string append_segment_reason(std::string_view action, MemorySegmentKind kind, std::size_t addr,
                                  std::string_view base_reason);

void apply_segment_reason(t81::axion::Verdict& verdict, std::string_view action,
                          MemorySegmentKind kind, std::size_t addr);

void log_memory_segment_access(State& state, std::size_t current_context, t81::tisc::Opcode opcode,
                               MemorySegmentKind kind, std::size_t addr, std::size_t size,
                               std::string_view action);

void log_bounds_fault(State& state, std::size_t current_context, t81::tisc::Opcode opcode,
                      MemorySegmentKind kind, int addr, std::string_view action);

/// Emit a canonical CanonFS meta-segment audit event (AX-M7).
/// Used by the AXSET handler to record a Write trace event when a CanonFS
/// driver is attached.  The reason string is produced by the AX-M6 canonical
/// builder: "meta slot axion event segment=meta addr=<meta_ptr> action=<action>".
void log_canonfs_operation(State& state, std::size_t current_context,
                           t81::tisc::Opcode opcode, std::string_view action);

void record_axion_event(State& state, std::size_t current_context, t81::tisc::Opcode opcode,
                        std::int32_t tag_val, std::int64_t val_data,
                        const t81::axion::Verdict& verdict);

}  // namespace t81::vm::internal
