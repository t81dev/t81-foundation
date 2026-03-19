// experimental/ternaryos/shell/shell_startup_snapshot.cpp

#include "t81/axion/shell/shell_session.hpp"

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

std::string make_startup_session_text(const t81::ternaryos::ShellSessionState& state) {
  std::ostringstream stream;
  stream << "AXION_STARTUP_SESSION\n";
  stream << "profile=" << state.profile_summary << "\n";
  stream << "storage=" << state.storage_binding_name << "\n";
  stream << "display=" << state.display_binding_name << "\n";
  stream << "recovered_entries=" << state.recovered_entries << "\n";
  stream << "rendered_glyphs=" << state.rendered_glyphs << "\n";
  stream << "session_command_count=" << state.session_command_count << "\n";
  stream << "durable_ref_count=" << state.durable_ref_count << "\n";
  stream << "durable_anchor=" << (state.durable_anchor_present ? "present" : "missing") << "\n";
  return stream.str();
}

std::optional<std::string> make_startup_history_text() {
  auto session = t81::ternaryos::ShellSession::create(true);
  if (!session.has_value()) return std::nullopt;

  for (const auto& command : t81::ternaryos::default_shell_command_sequence()) {
    if (!session->execute_command(command)) return std::nullopt;
  }
  if (!session->execute_command("history show durable")) return std::nullopt;

  const auto& state = session->state();
  if (state.command_records.empty()) return std::nullopt;

  std::ostringstream stream;
  stream << "AXION_STARTUP_HISTORY\n";
  stream << "command=history show durable\n";
  stream << "result=" << state.command_records.back().result << "\n";
  return stream.str();
}

std::optional<std::string> make_startup_store_text() {
  auto session = t81::ternaryos::ShellSession::create(true);
  if (!session.has_value()) return std::nullopt;

  for (const auto& command : t81::ternaryos::default_shell_command_sequence()) {
    if (!session->execute_command(command)) return std::nullopt;
  }
  if (!session->execute_command("store ls")) return std::nullopt;

  const auto& state = session->state();
  if (state.command_records.empty()) return std::nullopt;

  std::ostringstream stream;
  stream << "AXION_STARTUP_STORE\n";
  stream << "command=store ls\n";
  stream << "result=" << state.command_records.back().result << "\n";
  return stream.str();
}

std::optional<std::string> make_startup_ref_text() {
  auto session = t81::ternaryos::ShellSession::create(true);
  if (!session.has_value()) return std::nullopt;

  for (const auto& command : t81::ternaryos::default_shell_command_sequence()) {
    if (!session->execute_command(command)) return std::nullopt;
  }
  if (!session->execute_command("store ls")) return std::nullopt;

  const auto& store_state = session->state();
  if (store_state.command_records.empty()) return std::nullopt;

  std::istringstream refs(store_state.command_records.back().result);
  std::string header;
  std::string ref_text;
  std::getline(refs, header);
  std::getline(refs, ref_text);
  if (ref_text.empty()) return std::nullopt;

  if (!session->execute_command("show ref " + ref_text)) return std::nullopt;
  const auto& ref_state = session->state();
  if (ref_state.command_records.empty()) return std::nullopt;

  std::ostringstream stream;
  stream << "AXION_STARTUP_REF\n";
  stream << "command=show ref " << ref_text << "\n";
  stream << "result=" << ref_state.command_records.back().result << "\n";
  return stream.str();
}

