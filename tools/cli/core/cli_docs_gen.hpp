#pragma once

#include <string>

namespace t81::cli {

std::string build_bash_completion();
std::string build_zsh_completion();
std::string build_fish_completion();
std::string build_manpage_text();

}  // namespace t81::cli
