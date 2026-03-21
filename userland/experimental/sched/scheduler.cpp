// experimental/ternaryos/sched/scheduler.cpp

#include "scheduler.hpp"

namespace t81::ternaryos::sched {

std::optional<Tid> Scheduler::spawn(TiscContext ctx) {
  return rq_.add_thread(std::move(ctx));
}

bool Scheduler::terminate(Tid tid) {
  // If we're terminating the currently running thread, we need a context
  // to switch to — but terminate() doesn't take cpu_ctx. Phase 3 callers
  // must call tick() before terminate() if removing the running thread.
  return rq_.remove_thread(tid);
}

bool Scheduler::tick(t81::vm::ThreadContext& cpu_ctx) {
  // 1. Save the current on-CPU thread into its RunQueue slot.
  TiscContext* cur = rq_.current();
  if (cur) {
    context_save(cpu_ctx, *cur);
    rq_.preempt_running();  // current → Ready
  }

  // 2. Pick the next Ready thread.
  TiscContext* nxt = rq_.next_ready();
  if (!nxt || (cur && nxt->tid == cur->tid)) {
    // No other thread available (or only one thread exists) — keep running.
    if (cur) {
      rq_.set_running(cur->tid);
      context_restore(*cur, cpu_ctx);
    }
    return false;
  }

  // 3. Restore it onto the CPU.
  rq_.set_running(nxt->tid);
  context_restore(*nxt, cpu_ctx);
  return true;
}

bool Scheduler::sleep(Tid tid, t81::vm::ThreadContext& cpu_ctx) {
  TiscContext* ctx = rq_.find(tid);
  if (!ctx) return false;

  bool was_running = (ctx->state == ThreadState::Running);
  rq_.sleep_thread(tid);

  // If we just slept the on-CPU thread, hand the CPU to the next Ready one.
  if (was_running) tick(cpu_ctx);
  return true;
}

bool Scheduler::wake(Tid tid) {
  return rq_.wake_thread(tid);
}

}  // namespace t81::ternaryos::sched
