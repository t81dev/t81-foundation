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
        "    while True:\n"
        "        return 1\n"
        "    return 0\n";
    std::string output;
    std::string error;
    check(!t81::python_frontend::compile_source_to_mlir_text(source, "bad_while.py", output, {},
                                                             &error),
          "expected while example to be rejected");
    check(error.find("'while' is not supported") != std::string::npos,
          "expected while rejection diagnostic");
  }

  {
    const std::string source =
        "def main() -> int:\n"
        "    xs = [1, 2, 3]\n"
        "    return 0\n";
    std::string output;
    std::string error;
    check(!t81::python_frontend::compile_source_to_mlir_text(source, "bad_list.py", output, {},
                                                             &error),
          "expected list example to be rejected");
    check(error.find("lists are not supported") != std::string::npos,
          "expected list rejection diagnostic");
  }

  std::cout << "Python frontend MLIR smoke test PASSED\n";
#else
  std::cout << "Python frontend not enabled; skipping smoke test\n";
#endif
  return 0;
}
