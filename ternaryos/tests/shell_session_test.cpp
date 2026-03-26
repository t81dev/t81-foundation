// experimental/ternaryos/tests/shell_session_test.cpp
//
// Phase 5 acceptance tests: Axion Shell typed command execution and durable
// history over the hosted VirtualBox guest bootstrap seam.

#include "t81/axion/shell/shell_session.hpp"

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

  check(state->available_commands.size() == 13, "RFC-00B9 product commands are exposed");
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
  check(session->execute_command("uname"), "uname executes");
  check(session->execute_command("version"), "version executes");
  check(session->execute_command("policy"), "policy executes");
  check(session->execute_command("canonfs"), "canonfs executes");
  check(session->execute_command("session status"), "session status executes");
  check(session->execute_command("show profile"), "show profile executes");
  check(session->execute_command("show session"), "show session executes");
  check(session->execute_command("store put \"typed shell payload\""), "quoted store put <text> executes");
  const auto stored_ref = suffix_after(session->state().command_records[8].result, "canon durable ok ");
  check(!stored_ref.empty(), "store put result includes a CanonRef");
  check(session->execute_command("session show durable"), "session show durable executes");
  check(session->execute_command("session refs"), "session refs executes");
  check(session->execute_command("session checkpoint"), "session checkpoint executes");
  const auto checkpoint_ref =
      suffix_after(session->state().command_records[11].result, "session checkpoint ok ");
  check(!checkpoint_ref.empty(), "session checkpoint result includes a CanonRef");
  check(session->execute_command("session export"), "session export executes");
  const auto export_ref =
      suffix_after(session->state().command_records[12].result, "session export ok ");
  check(!export_ref.empty(), "session export result includes a CanonRef");
  check(session->execute_command(std::string("session diff ") + checkpoint_ref),
        "session diff <ref> executes");
  check(session->execute_command(std::string("history show object ") + checkpoint_ref),
        "history show object <canonref> executes");
  check(session->execute_command(std::string("show ref ") + checkpoint_ref),
        "show ref on checkpoint <canonref> executes");
  check(session->execute_command("store ls"), "store ls executes");
  check(session->execute_command(std::string("store get ") + stored_ref), "store get <ref> executes");
  check(session->execute_command(std::string("show ref ") + stored_ref), "show ref <canonref> executes");
  check(session->execute_command(std::string("store put ref ") + stored_ref), "store put ref <ref> executes");
  const auto copied_ref = suffix_after(session->state().command_records[19].result, "canon durable ref ok ");
  check(!copied_ref.empty(), "store put ref result includes a CanonRef");
  check(session->execute_command(std::string("store cp ") + stored_ref), "store cp <ref> executes");
  const auto cp_ref = suffix_after(session->state().command_records[20].result, "store cp ok ");
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
  check(session->execute_command(std::string("history use ") + checkpoint_ref),
        "history use <ref> executes");
  check(session->execute_command("history show durable"),
        "history show durable executes after history use");
  check(session->execute_command("store put \"unterminated"), "parse errors still refresh state");
  check(session->execute_command("bogus"), "unknown command still refreshes state");

  const auto& state_before_clear = session->state();
  check(state_before_clear.command_records.size() == 33, "interactive session records thirty-three commands");
  check(state_before_clear.command_records[1].result == "T81 TernaryOS 1.0 hosted axion-shell",
        "uname reports hosted shell identity");
  check(state_before_clear.command_records[2].result.starts_with("T81 / Axion  --  hosted shell session"),
        "version reports hosted shell build path");
  check(state_before_clear.command_records[3].result.starts_with("[axion policy]\n"),
        "policy reports hosted shell governance summary");
  check(state_before_clear.command_records[3].result.find("audit trail : canonfs-backed hosted session") != std::string::npos,
        "policy reports hosted shell audit trail");
  check(state_before_clear.command_records[4].result.starts_with("[canonfs]\n"),
        "canonfs reports hosted shell storage summary");
  check(state_before_clear.command_records[4].result.find("binding shell-demo-ahci") != std::string::npos,
        "canonfs reports hosted shell storage binding");
  check(state_before_clear.command_records[5].result.starts_with("session profile "),
        "session status reports profile and shell metadata");
  check(state_before_clear.command_records[6].result == "show profile\n" + state_before_clear.profile_summary,
        "show profile reports object-native profile view");
  check(state_before_clear.command_records[7].result.starts_with("show session\nprofile "),
        "show session reports object-native session view");
  check(state_before_clear.command_records[8].command == "store put \"typed shell payload\"",
        "typed parser preserves quoted store put command");
  check(state_before_clear.command_records[8].result.starts_with("canon durable ok "),
        "store put reports durable success plus CanonRef");
  check(state_before_clear.command_records[9].result.find("anchor present") != std::string::npos,
        "session show durable reports durable anchor state");
  check(state_before_clear.command_records[9].result.find(stored_ref) != std::string::npos,
        "session show durable exposes the tracked CanonRef");
  check(state_before_clear.command_records[10].result.starts_with("session refs 1"),
        "session refs reports one tracked ref");
  check(state_before_clear.command_records[10].result.find(stored_ref) != std::string::npos,
        "session refs exposes the stored CanonRef");
  check(state_before_clear.command_records[11].result.starts_with("session checkpoint ok "),
        "session checkpoint reports durable success");
  check(state_before_clear.command_records[11].result.find(checkpoint_ref) != std::string::npos,
        "session checkpoint returns a CanonRef");
  check(state_before_clear.command_records[12].result.starts_with("session export ok "),
        "session export reports durable success");
  check(state_before_clear.command_records[12].result.find(export_ref) != std::string::npos,
        "session export returns a CanonRef");
  check(export_ref == checkpoint_ref,
        "session export reuses checkpoint identity for identical transcript");
  check(state_before_clear.command_records[13].result.starts_with("session diff mismatch"),
        "session diff reports transcript mismatch before clear");
  check(state_before_clear.command_records[13].result.find("current>") != std::string::npos,
        "session diff reports the current transcript side");
  check(state_before_clear.command_records[13].result.find("durable>") != std::string::npos,
        "session diff reports the durable transcript side");
  check(state_before_clear.command_records[14].result.starts_with("history object " + checkpoint_ref),
        "history show object exposes the checkpoint CanonRef");
  check(state_before_clear.command_records[14].result.find("SESSION TRANSCRIPT") != std::string::npos,
        "history show object exposes the persisted transcript");
  check(state_before_clear.command_records[15].result.starts_with("show ref " + checkpoint_ref),
        "show ref exposes the checkpoint CanonRef");
  check(state_before_clear.command_records[15].result.find("SESSION TRANSCRIPT") != std::string::npos,
        "show ref on checkpoint exposes the persisted transcript");
  check(state_before_clear.command_records[16].result.starts_with("store refs 2"),
        "store ls reports both tracked refs");
  check(state_before_clear.command_records[16].result.find(stored_ref) != std::string::npos,
        "store ls exposes the stored payload CanonRef");
  check(state_before_clear.command_records[16].result.find(checkpoint_ref) != std::string::npos,
        "store ls exposes the checkpoint CanonRef");
  check(state_before_clear.command_records[17].result == "store get typed shell payload",
        "store get decodes stored payload");
  check(state_before_clear.command_records[18].result.starts_with("show ref " + stored_ref),
        "show ref exposes the requested CanonRef");
  check(state_before_clear.command_records[18].result.find("typed shell payload") != std::string::npos,
        "show ref exposes the durable payload");
  check(state_before_clear.command_records[19].result.starts_with("canon durable ref ok "),
        "store put ref reports durable success");
  check(state_before_clear.command_records[19].result.find(copied_ref) != std::string::npos,
        "store put ref returns a CanonRef");
  check(copied_ref == stored_ref,
        "store put ref preserves canonical identity for identical payload");
  check(state_before_clear.command_records[20].result.starts_with("store cp ok "),
        "store cp reports durable success");
  check(state_before_clear.command_records[20].result.find(cp_ref) != std::string::npos,
        "store cp returns a CanonRef");
  check(cp_ref == stored_ref,
        "store cp preserves canonical identity for identical payload");
  check(state_before_clear.command_records[21].result == "store get typed shell payload",
        "store get on copied ref decodes stored payload");
  check(state_before_clear.command_records[22].result.starts_with("history session 22"),
        "history show session reports the current session window");
  check(state_before_clear.command_records[22].result.find("store cp " + stored_ref) != std::string::npos,
        "history show session includes object-copy command");
  check(state_before_clear.command_records[22].result.find("store put ref " + stored_ref) != std::string::npos,
        "history show session includes object-composition command");
  check(state_before_clear.command_records[23].result.starts_with("history durable " + copied_ref),
        "history show durable exposes the durable anchor ref");
  check(state_before_clear.command_records[23].result.find("typed shell payload") != std::string::npos,
        "history show durable exposes the durable payload");
  check(state_before_clear.command_records[24].result == "store rm ok " + stored_ref,
        "store rm reports durable removal");
  check(state_before_clear.command_records[25].result == "store get missing",
        "removed ref is no longer readable");
  check(state_before_clear.command_records[26].result == "show ref missing",
        "show ref reports missing after removal");
  check(state_before_clear.command_records[27].result == "reboot history missing",
        "history reports missing durable anchor after removal");
  check(state_before_clear.command_records[28].result == "history durable missing",
        "history show durable reports missing anchor after removal");
  check(state_before_clear.command_records[29].result == "history use ok " + checkpoint_ref,
        "history use rebinds the durable anchor to the checkpoint");
  check(state_before_clear.command_records[30].result.starts_with("history durable " + checkpoint_ref),
        "history show durable exposes rebound checkpoint anchor");
  check(state_before_clear.command_records[30].result.find("SESSION TRANSCRIPT") != std::string::npos,
        "history show durable exposes rebound checkpoint transcript");
  check(state_before_clear.command_records[31].result == "parse error: unmatched quote",
        "unmatched quote is surfaced as a parse error");
  check(state_before_clear.command_records[32].result == "unknown command", "unknown command is surfaced");
  check(state_before_clear.recovered_entries == 1, "interactive history refresh tracks current recovered_entries");
  check(state_before_clear.session_command_count == 33,
        "interactive state tracks session command count before clear");
  check(state_before_clear.durable_ref_count == 1,
        "interactive state tracks checkpoint ref after store rm");
  check(state_before_clear.durable_anchor_present,
        "interactive state reports rebound durable anchor present");

  check(session->execute_command("clear"), "clear executes");
  check(session->execute_command("session status"), "session status executes after clear");
  check(session->execute_command("show profile"), "show profile executes after clear");
  check(session->execute_command("show session"), "show session executes after clear");
  check(session->execute_command("session show durable"), "session show durable executes after clear");
  check(session->execute_command("session refs"), "session refs executes after clear");
  check(session->execute_command("history show session"), "history show session executes after clear");
  check(session->execute_command("history show durable"), "history show durable executes after clear");
  check(session->execute_command(std::string("session import ") + checkpoint_ref),
        "session import <ref> executes after clear");
  check(session->execute_command(std::string("session diff ") + checkpoint_ref),
        "session diff <ref> executes after clear");

  const auto& state_after_clear = session->state();
  check(state_after_clear.command_records.size() == 10, "clear resets transcript to the new session window");
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
  check(state_after_clear.command_records[4].result.find("anchor present") != std::string::npos,
        "post-clear session show durable sees rebound anchor");
  check(state_after_clear.command_records[4].result.find(checkpoint_ref) != std::string::npos,
        "post-clear session show durable exposes rebound checkpoint ref");
  check(state_after_clear.command_records[5].result.starts_with("session refs 1"),
        "post-clear session refs shows retained checkpoint ref");
  check(state_after_clear.command_records[5].result.find(checkpoint_ref) != std::string::npos,
        "post-clear session refs exposes the checkpoint ref");
  check(state_after_clear.command_records[6].result.starts_with("history session 6"),
        "post-clear history show session reflects the new session window");
  check(state_after_clear.command_records[7].result.starts_with("history durable " + checkpoint_ref),
        "post-clear history show durable sees rebound checkpoint anchor");
  check(state_after_clear.command_records[7].result.find("SESSION TRANSCRIPT") != std::string::npos,
        "post-clear history show durable exposes rebound checkpoint transcript");
  check(state_after_clear.command_records[8].result == "session import ok " + checkpoint_ref,
        "session import reports success with the checkpoint ref");
  check(state_after_clear.command_records[9].result.starts_with("session diff mismatch"),
        "session diff reports mismatch after importing plus appending commands");
  check(state_after_clear.command_records[9].result.find("current>") != std::string::npos,
        "post-import session diff reports the current transcript side");
  check(state_after_clear.session_command_count == 10,
        "post-clear state tracks new session command window");
  check(state_after_clear.transcript_text.find("UNKNOWN COMMAND") == std::string::npos,
        "cleared transcript no longer shows pre-clear shell history");
  check(state_after_clear.transcript_text.find("SESSION TRANSCRIPT") != std::string::npos,
        "transcript keeps the session header after clear");
  check(state_after_clear.transcript_text.find("IMPORTED SESSION ACTIVE") != std::string::npos,
        "session import marks imported transcript mode");
  check(state_after_clear.transcript_text.find("SESSION EXPORT") != std::string::npos,
        "session import restores exported transcript content");
  check(state_after_clear.transcript_text.find("tsh> session import ") != std::string::npos,
        "session import appends the new command to the imported transcript");
}

