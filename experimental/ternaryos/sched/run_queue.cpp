// experimental/ternaryos/sched/run_queue.cpp

#include "run_queue.hpp"

#include <algorithm>
#include <sstream>

namespace t81::ternaryos::sched {

// ─── Thread lifecycle ─────────────────────────────────────────────────────────

std::optional<Tid> RunQueue::add_thread(TiscContext ctx) {
  if (full()) return std::nullopt;
  ctx.tid   = next_tid_++;
  ctx.state = ThreadState::Ready;
  slots_.push_back(std::move(ctx));
  return slots_.back().tid;
}

bool RunQueue::remove_thread(Tid tid) {
  auto it = std::find_if(slots_.begin(), slots_.end(),
                         [tid](const TiscContext& c) { return c.tid == tid; });
  if (it == slots_.end()) return false;

  // Adjust round-robin cursor if needed.
  std::size_t idx = static_cast<std::size_t>(it - slots_.begin());
  slots_.erase(it);
  if (rr_index_ > 0 && rr_index_ >= slots_.size()) rr_index_ = 0;
  else if (idx < rr_index_) --rr_index_;
  return true;
}

// ─── Scheduling ──────────────────────────────────────────────────────────────

TiscContext* RunQueue::next_ready() {
  if (slots_.empty()) return nullptr;

  const std::size_t n = slots_.size();
  for (std::size_t i = 0; i < n; ++i) {
    std::size_t idx = (rr_index_ + i) % n;
    if (slots_[idx].state == ThreadState::Ready) {
      rr_index_ = (idx + 1) % n;  // advance past this slot for next call
      return &slots_[idx];
    }
  }
  return nullptr;
}

bool RunQueue::set_running(Tid tid) {
  TiscContext* ctx = find(tid);
  if (!ctx) return false;
  ctx->state = ThreadState::Running;
  return true;
}

bool RunQueue::preempt_running() {
  TiscContext* ctx = current();
  if (!ctx) return false;
  ctx->state = ThreadState::Ready;
  return true;
}

bool RunQueue::sleep_thread(Tid tid) {
  TiscContext* ctx = find(tid);
  if (!ctx) return false;
  ctx->state = ThreadState::Sleeping;
  return true;
}

bool RunQueue::wake_thread(Tid tid) {
  TiscContext* ctx = find(tid);
  if (!ctx || ctx->state != ThreadState::Sleeping) return false;
  ctx->state = ThreadState::Ready;
  return true;
}

// ─── Introspection ────────────────────────────────────────────────────────────

std::size_t RunQueue::ready_count() const noexcept {
  return static_cast<std::size_t>(
      std::count_if(slots_.begin(), slots_.end(),
                    [](const TiscContext& c) { return c.state == ThreadState::Ready; }));
}

std::size_t RunQueue::running_count() const noexcept {
  return static_cast<std::size_t>(
      std::count_if(slots_.begin(), slots_.end(),
                    [](const TiscContext& c) { return c.state == ThreadState::Running; }));
}

std::size_t RunQueue::sleeping_count() const noexcept {
  return static_cast<std::size_t>(
      std::count_if(slots_.begin(), slots_.end(),
                    [](const TiscContext& c) { return c.state == ThreadState::Sleeping; }));
}

TiscContext* RunQueue::current() noexcept {
  for (auto& c : slots_)
    if (c.state == ThreadState::Running) return &c;
  return nullptr;
}

const TiscContext* RunQueue::current() const noexcept {
  for (const auto& c : slots_)
    if (c.state == ThreadState::Running) return &c;
  return nullptr;
}

TiscContext* RunQueue::find(Tid tid) noexcept {
  for (auto& c : slots_)
    if (c.tid == tid) return &c;
  return nullptr;
}

const TiscContext* RunQueue::find(Tid tid) const noexcept {
  for (const auto& c : slots_)
    if (c.tid == tid) return &c;
  return nullptr;
}

std::string RunQueue::stats_string() const {
  std::ostringstream ss;
  ss << "RunQueue: " << size() << "/" << kMaxSlots << " slots"
     << " (ready=" << ready_count()
     << " running=" << running_count()
     << " sleeping=" << sleeping_count() << ")";
  return ss.str();
}

}  // namespace t81::ternaryos::sched
