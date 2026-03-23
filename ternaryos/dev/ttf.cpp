// experimental/ternaryos/dev/ttf.cpp

#include "ttf.hpp"

namespace t81::ternaryos::dev {

namespace {

constexpr int kTtfBias = 64;  // Centers ASCII 0..127 inside balanced 3^6 range.

bool valid_trit(int8_t t) noexcept {
  return t >= -1 && t <= 1;
}

}  // namespace

std::optional<TtfGlyph> ttf_encode_ascii(char ch) noexcept {
  const uint8_t code = static_cast<uint8_t>(ch);
  if (code > 0x7F) return std::nullopt;

  int value = static_cast<int>(code) - kTtfBias;
  TtfGlyph glyph{};
  for (std::size_t i = 0; i < glyph.size(); ++i) {
    const int rem = value % 3;
    if (rem == 0) {
      glyph[i] = 0;
    } else if (rem == 1) {
      glyph[i] = 1;
      value -= 1;
    } else {
      glyph[i] = -1;
      value += 1;
    }
    value /= 3;
  }
  return glyph;
}

std::optional<char> ttf_decode_ascii(const TtfGlyph& glyph) noexcept {
  int value = 0;
  int weight = 1;
  for (int8_t trit : glyph) {
    if (!valid_trit(trit)) return std::nullopt;
    value += static_cast<int>(trit) * weight;
    weight *= 3;
  }

  const int code = value + kTtfBias;
  if (code < 0 || code > 0x7F) return std::nullopt;
  return static_cast<char>(code);
}

bool ttf_render_char(TernaryFramebuffer& fb,
                     uint32_t x,
                     uint32_t y,
                     char ch) noexcept {
  const auto glyph = ttf_encode_ascii(ch);
  if (!glyph.has_value()) return false;

  // Require the full 3x3 cell to fit so rendering is all-or-nothing.
  if (x + kTtfGlyphWidth > fb.width() || y + kTtfGlyphHeight > fb.height()) {
    return false;
  }

  std::size_t idx = 0;
  for (uint32_t row = 0; row < 2; ++row) {
    for (uint32_t col = 0; col < 3; ++col) {
      if (!fb.set_pixel(x + col, y + row, TritPixel{(*glyph)[idx++]})) {
        return false;
      }
    }
  }

  for (uint32_t col = 0; col < 3; ++col) {
    if (!fb.set_pixel(x + col, y + 2, TritPixel{0})) return false;
  }
  return true;
}

std::size_t ttf_render_text(TernaryFramebuffer& fb,
                            uint32_t x,
                            uint32_t y,
                            std::string_view text) noexcept {
  std::size_t rendered = 0;
  uint32_t cursor_x = x;
  uint32_t cursor_y = y;

  for (char ch : text) {
    if (ch == '\n') {
      cursor_x = x;
      cursor_y += kTtfGlyphHeight + 1;
      continue;
    }

    if (cursor_x + kTtfGlyphWidth > fb.width() ||
        cursor_y + kTtfGlyphHeight > fb.height()) {
      continue;
    }

    if (ttf_render_char(fb, cursor_x, cursor_y, ch)) {
      ++rendered;
      cursor_x += kTtfGlyphAdvance;
    }
  }

  return rendered;
}

}  // namespace t81::ternaryos::dev
