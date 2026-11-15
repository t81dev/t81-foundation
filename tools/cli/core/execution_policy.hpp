#pragma once

#include <array>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

#include "t81/axion/policy.hpp"

namespace t81::cli::detail {

// Validate the bytes read from one opened file before exposing them to a VM.
// This is CLI input binding, not a new policy model or authorization verdict.
inline bool read_execution_policy(const std::filesystem::path& path, std::string& text,
                                  std::string& diagnostic) {
  std::error_code status_error;
  if (path.empty() || std::filesystem::is_directory(path, status_error)) {
    diagnostic = "Could not open policy file: " + path.string();
    return false;
  }
  std::ifstream input;
  try {
    input.open(path, std::ios::binary);
  } catch (const std::exception& exception) {
    diagnostic = "Could not open policy file: " + path.string() + ": " + exception.what();
    return false;
  }
  if (!input) {
    diagnostic = "Could not open policy file: " + path.string();
    return false;
  }
  std::string candidate;
  try {
    std::array<char, 4096> buffer;
    while (input) {
      input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
      candidate.append(buffer.data(), static_cast<std::size_t>(input.gcount()));
    }
  } catch (const std::exception& exception) {
    diagnostic = "Could not read policy file: " + path.string() + ": " + exception.what();
    return false;
  }
  if (input.bad() || (input.fail() && !input.eof())) {
    diagnostic = "Could not read policy file: " + path.string();
    return false;
  }
  try {
    const auto parsed = t81::axion::parse_policy(candidate);
    if (!parsed.has_value()) {
      diagnostic = "Could not parse policy file: " + path.string() + ": " + parsed.error();
      return false;
    }
  } catch (const std::exception& exception) {
    diagnostic = "Could not parse policy file: " + path.string() + ": " + exception.what();
    return false;
  }
  text = std::move(candidate);
  return true;
}

}  // namespace t81::cli::detail
