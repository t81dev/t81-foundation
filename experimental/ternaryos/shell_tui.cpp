// experimental/ternaryos/shell_tui.cpp

#include "shell_session.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>

#include <cstdio>
#include <string>
#include <vector>

using namespace ftxui;

namespace {

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
                           const std::string& command_buffer,
                           const std::string& status_line) {
  Elements transcript;
  for (const auto& line : state.transcript_lines) {
    const bool is_prompt = line.rfind("tsh>", 0) == 0;
    transcript.push_back(text(line) | color(is_prompt ? Color::Cyan : Color::White));
  }

  Elements status = {
      text("Profile: " + state.profile_summary) | color(Color::Green),
      text("Storage: " + state.storage_binding_name),
      text("Display: " + state.display_binding_name),
      text("Session Commands: " + std::to_string(state.session_command_count)),
      text("Durable Refs: " + std::to_string(state.durable_ref_count)),
      text(std::string("Durable Anchor: ") +
           (state.durable_anchor_present ? "present" : "missing")) |
          color(state.durable_anchor_present ? Color::Green : Color::Yellow),
      text("Recovered: " + std::to_string(state.recovered_entries) + " block(s)"),
      text("Glyphs: " + std::to_string(state.rendered_glyphs)),
      text("Backend: guest-bootstrap + CanonStore + VMSVGA") | color(Color::Yellow),
  };

  Elements commands;
  for (const auto& command : state.available_commands) {
    commands.push_back(text(command) | color(Color::Magenta));
  }

  Elements framebuffer;
  for (const auto& line : split_lines(state.framebuffer_ascii)) {
    framebuffer.push_back(text(line) | color(Color::GrayLight));
  }

  auto header = hbox({
      text(" Axion Shell ") | bold | color(Color::Black) | bgcolor(Color::Cyan),
      text("  Phase 5 typed shell TUI ") | color(Color::White),
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
  auto scripted = t81::ternaryos::build_scripted_shell_session(true);
  if (!scripted.has_value()) {
    std::fputs("build_scripted_shell_session failed\n", stderr);
    return 1;
  }

  if (argc > 1 && std::string(argv[1]) == "--snapshot") {
    auto screen = Screen::Create(Dimension::Fixed(100), Dimension::Fixed(30));
    Render(screen, shell_tui_document(*scripted, "help", "snapshot replay"));
    std::printf("%s", screen.ToString().c_str());
    return 0;
  }

  auto session = t81::ternaryos::ShellSession::create(true);
  if (!session.has_value()) {
    std::fputs("ShellSession::create failed\n", stderr);
    return 1;
  }

  std::string command_buffer = "help";
  std::string status_line = "ready";

  auto renderer = Renderer([&] {
    return shell_tui_document(session->state(), command_buffer, status_line);
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
      if (session->execute_command(submitted)) {
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
