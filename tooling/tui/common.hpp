// SPDX-License-Identifier: MIT
// T81 TUI — shared types and utilities
#pragma once

#include <functional>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace t81::tui {

// ── Command palette entry ──────────────────────────────────────────────────
struct CommandEntry {
    std::string name;         // Display name / search key
    std::string description;  // Short description shown in palette
    std::string cli_command;  // Underlying `t81 …` invocation
};

// Returns the full set of discoverable T81 commands for the palette.
std::vector<CommandEntry> all_commands();

// ── Session state ──────────────────────────────────────────────────────────
// Lightweight snapshot of runtime context displayed in status bars.
struct SessionState {
    std::string model_name  = "None";
    std::string axion_mode  = "Strict";
    int         vm_tier     = 1;
    std::string trace_hash  = "—";
    // Trit probability distribution derived from last llama-run inference.
    float       trit_pos    = 0.0f;  // P(+1)
    float       trit_zero   = 0.0f;  // P(0)
    float       trit_neg    = 0.0f;  // P(-1)
    int         infer_tokens = 0;    // Token count from last /infer run
};

enum class TargetKind {
    None,
    SourceFile,
    ArtifactFile,
    TraceFile,
    PolicyFile,
    Directory,
    CanonFsObject,
};

struct TargetRef {
    TargetKind   kind = TargetKind::None;
    std::string  path;
    std::string  label;
    std::string  canon_hash;
};

struct WorkspaceState {
    std::string project_root;
    TargetRef current_target;
    TargetRef current_source;
    TargetRef current_artifact;
    TargetRef current_trace;
    TargetRef current_policy;
    TargetRef lhs_target;
    TargetRef rhs_target;
    std::vector<TargetRef> recent_targets;
};

// ── Subprocess helpers ─────────────────────────────────────────────────────
// exec_command: runs a pre-built shell string via popen.
//   Use only for hardcoded commands that contain no user-supplied tokens.
std::string exec_command(const std::string& cmd);

// exec_argv: preferred API — spawns the process directly (no shell).
//   Each element of argv is passed as a separate argument, so shell
//   metacharacters in user-supplied paths or prompts are never interpreted.
//   stderr is merged with stdout in the returned string.
std::string exec_argv(const std::vector<std::string>& argv);
void set_cli_program_path(std::string path);
std::string cli_program_path();
std::vector<std::string> t81_cli_argv(std::initializer_list<std::string> tail);
std::vector<std::string> t81_cli_argv(const std::vector<std::string>& tail);

// Splits a shell-like command line into argv tokens without executing it.
// Supports simple single quotes, double quotes, and backslash escapes.
std::vector<std::string> split_command_words(const std::string& text);

// Small TUI helpers used by the renderers and snapshot tests.
std::string compact_path(const std::string& path, size_t width = 40);
TargetKind infer_target_kind(const std::string& path);
std::string target_kind_label(TargetKind kind);
TargetRef make_target_from_path(const std::string& path);
std::string format_target_summary(const TargetRef& target);
void remember_target(std::vector<TargetRef>& recent_targets, TargetRef& current_target,
                     TargetRef& current_source, TargetRef& current_artifact,
                     TargetRef& current_trace, TargetRef& current_policy,
                     const TargetRef& target, size_t max_recent = 8);
std::vector<std::string> split_lines(const std::string& s);
std::vector<const CommandEntry*> filter_palette(
    const std::vector<CommandEntry>& cmds, const std::string& query);
int palette_window_start(int selected, int count, int visible);
std::string scroll_indicator_text(
    int scroll_offset, int total_lines, int visible, const std::string& suffix);

// ── Agent conversation record ──────────────────────────────────────────────
struct Message {
    enum class Role { User, Agent, System };
    Role        role;
    std::string text;
};

// Serialises/deserialises a message list to/from JSONL for session persistence.
bool save_session(const std::string& path, const std::vector<Message>& msgs);
bool load_session(const std::string& path, std::vector<Message>& msgs);
std::string default_workspace_state_path();
bool save_workspace_state(const std::string& path, const WorkspaceState& state);
bool load_workspace_state(const std::string& path, WorkspaceState& state);

} // namespace t81::tui
