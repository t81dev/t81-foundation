#pragma once

// experimental/ternaryos/dev/framebuffer.hpp
//
// Ternary framebuffer for TernOS Phase 4.
// RFC-00B2 §4.
//
// Pixels are balanced-ternary values {-1, 0, +1}.
// Default dimensions: 81 × 27 (3^4 × 3^3).

#include <cstdint>
#include <string>
#include <vector>

namespace t81::ternaryos::dev {

/// A single pixel: balanced-ternary value {-1, 0, +1}.
struct TritPixel {
  int8_t value{0};  ///< Must be -1, 0, or +1.
  bool operator==(const TritPixel& o) const noexcept { return value == o.value; }
};

inline constexpr uint32_t kDefaultFbWidth  = 81;  ///< 3^4
inline constexpr uint32_t kDefaultFbHeight = 27;  ///< 3^3

/**
 * @brief 2-D ternary framebuffer.
 *
 * Layout: row-major, (0,0) = top-left.
 * ASCII render: '+' for +1, '·' for 0, '-' for -1.
 */
class TernaryFramebuffer {
public:
  explicit TernaryFramebuffer(uint32_t width  = kDefaultFbWidth,
                               uint32_t height = kDefaultFbHeight);

  uint32_t width()  const noexcept { return width_; }
  uint32_t height() const noexcept { return height_; }
  std::size_t pixel_count() const noexcept { return pixels_.size(); }

  /// Set one pixel (bounds-checked; returns false if out of range).
  bool set_pixel(uint32_t x, uint32_t y, TritPixel p) noexcept;

  /// Read one pixel (returns {0} if out of range).
  TritPixel get_pixel(uint32_t x, uint32_t y) const noexcept;

  /// Fill all pixels with `fill`.
  void clear(TritPixel fill = TritPixel{0}) noexcept;

  /// Render to ASCII string (rows separated by '\n').
  /// '+' for +1 · '·' for 0 · '-' for -1.
  std::string dump_ascii() const;

  /// Count pixels with value v.
  std::size_t count(int8_t v) const noexcept;

private:
  uint32_t               width_;
  uint32_t               height_;
  std::vector<TritPixel> pixels_;
};

}  // namespace t81::ternaryos::dev
