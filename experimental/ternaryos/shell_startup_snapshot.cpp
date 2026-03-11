// experimental/ternaryos/shell_startup_snapshot.cpp

#include "shell_session.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

std::string join_commands(const std::vector<std::string>& commands) {
  std::ostringstream stream;
  for (std::size_t i = 0; i < commands.size(); ++i) {
    if (i != 0) stream << ',';
    stream << commands[i];
  }
  return stream.str();
}

std::string make_startup_shell_text(const t81::ternaryos::ShellSessionState& state) {
  std::ostringstream stream;
  stream << "AXION_STARTUP_SHELL\n";
  stream << "prompt=axion> \n";
  stream << "mode=typed-builtins\n";
  stream << "commands=" << join_commands(state.available_commands) << "\n";
  stream << "history_anchor=" << (state.durable_anchor_present ? "durable" : "missing") << "\n";
  stream << "session_view=local+durable\n";
  stream << "durable_ref_count=" << state.durable_ref_count << "\n";
  stream << "session_command_count=" << state.session_command_count << "\n";
  return stream.str();
}

std::string c_string_literal(std::string_view text) {
  std::ostringstream stream;
  stream << "\"";
  for (char ch : text) {
    switch (ch) {
      case '\\':
        stream << "\\\\";
        break;
      case '"':
        stream << "\\\"";
        break;
      case '\n':
        stream << "\\n";
        break;
      case '\r':
        stream << "\\r";
        break;
      case '\t':
        stream << "\\t";
        break;
      default:
        stream << ch;
        break;
    }
  }
  stream << "\"";
  return stream.str();
}

bool write_file(const std::string& path, const std::string& contents) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return false;
  out << contents;
  return out.good();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    std::fputs("usage: shell_startup_snapshot <output-text> <output-header>\n", stderr);
    return 2;
  }

  const auto state = t81::ternaryos::build_scripted_shell_session(true);
  if (!state.has_value()) {
    std::fputs("build_scripted_shell_session failed\n", stderr);
    return 1;
  }

  const std::string text = make_startup_shell_text(*state);
  const std::string header =
      "#pragma once\n"
      "static const char kGeneratedStartupShell[] = " +
      c_string_literal(text) + ";\n";

  if (!write_file(argv[1], text)) {
    std::fputs("failed to write startup shell text\n", stderr);
    return 1;
  }
  if (!write_file(argv[2], header)) {
    std::fputs("failed to write startup shell header\n", stderr);
    return 1;
  }
  return 0;
}
