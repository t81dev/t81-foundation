#pragma once

// experimental/ternaryos/hal/qemu_platform.hpp
//
// QEMU virt AArch64 platform profile for TernOS HAL.
//
// Models the fixed QEMU virt machine memory map and device topology so the
// kernel boot path is testable and explicit without a real QEMU process.
// Parallel structure to virtualbox_platform.hpp.
//
// QEMU virt AArch64 reference addresses (qemu/hw/arm/virt.c):
//   GIC distributor  0x08000000  64 KB
//   GIC redistributor 0x080A0000  128 KB (2 × 64 KB per CPU, up to 4)
//   PL011 UART       0x09000000   4 KB   SPI 33
//   Virtio-MMIO[0]   0x0A000000   512 B  SPI 48
//   Virtio-MMIO[1]   0x0A000200   512 B  SPI 49
//   RAM              0x40000000  configurable
//   Kernel load      0x40080000  (or 0x48000000 via EFI stub)

#include "hal.hpp"

#include <optional>
#include <string>
#include <vector>

namespace t81::ternaryos::hal {

// ── Machine profile ──────────────────────────────────────────────────────────

enum class QemuMachine : uint8_t {
  Virt = 1,  ///< QEMU "virt" platform (generic AArch64 board)
};

enum class QemuArch : uint8_t {
  AArch64 = 1,
};

enum class QemuConsole : uint8_t {
  Pl011 = 1,  ///< ARM PL011 UART @ 0x09000000
};

enum class QemuInterruptController : uint8_t {
  GicV3 = 1,  ///< ARM GICv3 distributor + redistributor
};

enum class QemuStorage : uint8_t {
  VirtioBlk = 1,  ///< Virtio-MMIO block device (slot 0)
};

enum class QemuNetwork : uint8_t {
  VirtioNet = 1,  ///< Virtio-MMIO network device (slot 1)
};

struct QemuProfile {
  QemuMachine             machine{QemuMachine::Virt};
  QemuArch                arch{QemuArch::AArch64};
  QemuConsole             console{QemuConsole::Pl011};
  QemuInterruptController gic{QemuInterruptController::GicV3};
  QemuStorage             storage{QemuStorage::VirtioBlk};
  QemuNetwork             network{QemuNetwork::VirtioNet};
  /// When true the run script passes -accel hvf (Apple HVF on M-series host).
  bool                    use_hvf_acceleration{true};
  /// Number of Virtio-MMIO slots exposed (-device virtio-blk-device, etc.).
  uint8_t                 virtio_slots{2};
};

struct QemuBootSpec {
  uint64_t ram_bytes{512ULL * 1024 * 1024};
  /// Kernel image load address.  Matches the EFI stub in qemu_armv8_efi_stub.c
  /// (0x48000000); override to 0x40080000 for a raw Linux-Image-format kernel.
  uint64_t kernel_load_address{0x0000'0000'4800'0000ULL};
  uint64_t stack_top{0x0000'0000'47FF'F000ULL};
  bool     ethics_boot_required{true};
  QemuProfile profile{};
};

// ── Device descriptor ────────────────────────────────────────────────────────

struct QemuDeviceDescriptor {
  const char* name;
  uint64_t    base;        ///< MMIO base address (all QEMU virt devices are MMIO)
  uint64_t    span_bytes;
  uint32_t    irq;         ///< GIC interrupt ID (SPI = 32 + SPI-number)
};

// ── Timer model ──────────────────────────────────────────────────────────────

/// ARM generic timer uses PPIs (not SPIs).  These are the standard INTID values
/// for the QEMU virt machine (ARM DDI 0487, Table G8-1):
///   Physical timer  PPI 14 → INTID 30
///   Virtual  timer  PPI 11 → INTID 27
constexpr uint32_t kQemuTimerPhysIntId = 30u;
constexpr uint32_t kQemuTimerVirtIntId = 27u;

struct QemuTimerTick {
  uint64_t tick_index{0};
  uint64_t period_ns{0};
  uint64_t cntpct_value{0};    ///< Synthetic CNTPCT_EL0 counter value
  uint32_t gic_intid{kQemuTimerPhysIntId};
};

// ── Public API ───────────────────────────────────────────────────────────────

/// Returns an error string if the profile is invalid, nullopt if OK.
std::optional<std::string> validate_qemu_profile(
    const QemuProfile& profile) noexcept;

/// Build a HAL BootContext from a QemuBootSpec (memory map + platform_id).
BootContext make_qemu_boot_context(const QemuBootSpec& spec);

/// Human-readable one-liner describing the profile.
std::string qemu_profile_summary(const QemuProfile& profile);

/// Return the device map for the given profile.
std::vector<QemuDeviceDescriptor> qemu_device_map(const QemuProfile& profile);

/// Build a synthetic timer tick descriptor.
std::optional<QemuTimerTick> make_qemu_timer_tick(
    const QemuProfile& profile,
    uint64_t tick_index,
    uint64_t period_ns) noexcept;

/// Dispatch a synthetic timer tick as a HAL HardwareInterrupt.
/// Returns false if the profile has no timer or period_ns == 0.
bool dispatch_qemu_timer_tick(const QemuProfile& profile,
                               uint64_t tick_index,
                               uint64_t period_ns) noexcept;

}  // namespace t81::ternaryos::hal
