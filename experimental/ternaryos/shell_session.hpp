#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace t81::ternaryos {

struct ShellSessionState {
  std::string              profile_summary;
  std::string              storage_binding_name;
  std::string              display_binding_name;
  std::size_t              recovered_entries{0};
  std::size_t              rendered_glyphs{0};
  std::vector<std::string> transcript_lines;
  std::string              transcript_text;
  std::string              framebuffer_ascii;
};

std::vector<std::string> scripted_shell_lines();

std::optional<ShellSessionState> build_scripted_shell_session(bool quiet_boot = false);

}  // namespace t81::ternaryos
