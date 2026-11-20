#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct Args {
  std::string command;
  std::filesystem::path input;
  std::optional<std::filesystem::path> output;
  std::optional<std::filesystem::path> trace_output;
  bool need_help = false;
  bool need_version = false;
  std::vector<std::string> benchmark_args;
  std::vector<std::string> command_args;
  std::optional<std::filesystem::path> weights_model;
  std::optional<std::filesystem::path> policy;
  bool trace = false;
};
