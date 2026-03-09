#include <cstdlib>
#include <iostream>
#include <string>

#include "t81/python_frontend/compile.hpp"

static void check(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "Check failed: " << msg << "\n";
    std::exit(1);
  }
}

int main() {
#ifdef T81_HAS_PYTHON_FRONTEND
  {
    const std::string source =
        "def inc(x: int) -> int:\n"
        "    return x + 1\n"
        "\n"
        "def main() -> int:\n"
        "    y: int = 2\n"
        "    if y == 2:\n"
        "        y = inc(y)\n"
        "    else:\n"
        "        y = 0\n"
        "    return y\n";
    std::string output;
    std::string error;
    t81::python_frontend::CompileOptions options;
    options.use_t81_dialect = true;
    check(t81::python_frontend::compile_source_to_mlir_text(source, "ok.py", output, options,
                                                            &error),
          error.c_str());
    check(output.find("\"t81.reg_set_i\"") != std::string::npos,
          "expected custom t81 register ops in emitted MLIR");
    check(output.find("cf.cond_br") != std::string::npos,
          "expected Python if lowering to emit conditional branches");
  }

  {
    const std::string source =
        "def main() -> int:\n"
        "    x: int = 0\n"
        "    while x < 3:\n"
        "        x = x + 1\n"
        "    return x\n";
    std::string output;
    std::string error;
    t81::python_frontend::CompileOptions options;
    options.use_t81_dialect = true;
    check(t81::python_frontend::compile_source_to_mlir_text(source, "ok_while.py", output,
                                                            options, &error),
          error.c_str());
    check(output.find("cf.cond_br") != std::string::npos,
          "expected Python while lowering to emit conditional branches");
  }

  {
    const std::string source =
        "def main() -> int:\n"
        "    xs = [1, 2 + 0, 3]\n"
        "    xs[1] = xs[1] + 4\n"
        "    return xs[1 + 0]\n";
    std::string output;
    std::string error;
    t81::python_frontend::CompileOptions options;
    options.use_t81_dialect = true;
    check(t81::python_frontend::compile_source_to_mlir_text(source, "ok_list.py", output,
                                                            options, &error),
          error.c_str());
    check(output.find("\"t81.mem_store\"") != std::string::npos ||
              output.find("memref.store") != std::string::npos,
          "expected fixed-list lowering to emit memory stores");
    check(output.find("\"t81.mem_load\"") != std::string::npos ||
              output.find("memref.load") != std::string::npos,
          "expected fixed-list lowering to emit memory loads");
  }

  {
    const std::string source =
        "def main() -> int:\n"
        "    for x in range(3):\n"
        "        return x\n"
        "    return 0\n";
    std::string output;
    std::string error;
    check(!t81::python_frontend::compile_source_to_mlir_text(source, "bad_for.py", output, {},
                                                             &error),
          "expected for example to be rejected");
    check(error.find("'for' is not supported") != std::string::npos,
          "expected for rejection diagnostic");
  }

  {
    const std::string source =
        "def main() -> int:\n"
        "    xs = [1, 2, 3]\n"
        "    i: int = 1\n"
        "    return xs[i]\n";
    std::string output;
    std::string error;
    check(!t81::python_frontend::compile_source_to_mlir_text(source, "bad_list.py", output, {},
                                                             &error),
          "expected runtime indexed list example to be rejected");
    check(error.find("only compile-time constant integer expressions are supported") !=
              std::string::npos,
          "expected list-index rejection diagnostic");
  }

  std::cout << "Python frontend MLIR smoke test PASSED\n";
#else
  std::cout << "Python frontend not enabled; skipping smoke test\n";
#endif
  return 0;
}
