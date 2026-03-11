#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace t81::ternaryos {

struct ShellCommandRecord {
  std::string command;
  std::string result;
};

struct ShellSessionState {
  std::string              profile_summary;
  std::string              storage_binding_name;
  std::string              display_binding_name;
  std::size_t              recovered_entries{0};
  std::size_t              rendered_glyphs{0};
  std::vector<std::string> available_commands;
  std::vector<ShellCommandRecord> command_records;
  std::vector<std::string> transcript_lines;
  std::string              transcript_text;
  std::string              framebuffer_ascii;
};

std::vector<std::string> default_shell_command_sequence();

std::optional<ShellSessionState> build_scripted_shell_session(bool quiet_boot = false);

}  // namespace t81::ternaryos
