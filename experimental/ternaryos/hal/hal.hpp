#pragma once

// experimental/ternaryos/hal/hal.hpp
//
// Hardware Abstraction Layer public interface for TernOS Phase 1.
// Implements the contract defined in spec/rfcs/RFC-00B0-hal-spec.md.
//
// NOTE: This is an experimental prototype. When Phase 1 acceptance criteria
// are met this header will be promoted to include/t81/hal/hal.hpp.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace t81::ternaryos::hal {

// ─── Memory Map ─────────────────────────────────────────────────────────────

/**
 * @brief Describes a contiguous region of physical memory available to T81VM.
 *
 * The HAL discovers these at boot and passes them in BootContext. The Ternary
 * MMU layer (Phase 2 / RFC-00B1) carves them into 3¹⁰ = 59,049-tryte pages.
 */
struct MemoryRegion {
  uint64_t base_phys;    ///< Physical base address (binary host address space)
  uint64_t size_bytes;   ///< Region size in bytes
  bool     writable;
  bool     executable;

  // Convenience: bytes rounded down to the nearest ternary page (3^10 trytes).
  static constexpr uint64_t kTernaryPageSize = 59049;  // 3^10 trytes
  uint64_t ternary_page_count() const noexcept {
    return size_bytes / kTernaryPageSize;
  }
};

// ─── Interrupt Dispatch ──────────────────────────────────────────────────────

/**
 * @brief Hardware interrupt source classes.
 *
 * TISC has no interrupt opcode (frozen ISA). The HAL catches hardware
 * interrupts via a shadow binary dispatch table, translates them to
 * HardwareInterrupt events, and injects them through Axion before the
 * VM sees them. This preserves ISA immutability.
 */
enum class InterruptSource : uint8_t {
  Timer    = 0,
  Storage  = 1,
  Network  = 2,
  Keyboard = 3,
  Unknown  = 0xFF,
};

/**
 * @brief A translated hardware interrupt event ready for Axion injection.
 */
struct HardwareInterrupt {
  InterruptSource source;
  uint64_t        timestamp_ns;   ///< Monotonic nanosecond timestamp at capture
  uint64_t        payload;        ///< Device-specific data (sector ID, key code, …)
};

using InterruptHandler = std::function<void(const HardwareInterrupt&)>;

/**
 * @brief Register a handler for a specific interrupt source.
 *
 * Handlers are called from the shadow dispatch table (interrupt_table.cpp)
 * after Axion policy evaluation. Safe to call before hal_main.
 */
void register_interrupt_handler(InterruptSource source, InterruptHandler handler);

/**
 * @brief Register a callback for interrupts with no registered handler.
 *
 * RFC-00B5 §3.5 (Slice 28): When dispatch_interrupt() finds neither a
 * source-specific handler nor an Unknown fallback, it calls this callback
 * instead of silently dropping the interrupt. The callback is responsible
 * for emitting a governance audit event (e.g. UnhandledInterruptDropped).
 *
 * Passing nullptr disarms the callback (default: silent drop).
 */
void register_unhandled_interrupt_callback(InterruptHandler callback);

/**
 * @brief Dispatch a HardwareInterrupt to its registered handler (if any).
 *
 * Called internally by the shadow dispatch table. Also exposed for testing.
 */
void dispatch_interrupt(const HardwareInterrupt& irq);

// ─── I/O Port Abstraction ────────────────────────────────────────────────────

/**
 * @brief Read/write binary I/O ports.
 *
 * Results are byte-width at the HAL boundary. Higher layers perform ternary
 * encoding. On hosted (non-bare-metal) builds these are no-ops / stubs.
 */
uint8_t  io_read8 (uint16_t port);
uint16_t io_read16(uint16_t port);
void     io_write8 (uint16_t port, uint8_t  val);
void     io_write16(uint16_t port, uint16_t val);

// ─── Boot Handoff ────────────────────────────────────────────────────────────

/**
 * @brief Context passed from the bootloader / UEFI stub to hal_main.
 */
struct BootContext {
  std::vector<MemoryRegion> memory_map;
  uint64_t                  kernel_load_address{0};
  uint64_t                  stack_top{0};
  /// When true, hal_main MUST evaluate Θ₁–Θ₉ via Axion before any TISC
  /// dispatch. Always true in production; may be false only in unit tests
  /// that explicitly test the pre-ethics code path.
  bool                      ethics_boot_required{true};
  /// Human-readable label for diagnostics (e.g. "qemu-x86_64", "rpi4-aarch64").
  std::string               platform_id;
};

/**
 * @brief HAL main entry point.
 *
 * Called by the UEFI stub (or hosted_stub.cpp on non-bare-metal builds).
 * Sequence:
 *   1. Validate BootContext (at least one writable MemoryRegion).
 *   2. If ethics_boot_required: evaluate Θ₁–Θ₉ via t81::axion::check_ethics.
 *      Any Deny verdict aborts with a non-zero exit code.
 *   3. Hand off to the Axion kernel-owned runtime entry.
 *
 * Returns 0 on success, non-zero on failure (ethics rejection, bad context, …).
 * On real bare-metal this will be [[noreturn]]; the hosted build returns for
 * testability.
 */
int hal_main(const BootContext& ctx);

// ─── Diagnostics ─────────────────────────────────────────────────────────────

/**
 * @brief Emit a HAL-level log line to stdout (or a serial port on bare metal).
 *
 * Prefixed with "[hal] " for easy filtering.
 */
void hal_log(const std::string& msg);

}  // namespace t81::ternaryos::hal
