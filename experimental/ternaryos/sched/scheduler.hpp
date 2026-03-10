#pragma once

// experimental/ternaryos/sched/scheduler.hpp
//
// TernOS Phase 3 cooperative/pre-emptive scheduler.
//
// In Phase 3 tick() is called cooperatively (at yield points in TISC code or
// from fire_simulated_interrupt). Phase 4 will wire it to the real timer IRQ
// from interrupt_table.cpp, making it fully pre-emptive.
//
// Invariant: at most one thread is Running at any time. The scheduler keeps
// a live t81::vm::ThreadContext that mirrors the on-CPU thread; all other
// threads are snapshotted as TiscContext inside the RunQueue.

#include "run_queue.hpp"
#include "context_switch.hpp"
#include "t81/vm/state.hpp"

#include <optional>
#include <string>

namespace t81::ternaryos::sched {

class Scheduler {
public:
  // ── Lifecycle ─────────────────────────────────────────────────────────────

  Scheduler() = default;

  /// Spawn a new thread from a pre-initialized TiscContext.
  /// The context is added to the RunQueue in Ready state and assigned a Tid.
  /// Returns the assigned Tid, or nullopt if the queue is full.
  std::optional<Tid> spawn(TiscContext ctx);

  /// Remove a thread. If it is the current Running thread the scheduler
  /// immediately ticks to the next Ready thread.
  bool terminate(Tid tid);

  // ── Scheduling ────────────────────────────────────────────────────────────

  /// Cooperative tick: save current thread, select next Ready thread,
  /// restore it into cpu_ctx.
  ///
  /// @param cpu_ctx  The live ThreadContext currently executing on the CPU.
  ///                 On return it contains the state of the next thread.
  /// @return true if a switch occurred; false if no other Ready thread exists
  ///         (cpu_ctx is unchanged in that case).
  bool tick(t81::vm::ThreadContext& cpu_ctx);

  /// Put a thread to sleep (Running or Ready → Sleeping).
  /// If the sleeping thread is currently Running, tick() is called to hand
  /// the CPU to the next Ready thread.
  bool sleep(Tid tid, t81::vm::ThreadContext& cpu_ctx);

  /// Wake a sleeping thread (Sleeping → Ready).
  bool wake(Tid tid);

  // ── Introspection ─────────────────────────────────────────────────────────

  const RunQueue& run_queue() const noexcept { return rq_; }
  std::size_t     thread_count() const noexcept { return rq_.size(); }

  /// Tid of the thread currently on-CPU, or 0 if none.
  Tid current_tid() const noexcept {
    const TiscContext* c = rq_.current();
    return c ? c->tid : 0;
  }

  std::string stats_string() const { return rq_.stats_string(); }

private:
  RunQueue rq_;
};

}  // namespace t81::ternaryos::sched
