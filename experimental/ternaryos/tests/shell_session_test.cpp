// experimental/ternaryos/tests/shell_session_test.cpp
//
// Phase 5 acceptance tests: Axion Shell typed command execution and durable
// history over the hosted VirtualBox guest bootstrap seam.

#include "../shell/shell_session.hpp"

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

  check(state->available_commands.size() == 21, "twenty-one builtins are exposed");
  check(state->command_records.size() == 6, "scripted session records six commands");
  check(state->command_records[2].result.starts_with("session profile "),
        "scripted session status reports shell state");
  check(state->recovered_entries == 1, "history recovers one durable entry");
  check(state->session_command_count == 6, "scripted session tracks session command count");
  check(state->durable_ref_count == 1, "scripted session tracks one durable ref");
  check(state->durable_anchor_present, "scripted session reports durable anchor present");
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
  check(session->execute_command("show profile"), "show profile executes");
  check(session->execute_command("show session"), "show session executes");
  check(session->execute_command("store put \"typed shell payload\""), "quoted store put <text> executes");
  const auto stored_ref = suffix_after(session->state().command_records[4].result, "canon durable ok ");
  check(!stored_ref.empty(), "store put result includes a CanonRef");
  check(session->execute_command("session show durable"), "session show durable executes");
  check(session->execute_command("session refs"), "session refs executes");
  check(session->execute_command("session checkpoint"), "session checkpoint executes");
  const auto checkpoint_ref =
      suffix_after(session->state().command_records[7].result, "session checkpoint ok ");
  check(!checkpoint_ref.empty(), "session checkpoint result includes a CanonRef");
  check(session->execute_command("session export"), "session export executes");
  const auto export_ref =
      suffix_after(session->state().command_records[8].result, "session export ok ");
  check(!export_ref.empty(), "session export result includes a CanonRef");
  check(session->execute_command(std::string("history show object ") + checkpoint_ref),
        "history show object <canonref> executes");
  check(session->execute_command(std::string("show ref ") + checkpoint_ref),
        "show ref on checkpoint <canonref> executes");
  check(session->execute_command("store ls"), "store ls executes");
  check(session->execute_command(std::string("store get ") + stored_ref), "store get <ref> executes");
  check(session->execute_command(std::string("show ref ") + stored_ref), "show ref <canonref> executes");
  check(session->execute_command(std::string("store put ref ") + stored_ref), "store put ref <ref> executes");
  const auto copied_ref = suffix_after(session->state().command_records[14].result, "canon durable ref ok ");
  check(!copied_ref.empty(), "store put ref result includes a CanonRef");
  check(session->execute_command(std::string("store cp ") + stored_ref), "store cp <ref> executes");
  const auto cp_ref = suffix_after(session->state().command_records[15].result, "store cp ok ");
  check(!cp_ref.empty(), "store cp result includes a CanonRef");
  check(session->execute_command(std::string("store get ") + copied_ref), "store get copied ref executes");
  check(session->execute_command("history show session"), "history show session executes");
  check(session->execute_command("history show durable"), "history show durable executes");
  check(session->execute_command(std::string("store rm ") + stored_ref), "store rm original <ref> executes");
  check(session->execute_command(std::string("store get ") + stored_ref),
        "store get reports missing after store rm");
  check(session->execute_command(std::string("show ref ") + stored_ref),
        "show ref reports missing after store rm");
  check(session->execute_command("history"), "history executes");
  check(session->execute_command("history show durable"), "history show durable reports missing after removal");
  check(session->execute_command("store put \"unterminated"), "parse errors still refresh state");
  check(session->execute_command("bogus"), "unknown command still refreshes state");

  const auto& state_before_clear = session->state();
  check(state_before_clear.command_records.size() == 26, "interactive session records twenty-six commands");
  check(state_before_clear.command_records[1].result.starts_with("session profile "),
        "session status reports profile and shell metadata");
  check(state_before_clear.command_records[2].result == "show profile\n" + state_before_clear.profile_summary,
        "show profile reports object-native profile view");
  check(state_before_clear.command_records[3].result.starts_with("show session\nprofile "),
        "show session reports object-native session view");
  check(state_before_clear.command_records[4].command == "store put \"typed shell payload\"",
        "typed parser preserves quoted store put command");
  check(state_before_clear.command_records[4].result.starts_with("canon durable ok "),
        "store put reports durable success plus CanonRef");
  check(state_before_clear.command_records[5].result.find("anchor present") != std::string::npos,
        "session show durable reports durable anchor state");
  check(state_before_clear.command_records[5].result.find(stored_ref) != std::string::npos,
        "session show durable exposes the tracked CanonRef");
  check(state_before_clear.command_records[6].result.starts_with("session refs 1"),
        "session refs reports one tracked ref");
  check(state_before_clear.command_records[6].result.find(stored_ref) != std::string::npos,
        "session refs exposes the stored CanonRef");
  check(state_before_clear.command_records[7].result.starts_with("session checkpoint ok "),
        "session checkpoint reports durable success");
  check(state_before_clear.command_records[7].result.find(checkpoint_ref) != std::string::npos,
        "session checkpoint returns a CanonRef");
  check(state_before_clear.command_records[8].result.starts_with("session export ok "),
        "session export reports durable success");
  check(state_before_clear.command_records[8].result.find(export_ref) != std::string::npos,
        "session export returns a CanonRef");
  check(export_ref == checkpoint_ref,
        "session export reuses checkpoint identity for identical transcript");
  check(state_before_clear.command_records[9].result.starts_with("history object " + checkpoint_ref),
        "history show object exposes the checkpoint CanonRef");
  check(state_before_clear.command_records[9].result.find("SESSION TRANSCRIPT") != std::string::npos,
        "history show object exposes the persisted transcript");
  check(state_before_clear.command_records[10].result.starts_with("show ref " + checkpoint_ref),
        "show ref exposes the checkpoint CanonRef");
  check(state_before_clear.command_records[10].result.find("SESSION TRANSCRIPT") != std::string::npos,
        "show ref on checkpoint exposes the persisted transcript");
  check(state_before_clear.command_records[11].result.starts_with("store refs 2"),
        "store ls reports both tracked refs");
  check(state_before_clear.command_records[11].result.find(stored_ref) != std::string::npos,
        "store ls exposes the stored payload CanonRef");
  check(state_before_clear.command_records[11].result.find(checkpoint_ref) != std::string::npos,
        "store ls exposes the checkpoint CanonRef");
  check(state_before_clear.command_records[12].result == "store get typed shell payload",
        "store get decodes stored payload");
  check(state_before_clear.command_records[13].result.starts_with("show ref " + stored_ref),
        "show ref exposes the requested CanonRef");
  check(state_before_clear.command_records[13].result.find("typed shell payload") != std::string::npos,
        "show ref exposes the durable payload");
  check(state_before_clear.command_records[14].result.starts_with("canon durable ref ok "),
        "store put ref reports durable success");
  check(state_before_clear.command_records[14].result.find(copied_ref) != std::string::npos,
        "store put ref returns a CanonRef");
  check(copied_ref == stored_ref,
        "store put ref preserves canonical identity for identical payload");
  check(state_before_clear.command_records[15].result.starts_with("store cp ok "),
        "store cp reports durable success");
  check(state_before_clear.command_records[15].result.find(cp_ref) != std::string::npos,
        "store cp returns a CanonRef");
  check(cp_ref == stored_ref,
        "store cp preserves canonical identity for identical payload");
  check(state_before_clear.command_records[16].result == "store get typed shell payload",
        "store get on copied ref decodes stored payload");
  check(state_before_clear.command_records[17].result.starts_with("history session 17"),
        "history show session reports the current session window");
  check(state_before_clear.command_records[17].result.find("store cp " + stored_ref) != std::string::npos,
        "history show session includes object-copy command");
  check(state_before_clear.command_records[17].result.find("store put ref " + stored_ref) != std::string::npos,
        "history show session includes object-composition command");
  check(state_before_clear.command_records[18].result.starts_with("history durable " + copied_ref),
        "history show durable exposes the durable anchor ref");
  check(state_before_clear.command_records[18].result.find("typed shell payload") != std::string::npos,
        "history show durable exposes the durable payload");
  check(state_before_clear.command_records[19].result == "store rm ok " + stored_ref,
        "store rm reports durable removal");
  check(state_before_clear.command_records[20].result == "store get missing",
        "removed ref is no longer readable");
  check(state_before_clear.command_records[21].result == "show ref missing",
        "show ref reports missing after removal");
  check(state_before_clear.command_records[22].result == "reboot history missing",
        "history reports missing durable anchor after removal");
  check(state_before_clear.command_records[23].result == "history durable missing",
        "history show durable reports missing anchor after removal");
  check(state_before_clear.command_records[24].result == "parse error: unmatched quote",
        "unmatched quote is surfaced as a parse error");
  check(state_before_clear.command_records[25].result == "unknown command", "unknown command is surfaced");
  check(state_before_clear.recovered_entries == 1, "interactive history refresh tracks current recovered_entries");
  check(state_before_clear.session_command_count == 26,
        "interactive state tracks session command count before clear");
  check(state_before_clear.durable_ref_count == 1,
        "interactive state tracks checkpoint ref after store rm");
  check(!state_before_clear.durable_anchor_present,
        "interactive state reports durable anchor missing after store rm");

  check(session->execute_command("clear"), "clear executes");
  check(session->execute_command("session status"), "session status executes after clear");
  check(session->execute_command("show profile"), "show profile executes after clear");
  check(session->execute_command("show session"), "show session executes after clear");
  check(session->execute_command("session show durable"), "session show durable executes after clear");
  check(session->execute_command("session refs"), "session refs executes after clear");
  check(session->execute_command("history show session"), "history show session executes after clear");
  check(session->execute_command("history show durable"), "history show durable executes after clear");

  const auto& state_after_clear = session->state();
  check(state_after_clear.command_records.size() == 8, "clear resets transcript to the new session window");
  check(state_after_clear.command_records[0].result == "session transcript cleared",
        "clear reports transcript reset");
  check(state_after_clear.command_records[1].result.find("commands 1") != std::string::npos,
        "post-clear session status sees one prior session command");
  check(state_after_clear.command_records[2].result == "show profile\n" + state_after_clear.profile_summary,
        "post-clear show profile reflects the current profile");
  check(state_after_clear.command_records[3].result.starts_with("show session\nprofile "),
        "post-clear show session reflects the new session window");
  check(state_after_clear.command_records[4].result.find("refs 1") != std::string::npos,
        "post-clear session show durable sees retained checkpoint ref");
  check(state_after_clear.command_records[4].result.find("anchor missing") != std::string::npos,
        "post-clear session show durable sees missing anchor");
  check(state_after_clear.command_records[5].result.starts_with("session refs 1"),
        "post-clear session refs shows retained checkpoint ref");
  check(state_after_clear.command_records[5].result.find(checkpoint_ref) != std::string::npos,
        "post-clear session refs exposes the checkpoint ref");
  check(state_after_clear.command_records[6].result.starts_with("history session 6"),
        "post-clear history show session reflects the new session window");
  check(state_after_clear.command_records[7].result == "history durable missing",
        "post-clear history show durable still sees missing durable anchor");
  check(state_after_clear.session_command_count == 8,
        "post-clear state tracks new session command window");
  check(state_after_clear.transcript_text.find("UNKNOWN COMMAND") == std::string::npos,
        "cleared transcript no longer shows pre-clear shell history");
  check(state_after_clear.transcript_text.find("SESSION TRANSCRIPT") != std::string::npos,
        "transcript keeps the session header after clear");
}

int main() {
  std::printf("== Axion Shell Session Test ==\n");
  test_scripted_shell_session();
  test_typed_shell_commands();

  std::printf("\nSummary: %d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