static void test_shell_script_objects() {
  std::printf("\n[S3] durable shell script objects\n");

  auto session = t81::ternaryos::ShellSession::create(true);
  check(session.has_value(), "script session creates");
  if (!session.has_value()) return;

  check(session->execute_command("store put script \"show profile|history\""),
        "store put script executes");
  const auto script_ref =
      suffix_after(session->state().command_records[0].result, "canon script ok ");
  check(!script_ref.empty(), "store put script returns a CanonRef");
  check(session->execute_command(std::string("session run ") + script_ref),
        "session run <ref> executes");

  const auto& state = session->state();
  check(state.command_records.size() == 4, "script run appends the scripted command results");
  check(state.command_records[1].command == "show profile",
        "script run executes first stored command");
  check(state.command_records[1].result == "show profile\n" + state.profile_summary,
        "script run exposes show profile output");
  check(state.command_records[2].command == "history",
        "script run executes second stored command");
  check(state.command_records[2].result == "reboot recovered 0",
        "script run preserves deterministic history output");
  check(state.command_records[3].result == "session run ok " + script_ref + " lines 2",
        "script run reports summary with executed line count");
}

static void test_named_shell_refs() {
  std::printf("\n[S4] named shell refs\n");

  auto session = t81::ternaryos::ShellSession::create(true);
  check(session.has_value(), "named-ref session creates");
  if (!session.has_value()) return;

  check(session->execute_command("store put \"named payload\""),
        "store put executes for named-ref flow");
  const auto stored_ref =
      suffix_after(session->state().command_records[0].result, "canon durable ok ");
  check(!stored_ref.empty(), "named-ref flow gets a CanonRef");

  check(session->execute_command(std::string("name set payload ") + stored_ref),
        "name set <label> <ref> executes");
  check(session->execute_command("name ls"), "name ls executes");
  check(session->execute_command("show ref @payload"), "show ref resolves @label");
  check(session->execute_command("store get @payload"), "store get resolves @label");
  check(session->execute_command("store put ref @payload"), "store put ref resolves @label");

  const auto& state = session->state();
  check(state.named_ref_count == 1, "state tracks one named ref");
  check(state.named_refs.size() == 1, "state exposes named ref list");
  check(state.named_refs[0].label == "payload", "named ref label is retained");
  check(state.named_refs[0].ref.hash.h.to_string() == stored_ref,
        "named ref points at stored CanonRef");
  check(state.command_records[1].result == "name set ok payload " + stored_ref,
        "name set reports success");
  check(state.command_records[2].result.starts_with("name refs 1"),
        "name ls reports one named ref");
  check(state.command_records[2].result.find("payload " + stored_ref) != std::string::npos,
        "name ls exposes label and ref");
  check(state.command_records[3].result.starts_with("show ref " + stored_ref),
        "show ref @label resolves to the stored CanonRef");
  check(state.command_records[3].result.find("named payload") != std::string::npos,
        "show ref @label exposes payload text");
  check(state.command_records[4].result == "store get named payload",
        "store get @label returns payload");
  check(state.command_records[5].result.starts_with("canon durable ref ok "),
        "store put ref @label reports durable success");
}

