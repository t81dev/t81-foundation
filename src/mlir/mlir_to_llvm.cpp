/**
 * @file mlir_to_llvm.cpp
 * @brief MLIR standard dialects → LLVM IR lowering pipeline.
 *
 * Runs the standard MLIR conversion pass pipeline:
 *   arith   → LLVM dialect
 *   math    → LLVM dialect (via intrinsics or libm)
 *   func    → LLVM dialect
 *   cf      → LLVM dialect
 *   memref  → LLVM dialect  (FinalizeMemRefToLLVM)
 *
 * Then translates the resulting LLVM dialect module to an llvm::Module
 * and writes LLVM IR text (.ll).
 */
#include "t81/mlir/tisc_to_mlir.hpp"

#ifdef T81_HAS_MLIR

#include <system_error>

#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVMPass.h"
#include "mlir/Conversion/MathToLLVM/MathToLLVM.h"
#include "mlir/Conversion/MemRefToLLVM/MemRefToLLVM.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Math/IR/Math.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/AsmState.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Export.h"
#include "mlir/Transforms/Passes.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"

namespace t81::mlir_frontend {

// Run standard conversion passes and write LLVM IR text.
bool lower_to_llvm_ir(
    mlir::OwningOpRef<mlir::ModuleOp>& module,
    mlir::MLIRContext&                  ctx,
    const std::filesystem::path&        output_path) {

  // Register translation interfaces required by translateModuleToLLVMIR.
  mlir::registerBuiltinDialectTranslation(ctx);
  mlir::registerLLVMDialectTranslation(ctx);

  // Build and run the lowering pass pipeline.
  mlir::PassManager pm(&ctx);
  // The first-pass CFG lowering can leave unreachable basic blocks behind.
  // Canonicalization removes them so FuncToLLVM sees a reachable CFG.
  pm.addPass(mlir::createCanonicalizerPass());
  pm.addPass(mlir::createCSEPass());
  // FinalizeMemRef must run before Func→LLVM.
  pm.addPass(mlir::createFinalizeMemRefToLLVMConversionPass());
  pm.addPass(mlir::createArithToLLVMConversionPass());
  pm.addPass(mlir::createConvertMathToLLVMPass());
  pm.addPass(mlir::createConvertFuncToLLVMPass());
  pm.addPass(mlir::createConvertControlFlowToLLVMPass());
  // Final CSE + canonicalize for cleaner output.
  pm.addPass(mlir::createCSEPass());
  pm.addPass(mlir::createCanonicalizerPass());

  if (mlir::failed(pm.run(*module))) return false;

  // Translate LLVM dialect → llvm::Module.
  llvm::LLVMContext llvm_ctx;
  auto llvm_module = mlir::translateModuleToLLVMIR(*module, llvm_ctx);
  if (!llvm_module) return false;

  // Write LLVM IR text.
  std::error_code ec;
  llvm::raw_fd_ostream os(output_path.string(), ec, llvm::sys::fs::OF_Text);
  if (ec) return false;
  llvm_module->print(os, nullptr);
  return true;
}

bool lower_mlir_file(
    const std::filesystem::path& mlir_input,
    const std::filesystem::path& output_path) {

  mlir::MLIRContext ctx;
  // Register all dialects we use so the parser can recognise them.
  ctx.loadDialect<mlir::arith::ArithDialect>();
  ctx.loadDialect<mlir::math::MathDialect>();
  ctx.loadDialect<mlir::func::FuncDialect>();
  ctx.loadDialect<mlir::cf::ControlFlowDialect>();
  ctx.loadDialect<mlir::memref::MemRefDialect>();

  mlir::ParserConfig parse_cfg(&ctx);
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(
      mlir_input.string(), parse_cfg);
  if (!module) return false;

  return lower_to_llvm_ir(module, ctx, output_path);
}

bool pipeline_to_llvm(
    const t81::tisc::Program&    prog,
    const std::filesystem::path& output_path,
    const TranslationConfig&     cfg) {

  mlir::MLIRContext ctx;
  auto module = translate(prog, ctx, cfg);
  if (!module) return false;
  return lower_to_llvm_ir(module, ctx, output_path);
}

}  // namespace t81::mlir_frontend

#endif  // T81_HAS_MLIR
