#pragma once

#include <filesystem>
#include <iostream>
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

struct ScopedStreamRedirect {
  std::ostream& from;
  std::streambuf* old_buf;

  ScopedStreamRedirect(std::ostream& from_stream, std::ostream& to_stream)
      : from(from_stream), old_buf(from_stream.rdbuf(to_stream.rdbuf())) {}

  ~ScopedStreamRedirect() { from.rdbuf(old_buf); }
};

template <typename Fn>
int emit_help(Fn&& fn, int rc = 0) {
  ScopedStreamRedirect redirect(std::cerr, std::cout);
  fn();
  return rc;
}
