#pragma once

// experimental/ternaryos/hal/virtualbox_platform.hpp
//
// VirtualBox-first promotion scaffold for TernOS HAL.
//
// This file does not attempt to boot a real VirtualBox guest yet. It captures
// the first supported VM hardware profile in code so the promotion path is
// testable and explicit in the HAL layer.

#include "hal.hpp"

#include <optional>
#include <string>
#include <vector>

namespace t81::ternaryos::hal {

enum class VBoxFirmware : uint8_t {
  VBoxEfi = 1,
};

enum class VBoxStorage : uint8_t {
  Ahci = 1,
  Nvme = 2,
};

enum class VBoxNetwork : uint8_t {
  E1000 = 1,
  PcNet = 2,
};

enum class VBoxDisplay : uint8_t {
  Vmsvga = 1,
  Vga = 2,
};

enum class VBoxTimerModel : uint8_t {
  HpetIoApic = 1,
};

struct VBoxProfile {
  VBoxFirmware   firmware{VBoxFirmware::VBoxEfi};
  VBoxStorage    storage{VBoxStorage::Ahci};
  VBoxNetwork    network{VBoxNetwork::E1000};
  VBoxDisplay    display{VBoxDisplay::Vmsvga};
  VBoxTimerModel timer{VBoxTimerModel::HpetIoApic};
  bool           x86_64_only{true};
  bool           use_nvme_experimental{false};
};

struct VBoxBootSpec {
  uint64_t ram_bytes{128ULL * 1024 * 1024};
  uint64_t kernel_load_address{0x0000'0000'0800'0000ULL};
  uint64_t stack_top{0x0000'0000'07FF'F000ULL};
  bool     ethics_boot_required{true};
  VBoxProfile profile{};
};

enum class VBoxBusKind : uint8_t {
  Mmio = 1,
  IoPort = 2,
};

struct VBoxDeviceDescriptor {
  const char* name;
  VBoxBusKind bus;
  uint64_t    base;
  uint64_t    span_bytes;
  uint8_t     irq;
};

struct VBoxTimerTick {
  uint64_t tick_index{0};
  uint64_t period_ns{0};
  uint64_t hpet_counter{0};
  uint8_t  ioapic_irq{2};
};

std::optional<std::string> validate_virtualbox_profile(
    const VBoxProfile& profile) noexcept;

BootContext make_virtualbox_boot_context(const VBoxBootSpec& spec);

std::string virtualbox_profile_summary(const VBoxProfile& profile);

std::vector<VBoxDeviceDescriptor> virtualbox_device_map(
    const VBoxProfile& profile);

std::optional<VBoxTimerTick> make_virtualbox_timer_tick(
    const VBoxProfile& profile,
    uint64_t tick_index,
    uint64_t period_ns) noexcept;

bool dispatch_virtualbox_timer_tick(const VBoxProfile& profile,
                                    uint64_t tick_index,
                                    uint64_t period_ns) noexcept;

}  // namespace t81::ternaryos::hal
