#pragma once

#include <cstddef>
#include <cstdint>

#include "t81/isa/program.hpp"
#include "t81/vm/state.hpp"

namespace t81::vm::internal {

std::int64_t compute_lineage_signature(const t81::tisc::Program& program);
std::int64_t compute_entropy_signature(std::size_t instruction_count,
                                       std::size_t contradiction_events, const ThreadContext& ctx);
std::int64_t compute_constitutional_mask(const State& state);

void sync_system_registers(State& state, const t81::tisc::Program& program,
                           std::size_t instruction_count, std::size_t current_context);

}  // namespace t81::vm::internal
