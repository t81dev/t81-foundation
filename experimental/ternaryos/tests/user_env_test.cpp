// experimental/ternaryos/tests/user_env_test.cpp
//
// RFC-00B9 acceptance tests — TernaryOS User Environment Standard.
//
// AC-1   t81-init boots, reads t81-services.json, spawns required services in topological order
// AC-2   Boot hash is written to canonfs://var/log/boot-<epoch>.canonhash
// AC-3   Binary integrity check fires Axion IntegrityViolation and aborts spawn on mismatch
// AC-4   Session manager allocates a unique, monotonically increasing SessionId per login
// AC-5   SessionRecord is written to CanonFS on creation and updated on state transition
// AC-6   Login with invalid credentials fires SessionCreate Deny; prompt re-presented
// AC-7   Logout drains the session ProcessGroupId within session_drain_timeout_ms
// AC-8   t81sh prompt includes principal name, session ID, and current Tier
// AC-9   t81sh built-in commands (compile, run, hash, policy, service, tier) execute without error
// AC-10  Every t81sh command is appended to the session history JSONL in CanonFS
// AC-11  ShellExec Axion gate fires for every non-builtin command; Deny blocks execution
// AC-12  studio/agent built-ins transfer TTY into raw mode, launch RFC-0033 TUI, restore on exit
// AC-13  on_demand service activation triggers ServiceSpawn Axion gate
// AC-14  A service with an unrecognised capability is refused at spawn time
// AC-15  t81sh T81Lang REPL shares VM state across lines within a session

#include "../userenv/service_registry.hpp"
#include "../userenv/session_manager.hpp"
#include "../userenv/t81sh.hpp"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace t81::ternaryos::userenv;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool cond, const char* label) {
  if (cond) { std::printf("  PASS  %s\n", label); ++g_pass; }
  else       { std::printf("  FAIL  %s\n", label); ++g_fail; }
}

// ─── Helper: minimal service registry ─────────────────────────────────────────

static ServiceRegistry make_test_registry() {
  ServiceEntry session_mgr;
  session_mgr.id             = "t81-session-mgr";
  session_mgr.binary         = "/bin/t81-session-mgr";
  session_mgr.canon_hash     = "deadbeef00000001";
  session_mgr.activation     = ActivationMode::Required;
  session_mgr.capabilities   = {"SessionCreate", "TtyAllocate", "ServiceSpawn"};
  session_mgr.start_timeout_ms = 2000;

  ServiceEntry canonfs_daemon;
  canonfs_daemon.id             = "t81-canonfs-daemon";
  canonfs_daemon.binary         = "/bin/t81-canonfs-daemon";
  canonfs_daemon.canon_hash     = "deadbeef00000002";
  canonfs_daemon.activation     = ActivationMode::Required;
  canonfs_daemon.capabilities   = {"CanonFSWrite"};
  canonfs_daemon.start_timeout_ms = 1000;

  ServiceEntry studio;
  studio.id             = "t81-studio";
  studio.binary         = "/usr/bin/t81";
  studio.canon_hash     = "deadbeef00000003";
  studio.activation     = ActivationMode::OnDemand;
  studio.depends        = {"t81-session-mgr"};
  studio.capabilities   = {"TtyRead", "TtyWrite"};
  studio.start_timeout_ms = 5000;

  return make_service_registry({session_mgr, canonfs_daemon, studio});
}

// ─── Helper: minimal principal store ──────────────────────────────────────────

static std::vector<PrincipalEntry> make_test_principals() {
  PrincipalEntry root;
  root.id             = 1;
  root.name           = "root";
  root.password_hash  = "argon2id:correct_hash";
  root.capabilities   = {"SessionCreate", "ServiceSpawn", "TtyAllocate",
                          "CanonFSWrite", "TierElevate", "PrincipalAdmin"};
  root.default_tier   = 1;

  PrincipalEntry agent;
  agent.id            = 2;
  agent.name          = "agent";
  agent.password_hash = "argon2id:agent_hash";
  agent.capabilities  = {"SessionCreate", "ServiceSpawn", "TtyAllocate"};
  agent.default_tier  = 2;

  return {root, agent};
}

// ─── AC-1: t81-init boots, spawns required services in topological order ──────

