// experimental/ternaryos/hal/virtualbox_guest_devices.cpp

#include "virtualbox_guest_devices.hpp"

#include "../dev/virtualbox_ahci_dev.hpp"

namespace t81::ternaryos::hal {

std::optional<std::string> validate_virtualbox_storage_binding(
    const VBoxProfile& profile) noexcept {
  if (const auto profile_err = validate_virtualbox_profile(profile); profile_err) {
    return profile_err;
  }

  switch (profile.storage) {
    case VBoxStorage::Ahci:
      return std::nullopt;
    case VBoxStorage::Nvme:
      return std::string("VirtualBox guest binding does not expose NVMe yet; AHCI is the only supported storage binding");
  }
  return std::string("unknown VirtualBox storage profile");
}

std::optional<VBoxStorageBinding> create_virtualbox_storage_binding(
    const VBoxProfile& profile,
    t81::ternaryos::dev::IBlockDevice& backing) {
  if (validate_virtualbox_storage_binding(profile).has_value()) {
    return std::nullopt;
  }

  switch (profile.storage) {
    case VBoxStorage::Ahci:
      return VBoxStorageBinding{
          .device = std::make_unique<t81::ternaryos::dev::VirtualBoxAhciDev>(
              backing, "vbox-ahci0"),
          .binding_name = "virtualbox-ahci",
      };
    case VBoxStorage::Nvme:
      break;
  }
  return std::nullopt;
}

}  // namespace t81::ternaryos::hal