static void test_named_shell_objects() {
  std::printf("\n[S5] named shell objects\n");

  auto session = t81::ternaryos::ShellSession::create(true);
  check(session.has_value(), "named-object session creates");
  if (!session.has_value()) return;

  check(session->execute_command("store put script \"show profile|history\""),
        "store put script executes for named-object flow");
  const auto script_ref =
      suffix_after(session->state().command_records[0].result, "canon script ok ");
  check(!script_ref.empty(), "named-object flow gets a script CanonRef");

  check(session->execute_command(std::string("object pin script bootstrap ") + script_ref),
        "object pin <kind> <name> <ref> executes");
  check(session->execute_command("object ls"), "object ls executes");
  check(session->execute_command("object show bootstrap"), "object show <name> executes");
  check(session->execute_command("session run @bootstrap"), "session run resolves pinned object alias");

  const auto& state = session->state();
  check(state.named_object_count == 1, "state tracks one named object");
  check(state.named_objects.size() == 1, "state exposes named object list");
  check(state.named_objects[0].kind == "script", "named object kind is retained");
  check(state.named_objects[0].name == "bootstrap", "named object name is retained");
  check(state.named_objects[0].ref.hash.h.to_string() == script_ref,
        "named object points at the script CanonRef");
  check(state.named_ref_count == 1, "named object installs a matching alias");
  check(state.named_refs[0].label == "bootstrap", "named object alias label matches name");
  check(state.command_records[1].result == "object pin ok script bootstrap " + script_ref,
        "object pin reports success");
  check(state.command_records[2].result.starts_with("object refs 1"),
        "object ls reports one object");
  check(state.command_records[2].result.find("script bootstrap " + script_ref) != std::string::npos,
        "object ls exposes kind, name, and ref");
  check(state.command_records[3].result == "object show bootstrap\nkind script\nref " + script_ref,
        "object show reports kind and CanonRef");
  check(state.command_records[4].command == "show profile",
        "session run @bootstrap replays first script command");
  check(state.command_records[5].command == "history",
        "session run @bootstrap replays second script command");
  check(state.command_records[6].result == "session run ok " + script_ref + " lines 2",
        "session run @bootstrap reports successful execution");
}

