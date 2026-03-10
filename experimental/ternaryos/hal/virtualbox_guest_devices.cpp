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

std::optional<VBoxGuestBootstrap> bootstrap_virtualbox_guest(
    const VBoxBootSpec& spec,
    t81::ternaryos::dev::IBlockDevice& backing) {
  if (validate_virtualbox_profile(spec.profile).has_value()) {
    return std::nullopt;
  }

  auto storage = create_virtualbox_storage_binding(spec.profile, backing);
  if (!storage.has_value()) {
    return std::nullopt;
  }

  return VBoxGuestBootstrap{
      .boot_context = make_virtualbox_boot_context(spec),
      .device_map = virtualbox_device_map(spec.profile),
      .storage = std::move(*storage),
      .profile_summary = virtualbox_profile_summary(spec.profile),
  };
}

}  // namespace t81::ternaryos::hal
