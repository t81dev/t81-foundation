#include <cstdlib>
#include <iostream>
#include <string>

#include "t81/c_frontend/compile.hpp"

static void check(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "Check failed: " << msg << "\n";
    std::exit(1);
  }
}

int main() {
#ifdef T81_HAS_C_FRONTEND
  {
    const std::string source =
        "int main() {\n"
        "  int x = 2;\n"
        "  int y = 5;\n"
        "  return x * y;\n"
        "}\n";
    std::string output;
    std::string error;
    t81::c_frontend::CompileOptions options;
    options.use_t81_dialect = true;
    check(t81::c_frontend::compile_source_to_mlir_text(source, "ok.c", output, options, &error),
          error.c_str());
    check(output.find("\"t81.reg_set_i\"") != std::string::npos,
          "expected custom t81 register ops in emitted MLIR");
  }

  {
    const std::string source =
        "int main() {\n"
        "  int *p = 0;\n"
        "  return 0;\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::c_frontend::compile_source_to_mlir_text(source, "bad_pointer.c", output, {}, &error),
          "expected pointer example to be rejected");
    check(error.find("only 'int' local variables are supported") != std::string::npos,
          "expected pointer rejection diagnostic");
  }

  {
    const std::string source =
        "float main() {\n"
        "  return 1.0f;\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::c_frontend::compile_source_to_mlir_text(source, "bad_float.c", output, {}, &error),
          "expected float example to be rejected");
    check(!error.empty() &&
              (error.find("int") != std::string::npos || error.find("main") != std::string::npos),
          "expected float-return rejection diagnostic");
  }

  std::cout << "C frontend MLIR smoke test PASSED\n";
#else
  std::cout << "C frontend not enabled; skipping smoke test\n";
#endif
  return 0;
}
