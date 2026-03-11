#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "t81/canonfs/canon_types.hpp"

namespace t81::ternaryos {

struct ShellCommandRecord {
  std::string command;
  std::string result;
};

struct ShellSessionState {
  std::string                   profile_summary;
  std::string                   storage_binding_name;
  std::string                   display_binding_name;
  std::size_t                   recovered_entries{0};
  std::size_t                   rendered_glyphs{0};
  std::size_t                   session_command_count{0};
  std::size_t                   durable_ref_count{0};
  bool                          durable_anchor_present{false};
  std::vector<std::string>      available_commands;
  std::vector<ShellCommandRecord> command_records;
  std::vector<std::string>      transcript_lines;
  std::string                   transcript_text;
  std::string                   framebuffer_ascii;
};

std::vector<std::string> default_shell_command_sequence();

class ShellSession {
public:
  static std::optional<ShellSession> create(bool quiet_boot = false);

  const ShellSessionState& state() const { return state_; }

  bool execute_command(std::string_view command);

private:
  explicit ShellSession(bool quiet_boot);

  bool initialize();
  bool refresh_render();

  bool        quiet_boot_{false};
  std::string store_path_;
  ShellSessionState state_;
  std::optional<t81::canonfs::CanonRef> history_ref_;
  std::vector<t81::canonfs::CanonRef> stored_refs_;
  std::vector<std::string> imported_transcript_lines_;
};

std::optional<ShellSessionState> build_scripted_shell_session(bool quiet_boot = false);

}  // namespace t81::ternaryos
