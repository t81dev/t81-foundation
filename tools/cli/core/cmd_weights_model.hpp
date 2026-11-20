#pragma once

#include <filesystem>
#include <string_view>

#include "tools/cli/core/cli_args.hpp"

bool has_extension_ci(const std::filesystem::path& path, std::string_view expected_ext);
int run_weights(const Args& args);
int run_model(const Args& args);
