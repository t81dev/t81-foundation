// experimental/ternaryos/tests/ipc_test.cpp
//
// Unit tests for the CanonRef IPC MessageBus — Phase 3 acceptance criteria.

#include "../ipc/canon_message.hpp"

#include <cassert>
#include <cstdio>
#include <string>

using namespace t81::ternaryos::ipc;
using namespace t81::ternaryos::sched;

static int g_pass = 0;
static int g_fail = 0;

static bool check(bool cond, const char* label) {
  if (cond) { std::printf("  PASS  %s\n", label); ++g_pass; }
  else       { std::printf("  FAIL  %s\n", label); ++g_fail; }
  return cond;
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

static t81::canonfs::CanonRef make_ref(const std::string& hex_prefix) {
  t81::canonfs::CanonRef ref;
  // Fill hash bytes with a simple pattern derived from prefix characters
  ref.hash.h.bytes.fill(0);
  for (std::size_t i = 0; i < hex_prefix.size() && i < ref.hash.h.bytes.size(); ++i) {
    ref.hash.h.bytes[i] = static_cast<uint8_t>(hex_prefix[i]);
  }
  return ref;
}

static CanonMessage make_msg(Tid sender, const std::string& tag, uint64_t payload = 0) {
  CanonMessage m;
  m.sender  = sender;
  m.ref     = make_ref(tag);
  m.tag     = tag;
  m.payload = payload;
  return m;
}

// ─── Registration tests ──────────────────────────────────────────────────────

static void test_register_unregister() {
  std::printf("\n[I1] register / deregister\n");
  MessageBus bus;

  check(!bus.is_registered(1),  "unregistered Tid: not registered");

  bus.register_thread(1);
  check(bus.is_registered(1),   "after register: is_registered");
  check(bus.pending(1) == 0,    "fresh inbox has 0 pending");

  bus.deregister_thread(1);
  check(!bus.is_registered(1),  "after deregister: not registered");
}

// ─── Send / Receive tests ────────────────────────────────────────────────────

static void test_send_recv_fifo() {
  std::printf("\n[I2] ipc_send + ipc_recv: FIFO order\n");
  MessageBus bus;
  bus.register_thread(2);

  auto m1 = make_msg(1, "alpha", 10);
  auto m2 = make_msg(1, "beta",  20);
  auto m3 = make_msg(1, "gamma", 30);

  check(bus.ipc_send(2, m1), "send alpha");
  check(bus.ipc_send(2, m2), "send beta");
  check(bus.ipc_send(2, m3), "send gamma");
  check(bus.pending(2) == 3, "3 pending");

  auto r1 = bus.ipc_recv(2);
  check(r1.has_value() && r1->tag == "alpha" && r1->payload == 10, "recv alpha first");

  auto r2 = bus.ipc_recv(2);
  check(r2.has_value() && r2->tag == "beta"  && r2->payload == 20, "recv beta second");

  auto r3 = bus.ipc_recv(2);
  check(r3.has_value() && r3->tag == "gamma" && r3->payload == 30, "recv gamma third");

  check(bus.pending(2) == 0, "inbox empty after 3 recvs");
}

static void test_recv_empty_inbox() {
  std::printf("\n[I3] ipc_recv on empty inbox returns nullopt\n");
  MessageBus bus;
  bus.register_thread(5);
  auto r = bus.ipc_recv(5);
  check(!r.has_value(), "empty inbox → nullopt");
}

static void test_recv_unregistered() {
  std::printf("\n[I4] ipc_recv / ipc_send on unregistered Tid\n");
  MessageBus bus;
  check(!bus.ipc_send(99, make_msg(1, "x")), "send to unregistered → false");
  auto r = bus.ipc_recv(99);
  check(!r.has_value(), "recv from unregistered → nullopt");
}

// ─── Queue depth limit test ──────────────────────────────────────────────────

static void test_queue_depth_limit() {
  std::printf("\n[I5] Queue depth limit (kMaxQueueDepth = 81)\n");
  MessageBus bus;
  bus.register_thread(3);

  for (std::size_t i = 0; i < kMaxQueueDepth; ++i) {
    check(bus.ipc_send(3, make_msg(1, "m", i)), "send within limit");
    if (!bus.ipc_send(3, make_msg(1, "m", i))) break; // stop if rejected early
  }
  // The inbox should now be full
  check(bus.pending(3) == kMaxQueueDepth, "inbox at capacity");

  bool rejected = !bus.ipc_send(3, make_msg(1, "overflow"));
  check(rejected, "send to full inbox → false");
}

// ─── Peek test ────────────────────────────────────────────────────────────────

static void test_ipc_peek() {
  std::printf("\n[I6] ipc_peek does not consume the message\n");
  MessageBus bus;
  bus.register_thread(4);

  bus.ipc_send(4, make_msg(1, "peek_me", 42));
  check(bus.pending(4) == 1, "1 pending before peek");

  const CanonMessage* p = bus.ipc_peek(4);
  check(p != nullptr,              "peek returns non-null pointer");
  check(p->tag     == "peek_me",   "peeked tag correct");
  check(p->payload == 42,          "peeked payload correct");
  check(bus.pending(4) == 1,       "still 1 pending after peek");

  auto r = bus.ipc_recv(4);
  check(r.has_value() && r->tag == "peek_me", "recv after peek succeeds");
  check(bus.pending(4) == 0,                  "inbox empty after recv");
}

static void test_ipc_peek_empty() {
  std::printf("\n[I7] ipc_peek on empty inbox returns nullptr\n");
  MessageBus bus;
  bus.register_thread(6);
  check(bus.ipc_peek(6) == nullptr, "peek on empty inbox → nullptr");
}

// ─── Two-thread exchange ──────────────────────────────────────────────────────

static void test_two_thread_exchange() {
  std::printf("\n[I8] Two TiscContexts exchanging CanonRef messages\n");
  MessageBus bus;
  const Tid tidA = 10, tidB = 11;
  bus.register_thread(tidA);
  bus.register_thread(tidB);

  // A sends two messages to B
  bus.ipc_send(tidB, make_msg(tidA, "req1", 1));
  bus.ipc_send(tidB, make_msg(tidA, "req2", 2));

  // B replies to each
  while (bus.pending(tidB) > 0) {
    auto req = bus.ipc_recv(tidB);
    if (!req) break;
    CanonMessage reply = make_msg(tidB, "ack:" + req->tag, req->payload * 10);
    bus.ipc_send(tidA, reply);
  }

  check(bus.pending(tidA) == 2,                "A received 2 replies");

  auto a1 = bus.ipc_recv(tidA);
  auto a2 = bus.ipc_recv(tidA);
  check(a1.has_value() && a1->tag == "ack:req1" && a1->payload == 10, "reply 1 correct");
  check(a2.has_value() && a2->tag == "ack:req2" && a2->payload == 20, "reply 2 correct");
  check(bus.pending(tidA) == 0, "A inbox empty after both recvs");

  // CanonRef hashes should be deterministic: same tag → same hash pattern
  auto ref_a = make_ref("req1");
  auto ref_b = make_ref("req1");
  check(ref_a.hash.h.bytes == ref_b.hash.h.bytes, "CanonRef with same seed is deterministic");
}

// ─── Message equality ────────────────────────────────────────────────────────

static void test_message_equality() {
  std::printf("\n[I9] CanonMessage operator==\n");
  auto m1 = make_msg(1, "same", 77);
  auto m2 = make_msg(1, "same", 77);
  auto m3 = make_msg(2, "same", 77);
  check(m1 == m2,   "identical messages are equal");
  check(!(m1 == m3), "different sender → not equal");
}

// ─── main ────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== TernOS IPC MessageBus tests (Phase 3) ===\n");

  test_register_unregister();
  test_send_recv_fifo();
  test_recv_empty_inbox();
  test_recv_unregistered();
  test_queue_depth_limit();
  test_ipc_peek();
  test_ipc_peek_empty();
  test_two_thread_exchange();
  test_message_equality();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return (g_fail == 0) ? 0 : 1;
}
