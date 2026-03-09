#pragma once

#ifdef T81_HAS_MLIR

#include <memory>

#include "mlir/IR/Dialect.h"
#include "mlir/Pass/Pass.h"

namespace t81::mlir_frontend {

class T81Dialect : public mlir::Dialect {
 public:
  explicit T81Dialect(mlir::MLIRContext* ctx);

  static llvm::StringLiteral getDialectNamespace() { return "t81"; }
};

inline constexpr const char* kRegGetIOpName = "t81.reg_get_i";
inline constexpr const char* kRegSetIOpName = "t81.reg_set_i";
inline constexpr const char* kRegGetFOpName = "t81.reg_get_f";
inline constexpr const char* kRegSetFOpName = "t81.reg_set_f";
inline constexpr const char* kMemLoadOpName = "t81.mem_load";
inline constexpr const char* kMemStoreOpName = "t81.mem_store";
inline constexpr const char* kStackPushOpName = "t81.stack_push";
inline constexpr const char* kStackPopOpName = "t81.stack_pop";

std::unique_ptr<mlir::Pass> createLowerT81RegisterOpsPass();

}  // namespace t81::mlir_frontend

#endif  // T81_HAS_MLIR
