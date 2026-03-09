#include "t81/rust_frontend/compile.hpp"

#include <fstream>
#include <sstream>

namespace t81::rust_frontend {

namespace {

constexpr const char* kNotImplemented =
    "experimental Rust frontend scaffold is present, but the compile pipeline is not implemented yet";

bool fail(std::string* error_message, const std::string& message) {
  if (error_message) {
    *error_message = message;
  }
  return false;
}

}  // namespace

bool compile_source_to_mlir_text(const std::string& source,
                                 const std::string& diag_name,
                                 std::string& output,
                                 const CompileOptions& options,
                                 std::string* error_message) {
  (void)source;
  (void)output;
  (void)options;
  return fail(error_message, diag_name + ":1:1: " + kNotImplemented);
}

bool compile_file_to_mlir(const std::filesystem::path& input,
                          const std::filesystem::path& output,
                          const CompileOptions& options,
                          std::string* error_message) {
  (void)output;
  std::ifstream in(input);
  if (!in) {
    return fail(error_message, "failed to open input file: " + input.string());
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  std::string ignored_output;
  return compile_source_to_mlir_text(buffer.str(), input.filename().string(), ignored_output, options,
                                     error_message);
}

}  // namespace t81::rust_frontend