std::optional<std::string> make_startup_report_text() {
  const auto shell_state = t81::ternaryos::build_scripted_shell_session(true);
  if (!shell_state.has_value()) return std::nullopt;

  const auto history_text = make_startup_history_text();
  const auto store_text = make_startup_store_text();
  const auto ref_text = make_startup_ref_text();
  if (!history_text.has_value() || !store_text.has_value() || !ref_text.has_value()) {
    return std::nullopt;
  }

  std::ostringstream stream;
  stream << "AXION_STARTUP_REPORT\n";
  stream << "[session]\n" << make_startup_session_text(*shell_state);
  stream << "[shell]\n" << make_startup_shell_text(*shell_state);
  stream << "[history]\n" << *history_text;
  stream << "[store]\n" << *store_text;
  stream << "[ref]\n" << *ref_text;
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
  if (argc != 13) {
    std::fputs("usage: shell_startup_snapshot <shell-text> <shell-header> <session-text> <session-header> <history-text> <history-header> <store-text> <store-header> <ref-text> <ref-header> <report-text> <report-header>\n", stderr);
    return 2;
  }

  const auto state = t81::ternaryos::build_scripted_shell_session(true);
  if (!state.has_value()) {
    std::fputs("build_scripted_shell_session failed\n", stderr);
    return 1;
  }

  const std::string shell_text = make_startup_shell_text(*state);
  const std::string shell_header =
      "#pragma once\n"
      "static const char kGeneratedStartupShell[] = " +
      c_string_literal(shell_text) + ";\n";
  const std::string session_text = make_startup_session_text(*state);
  const std::string session_header =
      "#pragma once\n"
      "static const char kGeneratedStartupSession[] = " +
      c_string_literal(session_text) + ";\n";
  const auto history_text = make_startup_history_text();
  if (!history_text.has_value()) {
    std::fputs("failed to build startup history text\n", stderr);
    return 1;
  }
  const std::string history_header =
      "#pragma once\n"
      "static const char kGeneratedStartupHistory[] = " +
      c_string_literal(*history_text) + ";\n";
  const auto store_text = make_startup_store_text();
  if (!store_text.has_value()) {
    std::fputs("failed to build startup store text\n", stderr);
    return 1;
  }
  const std::string store_header =
      "#pragma once\n"
      "static const char kGeneratedStartupStore[] = " +
      c_string_literal(*store_text) + ";\n";
  const auto ref_text = make_startup_ref_text();
  if (!ref_text.has_value()) {
    std::fputs("failed to build startup ref text\n", stderr);
    return 1;
  }
  const std::string ref_header =
      "#pragma once\n"
      "static const char kGeneratedStartupRef[] = " +
      c_string_literal(*ref_text) + ";\n";
  const auto report_text = make_startup_report_text();
  if (!report_text.has_value()) {
    std::fputs("failed to build startup report text\n", stderr);
    return 1;
  }
  const std::string report_header =
      "#pragma once\n"
      "static const char kGeneratedStartupReport[] = " +
      c_string_literal(*report_text) + ";\n";

  if (!write_file(argv[1], shell_text)) {
    std::fputs("failed to write startup shell text\n", stderr);
    return 1;
  }
  if (!write_file(argv[2], shell_header)) {
    std::fputs("failed to write startup shell header\n", stderr);
    return 1;
  }
  if (!write_file(argv[3], session_text)) {
    std::fputs("failed to write startup session text\n", stderr);
    return 1;
  }
  if (!write_file(argv[4], session_header)) {
    std::fputs("failed to write startup session header\n", stderr);
    return 1;
  }
  if (!write_file(argv[5], *history_text)) {
    std::fputs("failed to write startup history text\n", stderr);
    return 1;
  }
  if (!write_file(argv[6], history_header)) {
    std::fputs("failed to write startup history header\n", stderr);
    return 1;
  }
  if (!write_file(argv[7], *store_text)) {
    std::fputs("failed to write startup store text\n", stderr);
    return 1;
  }
  if (!write_file(argv[8], store_header)) {
    std::fputs("failed to write startup store header\n", stderr);
    return 1;
  }
  if (!write_file(argv[9], *ref_text)) {
    std::fputs("failed to write startup ref text\n", stderr);
    return 1;
  }
  if (!write_file(argv[10], ref_header)) {
    std::fputs("failed to write startup ref header\n", stderr);
    return 1;
  }
  if (!write_file(argv[11], *report_text)) {
    std::fputs("failed to write startup report text\n", stderr);
    return 1;
  }
  if (!write_file(argv[12], report_header)) {
    std::fputs("failed to write startup report header\n", stderr);
    return 1;
  }
  return 0;
}
