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
        "    let mut x: i32 = 0;\n"
        "    while x < 3 {\n"
        "        x = x + 1;\n"
        "    }\n"
        "    return x;\n"
        "}\n";
    std::string output;
    std::string error;
    t81::rust_frontend::CompileOptions options;
    options.use_t81_dialect = true;
    check(t81::rust_frontend::compile_source_to_mlir_text(source, "ok_while.rs", output, options,
                                                          &error),
          error.c_str());
    check(output.find("cf.cond_br") != std::string::npos,
          "expected Rust while lowering to emit conditional branches");
  }

  {
    const std::string source =
        "fn main() -> i32 {\n"
        "    let mut xs: [i32; 3] = [1, 2 + 0, 3];\n"
        "    xs[1] = xs[1] + 4;\n"
        "    return xs[1 + 0];\n"
        "}\n";
    std::string output;
    std::string error;
    t81::rust_frontend::CompileOptions options;
    options.use_t81_dialect = true;
    check(t81::rust_frontend::compile_source_to_mlir_text(source, "ok_array.rs", output, options,
                                                          &error),
          error.c_str());
    check(output.find("\"t81.mem_store\"") != std::string::npos ||
              output.find("memref.store") != std::string::npos,
          "expected Rust fixed-array lowering to emit memory stores");
    check(output.find("\"t81.mem_load\"") != std::string::npos ||
              output.find("memref.load") != std::string::npos,
          "expected Rust fixed-array lowering to emit memory loads");
  }

  {
    const std::string source =
        "fn main() -> i32 {\n"
        "    let mut xs: [i32; 3] = [1, 2, 3];\n"
        "    let i: i32 = 1;\n"
        "    return xs[i];\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::rust_frontend::compile_source_to_mlir_text(source, "bad_array.rs", output, {},
                                                           &error),
          "expected runtime-indexed array example to be rejected");
    check(error.find("only compile-time constant Rust array indices are supported") !=
              std::string::npos,
          "expected array-index rejection diagnostic");
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
