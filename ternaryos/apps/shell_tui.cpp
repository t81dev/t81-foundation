// experimental/ternaryos/apps/shell_tui.cpp

#include "t81/axion/shell/shell_session.hpp"

#include "../shell/command_catalog.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace ftxui;

namespace {

struct ShellTuiHandoff {
  bool        present{false};
  std::string source;
  std::string profile_summary;
  std::string storage_binding_name;
  std::string display_binding_name;
  std::string canonfs_mode_summary;
  std::string prompt;
  std::string command;
  std::string status_line;
  std::vector<std::string> transcript_lines;
};

const char* env_or_null(const char* name) {
  const char* value = std::getenv(name);
  return (value != nullptr && *value != '\0') ? value : nullptr;
}

ShellTuiHandoff shell_tui_handoff_from_env() {
  ShellTuiHandoff handoff;
  const char* source = env_or_null("T81_TUI_HANDOFF_SOURCE");
  if (source == nullptr) return handoff;

  handoff.present = true;
  handoff.source = source;
  if (const char* value = env_or_null("T81_TUI_HANDOFF_PROFILE")) {
    handoff.profile_summary = value;
  }
  if (const char* value = env_or_null("T81_TUI_HANDOFF_STORAGE")) {
    handoff.storage_binding_name = value;
  }
  if (const char* value = env_or_null("T81_TUI_HANDOFF_DISPLAY")) {
    handoff.display_binding_name = value;
  }
  if (const char* value = env_or_null("T81_TUI_HANDOFF_CANONFS")) {
    handoff.canonfs_mode_summary = value;
  }
  if (const char* value = env_or_null("T81_TUI_HANDOFF_PROMPT")) {
    handoff.prompt = value;
  }
  if (const char* value = env_or_null("T81_TUI_HANDOFF_COMMAND")) {
    handoff.command = value;
  }
  if (const char* value = env_or_null("T81_TUI_HANDOFF_STATUS")) {
    handoff.status_line = value;
  }
  if (const char* value = env_or_null("T81_TUI_HANDOFF_TRANSCRIPT")) {
    std::string current;
    for (char ch : std::string(value)) {
      if (ch == '\n') {
        handoff.transcript_lines.push_back(current);
        current.clear();
      } else {
        current.push_back(ch);
      }
    }
    if (!current.empty()) {
      handoff.transcript_lines.push_back(current);
    }
  }
  return handoff;
}

std::string shell_tui_status_value(const char* value, const std::string& override_value) {
  if (!override_value.empty()) return override_value;
  return value != nullptr ? value : "";
}

std::string shell_tui_canonfs_value(const t81::ternaryos::ShellCommandContext& context,
                                    const ShellTuiHandoff& handoff) {
  if (!handoff.canonfs_mode_summary.empty()) return handoff.canonfs_mode_summary;
  return context.canonfs_mode_summary != nullptr ? context.canonfs_mode_summary : "n/a";
}

std::vector<std::string> split_lines(const std::string& text) {
  std::vector<std::string> out;
  std::string current;
  for (char ch : text) {
    if (ch == '\n') {
      out.push_back(current);
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  out.push_back(current);
  return out;
}

Element shell_tui_document(const t81::ternaryos::ShellSessionState& state,
                           const t81::ternaryos::ShellCommandContext& context,
                           const ShellTuiHandoff& handoff,
                           const std::string& command_buffer,
                           const std::string& status_line) {
  Elements transcript;
  if (handoff.present) {
    transcript.push_back(text("HANDOFF FROM AXION SERIAL SHELL") | color(Color::Yellow) | bold);
    if (!handoff.transcript_lines.empty()) {
      for (const auto& line : handoff.transcript_lines) {
        const bool is_prompt = line.rfind("[axion@T81", 0) == 0;
        transcript.push_back(text(line) | color(is_prompt ? Color::Cyan : Color::White));
      }
    } else if (!handoff.prompt.empty()) {
      transcript.push_back(text(handoff.prompt + " " + handoff.command) | color(Color::Cyan));
    }
  }
  for (const auto& line : state.transcript_lines) {
    const bool is_prompt = line.rfind("tsh>", 0) == 0;
    transcript.push_back(text(line) | color(is_prompt ? Color::Cyan : Color::White));
  }

  Elements status = {
      text("Profile: " +
           shell_tui_status_value(context.profile_summary, handoff.profile_summary)) |
          color(Color::Green),
      text("Storage: " +
           shell_tui_status_value(context.storage_binding_name, handoff.storage_binding_name)),
      text("Display: " +
           shell_tui_status_value(context.display_binding_name, handoff.display_binding_name)),
      text("Session Commands: " + std::to_string(context.command_count)),
      text("Durable Refs: " + std::to_string(context.durable_ref_count)),
      text(std::string("Durable Anchor: ") +
           (context.durable_anchor_present ? "present" : "missing")) |
          color(context.durable_anchor_present ? Color::Green : Color::Yellow),
      text("Recovered: " + std::to_string(context.recovered_entries) + " block(s)"),
      text("Glyphs: " + std::to_string(context.rendered_glyphs)),
      text("CanonFS: " + shell_tui_canonfs_value(context, handoff)) |
          color(Color::Yellow),
  };
  if (handoff.present) {
    status.push_back(text("Handoff: " + handoff.source) | color(Color::Cyan));
  }

  Elements commands;
  for (const auto& spec : t81::ternaryos::kShellCommandCatalog) {
    if (!t81::ternaryos::shell_command_visible(
            spec, t81::ternaryos::ShellSurface::HostedPhase5)) {
      continue;
    }
    commands.push_back(text(spec.name) | color(Color::Magenta));
  }

  Elements framebuffer;
  for (const auto& line : split_lines(state.framebuffer_ascii)) {
    framebuffer.push_back(text(line) | color(Color::GrayLight));
  }

  auto header = hbox({
      text(" Axion Shell ") | bold | color(Color::Black) | bgcolor(Color::Cyan),
      text("  v0.1.0-alpha  Phase 5 typed shell TUI ") | color(Color::White),
      filler(),
      text(" type command  enter run  backspace edit  q quit ") | color(Color::GrayDark),
  });

  auto input_panel = window(
      text(" Command ") | bold | color(Color::Cyan),
      hbox({
          text("tsh> ") | color(Color::Cyan),
          text(command_buffer.empty() ? "<type a command>" : command_buffer) |
              color(command_buffer.empty() ? Color::GrayDark : Color::White),
      }));

  auto transcript_panel = window(
      text(" Transcript ") | bold | color(Color::Cyan),
      vbox(std::move(transcript)) | flex);

  auto status_panel = window(
      text(" Session ") | bold | color(Color::Green),
      vbox({
          vbox(std::move(status)) | flex,
          separator(),
          text(status_line) | color(Color::Yellow),
      }) | flex);

  auto commands_panel = window(
      text(" Builtins ") | bold | color(Color::Magenta),
      vbox(std::move(commands)) | flex);

  auto framebuffer_panel = window(
      text(" Framebuffer Preview ") | bold | color(Color::Yellow),
      vbox(std::move(framebuffer)) | flex);

  return vbox({
             header,
             separator(),
             hbox({
                 transcript_panel | flex,
                 vbox({
                     input_panel | size(HEIGHT, LESS_THAN, 3),
                     status_panel | flex,
                     commands_panel | size(HEIGHT, LESS_THAN, 9),
                 }) | size(WIDTH, GREATER_THAN, 34),
             }) | flex,
             framebuffer_panel | size(HEIGHT, LESS_THAN, 12),
         }) |
         border;
}

}  // namespace

int main(int argc, char** argv) {
  const auto handoff = shell_tui_handoff_from_env();
  auto scripted = t81::ternaryos::build_scripted_shell_session(true);
  if (!scripted.has_value()) {
    std::fputs("build_scripted_shell_session failed\n", stderr);
    return 1;
  }

  if (argc > 1 && std::string(argv[1]) == "--snapshot") {
    t81::ternaryos::ShellCommandContext snapshot_context{};
    snapshot_context.surface = t81::ternaryos::ShellSurface::HostedPhase5;
    snapshot_context.profile_summary = scripted->profile_summary.c_str();
    snapshot_context.storage_binding_name = scripted->storage_binding_name.c_str();
    snapshot_context.display_binding_name = scripted->display_binding_name.c_str();
    snapshot_context.durable_anchor_present = scripted->durable_anchor_present;
    snapshot_context.command_count = scripted->session_command_count;
    snapshot_context.durable_ref_count = scripted->durable_ref_count;
    snapshot_context.recovered_entries = scripted->recovered_entries;
    snapshot_context.rendered_glyphs = scripted->rendered_glyphs;
    snapshot_context.canonfs_mode_summary = "persistent (CanonStore-backed)";
    snapshot_context.has_hosted_session_status = true;
    snapshot_context.has_canonfs_status = true;
    auto screen = Screen::Create(Dimension::Fixed(100), Dimension::Fixed(30));
    Render(screen, shell_tui_document(
                       *scripted,
                       snapshot_context,
                       handoff,
                       handoff.present ? std::string() : "help",
                       handoff.present ? handoff.status_line : "snapshot replay"));
    std::printf("%s", screen.ToString().c_str());
    return 0;
  }

  auto session = t81::ternaryos::ShellSession::create(true);
  if (!session.has_value()) {
    std::fputs("ShellSession::create failed\n", stderr);
    return 1;
  }
  t81::ternaryos::ShellBackend& backend = *session;

  std::string command_buffer = handoff.present ? std::string() : "help";
  std::string status_line = handoff.present ? handoff.status_line : "ready";

  auto renderer = Renderer([&] {
    return shell_tui_document(
        backend.state(), backend.command_context(), handoff, command_buffer, status_line);
  });
  auto screen = ScreenInteractive::Fullscreen();
  auto app = CatchEvent(renderer, [&](Event event) {
    if (event == Event::Character('q') || event == Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    if (event == Event::Backspace) {
      if (!command_buffer.empty()) command_buffer.pop_back();
      return true;
    }
    if (event == Event::Return) {
      const auto submitted = command_buffer;
      if (backend.execute_command(submitted)) {
        status_line = submitted.empty() ? "executed: <empty>" : "executed: " + submitted;
      } else {
        status_line = submitted.empty() ? "execution failed: <empty>"
                                        : "execution failed: " + submitted;
      }
      command_buffer.clear();
      return true;
    }
    if (event.is_character()) {
      const auto chars = event.character();
      if (chars.size() == 1 && chars[0] >= 32 && chars[0] != 127) {
        command_buffer += chars;
        return true;
      }
    }
    if (event == Event::ArrowUp) {
      command_buffer = "history";
      return true;
    }
    if (event == Event::ArrowDown) {
      command_buffer = "profile";
      return true;
    }
    return false;
  });

  screen.Loop(app);
  return 0;
}
