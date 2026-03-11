// experimental/ternaryos/tests/shell_session_test.cpp
//
// Phase 5 acceptance tests: Axion Shell typed command execution and durable
// history over the hosted VirtualBox guest bootstrap seam.

#include "../shell_session.hpp"

#include <cstdio>
#include <optional>
#include <string>

static int g_pass = 0;
static int g_fail = 0;

static bool check(bool cond, const char* label) {
  if (cond) {
    std::printf("  PASS  %s\n", label);
    ++g_pass;
  } else {
    std::printf("  FAIL  %s\n", label);
    ++g_fail;
  }
  return cond;
}

static void test_scripted_shell_session() {
  std::printf("\n[S1] scripted shell session durable history\n");

  auto state = t81::ternaryos::build_scripted_shell_session(true);
  check(state.has_value(), "scripted shell session builds");
  if (!state.has_value()) return;

  check(state->available_commands.size() == 4, "four builtins are exposed");
  check(state->command_records.size() == 4, "scripted session records four commands");
  check(state->recovered_entries == 1, "history recovers one durable entry");
  check(state->transcript_text.find("REBOOT RECOVERED 1") != std::string::npos,
        "transcript records durable history recovery");
  check(state->framebuffer_ascii.find('+') != std::string::npos,
        "framebuffer preview contains rendered glyph data");
}

static void test_typed_shell_commands() {
  std::printf("\n[S2] typed shell command execution\n");

  auto session = t81::ternaryos::ShellSession::create(true);
  check(session.has_value(), "interactive shell session creates");
  if (!session.has_value()) return;

  check(session->execute_command("help"), "help executes");
  check(session->execute_command("store put typed shell payload"), "store put <text> executes");
  check(session->execute_command("history"), "history executes");
  check(session->execute_command("bogus"), "unknown command still refreshes state");

  const auto& state = session->state();
  check(state.command_records.size() == 4, "interactive session records four commands");
  check(state.command_records[1].command == "store put typed shell payload",
        "typed parser preserves the full store put command");
  check(state.command_records[1].result == "canon durable ok", "store put reports durable success");
  check(state.command_records[2].result == "reboot recovered 1", "history reports durable recovery");
  check(state.command_records[3].result == "unknown command", "unknown command is surfaced");
  check(state.recovered_entries == 1, "interactive history refresh sets recovered_entries");
}

int main() {
  std::printf("== Axion Shell Session Test ==\n");
  test_scripted_shell_session();
  test_typed_shell_commands();

  std::printf("\nSummary: %d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