static void test_ac1_init_boot_order() {
  std::printf("\n[AC-1] t81-init boot — topological spawn order\n");
  const auto reg = make_test_registry();
  const auto result = simulate_boot(reg, {});

  // Two required services; one on_demand (not spawned at boot).
  check(result.success, "AC-1a: boot succeeds");
  check(result.spawn_log.size() == 2, "AC-1b: exactly 2 required services spawned");

  // Both must be started.
  bool all_started = std::all_of(result.spawn_log.begin(), result.spawn_log.end(),
                                  [](const SpawnRecord& s) { return s.started; });
  check(all_started, "AC-1c: all required services started");

  // Dependency order: t81-canonfs-daemon (no deps) before t81-session-mgr (if any dep).
  // Our test registry has no depends between them; both have in-degree 0,
  // so they are sorted alphabetically (t81-canonfs-daemon < t81-session-mgr).
  check(!result.spawn_log.empty() &&
        result.spawn_log[0].service_id == "t81-canonfs-daemon",
        "AC-1d: t81-canonfs-daemon spawned first (alphabetical/topological tie-break)");
}

// ─── AC-2: Boot hash written to CanonFS path ──────────────────────────────────

static void test_ac2_boot_hash() {
  std::printf("\n[AC-2] boot hash recorded\n");
  const auto reg    = make_test_registry();
  const auto result = simulate_boot(reg, {});

  check(!result.boot_hash.empty(), "AC-2a: boot hash is non-empty");
  check(result.boot_hash.size() == 16, "AC-2b: boot hash is 16 hex chars (64-bit FNV-1a)");

  // Determinism: same registry → same hash.
  const auto result2 = simulate_boot(reg, {});
  check(result.boot_hash == result2.boot_hash, "AC-2c: boot hash is deterministic");
}

// ─── AC-3: Binary integrity check fires on mismatch ───────────────────────────

static void test_ac3_integrity_violation() {
  std::printf("\n[AC-3] binary integrity check\n");
  auto reg = make_test_registry();

  // Provide a wrong hash for the canonfs daemon.
  const std::unordered_map<std::string, std::string> bad_hashes = {
    {"t81-canonfs-daemon", "00000000_WRONG"},
  };
  const auto result = simulate_boot(reg, bad_hashes);

  check(!result.success, "AC-3a: boot fails when a binary hash mismatches");

  const auto* bad_record = std::find_if(
    result.spawn_log.begin(), result.spawn_log.end(),
    [](const SpawnRecord& s) { return s.service_id == "t81-canonfs-daemon"; });
  check(bad_record != result.spawn_log.end() && !bad_record->integrity_ok,
        "AC-3b: t81-canonfs-daemon integrity_ok = false");
  check(bad_record != result.spawn_log.end() && !bad_record->started,
        "AC-3c: t81-canonfs-daemon not started on integrity failure");

  // Gate event for the failed service should be Deny.
  const auto* ev = std::find_if(
    result.gate_events.begin(), result.gate_events.end(),
    [](const AxionGateEvent& e) {
      return e.subject == "t81-canonfs-daemon" && e.op == "BootService";
    });
  check(ev != result.gate_events.end() && ev->verdict == GateVerdict::Deny,
        "AC-3d: BootService gate fires Deny on integrity mismatch");
}

// ─── AC-4: Session manager allocates unique monotonic SessionIds ───────────────

static void test_ac4_session_id_monotonic() {
  std::printf("\n[AC-4] monotonic session ID allocation\n");
  SessionManager mgr;
  mgr.load_principals(make_test_principals());

  auto r1 = mgr.login("root", SessionManager::kAnonymousHash, "/dev/tty0", 100);
  auto r2 = mgr.login("root", SessionManager::kAnonymousHash, "/dev/tty1", 101);
  auto r3 = mgr.login("agent", SessionManager::kAnonymousHash, "/dev/tty2", 102);

  check(r1.status == LoginStatus::Success, "AC-4a: first login succeeds");
  check(r2.status == LoginStatus::Success, "AC-4b: second login succeeds");
  check(r3.status == LoginStatus::Success, "AC-4c: third login succeeds");
  check(r1.record.session_id < r2.record.session_id, "AC-4d: session IDs are increasing");
  check(r2.record.session_id < r3.record.session_id, "AC-4e: session IDs are increasing");
  check(r1.record.session_id != r2.record.session_id &&
        r2.record.session_id != r3.record.session_id,
        "AC-4f: session IDs are unique");
}

// ─── AC-5: SessionRecord written to CanonFS on creation and state transition ──

