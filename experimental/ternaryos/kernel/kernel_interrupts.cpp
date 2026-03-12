#include "kernel_main.hpp"

#include <algorithm>

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
  return true;
}

}  // namespace t81::ternaryos::kernel
