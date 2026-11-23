#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace t81::cli {

struct ProcessCaptureResult {
  int exit_code = -1;
  std::string stdout_text;
  std::string stderr_text;
  // Launcher/capture failures use exit_code=-1; child nonzero exits remain intact.
  std::string error_message;
};

// Windows arguments are UTF-8; embedded NULs cannot be represented on either OS.
ProcessCaptureResult run_process_capture(
    const std::vector<std::string>& argv,
    const std::optional<std::filesystem::path>& cwd = std::nullopt,
    const std::vector<std::pair<std::string, std::string>>& env_overrides = {});

}  // namespace t81::cli
