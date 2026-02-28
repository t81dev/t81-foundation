#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

// Shared fixture I/O helpers used by CLI stdlib fixture tests.
// normalize_text() and join_lines() define the golden-output comparison contract;
// do not alter their semantics without regenerating all .out fixture files.

namespace t81::test {

inline std::string read_text(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("Failed to open file: " + path.string());
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

inline std::string normalize_text(std::string text) {
  text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
  while (!text.empty() && (text.back() == '\n' || text.back() == ' ' || text.back() == '\t'))
    text.pop_back();
  return text;
}

inline std::string join_lines(const std::vector<std::string>& lines) {
  std::string out;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    out += lines[i];
    if (i + 1 < lines.size()) out.push_back('\n');
  }
  return out;
}

}  // namespace t81::test
