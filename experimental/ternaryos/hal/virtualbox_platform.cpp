// experimental/ternaryos/hal/virtualbox_platform.cpp

#include "virtualbox_platform.hpp"

#include <algorithm>

namespace t81::ternaryos::hal {

namespace {

constexpr uint64_t kMinRamBytes = 64ULL * 1024 * 1024;
constexpr uint64_t kMmioHoleBase = 0x0000'0000'F000'0000ULL;
constexpr uint64_t kMmioHoleSize = 16ULL * 1024 * 1024;

const char* to_string(VBoxFirmware fw) noexcept {
  switch (fw) {
    case VBoxFirmware::VBoxEfi: return "VBoxEFI";
  }
  return "unknown-fw";
}

const char* to_string(VBoxStorage storage) noexcept {
  switch (storage) {
    case VBoxStorage::Ahci: return "AHCI";
    case VBoxStorage::Nvme: return "NVMe";
  }
  return "unknown-storage";
}

const char* to_string(VBoxNetwork net) noexcept {
  switch (net) {
    case VBoxNetwork::E1000: return "E1000";
    case VBoxNetwork::PcNet: return "PCNet";
  }
  return "unknown-net";
}

const char* to_string(VBoxDisplay display) noexcept {
  switch (display) {
    case VBoxDisplay::Vmsvga: return "VMSVGA";
    case VBoxDisplay::Vga: return "VGA";
  }
  return "unknown-display";
}

const char* to_string(VBoxTimerModel timer) noexcept {
  switch (timer) {
    case VBoxTimerModel::HpetIoApic: return "HPET+IOAPIC";
  }
  return "unknown-timer";
}

}  // namespace

std::optional<std::string> validate_virtualbox_profile(
    const VBoxProfile& profile) noexcept {
  if (!profile.x86_64_only) {
    return std::string("VirtualBox promotion profile currently supports x86_64 only");
  }
  if (profile.firmware != VBoxFirmware::VBoxEfi) {
    return std::string("first promotion target requires VBox EFI firmware");
  }
  if (profile.storage == VBoxStorage::Nvme && !profile.use_nvme_experimental) {
    return std::string("NVMe is deferred; use AHCI unless experimental NVMe is explicitly enabled");
  }
  if (profile.network != VBoxNetwork::E1000) {
    return std::string("first promotion target requires E1000 networking");
  }
  if (profile.display != VBoxDisplay::Vmsvga &&
      profile.display != VBoxDisplay::Vga) {
    return std::string("first promotion target requires VMSVGA or VGA display");
  }
  if (profile.timer != VBoxTimerModel::HpetIoApic) {
    return std::string("first promotion target requires HPET + IOAPIC timing");
  }
  return std::nullopt;
}

BootContext make_virtualbox_boot_context(const VBoxBootSpec& spec) {
  const uint64_t ram_bytes = std::max(spec.ram_bytes, kMinRamBytes);

  BootContext ctx;
  ctx.kernel_load_address = spec.kernel_load_address;
  ctx.stack_top = spec.stack_top;
  ctx.ethics_boot_required = spec.ethics_boot_required;
  ctx.platform_id = "virtualbox-x86_64:" + virtualbox_profile_summary(spec.profile);

  ctx.memory_map.push_back(MemoryRegion{
      .base_phys = 0x0000'0000'0010'0000ULL,
      .size_bytes = ram_bytes,
      .writable = true,
      .executable = false,
  });
  ctx.memory_map.push_back(MemoryRegion{
      .base_phys = spec.kernel_load_address,
      .size_bytes = 4ULL * 1024 * 1024,
      .writable = true,
      .executable = true,
  });
  ctx.memory_map.push_back(MemoryRegion{
      .base_phys = kMmioHoleBase,
      .size_bytes = kMmioHoleSize,
      .writable = false,
      .executable = false,
  });
  return ctx;
}

std::string virtualbox_profile_summary(const VBoxProfile& profile) {
  return std::string(to_string(profile.firmware)) + "/" +
         to_string(profile.storage) + "/" +
         to_string(profile.network) + "/" +
         to_string(profile.display) + "/" +
         to_string(profile.timer);
}

std::vector<VBoxDeviceDescriptor> virtualbox_device_map(
    const VBoxProfile& profile) {
  std::vector<VBoxDeviceDescriptor> devices;

  if (profile.timer == VBoxTimerModel::HpetIoApic) {
    devices.push_back(VBoxDeviceDescriptor{
        .name = "ioapic",
        .bus = VBoxBusKind::Mmio,
        .base = 0xFEC00000ULL,
        .span_bytes = 0x1000ULL,
        .irq = 0,
    });
    devices.push_back(VBoxDeviceDescriptor{
        .name = "hpet",
        .bus = VBoxBusKind::Mmio,
        .base = 0xFED00000ULL,
        .span_bytes = 0x400ULL,
        .irq = 2,
    });
  }

  if (profile.storage == VBoxStorage::Ahci) {
    devices.push_back(VBoxDeviceDescriptor{
        .name = "ahci",
        .bus = VBoxBusKind::Mmio,
        .base = 0xF0400000ULL,
        .span_bytes = 0x2000ULL,
        .irq = 19,
    });
  } else if (profile.storage == VBoxStorage::Nvme) {
    devices.push_back(VBoxDeviceDescriptor{
        .name = "nvme",
        .bus = VBoxBusKind::Mmio,
        .base = 0xF0800000ULL,
        .span_bytes = 0x4000ULL,
        .irq = 20,
    });
  }

  if (profile.network == VBoxNetwork::E1000) {
    devices.push_back(VBoxDeviceDescriptor{
        .name = "e1000",
        .bus = VBoxBusKind::Mmio,
        .base = 0xF0200000ULL,
        .span_bytes = 0x20000ULL,
        .irq = 11,
    });
  } else if (profile.network == VBoxNetwork::PcNet) {
    devices.push_back(VBoxDeviceDescriptor{
        .name = "pcnet",
        .bus = VBoxBusKind::IoPort,
        .base = 0xD000ULL,
        .span_bytes = 0x20ULL,
        .irq = 10,
    });
  }

  if (profile.display == VBoxDisplay::Vmsvga) {
    devices.push_back(VBoxDeviceDescriptor{
        .name = "vmsvga",
        .bus = VBoxBusKind::Mmio,
        .base = 0xE0000000ULL,
        .span_bytes = 0x01000000ULL,
        .irq = 16,
    });
  } else if (profile.display == VBoxDisplay::Vga) {
    devices.push_back(VBoxDeviceDescriptor{
        .name = "vga",
        .bus = VBoxBusKind::IoPort,
        .base = 0x03C0ULL,
        .span_bytes = 0x20ULL,
        .irq = 0,
    });
  }

  return devices;
}

std::optional<VBoxTimerTick> make_virtualbox_timer_tick(
    const VBoxProfile& profile,
    uint64_t tick_index,
    uint64_t period_ns) noexcept {
  if (profile.timer != VBoxTimerModel::HpetIoApic || period_ns == 0) {
    return std::nullopt;
  }

  return VBoxTimerTick{
      .tick_index = tick_index,
      .period_ns = period_ns,
      .hpet_counter = tick_index * period_ns,
      .ioapic_irq = 2,
  };
}

bool dispatch_virtualbox_timer_tick(const VBoxProfile& profile,
                                    uint64_t tick_index,
                                    uint64_t period_ns) noexcept {
  const auto tick = make_virtualbox_timer_tick(profile, tick_index, period_ns);
  if (!tick.has_value()) return false;

  HardwareInterrupt irq{
      .source = InterruptSource::Timer,
      .timestamp_ns = tick->hpet_counter,
      .payload = tick->tick_index,
  };
  dispatch_interrupt(irq);
  return true;
}

}  // namespace t81::ternaryos::hal
