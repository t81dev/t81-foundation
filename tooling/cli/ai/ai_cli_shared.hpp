#pragma once

#include <string_view>
#include <vector>

namespace t81::cli::ai {

void print_usage(std::string_view prog);

int run(std::string_view prog, const std::vector<std::string_view>& args);

}  // namespace t81::cli::ai
