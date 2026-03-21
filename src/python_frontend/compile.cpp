#include "t81/python_frontend/compile.hpp"

#ifdef T81_HAS_PYTHON_FRONTEND

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include "t81/frontend_adapter/c_bridge.hpp"

#ifndef T81_PYTHON3_EXECUTABLE
#define T81_PYTHON3_EXECUTABLE "python3"
#endif

#ifndef T81_PYTHON_FRONTEND_SCRIPT_PATH
#define T81_PYTHON_FRONTEND_SCRIPT_PATH ""
#endif

namespace t81::python_frontend {

namespace {

bool fail(std::string* error_message, const std::string& message) {
  if (error_message) {
    *error_message = message;
  }
  return false;
}

std::string shell_quote(const std::string& text) {
  std::string quoted = "'";
  for (char c : text) {
    if (c == '\'') {
      quoted += "'\\''";
    } else {
      quoted.push_back(c);
    }
  }
  quoted.push_back('\'');
  return quoted;
}

std::filesystem::path make_temp_path(std::string_view stem, std::string_view ext) {
  const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         (std::string(stem) + "-" + std::to_string(now) + std::string(ext));
}

bool normalize_python_to_c(const std::string& source, const std::string& diag_name,
                           std::string& c_source, std::string* error_message) {
  const std::filesystem::path input_path = make_temp_path("t81-python-front", ".py");
  const std::filesystem::path output_path = make_temp_path("t81-python-front", ".c");
  const std::filesystem::path error_path = make_temp_path("t81-python-front", ".err");

  std::error_code ignore_ec;
  auto cleanup = [&] {
    std::filesystem::remove(input_path, ignore_ec);
    std::filesystem::remove(output_path, ignore_ec);
    std::filesystem::remove(error_path, ignore_ec);
  };

  {
    std::ofstream in(input_path, std::ios::binary | std::ios::trunc);
    if (!in) {
      cleanup();
      return fail(error_message,
                  "failed to create temporary Python input for frontend normalization");
    }
    in << source;
    if (!in.good()) {
      cleanup();
      return fail(error_message,
                  "failed writing temporary Python input for frontend normalization");
    }
  }

  std::string command = shell_quote(T81_PYTHON3_EXECUTABLE) + " " +
                        shell_quote(T81_PYTHON_FRONTEND_SCRIPT_PATH) + " --input " +
                        shell_quote(input_path.string()) + " --output " +
                        shell_quote(output_path.string()) + " --diag-name " +
                        shell_quote(diag_name) + " 2> " + shell_quote(error_path.string());
#if defined(_WIN32)
  {
    int quotes = 0;
    for (char c : command) {
      if (c == '"') quotes++;
    }
    if (quotes >= 2) {
      command = "\"" + command + "\"";
    }
  }
#endif
  const int rc = std::system(command.c_str());
  if (rc != 0) {
    std::ifstream err(error_path, std::ios::binary);
    std::ostringstream buffer;
    buffer << err.rdbuf();
    const std::string message = buffer.str();
    cleanup();
    return fail(error_message,
                message.empty() ? diag_name + ":1:1: Python subset normalization failed" : message);
  }

  std::ifstream out(output_path, std::ios::binary);
  if (!out) {
    cleanup();
    return fail(error_message,
                diag_name + ":1:1: Python subset normalization did not produce C output");
  }
  std::ostringstream buffer;
  buffer << out.rdbuf();
  c_source = buffer.str();
  cleanup();
  return true;
}

}  // namespace

bool compile_source_to_mlir_text(const std::string& source, const std::string& diag_name,
                                 std::string& output, const CompileOptions& options,
                                 std::string* error_message) {
  std::string c_source;
  if (!normalize_python_to_c(source, diag_name, c_source, error_message)) {
    return false;
  }
  t81::frontend_adapter::CompileOptions bridge_options;
  bridge_options.module_name = options.module_name;
  bridge_options.dcp_floats = options.dcp_floats;
  bridge_options.use_t81_dialect = options.use_t81_dialect;
  bridge_options.emit_comments = options.emit_comments;
  return t81::frontend_adapter::compile_normalized_c_to_mlir_text(c_source, diag_name, output,
                                                                  bridge_options, error_message);
}

bool compile_file_to_mlir(const std::filesystem::path& input,
                          const std::filesystem::path& output_path, const CompileOptions& options,
                          std::string* error_message) {
  std::string source;
  if (!t81::frontend_adapter::read_text_file(input, source, error_message)) {
    return false;
  }
  std::string output;
  if (!compile_source_to_mlir_text(source, input.string(), output, options, error_message)) {
    return false;
  }
  return t81::frontend_adapter::write_text_file(output_path, output, error_message);
}

}  // namespace t81::python_frontend

#else

namespace t81::python_frontend {

bool compile_source_to_mlir_text(const std::string&, const std::string&, std::string&,
                                 const CompileOptions&, std::string* error_message) {
  if (error_message) {
    *error_message = "t81 was built without the experimental Python frontend";
  }
  return false;
}

bool compile_file_to_mlir(const std::filesystem::path&, const std::filesystem::path&,
                          const CompileOptions&, std::string* error_message) {
  if (error_message) {
    *error_message = "t81 was built without the experimental Python frontend";
  }
  return false;
}

}  // namespace t81::python_frontend

#endif
