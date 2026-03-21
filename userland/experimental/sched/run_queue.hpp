#pragma once

// experimental/ternaryos/sched/run_queue.hpp
//
// Pre-emptive round-robin run queue for TernOS Phase 3.
//
// Maintains up to kMaxSlots = 81 TiscContext slots (mirroring the Hanoi
// 81-slot scheduler cap). Threads move through three balanced-ternary states:
//   Sleeping (-1) — blocked; excluded from the round-robin cycle
//   Ready    ( 0) — eligible; returned by next_ready() in FIFO order
//   Running  (+1) — on-CPU; at most one slot is Running at a time

#include "tisc_context.hpp"

#include <optional>
#include <string>
#include <vector>

namespace t81::ternaryos::sched {

/// Maximum concurrent threads: 3^4 = 81.
inline constexpr std::size_t kMaxSlots = 81;

enum class RunQueueError { Full, NotFound, AlreadyRunning };

class RunQueue {
public:
  // ── Thread lifecycle ──────────────────────────────────────────────────────

  /// Add a new thread. Assigns and returns its Tid (1-based).
  /// Returns nullopt if the queue is at capacity (kMaxSlots).
  std::optional<Tid> add_thread(TiscContext ctx);

  /// Remove a thread by Tid. Returns false if not found.
  bool remove_thread(Tid tid);

  // ── Scheduling ────────────────────────────────────────────────────────────

  /// Return a pointer to the next Ready thread using round-robin order,
  /// or nullptr if no Ready thread exists.
  /// Does NOT change any state — the caller (Scheduler) drives transitions.
  TiscContext* next_ready();

  /// Mark a thread Running (transition from Ready or Sleeping).
  /// Returns false if the Tid is not found.
  bool set_running(Tid tid);

  /// Move the currently Running thread back to Ready.
  /// Returns false if no thread is currently Running.
  bool preempt_running();

  /// Put a thread to sleep (Running or Ready → Sleeping).
  bool sleep_thread(Tid tid);

  /// Wake a sleeping thread (Sleeping → Ready).
  bool wake_thread(Tid tid);

  // ── Introspection ─────────────────────────────────────────────────────────

  std::size_t size()           const noexcept { return slots_.size(); }
  bool        full()           const noexcept { return slots_.size() >= kMaxSlots; }

  std::size_t ready_count()    const noexcept;
  std::size_t running_count()  const noexcept;
  std::size_t sleeping_count() const noexcept;

  /// Pointer to the currently Running thread, or nullptr.
  TiscContext*       current() noexcept;
  const TiscContext* current() const noexcept;

  /// Find a thread by Tid (mutable).
  TiscContext*       find(Tid tid) noexcept;
  const TiscContext* find(Tid tid) const noexcept;

  std::string stats_string() const;

private:
  std::vector<TiscContext> slots_;
  std::size_t              rr_index_{0};  // round-robin cursor into slots_
  Tid                      next_tid_{1};
};

}  // namespace t81::ternaryos::sched