static void test_rfc00b9_shell_core() {
  std::printf("\n[S6] RFC-00B9 hosted shell core\n");

  auto session = t81::ternaryos::ShellSession::create(true);
  check(session.has_value(), "rfc00b9 session creates");
  if (!session.has_value()) return;

  check(session->execute_command("tier 2"), "tier builtin executes");
  check(session->execute_command("service list"), "service list executes");
  check(session->execute_command("service start studio"), "service start studio executes");
  check(session->execute_command("hash /usr/bin/t81"), "hash on service binary executes");
  check(session->execute_command("store put script \"show profile|history\""),
        "store put script executes for run alias");
  const auto script_ref =
      suffix_after(session->state().command_records[4].result, "canon script ok ");
  check(!script_ref.empty(), "store put script returns a CanonRef");
  check(session->execute_command(std::string("object pin script bootstrap ") + script_ref),
        "object pin bootstrap executes");
  check(session->execute_command("ls"), "ls executes");
  check(session->execute_command("cat @bootstrap"), "cat @bootstrap executes");
  check(session->execute_command(std::string("hash ") + script_ref),
        "hash on CanonFS object executes");
  check(session->execute_command(std::string("run ") + script_ref),
        "run on CanonFS object executes");
  check(session->execute_command("session status"), "session status exposes userenv shell state");

  const auto& records = session->state().command_records;
  check(records[0].result == "tier ok 2", "tier updates hosted session tier");
  check(records[1].result.starts_with("service list "), "service list reports registry inventory");
  check(records[1].result.find("t81-studio on_demand") != std::string::npos,
        "service list exposes studio service");
  check(records[2].result == "service start ok t81-studio",
        "service start routes through userenv activation");
  check(records[3].result == "hash /usr/bin/t81\ncanon_hash deadbeef00000003",
        "hash exposes service canon_hash");
  check(records[5].result == "object pin ok script bootstrap " + script_ref,
        "object pin records named script alias");
  check(records[6].result.starts_with("ls\nentries "), "ls reports RFC-00B9 product inventory");
  check(records[6].result.find("script bootstrap " + script_ref) != std::string::npos,
        "ls exposes named script object");
  check(records[7].result == "cat @bootstrap\nAXION_SCRIPT\nshow profile\nhistory",
        "cat resolves named ref alias and decodes script payload");
  check(records[8].result == "hash " + script_ref + "\ncanon_hash " + script_ref,
        "hash on stored object exposes CanonRef identity");
  check(records[9].result == "run ok " + script_ref,
        "run aliases through session-run path");
  check(records[10].result.find("history canonfs://var/sessions/") != std::string::npos,
        "session status exposes CanonFS history path");
  check(records[10].result.find("tier 2") != std::string::npos,
        "session status exposes current tier");
}

int main() {
  std::printf("== Axion Shell Session Test ==\n");
  test_scripted_shell_session();
  test_typed_shell_commands();
  test_shell_script_objects();
  test_named_shell_refs();
  test_named_shell_objects();
  test_rfc00b9_shell_core();

  std::printf("\nSummary: %d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
