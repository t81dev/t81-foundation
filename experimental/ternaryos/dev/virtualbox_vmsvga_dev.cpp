// experimental/ternaryos/dev/virtualbox_vmsvga_dev.cpp

#include "virtualbox_vmsvga_dev.hpp"

#include <utility>

namespace t81::ternaryos::dev {

VirtualBoxVmsvgaDev::VirtualBoxVmsvgaDev(std::string device_id,
                                         uint32_t width,
                                         uint32_t height)
    : device_id_(std::move(device_id)),
      framebuffer_(width, height) {
  info_.width = width;
  info_.height = height;
}

void VirtualBoxVmsvgaDev::clear(TritPixel fill) noexcept {
  framebuffer_.clear(fill);
}

bool VirtualBoxVmsvgaDev::present() noexcept {
  last_present_ascii_ = framebuffer_.dump_ascii();
  ++present_count_;
  return true;
}

}  // namespace t81::ternaryos::dev
