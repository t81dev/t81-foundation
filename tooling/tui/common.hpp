// SPDX-License-Identifier: MIT
// RFC-0033: Dual TUI Frontends — shared types and utilities
#pragma once

#include <functional>
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
};

// ── Subprocess helper ──────────────────────────────────────────────────────
// Runs `cmd` via popen, captures combined stdout+stderr.
// Returns trimmed output or an error message on failure.
std::string exec_command(const std::string& cmd);

// ── Agent conversation record ──────────────────────────────────────────────
struct Message {
    enum class Role { User, Agent, System };
    Role        role;
    std::string text;
};

// Serialises/deserialises a message list to/from JSONL for session persistence.
bool save_session(const std::string& path, const std::vector<Message>& msgs);
bool load_session(const std::string& path, std::vector<Message>& msgs);

} // namespace t81::tui
