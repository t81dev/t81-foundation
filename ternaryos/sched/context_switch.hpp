#pragma once

// experimental/ternaryos/sched/context_switch.hpp

#include "tisc_context.hpp"
#include "t81/vm/state.hpp"

namespace t81::ternaryos::sched {

void context_save(const t81::vm::ThreadContext& src, TiscContext& dst);
void context_restore(const TiscContext& src, t81::vm::ThreadContext& dst);
void context_yield(t81::vm::ThreadContext& current,
                   TiscContext&            out_ctx,
                   const TiscContext&      next_ctx);

}  // namespace t81::ternaryos::sched
