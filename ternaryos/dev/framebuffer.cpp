// experimental/ternaryos/dev/framebuffer.cpp

#include "framebuffer.hpp"

namespace t81::ternaryos::dev {

TernaryFramebuffer::TernaryFramebuffer(uint32_t width, uint32_t height)
    : width_(width), height_(height),
      pixels_(static_cast<std::size_t>(width) * height, TritPixel{0}) {}

bool TernaryFramebuffer::set_pixel(uint32_t x, uint32_t y, TritPixel p) noexcept {
  if (x >= width_ || y >= height_) return false;
  pixels_[y * width_ + x] = p;
  return true;
}

TritPixel TernaryFramebuffer::get_pixel(uint32_t x, uint32_t y) const noexcept {
  if (x >= width_ || y >= height_) return TritPixel{0};
  return pixels_[y * width_ + x];
}

void TernaryFramebuffer::clear(TritPixel fill) noexcept {
  for (auto& p : pixels_) p = fill;
}

std::string TernaryFramebuffer::dump_ascii() const {
  std::string out;
  out.reserve(static_cast<std::size_t>(height_) * (width_ + 1));
  for (uint32_t y = 0; y < height_; ++y) {
    for (uint32_t x = 0; x < width_; ++x) {
      switch (pixels_[y * width_ + x].value) {
        case  1: out += '+'; break;
        case -1: out += '-'; break;
        default: out += '\xc2'; out += '\xb7';  // UTF-8 '·' (U+00B7)
      }
    }
    out += '\n';
  }
  return out;
}

std::size_t TernaryFramebuffer::count(int8_t v) const noexcept {
  std::size_t n = 0;
  for (const auto& p : pixels_) if (p.value == v) ++n;
  return n;
}

}  // namespace t81::ternaryos::dev
