#pragma once

// experimental/ternaryos/hal/virtualbox_guest_devices.hpp
//
// VirtualBox guest-device bindings for the first supported TernOS promotion
// profile. This is the seam where HAL-side profile selection composes with the
// Phase 4 device wrappers.

#include "virtualbox_platform.hpp"

#include "../dev/block_device.hpp"
#include "../dev/virtualbox_e1000_dev.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace t81::ternaryos::hal {

struct VBoxStorageBinding {
  std::unique_ptr<t81::ternaryos::dev::IBlockDevice> device;
  std::string                                        binding_name;
};

struct VBoxNetworkBinding {
  std::unique_ptr<t81::ternaryos::dev::VirtualBoxE1000Dev> device;
  std::string                                              binding_name;
};

struct VBoxGuestBootstrap {
  BootContext                       boot_context;
  std::vector<VBoxDeviceDescriptor> device_map;
  VBoxStorageBinding                storage;
  VBoxNetworkBinding                network;
  std::string                       profile_summary;
};

std::optional<std::string> validate_virtualbox_storage_binding(
    const VBoxProfile& profile) noexcept;

std::optional<VBoxStorageBinding> create_virtualbox_storage_binding(
    const VBoxProfile& profile,
    t81::ternaryos::dev::IBlockDevice& backing);

std::optional<std::string> validate_virtualbox_network_binding(
    const VBoxProfile& profile) noexcept;

std::optional<VBoxNetworkBinding> create_virtualbox_network_binding(
    const VBoxProfile& profile);

std::optional<VBoxGuestBootstrap> bootstrap_virtualbox_guest(
    const VBoxBootSpec& spec,
    t81::ternaryos::dev::IBlockDevice& backing);

}  // namespace t81::ternaryos::hal
