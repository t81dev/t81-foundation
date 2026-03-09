#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/mlir/tisc_to_mlir.hpp"

#ifdef T81_HAS_MLIR

#include "llvm/Support/raw_ostream.h"

namespace fs = std::filesystem;

static void check(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "Check failed: " << msg << "\n";
    std::exit(1);
  }
}

static std::string module_to_string(mlir::ModuleOp module) {
  std::string buffer;
  llvm::raw_string_ostream os(buffer);
  module.print(os);
  os.flush();
  return buffer;
}

int main() {
  t81::tisc::Program prog;
  prog.insns = {
      {t81::tisc::Opcode::LoadImm, 242, 100, 0},
      {t81::tisc::Opcode::LoadImm, 5, 42, 0},
      {t81::tisc::Opcode::Push, 5, 0, 0},
      {t81::tisc::Opcode::Pop, 7, 0, 0},
      {t81::tisc::Opcode::Store, 90, 5, 0},
      {t81::tisc::Opcode::Load, 6, 90, 0},
      {t81::tisc::Opcode::Add, 0, 6, 7},
      {t81::tisc::Opcode::Halt, 0, 0, 0},
  };

  mlir::MLIRContext ctx;
  t81::mlir_frontend::TranslationConfig cfg;
  cfg.use_t81_dialect = true;

  auto module = t81::mlir_frontend::translate(prog, ctx, cfg);
  check(static_cast<bool>(module), "expected MLIR translation to succeed");

  const std::string text = module_to_string(*module);
  check(text.find("\"t81.mem_store\"") != std::string::npos,
        "expected emitted MLIR to contain t81.mem_store");
  check(text.find("\"t81.mem_load\"") != std::string::npos,
        "expected emitted MLIR to contain t81.mem_load");
  check(text.find("\"t81.stack_push\"") != std::string::npos,
        "expected emitted MLIR to contain t81.stack_push");
  check(text.find("\"t81.stack_pop\"") != std::string::npos,
        "expected emitted MLIR to contain t81.stack_pop");
  check(text.find("t81.dialect = \"t81\"") != std::string::npos,
        "expected emitted MLIR to advertise the t81 dialect");

  const fs::path out = fs::temp_directory_path() / "t81_mlir_memops_test.ll";
  check(t81::mlir_frontend::lower_to_llvm_ir(module, ctx, out),
        "expected MLIR lowering to LLVM IR to succeed");

  std::ifstream in(out);
  check(static_cast<bool>(in), "expected lowered LLVM IR output to exist");
  const std::string lowered_text((std::istreambuf_iterator<char>(in)),
                                 std::istreambuf_iterator<char>());
  check(lowered_text.find("t81.mem_load") == std::string::npos,
        "expected lowering to eliminate t81.mem_load");
  check(lowered_text.find("t81.mem_store") == std::string::npos,
        "expected lowering to eliminate t81.mem_store");
  check(lowered_text.find("t81.stack_push") == std::string::npos,
        "expected lowering to eliminate t81.stack_push");
  check(lowered_text.find("t81.stack_pop") == std::string::npos,
        "expected lowering to eliminate t81.stack_pop");

  std::error_code ignore_ec;
  fs::remove(out, ignore_ec);

  std::cout << "MLIR T81 dialect memory op test PASSED\n";
  return 0;
}

#else

int main() {
  std::cout << "MLIR not enabled; skipping T81 dialect memory op test\n";
  return 0;
}

#endif
