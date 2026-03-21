// experimental/ternaryos/hal/qemu_platform.cpp

#include "qemu_platform.hpp"

#include <algorithm>

namespace t81::ternaryos::hal {

namespace {

// ── QEMU virt AArch64 physical memory map constants ──────────────────────────

constexpr uint64_t kQemuRamBase        = 0x0000'0000'4000'0000ULL;
constexpr uint64_t kMinRamBytes        = 128ULL * 1024 * 1024;
constexpr uint64_t kKernelRegionBytes  = 16ULL  * 1024 * 1024;

// GICv3
constexpr uint64_t kGicDistBase        = 0x0000'0000'0800'0000ULL;
constexpr uint64_t kGicDistSpan        = 0x0001'0000ULL;  // 64 KB
constexpr uint64_t kGicRedistBase      = 0x0000'0000'080A'0000ULL;
constexpr uint64_t kGicRedistSpan      = 0x0002'0000ULL;  // 128 KB (2 CPUs × 64 KB)

// PL011 UART
constexpr uint64_t kPl011Base          = 0x0000'0000'0900'0000ULL;
constexpr uint64_t kPl011Span          = 0x0000'1000ULL;  // 4 KB
constexpr uint32_t kPl011Irq           = 33u;             // GIC SPI 1 → INTID 33

// Virtio-MMIO slots (each 512 bytes, IRQs 48..55)
constexpr uint64_t kVirtioBase         = 0x0000'0000'0A00'0000ULL;
constexpr uint64_t kVirtioSlotSpan     = 0x200ULL;
constexpr uint32_t kVirtioBaseIrq      = 48u;

// ── String helpers ────────────────────────────────────────────────────────────

const char* to_string(QemuMachine m) noexcept {
  switch (m) {
    case QemuMachine::Virt: return "virt";
  }
  return "unknown-machine";
}

const char* to_string(QemuArch a) noexcept {
  switch (a) {
    case QemuArch::AArch64: return "AArch64";
  }
  return "unknown-arch";
}

const char* to_string(QemuConsole c) noexcept {
  switch (c) {
    case QemuConsole::Pl011: return "PL011";
  }
  return "unknown-console";
}

const char* to_string(QemuInterruptController g) noexcept {
  switch (g) {
    case QemuInterruptController::GicV3: return "GICv3";
  }
  return "unknown-gic";
}

const char* to_string(QemuStorage s) noexcept {
  switch (s) {
    case QemuStorage::VirtioBlk: return "virtio-blk";
  }
  return "unknown-storage";
}

const char* to_string(QemuNetwork n) noexcept {
  switch (n) {
    case QemuNetwork::VirtioNet: return "virtio-net";
  }
  return "unknown-net";
}

}  // namespace

// ── Public API ────────────────────────────────────────────────────────────────

std::optional<std::string> validate_qemu_profile(
    const QemuProfile& profile) noexcept {
  if (profile.machine != QemuMachine::Virt) {
    return std::string("only QEMU virt machine is supported");
  }
  if (profile.arch != QemuArch::AArch64) {
    return std::string("only AArch64 is supported for QEMU promotion target");
  }
  if (profile.gic != QemuInterruptController::GicV3) {
    return std::string("QEMU virt requires GICv3 interrupt controller");
  }
  if (profile.console != QemuConsole::Pl011) {
    return std::string("QEMU virt console must be PL011 UART");
  }
  if (profile.virtio_slots == 0 || profile.virtio_slots > 8) {
    return std::string("virtio_slots must be in [1, 8]");
  }
  return std::nullopt;
}

BootContext make_qemu_boot_context(const QemuBootSpec& spec) {
  const uint64_t ram_bytes = std::max(spec.ram_bytes, kMinRamBytes);

  BootContext ctx;
  ctx.kernel_load_address  = spec.kernel_load_address;
  ctx.stack_top            = spec.stack_top;
  ctx.ethics_boot_required = spec.ethics_boot_required;
  ctx.platform_id          = "qemu-aarch64:" + qemu_profile_summary(spec.profile);

  // Main RAM region (read/write, not executable).
  ctx.memory_map.push_back(MemoryRegion{
      .base_phys  = kQemuRamBase,
      .size_bytes = ram_bytes,
      .writable   = true,
      .executable = false,
  });

  // Kernel load region (read/write/execute).
  ctx.memory_map.push_back(MemoryRegion{
      .base_phys  = spec.kernel_load_address,
      .size_bytes = kKernelRegionBytes,
      .writable   = true,
      .executable = true,
  });

  // GIC MMIO (non-writable from the kernel perspective; accessed via drivers).
  ctx.memory_map.push_back(MemoryRegion{
      .base_phys  = kGicDistBase,
      .size_bytes = kGicDistSpan + kGicRedistSpan + 0x0A0000ULL,  // dist+redist gap
      .writable   = false,
      .executable = false,
  });

  // PL011 + Virtio MMIO region.
  ctx.memory_map.push_back(MemoryRegion{
      .base_phys  = kPl011Base,
      .size_bytes = 0x0200'0000ULL,  // covers PL011 @ 0x09000000 + virtio @ 0x0A000000
      .writable   = false,
      .executable = false,
  });

  return ctx;
}

std::string qemu_profile_summary(const QemuProfile& profile) {
  return std::string(to_string(profile.machine)) + "/" +
         to_string(profile.arch) + "/" +
         to_string(profile.console) + "/" +
         to_string(profile.gic) + "/" +
         to_string(profile.storage) + "/" +
         to_string(profile.network);
}

std::vector<QemuDeviceDescriptor> qemu_device_map(const QemuProfile& profile) {
  std::vector<QemuDeviceDescriptor> devices;

  // GIC distributor — always present on virt/GICv3.
  devices.push_back(QemuDeviceDescriptor{
      .name       = "gic-dist",
      .base       = kGicDistBase,
      .span_bytes = kGicDistSpan,
      .irq        = 0,  // no IRQ; the GIC itself is the interrupt controller
  });
  devices.push_back(QemuDeviceDescriptor{
      .name       = "gic-redist",
      .base       = kGicRedistBase,
      .span_bytes = kGicRedistSpan,
      .irq        = 0,
  });

  // PL011 UART.
  if (profile.console == QemuConsole::Pl011) {
    devices.push_back(QemuDeviceDescriptor{
        .name       = "pl011",
        .base       = kPl011Base,
        .span_bytes = kPl011Span,
        .irq        = kPl011Irq,
    });
  }

  // Virtio-MMIO slots.
  const uint8_t slots = profile.virtio_slots;
  for (uint8_t i = 0; i < slots; ++i) {
    const bool is_blk = (i == 0 && profile.storage == QemuStorage::VirtioBlk);
    const bool is_net = (i == 1 && profile.network == QemuNetwork::VirtioNet);
    devices.push_back(QemuDeviceDescriptor{
        .name       = is_blk ? "virtio-blk" : is_net ? "virtio-net" : "virtio-mmio",
        .base       = kVirtioBase + i * kVirtioSlotSpan,
        .span_bytes = kVirtioSlotSpan,
        .irq        = kVirtioBaseIrq + i,
    });
  }

  return devices;
}

std::optional<QemuTimerTick> make_qemu_timer_tick(
    const QemuProfile& profile,
    uint64_t tick_index,
    uint64_t period_ns) noexcept {
  (void)profile;
  if (period_ns == 0) return std::nullopt;

  // ARM generic timer: CNTFRQ_EL0 is typically 62.5 MHz on QEMU virt.
  // Synthesise a CNTPCT_EL0 value proportional to elapsed nanoseconds.
  constexpr uint64_t kCntFreqHz = 62'500'000ULL;
  const uint64_t cntpct = (tick_index * period_ns * kCntFreqHz) / 1'000'000'000ULL;

  return QemuTimerTick{
      .tick_index   = tick_index,
      .period_ns    = period_ns,
      .cntpct_value = cntpct,
      .gic_intid    = kQemuTimerPhysIntId,
  };
}

bool dispatch_qemu_timer_tick(const QemuProfile& profile,
                               uint64_t tick_index,
                               uint64_t period_ns) noexcept {
  const auto tick = make_qemu_timer_tick(profile, tick_index, period_ns);
  if (!tick.has_value()) return false;

  HardwareInterrupt irq{
      .source       = InterruptSource::Timer,
      .timestamp_ns = tick->cntpct_value,
      .payload      = tick->tick_index,
  };
  dispatch_interrupt(irq);
  return true;
}

}  // namespace t81::ternaryos::hal
