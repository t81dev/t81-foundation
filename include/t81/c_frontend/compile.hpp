#pragma once

#include <filesystem>
#include <string>

namespace t81::c_frontend {

struct CompileOptions {
  std::string module_name;
  bool dcp_floats = false;
  bool use_t81_dialect = false;
  bool emit_comments = true;
};

bool compile_source_to_mlir_text(const std::string& source,
                                 const std::string& diag_name,
                                 std::string& output,
                                 const CompileOptions& options = {},
                                 std::string* error_message = nullptr);

bool compile_file_to_mlir(const std::filesystem::path& input,
                          const std::filesystem::path& output,
                          const CompileOptions& options = {},
                          std::string* error_message = nullptr);

}  // namespace t81::c_frontend
