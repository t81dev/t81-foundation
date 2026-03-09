// SPDX-License-Identifier: MIT
// RFC-0033: Dual TUI Frontends — Human Operator Interface
#pragma once

#include <string>
#include <vector>

namespace t81::tui {

// Launches the Human Operator Studio TUI (t81 studio).
// Returns 0 on clean exit, non-zero on error.
int run_studio(const std::vector<std::string>& args);

} // namespace t81::tui
