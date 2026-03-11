// experimental/ternaryos/tests/shell_session_test.cpp
//
// Phase 5 acceptance tests: Axion Shell typed command execution and durable
// history over the hosted VirtualBox guest bootstrap seam.

#include "../shell_session.hpp"

#include <cstdio>
#include <optional>
#include <string_view>
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

static std::string suffix_after(std::string_view text, std::string_view prefix) {
  if (!text.starts_with(prefix)) return {};
  return std::string(text.substr(prefix.size()));
}

static void test_scripted_shell_session() {
  std::printf("\n[S1] scripted shell session durable history\n");

  auto state = t81::ternaryos::build_scripted_shell_session(true);
  check(state.has_value(), "scripted shell session builds");
  if (!state.has_value()) return;

  check(state->available_commands.size() == 8, "eight builtins are exposed");
  check(state->command_records.size() == 6, "scripted session records six commands");
  check(state->command_records[2].result.starts_with("session profile "),
        "scripted session status reports shell state");
  check(state->recovered_entries == 1, "history recovers one durable entry");
  check(state->command_records[3].result.starts_with("canon durable ok "),
        "scripted store put emits a CanonRef");
  check(state->command_records[4].result.starts_with("store refs 1"),
        "scripted store ls sees one durable ref");
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
  check(session->execute_command("session status"), "session status executes");
  check(session->execute_command("store put \"typed shell payload\""), "quoted store put <text> executes");
  const auto stored_ref = suffix_after(session->state().command_records[2].result, "canon durable ok ");
  check(!stored_ref.empty(), "store put result includes a CanonRef");
  check(session->execute_command("store ls"), "store ls executes");
  check(session->execute_command(std::string("store get ") + stored_ref), "store get <ref> executes");
  check(session->execute_command(std::string("store rm ") + stored_ref), "store rm <ref> executes");
  check(session->execute_command(std::string("store get ") + stored_ref),
        "store get reports missing after store rm");
  check(session->execute_command("history"), "history executes");
  check(session->execute_command("store put \"unterminated"), "parse errors still refresh state");
  check(session->execute_command("bogus"), "unknown command still refreshes state");

  const auto& state = session->state();
  check(state.command_records.size() == 10, "interactive session records ten commands");
  check(state.command_records[1].result.starts_with("session profile "),
        "session status reports profile and shell metadata");
  check(state.command_records[2].command == "store put \"typed shell payload\"",
        "typed parser preserves quoted store put command");
  check(state.command_records[2].result.starts_with("canon durable ok "),
        "store put reports durable success plus CanonRef");
  check(state.command_records[3].result.starts_with("store refs 1"),
        "store ls reports one tracked ref");
  check(state.command_records[3].result.find(stored_ref) != std::string::npos,
        "store ls exposes the stored CanonRef");
  check(state.command_records[4].result == "store get typed shell payload",
        "store get decodes stored payload");
  check(state.command_records[5].result == "store rm ok " + stored_ref,
        "store rm reports durable removal");
  check(state.command_records[6].result == "store get missing",
        "removed ref is no longer readable");
  check(state.command_records[7].result == "reboot history missing",
        "history reports missing durable anchor after removal");
  check(state.command_records[8].result == "parse error: unmatched quote",
        "unmatched quote is surfaced as a parse error");
  check(state.command_records[9].result == "unknown command", "unknown command is surfaced");
  check(state.recovered_entries == 0, "interactive history refresh tracks current recovered_entries");
}

int main() {
  std::printf("== Axion Shell Session Test ==\n");
  test_scripted_shell_session();
  test_typed_shell_commands();

  std::printf("\nSummary: %d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
