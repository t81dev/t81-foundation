// experimental/ternaryos/ipc/canon_message.cpp

#include "canon_message.hpp"

namespace t81::ternaryos::ipc {

void MessageBus::register_thread(sched::Tid tid) {
  inboxes_.emplace(tid, std::deque<CanonMessage>{});
}

void MessageBus::deregister_thread(sched::Tid tid) {
  inboxes_.erase(tid);
}

bool MessageBus::ipc_send(sched::Tid dst, CanonMessage msg) {
  auto it = inboxes_.find(dst);
  if (it == inboxes_.end()) return false;
  if (it->second.size() >= kMaxQueueDepth) return false;
  it->second.push_back(std::move(msg));
  return true;
}

std::optional<CanonMessage> MessageBus::ipc_recv(sched::Tid tid) {
  auto it = inboxes_.find(tid);
  if (it == inboxes_.end() || it->second.empty()) return std::nullopt;
  CanonMessage msg = std::move(it->second.front());
  it->second.pop_front();
  return msg;
}

const CanonMessage* MessageBus::ipc_peek(sched::Tid tid) const {
  auto it = inboxes_.find(tid);
  if (it == inboxes_.end() || it->second.empty()) return nullptr;
  return &it->second.front();
}

std::size_t MessageBus::pending(sched::Tid tid) const {
  auto it = inboxes_.find(tid);
  if (it == inboxes_.end()) return 0;
  return it->second.size();
}

bool MessageBus::is_registered(sched::Tid tid) const {
  return inboxes_.count(tid) > 0;
}

}  // namespace t81::ternaryos::ipc
