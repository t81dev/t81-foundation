#pragma once

// experimental/ternaryos/dev/ttf.hpp
//
// Ternary Text Format (TTF) helpers for TernOS Phase 4.
//
// This is a minimal hosted-simulation codec for rendering ASCII text into a
// ternary framebuffer. Each ASCII byte is encoded into six balanced trits,
// which fits comfortably inside one tryte-aligned glyph cell.

#include "framebuffer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace t81::ternaryos::dev {

inline constexpr uint32_t kTtfGlyphWidth = 3;
inline constexpr uint32_t kTtfGlyphHeight = 3;
inline constexpr uint32_t kTtfGlyphAdvance = 4;

using TtfGlyph = std::array<int8_t, 6>;

// Encode one ASCII byte into six balanced trits using a centered ASCII bias.
std::optional<TtfGlyph> ttf_encode_ascii(char ch) noexcept;

// Decode a six-trit glyph back into an ASCII byte.
std::optional<char> ttf_decode_ascii(const TtfGlyph& glyph) noexcept;

// Render one ASCII character into a framebuffer at the glyph origin.
// The top two rows hold the six encoded trits; the bottom row is blank.
bool ttf_render_char(TernaryFramebuffer& fb,
                     uint32_t x,
                     uint32_t y,
                     char ch) noexcept;

// Render text left-to-right with '\n' support. Characters outside ASCII are
// skipped. Returns the number of characters successfully rendered.
std::size_t ttf_render_text(TernaryFramebuffer& fb,
                            uint32_t x,
                            uint32_t y,
                            std::string_view text) noexcept;

}  // namespace t81::ternaryos::dev
