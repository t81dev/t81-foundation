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

Element shell_tui_document(const t81::ternaryos::ShellSessionState& state) {
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
  for (const auto& command : state.available_commands) {
    commands.push_back(text(command) | color(Color::Magenta));
  }

  Elements framebuffer;
  for (const auto& line : split_lines(state.framebuffer_ascii)) {
    framebuffer.push_back(text(line) | color(Color::GrayLight));
  }

  auto header = hbox({
      text(" TernOS Shell ") | bold | color(Color::Black) | bgcolor(Color::Cyan),
      text("  Phase 5 scripted TUI ") | color(Color::White),
      filler(),
      text(" q quit ") | color(Color::GrayDark),
  });

  auto transcript_panel = window(
      text(" Transcript ") | bold | color(Color::Cyan),
      vbox(std::move(transcript)) | flex);

  auto status_panel = window(
      text(" Session ") | bold | color(Color::Green),
      vbox(std::move(status)) | flex);

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
  const auto state = t81::ternaryos::build_scripted_shell_session(true);
  if (!state.has_value()) {
    std::fputs("build_scripted_shell_session failed\n", stderr);
    return 1;
  }

  if (argc > 1 && std::string(argv[1]) == "--snapshot") {
    auto screen = Screen::Create(Dimension::Fixed(100), Dimension::Fixed(30));
    Render(screen, shell_tui_document(*state));
    std::printf("%s", screen.ToString().c_str());
    return 0;
  }

  auto renderer = Renderer([&] { return shell_tui_document(*state); });
  auto screen = ScreenInteractive::Fullscreen();
  auto app = CatchEvent(renderer, [&](Event event) {
    if (event == Event::Character('q') || event == Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(app);
  return 0;
}
