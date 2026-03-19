#pragma once
// experimental/ternaryos/userenv/t81sh.hpp
//
// RFC-00B9 §8 — t81sh: ternary-native shell simulation.
//
// Models the shell's prompt generation, builtin dispatch, REPL state tracking,
// pipeline semantics, CanonFS history JSONL append, and ShellExec Axion gate.
// All "execution" is simulated deterministically — no subprocesses are spawned.

#include "t81/axion/userenv/session_manager.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace t81::ternaryos::userenv {

// ─── History record (RFC-00B9 §8.6) ──────────────────────────────────────────

struct ShellHistoryEntry {
  uint64_t    epoch{0};
  std::string cmd;
  int         exit_code{0};
  uint32_t    duration_ms{0};
  std::string axion_verdict;  // "Allow" | "Deny" | "Warn"
};

[[nodiscard]] std::string history_entry_to_jsonl(const ShellHistoryEntry& e);

// ─── REPL state (RFC-00B9 §8.4) ──────────────────────────────────────────────
//
// The T81Lang REPL is shared across all exec_command() calls within a session.
// Variables bound in one invocation persist for subsequent ones.
// This is modeled as a string→string name table (simulates the VM state).

struct ReplState {
  std::unordered_map<std::string, std::string> bindings;  // name → value
  uint32_t                                      exec_count{0};
};

// ─── Shell session (RFC-00B9 §8) ─────────────────────────────────────────────

class T81Shell {
public:
  // Create a shell bound to an active session.
  explicit T81Shell(SessionRecord session, uint32_t tier = 1);

  // RFC-00B9 §8.2 — prompt string: "[principal@session_id tier=N]$ "
  [[nodiscard]] std::string prompt() const;

  // Execute one line of input.  Returns true on success, false if the
  // ShellExec gate denied the command or the command itself failed.
  // The line is always appended to history (with the appropriate verdict).
  bool exec_command(const std::string& line, uint64_t epoch = 0);

  // T81Lang REPL: detect T81Lang input and evaluate it.
  // Shares the internal ReplState across calls (AC-15).
  bool exec_repl(const std::string& expr, uint64_t epoch = 0);

  // ── Observers ──────────────────────────────────────────────────────────────

  [[nodiscard]] const SessionRecord& session() const { return session_; }
  [[nodiscard]] uint32_t tier() const { return tier_; }

  [[nodiscard]] const std::vector<ShellHistoryEntry>& history() const { return history_; }
  [[nodiscard]] const std::vector<AxionGateEvent>&    gate_events() const { return gate_events_; }

  // CanonFS history path: canonfs://var/sessions/<sid>/history.jsonl
  [[nodiscard]] std::string history_canon_path() const;

  // In-memory JSONL lines written to the history (one per command).
  [[nodiscard]] const std::vector<std::string>& history_jsonl() const { return history_jsonl_; }

  // REPL state (for AC-15 inspection).
  [[nodiscard]] const ReplState& repl_state() const { return repl_; }

  // Last exit code.
  [[nodiscard]] int last_exit_code() const { return last_exit_code_; }

  // ── Configuration ──────────────────────────────────────────────────────────

  // Install a custom ShellExec policy: return Deny for commands matching a set.
  void deny_commands(std::vector<std::string> denied);

private:
  SessionRecord              session_;
  uint32_t                   tier_{1};
  std::vector<ShellHistoryEntry> history_;
  std::vector<AxionGateEvent>    gate_events_;
  std::vector<std::string>       history_jsonl_;
  ReplState                      repl_;
  int                            last_exit_code_{0};
  std::vector<std::string>       denied_commands_;
  uint64_t                       exec_epoch_{0};

  // Returns true if line is a T81Lang expression (starts with T81Lang keyword).
  [[nodiscard]] static bool is_repl_input(const std::string& line);

  // Returns true if line is a builtin command name (first token).
  [[nodiscard]] static bool is_builtin(const std::string& first_token);

  // Fire ShellExec Axion gate for a non-builtin command.
  [[nodiscard]] AxionGateEvent fire_shell_exec_gate(const std::string& cmd,
                                                     const std::string& args) const;

  // Dispatch a builtin command.  Returns {true, exit_code 0} on success.
  [[nodiscard]] bool dispatch_builtin(const std::string& line);

  // Record to history (always; independent of gate verdict).
  void record_history(const std::string& cmd, int exit_code,
                      GateVerdict verdict, uint64_t epoch, uint32_t duration_ms = 0);
};

// ─── RFC-00B9 §10 — TTY raw-mode handoff simulation ─────────────────────────

enum class TtyMode : uint8_t { Cooked = 0, Raw = 1 };

struct TtyHandoff {
  std::string tty_handle;
  TtyMode     mode{TtyMode::Cooked};
  bool        transferred{false};  // true after capability delegation to TUI process
};

// Simulate the studio/agent TTY handoff (AC-12).
// Returns the updated handoff state: mode=Raw, transferred=true on success.
[[nodiscard]] TtyHandoff handoff_tty_to_tui(const std::string& tty_handle,
                                             const std::string& tui_command);

// Restore TTY to cooked mode after TUI process exits.
[[nodiscard]] TtyHandoff restore_tty(TtyHandoff handoff);

}  // namespace t81::ternaryos::userenv
