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

std::optional<std::string> validate_virtualbox_network_binding(
    const VBoxProfile& profile) noexcept {
  if (const auto profile_err = validate_virtualbox_profile(profile); profile_err) {
    return profile_err;
  }

  switch (profile.network) {
    case VBoxNetwork::E1000:
      return std::nullopt;
    case VBoxNetwork::PcNet:
      return std::string("VirtualBox guest binding does not expose PCNet yet; E1000 is the only supported network binding");
  }
  return std::string("unknown VirtualBox network profile");
}

std::optional<VBoxNetworkBinding> create_virtualbox_network_binding(
    const VBoxProfile& profile) {
  if (validate_virtualbox_network_binding(profile).has_value()) {
    return std::nullopt;
  }

  switch (profile.network) {
    case VBoxNetwork::E1000:
      return VBoxNetworkBinding{
          .device = std::make_unique<t81::ternaryos::dev::VirtualBoxE1000Dev>(
              "vbox-e1000"),
          .binding_name = "virtualbox-e1000",
      };
    case VBoxNetwork::PcNet:
      break;
  }
  return std::nullopt;
}

std::optional<std::string> validate_virtualbox_display_binding(
    const VBoxProfile& profile) noexcept {
  if (const auto profile_err = validate_virtualbox_profile(profile); profile_err) {
    return profile_err;
  }

  switch (profile.display) {
    case VBoxDisplay::Vmsvga:
      return std::nullopt;
    case VBoxDisplay::Vga:
      return std::string("VirtualBox guest binding does not expose VGA yet; VMSVGA is the only supported display binding");
  }
  return std::string("unknown VirtualBox display profile");
}

std::optional<VBoxDisplayBinding> create_virtualbox_display_binding(
    const VBoxProfile& profile) {
  if (validate_virtualbox_display_binding(profile).has_value()) {
    return std::nullopt;
  }

  switch (profile.display) {
    case VBoxDisplay::Vmsvga:
      return VBoxDisplayBinding{
          .device = std::make_unique<t81::ternaryos::dev::VirtualBoxVmsvgaDev>(
              "vbox-vmsvga0"),
          .binding_name = "virtualbox-vmsvga",
      };
    case VBoxDisplay::Vga:
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
  auto network = create_virtualbox_network_binding(spec.profile);
  if (!network.has_value()) {
    return std::nullopt;
  }
  auto display = create_virtualbox_display_binding(spec.profile);
  if (!display.has_value()) {
    return std::nullopt;
  }

  return VBoxGuestBootstrap{
      .boot_context = make_virtualbox_boot_context(spec),
      .device_map = virtualbox_device_map(spec.profile),
      .storage = std::move(*storage),
      .network = std::move(*network),
      .display = std::move(*display),
      .profile_summary = virtualbox_profile_summary(spec.profile),
  };
}

}  // namespace t81::ternaryos::hal
