#pragma once

// experimental/ternaryos/ipc/canon_message.hpp
//
// CanonRef-based IPC for TernOS Phase 3.
//
// Processes exchange CanonRef handles (content-addressed object identities)
// rather than raw pointers, preserving determinism and Axion audit trails.
// Each thread has a bounded message queue (kMaxQueueDepth = 81).

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>

#include "t81/canonfs/canon_types.hpp"
#include "../sched/tisc_context.hpp"  // Tid

namespace t81::ternaryos::ipc {

/// Maximum pending messages per thread inbox: 3^4 = 81.
inline constexpr std::size_t kMaxQueueDepth = 81;

// ─── Message ─────────────────────────────────────────────────────────────────

/// A single IPC message: a CanonRef handle + optional 64-bit scalar payload.
struct CanonMessage {
  sched::Tid          sender{0};       ///< Sending thread's Tid (0 = kernel)
  t81::canonfs::CanonRef ref;          ///< Content-addressed object handle
  uint64_t            payload{0};      ///< Inline scalar (tag, size hint, etc.)
  std::string         tag;             ///< Human-readable message kind label

  bool operator==(const CanonMessage& o) const noexcept {
    return sender == o.sender &&
           ref.hash == o.ref.hash &&
           payload  == o.payload &&
           tag      == o.tag;
  }
};

// ─── Message Bus ─────────────────────────────────────────────────────────────

/**
 * @brief Per-system message bus: one inbox deque per Tid.
 *
 * ipc_send enqueues to the destination inbox.
 * ipc_recv pops from the caller's inbox (FIFO).
 *
 * Thread safety: not thread-safe in Phase 3 (single-core cooperative model).
 */
class MessageBus {
public:
  /// Register a Tid inbox. Must be called before send/recv for that Tid.
  void register_thread(sched::Tid tid);

  /// Deregister a Tid and discard its inbox.
  void deregister_thread(sched::Tid tid);

  /// Enqueue a message to `dst`'s inbox.
  /// Returns false if `dst` is not registered or inbox is full (kMaxQueueDepth).
  bool ipc_send(sched::Tid dst, CanonMessage msg);

  /// Pop the oldest message from `tid`'s inbox.
  /// Returns nullopt if the inbox is empty or `tid` is not registered.
  std::optional<CanonMessage> ipc_recv(sched::Tid tid);

  /// Peek at the oldest message without removing it.
  const CanonMessage* ipc_peek(sched::Tid tid) const;

  /// Number of pending messages in `tid`'s inbox.
  std::size_t pending(sched::Tid tid) const;

  bool is_registered(sched::Tid tid) const;

private:
  std::unordered_map<sched::Tid, std::deque<CanonMessage>> inboxes_;
};

}  // namespace t81::ternaryos::ipc