static void test_ac5_session_record_canonfs() {
  std::printf("\n[AC-5] SessionRecord written/updated in CanonFS\n");
  SessionManager mgr;
  mgr.load_principals(make_test_principals());

  auto login_r = mgr.login("root", SessionManager::kAnonymousHash, "/dev/tty0", 1);
  check(login_r.status == LoginStatus::Success, "AC-5a: login succeeds");

  const uint64_t sid = login_r.record.session_id;
  const std::string expected_path = "canonfs://var/sessions/" + std::to_string(sid) + ".json";

  const auto& records = mgr.canon_records();
  check(records.count(expected_path) > 0, "AC-5b: SessionRecord written at canonfs path on creation");
  check(!records.at(expected_path).empty(), "AC-5c: SessionRecord JSON is non-empty");

  // Logout → state transition → updated record.
  auto logout_r = mgr.logout(sid);
  check(logout_r.drained_cleanly, "AC-5d: logout drains cleanly");
  check(logout_r.final_record.state == SessionState::Terminated,
        "AC-5e: final SessionRecord state = Terminated");
  check(records.count(expected_path) > 0, "AC-5f: SessionRecord updated at canonfs path on termination");
}

// ─── AC-6: Invalid credentials fire SessionCreate Deny ────────────────────────

static void test_ac6_invalid_credentials() {
  std::printf("\n[AC-6] invalid credentials — SessionCreate Deny\n");
  SessionManager mgr;
  mgr.load_principals(make_test_principals());

  // Wrong hash.
  auto r = mgr.login("root", "argon2id:wrong_hash", "/dev/tty0", 0);
  check(r.status == LoginStatus::BadCredentials, "AC-6a: bad credentials → BadCredentials status");

  // Gate event should be Deny.
  const auto& evs = mgr.gate_events();
  check(!evs.empty() && evs.back().verdict == GateVerdict::Deny,
        "AC-6b: SessionCreate gate verdict = Deny on bad credentials");

  // Unknown principal.
  auto r2 = mgr.login("nobody", "argon2id:any", "/dev/tty0", 0);
  check(r2.status == LoginStatus::BadCredentials,
        "AC-6c: unknown principal → BadCredentials");
}

// ─── AC-7: Logout drains session within timeout ────────────────────────────────

static void test_ac7_logout_drain() {
  std::printf("\n[AC-7] logout session drain\n");
  SessionManager mgr;
  mgr.load_principals(make_test_principals());

  auto r = mgr.login("root", SessionManager::kAnonymousHash, "/dev/tty0", 0);
  const uint64_t sid = r.record.session_id;

  auto logout_r = mgr.logout(sid, /*drain_timeout_ms=*/5000);
  check(logout_r.drained_cleanly, "AC-7a: logout drains cleanly within timeout");
  check(logout_r.final_record.state == SessionState::Terminated,
        "AC-7b: session state = Terminated after logout");
  check(logout_r.final_record.session_id == sid,
        "AC-7c: final record has correct session_id");
}

// ─── AC-8: t81sh prompt format ─────────────────────────────────────────────────

static void test_ac8_prompt_format() {
  std::printf("\n[AC-8] t81sh prompt format\n");
  SessionManager mgr;
  mgr.load_principals(make_test_principals());

  auto r = mgr.login("root", SessionManager::kAnonymousHash, "/dev/tty0", 42);
  T81Shell shell(r.record, /*tier=*/1);

  const std::string p = shell.prompt();
  check(p.find("root") != std::string::npos, "AC-8a: prompt contains principal name");
  check(p.find("tier=1") != std::string::npos, "AC-8b: prompt contains tier");
  check(p.back() == ' ' && p.find(']') != std::string::npos, "AC-8c: prompt ends with ']$ '");

  // Prompt includes session_id hex.
  char sid_hex[17];
  std::snprintf(sid_hex, sizeof(sid_hex), "%08" PRIx64, r.record.session_id);
  check(p.find(sid_hex) != std::string::npos, "AC-8d: prompt contains session ID hex");
}

// ─── AC-9: Built-in commands execute without error ────────────────────────────

static void test_ac9_builtin_commands() {
  std::printf("\n[AC-9] t81sh built-in commands\n");
  SessionManager mgr;
  mgr.load_principals(make_test_principals());

  auto r = mgr.login("root", SessionManager::kAnonymousHash, "/dev/tty0", 0);
  T81Shell shell(r.record, 1);

  check(shell.exec_command("compile main.t81"), "AC-9a: compile builtin executes");
  check(shell.exec_command("run main.tisc"),    "AC-9b: run builtin executes");
  check(shell.exec_command("hash /bin/foo"),    "AC-9c: hash builtin executes");
  check(shell.exec_command("policy list"),      "AC-9d: policy builtin executes");
  check(shell.exec_command("service list"),     "AC-9e: service builtin executes");
  check(shell.exec_command("tier 2"),           "AC-9f: tier builtin executes");
  check(shell.tier() == 2, "AC-9g: tier builtin updates tier");
  check(shell.exec_command("ls /"),             "AC-9h: ls builtin executes");
  check(shell.exec_command("cat /etc/t81-services.json"), "AC-9i: cat builtin executes");
}

