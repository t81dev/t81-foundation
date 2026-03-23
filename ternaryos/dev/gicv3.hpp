#pragma once

// experimental/ternaryos/dev/gicv3.hpp
//
// ARM GICv3 minimal driver for TernOS on the QEMU virt AArch64 machine.
//
// Implements exactly what is needed to:
//   1. Bring the GICv3 distributor and per-CPU redistributor online.
//   2. Configure SPIs (Shared Peripheral Interrupts) and PPIs.
//   3. Acknowledge and end interrupts from the CPU interface via system regs.
//
// All registers are accessed as volatile 32-bit MMIO unless noted.
// System registers (ICC_*_EL1) are accessed with inline MSR/MRS on AArch64.
// On hosted (non-AArch64) builds every MMIO and MSR write is a no-op so the
// driver can be instantiated in unit tests without faulting.
//
// QEMU virt AArch64 addresses (qemu/hw/arm/virt.c):
//   Distributor  base: 0x08000000   (64 KB)
//   Redistributor base: 0x080A0000  (64 KB LP frame + 64 KB SGI frame per CPU)
//
// References:
//   ARM IHI0069 — GICv3 and GICv4 Architecture Specification
//   ARM DDI 0487 — ARMv8-A Architecture Reference Manual

#include <cstdint>

namespace t81::ternaryos::dev {

// ── QEMU virt constants ──────────────────────────────────────────────────────

inline constexpr uint64_t kQemuVirtGicDistBase   = 0x0000'0000'0800'0000ULL;
inline constexpr uint64_t kQemuVirtGicRedistBase  = 0x0000'0000'080A'0000ULL;

// ── GICD register offsets ────────────────────────────────────────────────────

inline constexpr uint64_t kGicdCtlr       = 0x0000;  ///< Distributor Control
inline constexpr uint64_t kGicdTyiper     = 0x0004;  ///< Interrupt Controller Type
inline constexpr uint64_t kGicdIgrouprN   = 0x0080;  ///< Interrupt Group [0..31]
inline constexpr uint64_t kGicdIsenabler0 = 0x0100;  ///< Set-Enable registers
inline constexpr uint64_t kGicdIcenabler0 = 0x0180;  ///< Clear-Enable registers
inline constexpr uint64_t kGicdIpriorityrN= 0x0400;  ///< Priority registers (byte each)
inline constexpr uint64_t kGicdIcfgrN     = 0x0C00;  ///< Configuration (edge/level)
inline constexpr uint64_t kGicdIrouterN   = 0x6100;  ///< Affinity routing (GICv3 ARE)

/// GICD_CTLR bits.
inline constexpr uint32_t kGicdCtlrEnableGrp1NS = (1u << 1);
inline constexpr uint32_t kGicdCtlrAreNs        = (1u << 4);
inline constexpr uint32_t kGicdCtlrRwp          = (1u << 31);  ///< Register Write Pending

// ── GICR (Redistributor) register offsets ───────────────────────────────────
// Each CPU has two 64 KB frames: LP frame (GICR base + 0) and SGI frame (+ 0x10000).

inline constexpr uint64_t kGicrWaker        = 0x0014;  ///< Wake-up register (LP frame)
inline constexpr uint64_t kGicrSgiBase      = 0x1'0000; ///< SGI frame offset from LP base

inline constexpr uint64_t kGicrIgroupr0     = kGicrSgiBase + 0x0080;  ///< SGI/PPI group
inline constexpr uint64_t kGicrIsenabler0   = kGicrSgiBase + 0x0100;  ///< SGI/PPI set-enable
inline constexpr uint64_t kGicrIpriorityrN  = kGicrSgiBase + 0x0400;  ///< SGI/PPI priority

/// GICR_WAKER bits.
inline constexpr uint32_t kGicrWakerProcessorSleep = (1u << 1);
inline constexpr uint32_t kGicrWakerChildrenAsleep  = (1u << 2);

// ── Special interrupt IDs ────────────────────────────────────────────────────

/// Spurious interrupt ID returned by ICC_IAR1_EL1 when no interrupt is pending.
inline constexpr uint32_t kGicSpuriousIntid = 1023u;

/// Maximum SPI INTID in a minimal GICv3 with 32 SPIs beyond the first 32.
inline constexpr uint32_t kGicMaxIntid = 1019u;

// ── Driver API ───────────────────────────────────────────────────────────────

/// Initialise the GICv3 distributor, the CPU0 redistributor, and the EL1 CPU
/// interface.  Must be called from EL1 after ExitBootServices.
///
/// Steps performed:
///   1. Enable distributor (ARE_NS + EnableGrp1NS).
///   2. Wake up CPU0 redistributor (clear ProcessorSleep, poll ChildrenAsleep).
///   3. Set all SGI/PPI to Group 1 NS in the SGI frame.
///   4. Enable ICC_SRE_EL1 system-register interface.
///   5. Set ICC_PMR_EL1 = 0xFF (accept all priorities).
///   6. Enable ICC_IGRPEN1_EL1.
///
/// On hosted builds this function is a documented no-op.
void gicv3_init(uint64_t dist_base, uint64_t redist_base) noexcept;

/// Set the Group assignment of SPI @intid to Group 1 NS (non-secure) so the
/// EL1 CPU interface can receive it.  intid must be in [32, kGicMaxIntid].
void gicv3_set_spi_group1(uint64_t dist_base, uint32_t intid) noexcept;

/// Enable (unmask) SPI @intid at the distributor.  intid >= 32.
void gicv3_enable_spi(uint64_t dist_base, uint32_t intid) noexcept;

/// Disable (mask) SPI @intid at the distributor.  intid >= 32.
void gicv3_disable_spi(uint64_t dist_base, uint32_t intid) noexcept;

/// Set the priority of SPI @intid (0 = highest, 0xFF = lowest).
/// intid >= 32.
void gicv3_set_priority(uint64_t dist_base, uint32_t intid,
                        uint8_t priority) noexcept;

/// Route SPI @intid to the CPU with the given Aff0 affinity value (usually 0
/// for CPU0).  intid >= 32.
void gicv3_route_spi(uint64_t dist_base, uint32_t intid,
                     uint8_t aff0) noexcept;

/// Read ICC_IAR1_EL1 to acknowledge the highest-priority pending Group 1
/// interrupt.  Returns the INTID (use kGicSpuriousIntid check before EOI).
uint32_t gicv3_acknowledge() noexcept;

/// Write ICC_EOIR1_EL1 to signal End-of-Interrupt for @intid.
void gicv3_eoi(uint32_t intid) noexcept;

/// Enable (unmask) PPI @intid [16..31] on CPU0 redistributor.
void gicv3_enable_ppi(uint64_t redist_base, uint32_t intid) noexcept;

}  // namespace t81::ternaryos::dev
