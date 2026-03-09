#pragma once

#include <filesystem>
#include <fstream>
#include <string>

#include "t81/c_frontend/compile.hpp"

namespace t81::frontend_adapter {

struct CompileOptions {
  std::string module_name;
  bool dcp_floats = false;
  bool use_t81_dialect = false;
  bool emit_comments = true;
};

inline bool fail(std::string* error_message, const std::string& message) {
  if (error_message) {
    *error_message = message;
  }
  return false;
}

inline bool read_text_file(const std::filesystem::path& input,
                           std::string& source,
                           std::string* error_message) {
  std::ifstream in(input, std::ios::binary);
  if (!in) {
    return fail(error_message, "failed to open input file: " + input.string());
  }
  source.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  return true;
}

inline bool write_text_file(const std::filesystem::path& output_path,
                            const std::string& text,
                            std::string* error_message) {
  std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    return fail(error_message, "could not write file: " + output_path.string());
  }
  out << text;
  return out.good() ? true
                    : fail(error_message, "failed writing file: " + output_path.string());
}

inline bool compile_normalized_c_to_mlir_text(const std::string& c_source,
                                              const std::string& diag_name,
                                              std::string& output,
                                              const CompileOptions& options,
                                              std::string* error_message) {
  t81::c_frontend::CompileOptions c_options;
  c_options.module_name = options.module_name;
  c_options.dcp_floats = options.dcp_floats;
  c_options.use_t81_dialect = options.use_t81_dialect;
  c_options.emit_comments = options.emit_comments;
  return t81::c_frontend::compile_source_to_mlir_text(c_source, diag_name, output, c_options,
                                                      error_message);
}

}  // namespace t81::frontend_adapter