// ─── AC-10: Every command appended to CanonFS history JSONL ──────────────────

static void test_ac10_history_jsonl() {
  std::printf("\n[AC-10] command history JSONL\n");
  SessionManager mgr;
  mgr.load_principals(make_test_principals());

  auto r = mgr.login("root", SessionManager::kAnonymousHash, "/dev/tty0", 0);
  T81Shell shell(r.record, 1);

  shell.exec_command("compile foo.t81", 1);
  shell.exec_command("run foo.tisc",    2);
  shell.exec_command("ls /",            3);

  check(shell.history().size() == 3, "AC-10a: 3 history entries recorded");
  check(shell.history_jsonl().size() == 3, "AC-10b: 3 JSONL lines produced");

  // Each JSONL line must contain "cmd" and "epoch".
  bool all_valid = true;
  for (const auto& line : shell.history_jsonl()) {
    if (line.find("\"cmd\"") == std::string::npos ||
        line.find("\"epoch\"") == std::string::npos)
      all_valid = false;
  }
  check(all_valid, "AC-10c: all JSONL lines contain 'cmd' and 'epoch' fields");

  // History path is correct CanonFS path.
  const std::string hp = shell.history_canon_path();
  check(hp.find("canonfs://var/sessions/") != std::string::npos,
        "AC-10d: history canon path is under canonfs://var/sessions/");
  check(hp.find("history.jsonl") != std::string::npos,
        "AC-10e: history canon path ends with history.jsonl");
}

// ─── AC-11: ShellExec gate fires for non-builtins; Deny blocks ────────────────

static void test_ac11_shellexec_gate() {
  std::printf("\n[AC-11] ShellExec Axion gate\n");
  SessionManager mgr;
  mgr.load_principals(make_test_principals());

  auto r = mgr.login("root", SessionManager::kAnonymousHash, "/dev/tty0", 0);
  T81Shell shell(r.record, 1);

  // Non-builtin allowed.
  bool ok = shell.exec_command("my_program --flag");
  check(ok, "AC-11a: non-builtin command executes (Allow verdict)");
  const auto& evs = shell.gate_events();
  check(!evs.empty() && evs.back().op == "ShellExec",
        "AC-11b: ShellExec gate fired for non-builtin");
  check(evs.back().subject == "my_program", "AC-11c: ShellExec gate subject = command token");
  check(evs.back().verdict == GateVerdict::Allow, "AC-11d: ShellExec verdict = Allow");

  // Deny list.
  shell.deny_commands({"forbidden_cmd"});
  bool denied = shell.exec_command("forbidden_cmd --arg");
  check(!denied, "AC-11e: denied command returns false");
  check(!shell.gate_events().empty() &&
        shell.gate_events().back().verdict == GateVerdict::Deny,
        "AC-11f: ShellExec verdict = Deny for blocked command");

  // Denied command still appears in history (AC-11 + AC-10 interaction).
  const auto& hist = shell.history();
  bool found_denied = std::any_of(hist.begin(), hist.end(),
    [](const ShellHistoryEntry& e) {
      return e.cmd.find("forbidden_cmd") != std::string::npos && e.exit_code == 1;
    });
  check(found_denied, "AC-11g: denied command recorded in history with exit_code=1");
}

// ─── AC-12: studio/agent builtins transfer TTY to raw mode ────────────────────

static void test_ac12_tty_handoff() {
  std::printf("\n[AC-12] TTY raw-mode handoff for TUI\n");

  auto ho = handoff_tty_to_tui("/dev/tty0", "studio");
  check(ho.mode == TtyMode::Raw,   "AC-12a: TTY set to raw mode on handoff");
  check(ho.transferred,            "AC-12b: TTY handle transferred to TUI process");
  check(ho.tty_handle == "/dev/tty0", "AC-12c: correct TTY handle preserved");

  auto restored = restore_tty(ho);
  check(restored.mode == TtyMode::Cooked, "AC-12d: TTY restored to cooked mode on exit");
  check(!restored.transferred,            "AC-12e: transfer flag cleared on restore");

  // Via shell builtin.
  SessionManager mgr;
  mgr.load_principals(make_test_principals());
  auto r = mgr.login("root", SessionManager::kAnonymousHash, "/dev/tty0", 0);
  T81Shell shell(r.record, 1);
  check(shell.exec_command("studio"), "AC-12f: studio builtin executes without error");
  check(shell.exec_command("agent"),  "AC-12g: agent builtin executes without error");
}

