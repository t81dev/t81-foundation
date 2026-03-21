// SPDX-License-Identifier: MIT
// T81 TUI — AI-Native / Agentic Interface
#pragma once

#include <string>
#include <vector>

namespace t81::tui {

// Launches the AI-Native Agent TUI (t81 agent).
// Supported flags (parsed from args):
//   --resume <path>   Resume a saved JSONL session file
//   --session <path>  Path for auto-saving the session (default: none)
// Returns 0 on clean exit, non-zero on error.
int run_agent(const std::vector<std::string>& args);

} // namespace t81::tui
