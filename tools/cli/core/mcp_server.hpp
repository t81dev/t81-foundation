#pragma once

#include <functional>
#include <string>
#include <vector>

namespace t81::cli {

// Native in-process stdio JSON-RPC 2.0 / MCP server (`t81 mcp serve`).
// The `run_canonfs_fsck_json` callback invokes `canonfs fsck --verify-hashes --json`
// for a given canonfs_root and writes JSON to stdout, returning exit code.
int run_mcp_serve(const std::vector<std::string>& command_args,
                  const std::function<int(const std::string&)>& run_canonfs_fsck_json);

}  // namespace t81::cli
