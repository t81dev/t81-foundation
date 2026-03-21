// experimental/ternaryos/hal/interrupt_table.cpp
//
// Shadow binary interrupt dispatch table (RFC-00B0 §4.2).
//
// TISC has no interrupt or trap-return opcode (frozen ISA). Hardware
// interrupts are caught here, translated to HardwareInterrupt events, and
// forwarded to registered handlers — without touching any TISC opcode.
//
// On bare metal this table is populated by the UEFI/hypervisor stub at
// initialization. On the hosted build it is populated by tests and
// hosted_stub.cpp.

#include "hal.hpp"

#include <array>
#include <chrono>
#include <mutex>

namespace t81::ternaryos::hal {

// ─── Handler registry ────────────────────────────────────────────────────────

namespace {

// One slot per InterruptSource value (max index = 0xFF = Unknown).
// Realistically only 4 sources (0–3) are used; Unknown (0xFF) is the catch-all.
constexpr std::size_t kTableSize = 256;

struct HandlerEntry {
  InterruptHandler fn;
  bool             registered{false};
};

std::array<HandlerEntry, kTableSize> g_table{};
std::mutex                           g_table_mutex;

// RFC-00B5 §3.5 (Slice 28): callback invoked when no handler is found.
InterruptHandler g_unhandled_callback;

std::size_t source_index(InterruptSource s) {
  return static_cast<std::size_t>(static_cast<uint8_t>(s));
}

uint64_t monotonic_ns() {
  using namespace std::chrono;
  return static_cast<uint64_t>(
      duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count());
}

}  // namespace

// ─── Public API ──────────────────────────────────────────────────────────────

void register_interrupt_handler(InterruptSource source, InterruptHandler handler) {
  std::lock_guard<std::mutex> lock(g_table_mutex);
  auto& entry      = g_table[source_index(source)];
  entry.fn         = std::move(handler);
  entry.registered = true;
}

void register_unhandled_interrupt_callback(InterruptHandler callback) {
  std::lock_guard<std::mutex> lock(g_table_mutex);
  g_unhandled_callback = std::move(callback);
}

void dispatch_interrupt(const HardwareInterrupt& irq) {
  // Stamp timestamp if caller left it at zero.
  HardwareInterrupt stamped = irq;
  if (stamped.timestamp_ns == 0) {
    stamped.timestamp_ns = monotonic_ns();
  }

  InterruptHandler fn;
  {
    std::lock_guard<std::mutex> lock(g_table_mutex);
    auto& entry = g_table[source_index(stamped.source)];
    if (entry.registered) {
      fn = entry.fn;
    } else {
      // Fall back to Unknown handler if registered.
      auto& unk = g_table[source_index(InterruptSource::Unknown)];
      if (unk.registered) fn = unk.fn;
    }
  }

  if (fn) {
    fn(stamped);
    return;
  }
  // RFC-00B5 §3.5 (Slice 28): no handler found — invoke the unhandled
  // interrupt callback so the kernel can emit a governance audit event.
  InterruptHandler unhandled_fn;
  {
    std::lock_guard<std::mutex> lock2(g_table_mutex);
    unhandled_fn = g_unhandled_callback;
  }
  if (unhandled_fn) unhandled_fn(stamped);
  // If no unhandled callback is registered, the interrupt is silently dropped.
}

// ─── Simulated interrupt injection (for tests and hosted_stub) ───────────────

/**
 * @brief Fire a simulated interrupt synchronously.
 *
 * Used by hosted_stub.cpp to exercise the dispatch path without actual
 * hardware. On bare metal this is replaced by the real IRQ trampoline.
 */
void fire_simulated_interrupt(InterruptSource source, uint64_t payload) {
  HardwareInterrupt irq{source, /*timestamp_ns=*/0, payload};
  dispatch_interrupt(irq);
}

}  // namespace t81::ternaryos::hal
