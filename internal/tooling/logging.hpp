#pragma once

#include <iostream>
#include <string_view>

struct Flags {
  bool verbose = false;
  bool quiet = false;
};

inline Flags g_flags;
inline bool g_json_error_mode = false;
inline bool g_json_error_emitted = false;

inline std::string json_escape_text(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 8);
  for (char c : text) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  return out;
}

inline void verbose(std::string_view msg) {
  if (g_flags.verbose) {
    std::cerr << "[verbose] " << msg << '\n';
  }
}

inline void info(std::string_view msg) {
  if (!g_flags.quiet) {
    std::cout << msg << '\n';
  }
}

inline void error(std::string_view msg) {
  if (g_json_error_mode) {
    if (!g_json_error_emitted) {
      std::cout << "{\n"
                << "  \"schema\": \"t81.error.v1\",\n"
                << "  \"ok\": false,\n"
                << "  \"error\": \"" << json_escape_text(msg) << "\"\n"
                << "}\n";
      g_json_error_emitted = true;
    }
    return;
  }
  std::cerr << "error: " << msg << '\n';
}
