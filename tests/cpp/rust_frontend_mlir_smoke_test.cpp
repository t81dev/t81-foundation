#include <cstdlib>
#include <iostream>
#include <string>

#include "t81/rust_frontend/compile.hpp"

static void check(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "Check failed: " << msg << "\n";
    std::exit(1);
  }
}

int main() {
#ifdef T81_HAS_RUST_FRONTEND
  {
    const std::string source =
        "fn bump(x: i32) -> i32 {\n"
        "    return x + 1;\n"
        "}\n"
        "fn main() -> i32 {\n"
        "    let mut y: i32 = 2;\n"
        "    if y == 2 {\n"
        "        y = bump(y);\n"
        "    } else {\n"
        "        y = 0;\n"
        "    }\n"
        "    return y;\n"
        "}\n";
    std::string output;
    std::string error;
    t81::rust_frontend::CompileOptions options;
    options.use_t81_dialect = true;
    check(t81::rust_frontend::compile_source_to_mlir_text(source, "ok.rs", output, options,
                                                          &error),
          error.c_str());
    check(output.find("\"t81.reg_set_i\"") != std::string::npos,
          "expected custom t81 register ops in emitted MLIR");
    check(output.find("cf.cond_br") != std::string::npos,
          "expected Rust if-expression lowering to emit conditional branches");
  }

  {
    const std::string source =
        "fn main() -> i32 {\n"
        "    while true {\n"
        "        return 1;\n"
        "    }\n"
        "    return 0;\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::rust_frontend::compile_source_to_mlir_text(source, "bad_while.rs", output, {},
                                                           &error),
          "expected while example to be rejected");
    check(error.find("'while' is not supported") != std::string::npos,
          "expected while rejection diagnostic");
  }

  {
    const std::string source =
        "fn main() -> i32 {\n"
        "    let p = &1;\n"
        "    return 0;\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::rust_frontend::compile_source_to_mlir_text(source, "bad_ref.rs", output, {},
                                                           &error),
          "expected reference example to be rejected");
    check(error.find("references are not supported") != std::string::npos,
          "expected reference rejection diagnostic");
  }

  std::cout << "Rust frontend MLIR smoke test PASSED\n";
#else
  std::cout << "Rust frontend not enabled; skipping smoke test\n";
#endif
  return 0;
}