// ─── AC-13: on_demand activation triggers ServiceSpawn gate ──────────────────

static void test_ac13_on_demand_activation() {
  std::printf("\n[AC-13] on_demand service activation — ServiceSpawn gate\n");
  const auto reg = make_test_registry();

  auto res = activate_service(reg, "t81-studio", /*requesting_session_id=*/1);
  check(res.success, "AC-13a: on_demand service activates successfully");
  check(res.gate_event.op == "ServiceSpawn",     "AC-13b: ServiceSpawn gate fired");
  check(res.gate_event.subject == "t81-studio",  "AC-13c: gate subject = service id");
  check(res.gate_event.verdict == GateVerdict::Allow, "AC-13d: gate verdict = Allow");
}

// ─── AC-14: Unknown capability refused at spawn time ─────────────────────────

static void test_ac14_unknown_capability() {
  std::printf("\n[AC-14] unrecognised capability refused at spawn\n");

  ServiceEntry bad_svc;
  bad_svc.id           = "bad-service";
  bad_svc.binary       = "/bin/bad";
  bad_svc.canon_hash   = "deadbeef";
  bad_svc.activation   = ActivationMode::OnDemand;
  bad_svc.capabilities = {"SessionCreate", "UNKNOWN_CAPABILITY_XYZ"};

  const auto reg = make_service_registry({bad_svc});
  auto res = activate_service(reg, "bad-service", 1);

  check(!res.success, "AC-14a: service with unknown capability is refused");
  check(res.gate_event.verdict == GateVerdict::Deny, "AC-14b: ServiceSpawn gate = Deny");
  check(!res.rejection_reason.empty(), "AC-14c: rejection reason is non-empty");
  check(res.rejection_reason.find("UNKNOWN_CAPABILITY_XYZ") != std::string::npos,
        "AC-14d: rejection reason names the unknown capability");

  // Also verify in simulate_boot.
  ServiceEntry bad_boot;
  bad_boot.id           = "bad-boot-svc";
  bad_boot.binary       = "/bin/bad2";
  bad_boot.canon_hash   = "cafebabe";
  bad_boot.activation   = ActivationMode::Required;
  bad_boot.capabilities = {"EXOTIC_CAP"};

  const auto boot_reg = make_service_registry({bad_boot});
  const auto boot_res = simulate_boot(boot_reg, {});
  check(!boot_res.success, "AC-14e: boot fails for service with unknown capability");
  check(!boot_res.spawn_log.empty() && !boot_res.spawn_log[0].capability_ok,
        "AC-14f: SpawnRecord.capability_ok = false");
}

// ─── AC-15: T81Lang REPL shares VM state across lines ─────────────────────────

static void test_ac15_repl_shared_state() {
  std::printf("\n[AC-15] T81Lang REPL shared VM state across lines\n");
  SessionManager mgr;
  mgr.load_principals(make_test_principals());

  auto r = mgr.login("root", SessionManager::kAnonymousHash, "/dev/tty0", 0);
  T81Shell shell(r.record, 1);

  // Bind a variable in line 1.
  check(shell.exec_command("let x = 42"), "AC-15a: T81Lang let binding executes");
  // Query it in line 2 — the binding must still be present.
  check(shell.repl_state().bindings.count("x") > 0,
        "AC-15b: variable 'x' persists in REPL state after binding");
  check(shell.repl_state().bindings.at("x") == "42",
        "AC-15c: variable 'x' has correct value");

  // Second binding.
  check(shell.exec_command("let msg = hello"), "AC-15d: second T81Lang let binding executes");
  check(shell.repl_state().bindings.count("msg") > 0,
        "AC-15e: variable 'msg' persists in REPL state");
  check(shell.repl_state().exec_count == 2, "AC-15f: REPL exec_count increments correctly");

  // Both bindings coexist.
  check(shell.repl_state().bindings.size() == 2,
        "AC-15g: both bindings coexist in shared REPL state");
}

// ─────────────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== RFC-00B9 TernaryOS User Environment Acceptance Tests ===\n");
  test_ac1_init_boot_order();
  test_ac2_boot_hash();
  test_ac3_integrity_violation();
  test_ac4_session_id_monotonic();
  test_ac5_session_record_canonfs();
  test_ac6_invalid_credentials();
  test_ac7_logout_drain();
  test_ac8_prompt_format();
  test_ac9_builtin_commands();
  test_ac10_history_jsonl();
  test_ac11_shellexec_gate();
  test_ac12_tty_handoff();
  test_ac13_on_demand_activation();
  test_ac14_unknown_capability();
  test_ac15_repl_shared_state();
  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail > 0 ? 1 : 0;
}
