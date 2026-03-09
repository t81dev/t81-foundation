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
        "int twice(int x) {\n"
        "  return x * 2;\n"
        "}\n"
        "int climb(int limit) {\n"
        "  int x = 0;\n"
        "  for (; x < limit; x = x + 1) {\n"
        "  }\n"
        "  for (int i = 0; i < 2; i = i + 1) {\n"
        "    x = x + 1;\n"
        "  }\n"
        "  return x;\n"
        "}\n"
        "int main() {\n"
        "  int y = climb(3);\n"
        "  if (y == 3) {\n"
        "    y = twice(y);\n"
        "  }\n"
        "  return y;\n"
        "}\n";
    std::string output;
    std::string error;
    t81::c_frontend::CompileOptions options;
    options.use_t81_dialect = true;
    check(t81::c_frontend::compile_source_to_mlir_text(source, "ok.c", output, options, &error),
          error.c_str());
    check(output.find("\"t81.reg_set_i\"") != std::string::npos,
          "expected custom t81 register ops in emitted MLIR");
    check(output.find("cf.cond_br") != std::string::npos,
          "expected structured control flow to lower to conditional branches");
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
    check(error.find("pointers are not supported") != std::string::npos,
          "expected pointer rejection diagnostic");
  }

  {
    const std::string source =
        "int main() {\n"
        "  int a[4] = {0};\n"
        "  return 0;\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::c_frontend::compile_source_to_mlir_text(source, "bad_array.c", output, {}, &error),
          "expected array example to be rejected");
    check(error.find("local arrays are not supported") != std::string::npos,
          "expected array rejection diagnostic");
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

  {
    const std::string source =
        "int loop(int x) {\n"
        "  if (x == 0) {\n"
        "    return 0;\n"
        "  }\n"
        "  return loop(x - 1);\n"
        "}\n"
        "int main() {\n"
        "  return loop(3);\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::c_frontend::compile_source_to_mlir_text(source, "bad_recursive.c", output, {},
                                                        &error),
          "expected recursive example to be rejected");
    check(error.find("recursive calls are not supported") != std::string::npos,
          "expected recursion rejection diagnostic");
  }

  {
    const std::string source =
        "int helper(int *p) {\n"
        "  return 0;\n"
        "}\n"
        "int main() {\n"
        "  return helper(0);\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::c_frontend::compile_source_to_mlir_text(source, "bad_param.c", output, {}, &error),
          "expected pointer parameter example to be rejected");
    check(error.find("pointer parameters are not supported") != std::string::npos,
          "expected pointer-parameter rejection diagnostic");
  }

  {
    const std::string source =
        "int main() {\n"
        "  int x = 0;\n"
        "  for (;; x = x + 1) {\n"
        "    x = x + 1;\n"
        "  }\n"
        "  return x;\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::c_frontend::compile_source_to_mlir_text(source, "bad_for.c", output, {}, &error),
          "expected conditionless for-loop example to be rejected");
    check(error.find("for statements must include a condition") != std::string::npos,
          "expected for-loop rejection diagnostic");
  }

  {
    const std::string source =
        "int main() {\n"
        "  while (1) {\n"
        "    break;\n"
        "  }\n"
        "  return 0;\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::c_frontend::compile_source_to_mlir_text(source, "bad_break.c", output, {}, &error),
          "expected break example to be rejected");
    check(error.find("'break' is not supported") != std::string::npos,
          "expected break rejection diagnostic");
  }

  {
    const std::string source =
        "int main() {\n"
        "  for (int i = 0; i < 3; i = i + 1) {\n"
        "    continue;\n"
        "  }\n"
        "  return 0;\n"
        "}\n";
    std::string output;
    std::string error;
    check(
        !t81::c_frontend::compile_source_to_mlir_text(source, "bad_continue.c", output, {}, &error),
        "expected continue example to be rejected");
    check(error.find("'continue' is not supported") != std::string::npos,
          "expected continue rejection diagnostic");
  }

  {
    const std::string source =
        "int main() {\n"
        "  int x = 1;\n"
        "  switch (x) {\n"
        "    case 1: return 1;\n"
        "    default: return 0;\n"
        "  }\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::c_frontend::compile_source_to_mlir_text(source, "bad_switch.c", output, {}, &error),
          "expected switch example to be rejected");
    check(error.find("'switch' is not supported") != std::string::npos,
          "expected switch rejection diagnostic");
  }

  {
    const std::string source =
        "int main() {\n"
        "  int x = 0;\n"
        "  do {\n"
        "    x = x + 1;\n"
        "  } while (x < 3);\n"
        "  return x;\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::c_frontend::compile_source_to_mlir_text(source, "bad_do.c", output, {}, &error),
          "expected do-while example to be rejected");
    check(error.find("'do-while' is not supported") != std::string::npos,
          "expected do-while rejection diagnostic");
  }

  {
    const std::string source =
        "int main() {\n"
        "  goto done;\n"
        "done:\n"
        "  return 0;\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::c_frontend::compile_source_to_mlir_text(source, "bad_goto.c", output, {}, &error),
          "expected goto example to be rejected");
    check(error.find("'goto' is not supported") != std::string::npos ||
              error.find("labels are not supported") != std::string::npos,
          "expected goto rejection diagnostic");
  }

  {
    const std::string source =
        "int main() {\n"
        "  int x = 1;\n"
        "  int *p = &x;\n"
        "  return x;\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::c_frontend::compile_source_to_mlir_text(source, "bad_addr.c", output, {}, &error),
          "expected address-of example to be rejected");
    check(error.find("address-of is not supported") != std::string::npos ||
              error.find("pointers are not supported") != std::string::npos,
          "expected address-of rejection diagnostic");
  }

  {
    const std::string source =
        "int main() {\n"
        "  int *p = 0;\n"
        "  return *p;\n"
        "}\n";
    std::string output;
    std::string error;
    check(
        !t81::c_frontend::compile_source_to_mlir_text(source, "bad_deref.c", output, {}, &error),
        "expected dereference example to be rejected");
    check(error.find("pointer dereference is not supported") != std::string::npos ||
              error.find("pointers are not supported") != std::string::npos,
          "expected dereference rejection diagnostic");
  }

  {
    const std::string source =
        "int main() {\n"
        "  int x = (int) 3;\n"
        "  return x;\n"
        "}\n";
    std::string output;
    std::string error;
    check(!t81::c_frontend::compile_source_to_mlir_text(source, "bad_cast.c", output, {}, &error),
          "expected cast example to be rejected");
    check(error.find("casts are not supported") != std::string::npos,
          "expected cast rejection diagnostic");
  }

  std::cout << "C frontend MLIR smoke test PASSED\n";
#else
  std::cout << "C frontend not enabled; skipping smoke test\n";
#endif
  return 0;
}
