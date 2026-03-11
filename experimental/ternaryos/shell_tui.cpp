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
                           std::size_t selected_command,
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
      text("Recovered: " + std::to_string(state.recovered_entries) + " block(s)"),
      text("Glyphs: " + std::to_string(state.rendered_glyphs)),
      text("Backend: guest-bootstrap + CanonStore + VMSVGA") | color(Color::Yellow),
  };

  Elements commands;
  for (std::size_t i = 0; i < state.available_commands.size(); ++i) {
    auto command = text(state.available_commands[i]);
    if (i == selected_command) {
      command = command | bold | color(Color::Black) | bgcolor(Color::Magenta);
    } else {
      command = command | color(Color::Magenta);
    }
    commands.push_back(command);
  }

  Elements framebuffer;
  for (const auto& line : split_lines(state.framebuffer_ascii)) {
    framebuffer.push_back(text(line) | color(Color::GrayLight));
  }

  auto header = hbox({
      text(" Axion Shell ") | bold | color(Color::Black) | bgcolor(Color::Cyan),
      text("  Phase 5 builtins TUI ") | color(Color::White),
      filler(),
      text(" arrows/jk move  enter run  q quit ") | color(Color::GrayDark),
  });

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
    Render(screen, shell_tui_document(*scripted, 0, "snapshot replay"));
    std::printf("%s", screen.ToString().c_str());
    return 0;
  }

  auto session = t81::ternaryos::ShellSession::create(true);
  if (!session.has_value()) {
    std::fputs("ShellSession::create failed\n", stderr);
    return 1;
  }

  std::size_t selected_command = 0;
  std::string status_line = "ready";

  auto renderer = Renderer([&] {
    return shell_tui_document(session->state(), selected_command, status_line);
  });
  auto screen = ScreenInteractive::Fullscreen();
  auto app = CatchEvent(renderer, [&](Event event) {
    const auto command_count = session->state().available_commands.size();
    if (event == Event::Character('q') || event == Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    if (command_count == 0) return false;
    if (event == Event::ArrowUp || event == Event::Character('k')) {
      selected_command =
          selected_command == 0 ? command_count - 1 : selected_command - 1;
      return true;
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
      selected_command = (selected_command + 1) % command_count;
      return true;
    }
    if (event == Event::Return || event == Event::Character(' ')) {
      const auto& command = session->state().available_commands[selected_command];
      if (session->execute_command(command)) {
        status_line = "executed: " + command;
      } else {
        status_line = "execution failed: " + command;
      }
      return true;
    }
    return false;
  });

  screen.Loop(app);
  return 0;
}
