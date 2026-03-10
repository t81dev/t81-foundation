#pragma once

// experimental/ternaryos/hal/virtualbox_guest_devices.hpp
//
// VirtualBox guest-device bindings for the first supported TernOS promotion
// profile. This is the seam where HAL-side profile selection composes with the
// Phase 4 device wrappers.

#include "virtualbox_platform.hpp"

#include "../dev/block_device.hpp"

#include <memory>
#include <optional>
#include <string>

namespace t81::ternaryos::hal {

struct VBoxStorageBinding {
  std::unique_ptr<t81::ternaryos::dev::IBlockDevice> device;
  std::string                                        binding_name;
};

std::optional<std::string> validate_virtualbox_storage_binding(
    const VBoxProfile& profile) noexcept;

std::optional<VBoxStorageBinding> create_virtualbox_storage_binding(
    const VBoxProfile& profile,
    t81::ternaryos::dev::IBlockDevice& backing);

}  // namespace t81::ternaryos::hal
