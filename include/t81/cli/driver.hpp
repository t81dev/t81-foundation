#pragma once

#include <filesystem>

namespace t81::cli {

int compile(const std::filesystem::path& input, const std::filesystem::path& output);
int run_tisc(const std::filesystem::path& path);
int check_syntax(const std::filesystem::path& path);

} // namespace t81::cli
