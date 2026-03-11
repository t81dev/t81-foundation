// experimental/ternaryos/shell_demo.cpp

#include "shell_session.hpp"

#include <cstdio>

int main() {
  std::puts("=== Axion v0.1.0-alpha Phase 5 Shell Demo ===");

  const auto state = t81::ternaryos::build_scripted_shell_session(false);
  if (!state.has_value()) {
    std::fputs("build_scripted_shell_session failed\n", stderr);
    return 1;
  }

  std::printf("Shell: stored transcript through %s\n",
              state->storage_binding_name.c_str());
  std::printf("Shell: recovered %zu entry(ies), rendered %zu glyphs through %s\n",
              state->recovered_entries,
              state->rendered_glyphs,
              state->display_binding_name.c_str());
  std::puts("Shell framebuffer:");
  std::puts(state->framebuffer_ascii.c_str());
  return 0;
}
