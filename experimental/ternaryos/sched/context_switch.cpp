// experimental/ternaryos/sched/context_switch.cpp

#include "context_switch.hpp"

namespace t81::ternaryos::sched {

void context_save(const t81::vm::ThreadContext& src, TiscContext& dst) {
  dst.registers     = src.registers;
  dst.register_tags = src.register_tags;
  dst.pc            = src.pc;
  dst.sp            = src.sp;
  dst.flags         = src.flags;
  dst.call_depth    = src.call_depth;
  dst.stack_frames  = src.stack_frames;
  dst.halted        = src.halted;
  dst.active        = src.active;
  dst.state         = ThreadState::Ready;
}

void context_restore(const TiscContext& src, t81::vm::ThreadContext& dst) {
  dst.registers     = src.registers;
  dst.register_tags = src.register_tags;
  dst.pc            = src.pc;
  dst.sp            = src.sp;
  dst.flags         = src.flags;
  dst.call_depth    = src.call_depth;
  dst.stack_frames  = src.stack_frames;
  dst.halted        = src.halted;
  dst.active        = src.active;
}

void context_yield(t81::vm::ThreadContext& current,
                   TiscContext&            out_ctx,
                   const TiscContext&      next_ctx) {
  context_save(current, out_ctx);
  context_restore(next_ctx, current);
}

}  // namespace t81::ternaryos::sched
