#pragma once

// experimental/ternaryos/dev/virtualbox_vmsvga_dev.hpp
//
// VirtualBox-first VMSVGA display scaffold for TernOS Phase 4.
// This wraps the existing ternary framebuffer so the VirtualBox promotion path
// has an explicit display-facing boundary in code.

#include "framebuffer.hpp"

#include <cstdint>
#include <string>

namespace t81::ternaryos::dev {

struct VBoxVmsvgaInfo {
  uint64_t mmio_base{0xE0000000ULL};
  uint64_t mmio_span_bytes{0x01000000ULL};
  uint8_t  irq{16};
  uint32_t width{kDefaultFbWidth};
  uint32_t height{kDefaultFbHeight};
  bool     accelerated{false};
};

class VirtualBoxVmsvgaDev final {
public:
  explicit VirtualBoxVmsvgaDev(std::string device_id = "vbox-vmsvga0",
                               uint32_t width = kDefaultFbWidth,
                               uint32_t height = kDefaultFbHeight);

  const std::string& device_id() const noexcept { return device_id_; }
  const VBoxVmsvgaInfo& vmsvga_info() const noexcept { return info_; }

  TernaryFramebuffer& framebuffer() noexcept { return framebuffer_; }
  const TernaryFramebuffer& framebuffer() const noexcept { return framebuffer_; }

  void clear(TritPixel fill = TritPixel{0}) noexcept;
  bool present() noexcept;

  std::size_t present_count() const noexcept { return present_count_; }
  const std::string& last_present_ascii() const noexcept { return last_present_ascii_; }

private:
  VBoxVmsvgaInfo     info_{};
  std::string        device_id_;
  TernaryFramebuffer framebuffer_;
  std::size_t        present_count_{0};
  std::string        last_present_ascii_;
};

}  // namespace t81::ternaryos::dev
