#include "kernel_main.hpp"

#include <algorithm>
#include <cstdint>

namespace t81::ternaryos::kernel {

namespace {

void increment_interrupt_source_counter(KernelInterruptSourceCounters& counters,
                                        hal::InterruptSource source) {
  switch (source) {
    case hal::InterruptSource::Timer:
      ++counters.timer;
      break;
    case hal::InterruptSource::Storage:
      ++counters.storage;
      break;
    case hal::InterruptSource::Network:
      ++counters.network;
      break;
    case hal::InterruptSource::Keyboard:
      ++counters.keyboard;
      break;
    case hal::InterruptSource::Unknown:
      ++counters.unknown;
      break;
  }
}

void record_interrupt(KernelRuntimeState& state,
                      const hal::HardwareInterrupt& interrupt) {
  record_audit_event(state,
                     KernelAuditEventKind::InterruptRecorded,
                     KernelRuntimeState::kKernelTid,
                     KernelRuntimeState::kKernelProcessGroup);
  state.last_recorded_interrupt_audit_sequence =
      state.last_audit_event.has_value()
          ? std::optional<uint64_t>{state.last_audit_event->sequence}
          : std::nullopt;
  state.last_interrupt_audit_kind = KernelAuditEventKind::InterruptRecorded;
  state.last_interrupt_audit_source = interrupt.source;
  state.last_interrupt_audit_interrupt_sequence = state.next_interrupt_sequence;
  state.last_interrupt_audit_payload = interrupt.payload;
  state.last_interrupt_audit_timestamp_ns = interrupt.timestamp_ns;
  state.pending_interrupts.push_back(KernelInterruptRecord{
      .source = interrupt.source,
      .timestamp_ns = interrupt.timestamp_ns,
      .payload = interrupt.payload,
      .sequence = state.next_interrupt_sequence++,
      .recorded_audit_sequence =
          state.last_recorded_interrupt_audit_sequence.value_or(0),
  });
  state.pending_interrupt_high_watermark =
      std::max(state.pending_interrupt_high_watermark, state.pending_interrupts.size());
  state.last_recorded_interrupt = state.pending_interrupts.back();
  ++state.counters.interrupts_recorded;
  increment_interrupt_source_counter(state.counters.interrupt_sources_recorded,
                                     interrupt.source);
}

}  // namespace

bool axion_kernel_record_interrupt(KernelRuntimeState& state,
                                   const hal::HardwareInterrupt& interrupt) noexcept {
  record_interrupt(state, interrupt);
  return true;
}

bool axion_kernel_deliver_pending_interrupt(KernelRuntimeState& state) noexcept {
  state.last_delivered_interrupt = state.pending_interrupts.front();
  state.pending_interrupts.pop_front();
  ++state.counters.interrupts_delivered;
  increment_interrupt_source_counter(
      state.counters.interrupt_sources_delivered,
      state.last_delivered_interrupt->source);
  record_audit_event(state,
                     KernelAuditEventKind::InterruptDelivered,
                     KernelRuntimeState::kKernelTid,
                     KernelRuntimeState::kKernelProcessGroup);
  state.last_interrupt_audit_sequence =
      state.last_audit_event.has_value()
          ? std::optional<uint64_t>{state.last_audit_event->sequence}
          : std::nullopt;
  if (state.last_audit_event.has_value()) {
    state.last_delivered_interrupt->delivered_audit_sequence =
        state.last_audit_event->sequence;
  }
  state.last_delivered_interrupt_audit_sequence =
      state.last_delivered_interrupt->delivered_audit_sequence != 0
          ? std::optional<uint64_t>{
                state.last_delivered_interrupt->delivered_audit_sequence}
          : std::nullopt;
  state.last_interrupt_audit_kind = KernelAuditEventKind::InterruptDelivered;
  state.last_interrupt_audit_source = state.last_delivered_interrupt->source;
  state.last_interrupt_audit_interrupt_sequence =
      state.last_delivered_interrupt->sequence;
  state.last_interrupt_audit_payload = state.last_delivered_interrupt->payload;
  state.last_interrupt_audit_timestamp_ns =
      state.last_delivered_interrupt->timestamp_ns;

  // RFC-00B5 §3.3 — source-specific interrupt policy dispatch.
  // Timer: force a scheduler preemption so the kernel loop behaves as a
  // timer-driven preemptive kernel rather than a purely cooperative one.
  // Storage/Network/Keyboard (Slice 26): wake any threads parked via
  // WaitForDevice for the matching source.
  // Unknown: no action.
  switch (state.last_delivered_interrupt->source) {
    case hal::InterruptSource::Timer: {
      ++state.counters.timer_interrupts_handled;
      const bool switched = axion_kernel_tick(state);
      if (switched) {
        ++state.counters.timer_preempts;
        state.last_timer_preempt_cycle = state.counters.loop_iterations;
        state.last_timer_preempt_sequence =
            state.last_delivered_interrupt->sequence;
      }
      break;
    }
    case hal::InterruptSource::Storage:
    case hal::InterruptSource::Network:
    case hal::InterruptSource::Keyboard: {
      ++state.counters.device_interrupts_handled;
      // RFC-00B5 §3.3 — wake any threads parked via WaitForDevice for this source.
      const uint8_t src_key =
          static_cast<uint8_t>(state.last_delivered_interrupt->source);
      auto it = state.device_waiting_tids.find(src_key);
      if (it != state.device_waiting_tids.end() && !it->second.empty()) {
        // Deliver a synthetic IPC message to each waiting thread then wake it.
        const t81::canonfs::CanonRef zero_ref{};
        for (const sched::Tid wtid : it->second) {
          axion_kernel_ipc_send(
              state, wtid,
              ipc::CanonMessage{
                  .sender  = KernelRuntimeState::kKernelTid,
                  .ref     = zero_ref,
                  .payload = state.last_delivered_interrupt->sequence,
                  .tag     = "device-wake",
              });
          state.scheduler.wake(wtid);
          ++state.counters.device_wakes;
          if (state.last_delivered_interrupt->source ==
              hal::InterruptSource::Keyboard) {
            ++state.counters.keyboard_wakes;
          }
        }
        it->second.clear();
      }
      break;
    }
    case hal::InterruptSource::Unknown:
      break;
  }

  return true;
}

}  // namespace t81::ternaryos::kernel
