#include "t81/mlir/t81_dialect.hpp"

#ifdef T81_HAS_MLIR

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"

namespace t81::mlir_frontend {

T81Dialect::T81Dialect(mlir::MLIRContext* ctx)
    : mlir::Dialect(getDialectNamespace(), ctx, mlir::TypeID::get<T81Dialect>()) {
  allowUnknownOperations();
}

namespace {

class LowerT81RegisterOpsPass
    : public mlir::PassWrapper<LowerT81RegisterOpsPass, mlir::OperationPass<mlir::func::FuncOp>> {
 public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerT81RegisterOpsPass)

  void getDependentDialects(mlir::DialectRegistry& registry) const override {
    registry.insert<mlir::arith::ArithDialect, mlir::memref::MemRefDialect>();
  }

  llvm::StringRef getArgument() const final { return "t81-lower-register-ops"; }
  llvm::StringRef getDescription() const final {
    return "Lower custom t81 register, memory, and stack ops to standard MLIR";
  }

  void runOnOperation() override {
    mlir::func::FuncOp func = getOperation();
    mlir::SmallVector<mlir::Operation*> worklist;
    func.walk([&](mlir::Operation* op) {
      llvm::StringRef name = op->getName().getStringRef();
      if (name == kRegGetIOpName || name == kRegSetIOpName || name == kRegGetFOpName ||
          name == kRegSetFOpName) {
        worklist.push_back(op);
      }
    });

    mlir::OpBuilder builder(func.getContext());
    for (mlir::Operation* op : worklist) {
      auto reg_attr = op->getAttrOfType<mlir::IntegerAttr>("reg");
      if (!reg_attr) {
        op->emitError("expected integer 'reg' attribute");
        signalPassFailure();
        return;
      }

      const auto reg = static_cast<int64_t>(reg_attr.getInt());
      builder.setInsertionPoint(op);
      auto reg_idx = builder.create<mlir::arith::ConstantIndexOp>(op->getLoc(), reg);
      llvm::StringRef name = op->getName().getStringRef();

      if (name == kRegGetIOpName || name == kRegGetFOpName) {
        if (op->getNumOperands() != 1 || op->getNumResults() != 1) {
          op->emitError("expected 1 operand and 1 result");
          signalPassFailure();
          return;
        }
        auto load = builder.create<mlir::memref::LoadOp>(
            op->getLoc(), op->getOperands()[0], mlir::ValueRange{reg_idx});
        op->getResult(0).replaceAllUsesWith(load.getResult());
      } else {
        if (op->getNumOperands() != 2 || op->getNumResults() != 0) {
          op->emitError("expected 2 operands and 0 results");
          signalPassFailure();
          return;
        }
        builder.create<mlir::memref::StoreOp>(
            op->getLoc(), op->getOperands()[1], op->getOperands()[0], mlir::ValueRange{reg_idx});
      }

      op->erase();
    }

    worklist.clear();
    func.walk([&](mlir::Operation* op) {
      llvm::StringRef name = op->getName().getStringRef();
      if (name == kMemLoadOpName || name == kMemStoreOpName) {
        worklist.push_back(op);
      }
    });

    for (mlir::Operation* op : worklist) {
      auto addr_attr = op->getAttrOfType<mlir::IntegerAttr>("addr");
      if (!addr_attr) {
        op->emitError("expected integer 'addr' attribute");
        signalPassFailure();
        return;
      }

      const auto addr = static_cast<int64_t>(addr_attr.getInt());
      builder.setInsertionPoint(op);
      auto addr_idx = builder.create<mlir::arith::ConstantIndexOp>(op->getLoc(), addr);
      llvm::StringRef name = op->getName().getStringRef();

      if (name == kMemLoadOpName) {
        if (op->getNumOperands() != 1 || op->getNumResults() != 1) {
          op->emitError("expected 1 operand and 1 result");
          signalPassFailure();
          return;
        }
        auto load = builder.create<mlir::memref::LoadOp>(
            op->getLoc(), op->getOperands()[0], mlir::ValueRange{addr_idx});
        op->getResult(0).replaceAllUsesWith(load.getResult());
      } else {
        if (op->getNumOperands() != 2 || op->getNumResults() != 0) {
          op->emitError("expected 2 operands and 0 results");
          signalPassFailure();
          return;
        }
        builder.create<mlir::memref::StoreOp>(
            op->getLoc(), op->getOperands()[1], op->getOperands()[0], mlir::ValueRange{addr_idx});
      }

      op->erase();
    }

    worklist.clear();
    func.walk([&](mlir::Operation* op) {
      llvm::StringRef name = op->getName().getStringRef();
      if (name == kStackPushOpName || name == kStackPopOpName) {
        worklist.push_back(op);
      }
    });

    for (mlir::Operation* op : worklist) {
      builder.setInsertionPoint(op);
      llvm::StringRef name = op->getName().getStringRef();

      if (name == kStackPushOpName) {
        if (op->getNumOperands() != 3 || op->getNumResults() != 1) {
          op->emitError("expected 3 operands and 1 result");
          signalPassFailure();
          return;
        }

        auto sp_idx = builder.create<mlir::arith::IndexCastOp>(
            op->getLoc(), builder.getIndexType(), op->getOperands()[2]);
        builder.create<mlir::memref::StoreOp>(
            op->getLoc(), op->getOperands()[1], op->getOperands()[0], mlir::ValueRange{sp_idx});
        auto new_sp = builder.create<mlir::arith::SubIOp>(
            op->getLoc(), op->getOperands()[2],
            builder.create<mlir::arith::ConstantOp>(op->getLoc(), builder.getI64IntegerAttr(1)));
        op->getResult(0).replaceAllUsesWith(new_sp.getResult());
      } else {
        if (op->getNumOperands() != 2 || op->getNumResults() != 2) {
          op->emitError("expected 2 operands and 2 results");
          signalPassFailure();
          return;
        }

        auto new_sp = builder.create<mlir::arith::AddIOp>(
            op->getLoc(), op->getOperands()[1],
            builder.create<mlir::arith::ConstantOp>(op->getLoc(), builder.getI64IntegerAttr(1)));
        auto sp_idx = builder.create<mlir::arith::IndexCastOp>(
            op->getLoc(), builder.getIndexType(), new_sp.getResult());
        auto load = builder.create<mlir::memref::LoadOp>(
            op->getLoc(), op->getOperands()[0], mlir::ValueRange{sp_idx});
        op->getResult(0).replaceAllUsesWith(load.getResult());
        op->getResult(1).replaceAllUsesWith(new_sp.getResult());
      }

      op->erase();
    }
  }
};

}  // namespace

std::unique_ptr<mlir::Pass> createLowerT81RegisterOpsPass() {
  return std::make_unique<LowerT81RegisterOpsPass>();
}

}  // namespace t81::mlir_frontend

#endif  // T81_HAS_MLIR
