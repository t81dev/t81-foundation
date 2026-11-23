#pragma once

#include <string_view>

#include "tools/cli/core/cli_args.hpp"

bool is_valid_snapshot_hash_text(std::string_view text);
int run_axion(const Args& args);
int run_canonfs_command(const Args& args);
