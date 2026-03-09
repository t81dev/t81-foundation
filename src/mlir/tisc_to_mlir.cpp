/**
 * @file tisc_to_mlir.cpp
 * @brief TISC → MLIR translator.
 *
 * Lowers a TISC Program to an MLIR ModuleOp using standard dialects:
 *   arith   — integer/float arithmetic, bitwise, comparisons, casts
 *   math    — transcendental functions (compat mode)
 *   func    — function definition; external dmath calls (DCP mode)
 *   cf      — unconditional/conditional branches
 *   memref  — register file allocation and memory load/store
 *
 * Register file model: memref-based at translation time.
 * The lowering pipeline scalar-replaces constant-index register accesses and
 * then promotes the resulting slots to SSA with SROA + mem2reg.
 *   Integer registers : memref<243xi64>
 *   Float   registers : memref<243xf64>
 *   Flat memory       : memref<65536xi64>
 */
#include "t81/mlir/tisc_to_mlir.hpp"
#include "t81/mlir/t81_dialect.hpp"

#ifdef T81_HAS_MLIR

#include <cassert>
#include <set>
#include <string>
#include <unordered_map>

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Math/IR/Math.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinDialect.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Support/FileUtilities.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"

#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"

namespace t81::mlir_frontend {

using Op = t81::tisc::Opcode;

// ---------------------------------------------------------------------------
// TISCToMLIRTranslator
// ---------------------------------------------------------------------------
class TISCToMLIRTranslator {
 public:
  TISCToMLIRTranslator(const t81::tisc::Program& prog,
                       mlir::MLIRContext&          ctx,
                       const TranslationConfig&    cfg)
      : prog_(prog), ctx_(ctx), cfg_(cfg),
        builder_(&ctx),
        loc_(mlir::UnknownLoc::get(&ctx)) {
    // Register required dialects.
    ctx_.loadDialect<mlir::arith::ArithDialect>();
    ctx_.loadDialect<mlir::math::MathDialect>();
    ctx_.loadDialect<mlir::func::FuncDialect>();
    ctx_.loadDialect<mlir::cf::ControlFlowDialect>();
    ctx_.loadDialect<mlir::memref::MemRefDialect>();
    if (cfg_.use_t81_dialect) {
      ctx_.loadDialect<t81::mlir_frontend::T81Dialect>();
    }

    // Common types.
    i64_ty_  = builder_.getI64Type();
    f64_ty_  = builder_.getF64Type();
    idx_ty_  = builder_.getIndexType();
    i1_ty_   = builder_.getI1Type();

    ireg_memref_ty_ = mlir::MemRefType::get({kRegCount}, i64_ty_);
    freg_memref_ty_ = mlir::MemRefType::get({kRegCount}, f64_ty_);
    mem_memref_ty_  = mlir::MemRefType::get({kMemSize},  i64_ty_);
  }

  mlir::OwningOpRef<mlir::ModuleOp> run() {
    if (prog_.insns.empty()) return {};

    module_ = mlir::ModuleOp::create(loc_, cfg_.module_name);
    (*module_)->setAttr("t81.float_mode",
                        builder_.getStringAttr(cfg_.float_mode == FloatMode::DCP ? "dcp" : "compat"));
    if (cfg_.float_mode == FloatMode::DCP) {
      (*module_)->setAttr("t81.runtime", builder_.getStringAttr("t81_dmath_runtime"));
    }
    if (cfg_.use_t81_dialect) {
      (*module_)->setAttr("t81.dialect", builder_.getStringAttr("register"));
    }
    setup_main_function();
    find_leaders();
    create_blocks();
    emit_instructions();
    finalize();

    if (cfg_.verify_module) {
      if (mlir::failed(mlir::verify(*module_))) {
        // Return the module anyway — caller can re-verify for diagnostics.
      }
    }
    return std::move(module_);
  }

 private:
  static constexpr int kRegCount = 243;
  static constexpr int kMemSize  = 65536;

  const t81::tisc::Program& prog_;
  mlir::MLIRContext&          ctx_;
  const TranslationConfig&    cfg_;
  mlir::OwningOpRef<mlir::ModuleOp> module_;
  mlir::OpBuilder             builder_;
  mlir::Location              loc_;

  // Types
  mlir::IntegerType  i64_ty_;
  mlir::FloatType    f64_ty_;
  mlir::IndexType    idx_ty_;
  mlir::IntegerType  i1_ty_;
  mlir::MemRefType   ireg_memref_ty_;
  mlir::MemRefType   freg_memref_ty_;
  mlir::MemRefType   mem_memref_ty_;

  // Generated function and register allocas
  mlir::func::FuncOp  main_fn_;
  mlir::Block*        entry_block_ = nullptr;
  mlir::Value         iregs_;
  mlir::Value         fregs_;
  mlir::Value         mem_;

  // CFG
  std::set<size_t>                        leaders_;
  std::unordered_map<size_t, mlir::Block*> pc_to_block_;

  // ── Setup ────────────────────────────────────────────────────────────────

  void setup_main_function() {
    auto fn_ty = mlir::FunctionType::get(&ctx_, {}, {i64_ty_});
    main_fn_ = builder_.create<mlir::func::FuncOp>(loc_, "t81_main", fn_ty);
    module_->push_back(main_fn_);

    // Entry block: allocate register files and memory.
    entry_block_ = main_fn_.addEntryBlock();
    builder_.setInsertionPointToStart(entry_block_);

    iregs_ = builder_.create<mlir::memref::AllocaOp>(loc_, ireg_memref_ty_);
    fregs_ = builder_.create<mlir::memref::AllocaOp>(loc_, freg_memref_ty_);
    mem_   = builder_.create<mlir::memref::AllocaOp>(loc_, mem_memref_ty_);

    // Register files are stack-allocated (alloca); TISC programs are expected
    // to write before reading.  memref.fill was removed in MLIR 20+; use
    // linalg.fill if zero-init is required by a downstream consumer.

    // Declare dmath externals if DCP mode.
    if (cfg_.float_mode == FloatMode::DCP) {
      declare_dmath_externals();
    }
  }

  /// Declare @t81_dmath_<name>(f64) -> f64 in the module (idempotent).
  void declare_dmath_extern(const char* name) {
    if (module_->lookupSymbol<mlir::func::FuncOp>(name)) return;
    auto fn_ty = mlir::FunctionType::get(&ctx_, {f64_ty_}, {f64_ty_});
    mlir::OpBuilder::InsertionGuard guard(builder_);
    builder_.setInsertionPointToStart(module_->getBody());
    auto decl = builder_.create<mlir::func::FuncOp>(loc_, name, fn_ty);
    decl.setPrivate();
  }

  /// Declare @t81_dmath_pow(f64, f64) -> f64.
  void declare_dmath_pow() {
    const char* name = "t81_dmath_pow";
    if (module_->lookupSymbol<mlir::func::FuncOp>(name)) return;
    auto fn_ty = mlir::FunctionType::get(&ctx_, {f64_ty_, f64_ty_}, {f64_ty_});
    mlir::OpBuilder::InsertionGuard guard(builder_);
    builder_.setInsertionPointToStart(module_->getBody());
    auto decl = builder_.create<mlir::func::FuncOp>(loc_, name, fn_ty);
    decl.setPrivate();
  }

  void declare_dmath_externals() {
    for (const char* fn : {"t81_dmath_sin",  "t81_dmath_cos",  "t81_dmath_tan",
                            "t81_dmath_exp",  "t81_dmath_log",  "t81_dmath_sqrt",
                            "t81_dmath_asin", "t81_dmath_acos", "t81_dmath_atan",
                            "t81_dmath_sinh", "t81_dmath_cosh", "t81_dmath_tanh"})
      declare_dmath_extern(fn);
    declare_dmath_pow();
  }

  // ── Leader / block discovery ─────────────────────────────────────────────

  static bool is_branch(Op op) {
    switch (op) {
      case Op::Jump: case Op::JumpIfZero: case Op::JumpIfNotZero:
      case Op::JumpIfNegative: case Op::JumpIfPositive:
      case Op::Call: case Op::Ret: case Op::Halt:
      case Op::Terminate: case Op::AxHalt:
        return true;
      default: return false;
    }
  }

  static bool has_direct_target(Op op) {
    switch (op) {
      case Op::Jump: case Op::JumpIfZero: case Op::JumpIfNotZero:
      case Op::JumpIfNegative: case Op::JumpIfPositive: case Op::Call:
        return true;
      default: return false;
    }
  }

  void find_leaders() {
    leaders_.insert(0);
    const size_t n = prog_.insns.size();
    for (size_t pc = 0; pc < n; ++pc) {
      const auto& insn = prog_.insns[pc];
      if (is_branch(insn.opcode)) {
        if (has_direct_target(insn.opcode)) {
          int32_t tgt = insn.a;
          if (tgt >= 0 && static_cast<size_t>(tgt) < n)
            leaders_.insert(static_cast<size_t>(tgt));
        }
        if (pc + 1 < n) leaders_.insert(pc + 1);
      }
    }
  }

  void create_blocks() {
    for (size_t pc : leaders_) {
      std::string name = cfg_.emit_comments ? ("bb_" + std::to_string(pc)) : "";
      auto* bb = new mlir::Block();
      main_fn_.getBody().push_back(bb);
      pc_to_block_[pc] = bb;
    }
    builder_.setInsertionPointToEnd(entry_block_);
    builder_.create<mlir::cf::BranchOp>(loc_, pc_to_block_.at(0), mlir::ValueRange{});
    builder_.setInsertionPointToEnd(pc_to_block_.at(0));
  }

  // ── Register helpers ─────────────────────────────────────────────────────

  mlir::Value idx(int64_t i) {
    return builder_.create<mlir::arith::ConstantIndexOp>(loc_, i);
  }

  mlir::Value load_ireg(int32_t i) {
    if (cfg_.use_t81_dialect) {
      mlir::OperationState state(loc_, kRegGetIOpName);
      state.addOperands(iregs_);
      state.addAttribute("reg", builder_.getI32IntegerAttr(i));
      state.addTypes(i64_ty_);
      auto* op = builder_.insert(mlir::Operation::create(state));
      return op->getResult(0);
    }
    return builder_.create<mlir::memref::LoadOp>(
        loc_, iregs_, mlir::ValueRange{idx(i)});
  }

  void store_ireg(int32_t i, mlir::Value v) {
    if (cfg_.use_t81_dialect) {
      mlir::OperationState state(loc_, kRegSetIOpName);
      state.addOperands({iregs_, v});
      state.addAttribute("reg", builder_.getI32IntegerAttr(i));
      builder_.insert(mlir::Operation::create(state));
      return;
    }
    builder_.create<mlir::memref::StoreOp>(
        loc_, v, iregs_, mlir::ValueRange{idx(i)});
  }

  mlir::Value load_freg(int32_t i) {
    if (cfg_.use_t81_dialect) {
      mlir::OperationState state(loc_, kRegGetFOpName);
      state.addOperands(fregs_);
      state.addAttribute("reg", builder_.getI32IntegerAttr(i));
      state.addTypes(f64_ty_);
      auto* op = builder_.insert(mlir::Operation::create(state));
      return op->getResult(0);
    }
    return builder_.create<mlir::memref::LoadOp>(
        loc_, fregs_, mlir::ValueRange{idx(i)});
  }

  void store_freg(int32_t i, mlir::Value v) {
    if (cfg_.use_t81_dialect) {
      mlir::OperationState state(loc_, kRegSetFOpName);
      state.addOperands({fregs_, v});
      state.addAttribute("reg", builder_.getI32IntegerAttr(i));
      builder_.insert(mlir::Operation::create(state));
      return;
    }
    builder_.create<mlir::memref::StoreOp>(
        loc_, v, fregs_, mlir::ValueRange{idx(i)});
  }

  mlir::Value mem_load(int64_t addr) {
    int64_t clamped = (addr >= 0 && addr < kMemSize) ? addr : 0;
    return builder_.create<mlir::memref::LoadOp>(
        loc_, mem_, mlir::ValueRange{idx(clamped)});
  }

  void mem_store(int64_t addr, mlir::Value v) {
    int64_t clamped = (addr >= 0 && addr < kMemSize) ? addr : 0;
    builder_.create<mlir::memref::StoreOp>(
        loc_, v, mem_, mlir::ValueRange{idx(clamped)});
  }

  // ── Constant helpers ─────────────────────────────────────────────────────

  mlir::Value ci64(int64_t v) {
    return builder_.create<mlir::arith::ConstantOp>(
        loc_, builder_.getI64IntegerAttr(v));
  }

  mlir::Value cf64(double v) {
    return builder_.create<mlir::arith::ConstantOp>(
        loc_, builder_.getF64FloatAttr(v));
  }

  // ── Float op helpers ─────────────────────────────────────────────────────

  // Emit a unary float op: math.* (compat) or func.call @t81_dmath_* (DCP).
  template <typename MathOp>
  mlir::Value float_unary(mlir::Value arg, const char* dmath_name) {
    if (cfg_.float_mode == FloatMode::DCP) {
      declare_dmath_extern(dmath_name);
      return builder_.create<mlir::func::CallOp>(
          loc_, dmath_name, mlir::TypeRange{f64_ty_},
          mlir::ValueRange{arg}).getResult(0);
    }
    return builder_.create<MathOp>(loc_, arg);
  }

  // Emit binary pow: math.powf or func.call @t81_dmath_pow.
  mlir::Value float_pow(mlir::Value base, mlir::Value exp) {
    if (cfg_.float_mode == FloatMode::DCP) {
      declare_dmath_pow();
      return builder_.create<mlir::func::CallOp>(
          loc_, "t81_dmath_pow", mlir::TypeRange{f64_ty_},
          mlir::ValueRange{base, exp}).getResult(0);
    }
    return builder_.create<mlir::math::PowFOp>(loc_, base, exp);
  }

  // ── Block resolution ─────────────────────────────────────────────────────

  mlir::Block* resolve_block(size_t target_pc, size_t fallback_pc) {
    auto it = pc_to_block_.find(target_pc);
    if (it != pc_to_block_.end()) return it->second;
    it = pc_to_block_.find(fallback_pc);
    if (it != pc_to_block_.end()) return it->second;
    // Last resort: create a return block.
    auto* bb = new mlir::Block();
    main_fn_.getBody().push_back(bb);
    mlir::OpBuilder tmp(&ctx_);
    tmp.setInsertionPointToStart(bb);
    tmp.create<mlir::func::ReturnOp>(loc_, mlir::ValueRange{ci64(0)});
    return bb;
  }

  // ── Instruction emission ─────────────────────────────────────────────────

  void emit_instructions() {
    const size_t n = prog_.insns.size();
    mlir::Block* cur = pc_to_block_[0];
    builder_.setInsertionPointToEnd(cur);

    for (size_t pc = 0; pc < n; ++pc) {
      if (pc != 0 && leaders_.count(pc)) {
        mlir::Block* next = pc_to_block_[pc];
        if (!cur->getTerminator())
          builder_.create<mlir::cf::BranchOp>(loc_, next,
                                               mlir::ValueRange{});
        cur = next;
        builder_.setInsertionPointToEnd(cur);
      }
      emit_one(pc, prog_.insns[pc]);
    }

    if (!cur->getTerminator())
      builder_.create<mlir::func::ReturnOp>(loc_,
          mlir::ValueRange{ci64(0)});
  }

  void emit_one(size_t pc, const t81::tisc::Insn& insn) {
    const int32_t a = insn.a;
    const int64_t b = insn.b;
    const int32_t c = insn.c;

    switch (insn.opcode) {
      // ── No-op / Halt ────────────────────────────────────────────────
      case Op::Nop:
        break;

      case Op::Halt:
      case Op::Terminate:
      case Op::AxHalt:
        builder_.create<mlir::func::ReturnOp>(
            loc_, mlir::ValueRange{load_ireg(0)});
        break;

      // ── Immediate load ───────────────────────────────────────────────
      case Op::LoadImm:
        store_ireg(a, ci64(b));
        break;

      // ── Register move ────────────────────────────────────────────────
      case Op::Mov:
        store_ireg(a, load_ireg(static_cast<int32_t>(b)));
        break;

      // ── Integer arithmetic ───────────────────────────────────────────
      case Op::Add:
        store_ireg(a, builder_.create<mlir::arith::AddIOp>(
            loc_, load_ireg(a), load_ireg(c)));
        break;
      case Op::Sub:
        store_ireg(a, builder_.create<mlir::arith::SubIOp>(
            loc_, load_ireg(a), load_ireg(c)));
        break;
      case Op::Mul:
        store_ireg(a, builder_.create<mlir::arith::MulIOp>(
            loc_, load_ireg(a), load_ireg(c)));
        break;
      case Op::Div:
        store_ireg(a, builder_.create<mlir::arith::DivSIOp>(
            loc_, load_ireg(a), load_ireg(c)));
        break;
      case Op::Mod:
        store_ireg(a, builder_.create<mlir::arith::RemSIOp>(
            loc_, load_ireg(a), load_ireg(c)));
        break;
      case Op::Neg:
        store_ireg(a, builder_.create<mlir::arith::SubIOp>(
            loc_, ci64(0), load_ireg(a)));
        break;
      case Op::Inc:
        store_ireg(a, builder_.create<mlir::arith::AddIOp>(
            loc_, load_ireg(a), ci64(1)));
        break;
      case Op::Dec:
        store_ireg(a, builder_.create<mlir::arith::SubIOp>(
            loc_, load_ireg(a), ci64(1)));
        break;

      // ── Bitwise ──────────────────────────────────────────────────────
      case Op::BitAnd:
        store_ireg(a, builder_.create<mlir::arith::AndIOp>(
            loc_, load_ireg(a), load_ireg(c)));
        break;
      case Op::BitOr:
        store_ireg(a, builder_.create<mlir::arith::OrIOp>(
            loc_, load_ireg(a), load_ireg(c)));
        break;
      case Op::BitXor:
        store_ireg(a, builder_.create<mlir::arith::XOrIOp>(
            loc_, load_ireg(a), load_ireg(c)));
        break;
      case Op::BitNot: {
        // ~x = x XOR -1
        auto all_ones = ci64(-1LL);
        store_ireg(a, builder_.create<mlir::arith::XOrIOp>(
            loc_, load_ireg(a), all_ones));
        break;
      }
      case Op::BitShl:
        store_ireg(a, builder_.create<mlir::arith::ShLIOp>(
            loc_, load_ireg(a), load_ireg(c)));
        break;
      case Op::BitShr:
        store_ireg(a, builder_.create<mlir::arith::ShRSIOp>(
            loc_, load_ireg(a), load_ireg(c)));
        break;
      case Op::BitUShr:
        store_ireg(a, builder_.create<mlir::arith::ShRUIOp>(
            loc_, load_ireg(a), load_ireg(c)));
        break;

      // ── Comparison ───────────────────────────────────────────────────
      case Op::Cmp: {
        // R[a] = sign(R[a] - R[c])  → -1, 0, or 1
        auto lv  = load_ireg(a);
        auto rv  = load_ireg(c);
        auto lt  = builder_.create<mlir::arith::CmpIOp>(
            loc_, mlir::arith::CmpIPredicate::slt, lv, rv);
        auto gt  = builder_.create<mlir::arith::CmpIOp>(
            loc_, mlir::arith::CmpIPredicate::sgt, lv, rv);
        auto neg = builder_.create<mlir::arith::SelectOp>(
            loc_, lt, ci64(-1), ci64(0));
        store_ireg(a, builder_.create<mlir::arith::SelectOp>(
            loc_, gt, ci64(1), neg));
        break;
      }
      case Op::Less:
        store_ireg(a, builder_.create<mlir::arith::ExtSIOp>(loc_, i64_ty_,
            builder_.create<mlir::arith::CmpIOp>(loc_,
                mlir::arith::CmpIPredicate::slt, load_ireg(a), load_ireg(c))));
        break;
      case Op::LessEqual:
        store_ireg(a, builder_.create<mlir::arith::ExtSIOp>(loc_, i64_ty_,
            builder_.create<mlir::arith::CmpIOp>(loc_,
                mlir::arith::CmpIPredicate::sle, load_ireg(a), load_ireg(c))));
        break;
      case Op::Greater:
        store_ireg(a, builder_.create<mlir::arith::ExtSIOp>(loc_, i64_ty_,
            builder_.create<mlir::arith::CmpIOp>(loc_,
                mlir::arith::CmpIPredicate::sgt, load_ireg(a), load_ireg(c))));
        break;
      case Op::GreaterEqual:
        store_ireg(a, builder_.create<mlir::arith::ExtSIOp>(loc_, i64_ty_,
            builder_.create<mlir::arith::CmpIOp>(loc_,
                mlir::arith::CmpIPredicate::sge, load_ireg(a), load_ireg(c))));
        break;
      case Op::Equal:
        store_ireg(a, builder_.create<mlir::arith::ExtSIOp>(loc_, i64_ty_,
            builder_.create<mlir::arith::CmpIOp>(loc_,
                mlir::arith::CmpIPredicate::eq, load_ireg(a), load_ireg(c))));
        break;
      case Op::NotEqual:
        store_ireg(a, builder_.create<mlir::arith::ExtSIOp>(loc_, i64_ty_,
            builder_.create<mlir::arith::CmpIOp>(loc_,
                mlir::arith::CmpIPredicate::ne, load_ireg(a), load_ireg(c))));
        break;

      // ── Control flow ─────────────────────────────────────────────────
      case Op::Jump:
        builder_.create<mlir::cf::BranchOp>(
            loc_, resolve_block(static_cast<size_t>(a), pc + 1),
            mlir::ValueRange{});
        break;

      case Op::JumpIfZero: {
        auto cond = builder_.create<mlir::arith::CmpIOp>(
            loc_, mlir::arith::CmpIPredicate::eq, load_ireg(c), ci64(0));
        builder_.create<mlir::cf::CondBranchOp>(
            loc_, cond,
            resolve_block(static_cast<size_t>(a), pc + 1), mlir::ValueRange{},
            resolve_block(pc + 1, pc + 1),                  mlir::ValueRange{});
        break;
      }
      case Op::JumpIfNotZero: {
        auto cond = builder_.create<mlir::arith::CmpIOp>(
            loc_, mlir::arith::CmpIPredicate::ne, load_ireg(c), ci64(0));
        builder_.create<mlir::cf::CondBranchOp>(
            loc_, cond,
            resolve_block(static_cast<size_t>(a), pc + 1), mlir::ValueRange{},
            resolve_block(pc + 1, pc + 1),                  mlir::ValueRange{});
        break;
      }
      case Op::JumpIfNegative: {
        auto cond = builder_.create<mlir::arith::CmpIOp>(
            loc_, mlir::arith::CmpIPredicate::slt, load_ireg(c), ci64(0));
        builder_.create<mlir::cf::CondBranchOp>(
            loc_, cond,
            resolve_block(static_cast<size_t>(a), pc + 1), mlir::ValueRange{},
            resolve_block(pc + 1, pc + 1),                  mlir::ValueRange{});
        break;
      }
      case Op::JumpIfPositive: {
        auto cond = builder_.create<mlir::arith::CmpIOp>(
            loc_, mlir::arith::CmpIPredicate::sgt, load_ireg(c), ci64(0));
        builder_.create<mlir::cf::CondBranchOp>(
            loc_, cond,
            resolve_block(static_cast<size_t>(a), pc + 1), mlir::ValueRange{},
            resolve_block(pc + 1, pc + 1),                  mlir::ValueRange{});
        break;
      }

      // Phase 1: Call lowered as direct branch (no return-address stack).
      case Op::Call:
        builder_.create<mlir::cf::BranchOp>(
            loc_, resolve_block(static_cast<size_t>(a), pc + 1),
            mlir::ValueRange{});
        break;

      case Op::Ret:
        builder_.create<mlir::func::ReturnOp>(
            loc_, mlir::ValueRange{load_ireg(0)});
        break;

      // ── Memory load/store ─────────────────────────────────────────────
      // Load:  R[a] = mem[b]   (b = immediate address)
      // Store: mem[a] = R[b]   (a = immediate address, b = register)
      case Op::Load:
        store_ireg(a, mem_load(b));
        break;
      case Op::Store:
        mem_store(a, load_ireg(static_cast<int32_t>(b)));
        break;

      // ── Stack ops (R[242] = SP) ──────────────────────────────────────
      case Op::Push: {
        auto sp = load_ireg(242);
        // Cast SP from i64 to index for memref access.
        auto sp_idx = builder_.create<mlir::arith::IndexCastOp>(
            loc_, idx_ty_, sp);
        builder_.create<mlir::memref::StoreOp>(
            loc_, load_ireg(a), mem_, mlir::ValueRange{sp_idx});
        store_ireg(242, builder_.create<mlir::arith::SubIOp>(
            loc_, sp, ci64(1)));
        break;
      }
      case Op::Pop: {
        auto sp = builder_.create<mlir::arith::AddIOp>(
            loc_, load_ireg(242), ci64(1));
        store_ireg(242, sp);
        auto sp_idx = builder_.create<mlir::arith::IndexCastOp>(
            loc_, idx_ty_, sp);
        store_ireg(a, builder_.create<mlir::memref::LoadOp>(
            loc_, mem_, mlir::ValueRange{sp_idx}));
        break;
      }

      // ── Float arithmetic ─────────────────────────────────────────────
      case Op::FAdd:
        store_freg(a, builder_.create<mlir::arith::AddFOp>(
            loc_, load_freg(a), load_freg(c)));
        break;
      case Op::FSub:
        store_freg(a, builder_.create<mlir::arith::SubFOp>(
            loc_, load_freg(a), load_freg(c)));
        break;
      case Op::FMul:
        store_freg(a, builder_.create<mlir::arith::MulFOp>(
            loc_, load_freg(a), load_freg(c)));
        break;
      case Op::FDiv:
        store_freg(a, builder_.create<mlir::arith::DivFOp>(
            loc_, load_freg(a), load_freg(c)));
        break;

      // ── Float transcendentals ────────────────────────────────────────
      case Op::FSin:
        store_freg(a, float_unary<mlir::math::SinOp>(load_freg(a), "t81_dmath_sin"));
        break;
      case Op::FCos:
        store_freg(a, float_unary<mlir::math::CosOp>(load_freg(a), "t81_dmath_cos"));
        break;
      case Op::FTan:
        store_freg(a, float_unary<mlir::math::TanOp>(load_freg(a), "t81_dmath_tan"));
        break;
      case Op::FExp:
        store_freg(a, float_unary<mlir::math::ExpOp>(load_freg(a), "t81_dmath_exp"));
        break;
      case Op::FLog:
        store_freg(a, float_unary<mlir::math::LogOp>(load_freg(a), "t81_dmath_log"));
        break;
      case Op::FSqrt:
        store_freg(a, float_unary<mlir::math::SqrtOp>(load_freg(a), "t81_dmath_sqrt"));
        break;
      case Op::FPow:
        store_freg(a, float_pow(load_freg(a), load_freg(c)));
        break;
      case Op::FAsin:
        store_freg(a, float_unary<mlir::math::AsinOp>(load_freg(a), "t81_dmath_asin"));
        break;
      case Op::FAcos:
        store_freg(a, float_unary<mlir::math::AcosOp>(load_freg(a), "t81_dmath_acos"));
        break;
      case Op::FAtan:
        store_freg(a, float_unary<mlir::math::AtanOp>(load_freg(a), "t81_dmath_atan"));
        break;
      case Op::FSinh:
        store_freg(a, float_unary<mlir::math::SinhOp>(load_freg(a), "t81_dmath_sinh"));
        break;
      case Op::FCosh:
        store_freg(a, float_unary<mlir::math::CoshOp>(load_freg(a), "t81_dmath_cosh"));
        break;
      case Op::FTanh:
        store_freg(a, float_unary<mlir::math::TanhOp>(load_freg(a), "t81_dmath_tanh"));
        break;

      // ── Type conversions ─────────────────────────────────────────────
      case Op::I2F:
        store_freg(a, builder_.create<mlir::arith::SIToFPOp>(
            loc_, f64_ty_, load_ireg(a)));
        break;
      case Op::F2I:
        store_ireg(a, builder_.create<mlir::arith::FPToSIOp>(
            loc_, i64_ty_, load_freg(a)));
        break;

      // ── Print ────────────────────────────────────────────────────────
      case Op::Print:
        // Stubbed as no-op in MLIR output: printf requires LLVM dialect which
        // complicates the standard lowering path.  The LLVM IR backend handles
        // Print directly via llvm::IRBuilder.
        break;

      // ── Everything else: no-op ───────────────────────────────────────
      default:
        break;
    }
  }

  // ── Finalize ─────────────────────────────────────────────────────────────

  void finalize() {
    for (auto& block : main_fn_.getBody()) {
      if (!block.getTerminator()) {
        mlir::OpBuilder tmp(&ctx_);
        tmp.setInsertionPointToEnd(&block);
        tmp.create<mlir::func::ReturnOp>(loc_, mlir::ValueRange{ci64(0)});
      }
    }
  }
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

mlir::OwningOpRef<mlir::ModuleOp> translate(
    const t81::tisc::Program& prog,
    mlir::MLIRContext&          ctx,
    const TranslationConfig&    cfg) {
  TISCToMLIRTranslator tr(prog, ctx, cfg);
  return tr.run();
}

bool emit_mlir_text(
    const t81::tisc::Program&    prog,
    const std::filesystem::path& output_path,
    const TranslationConfig&     cfg) {
  mlir::MLIRContext ctx;
  auto module = translate(prog, ctx, cfg);
  if (!module) return false;

  std::error_code ec;
  llvm::raw_fd_ostream os(output_path.string(), ec, llvm::sys::fs::OF_Text);
  if (ec) return false;
  module->print(os);
  return true;
}

}  // namespace t81::mlir_frontend

#endif  // T81_HAS_MLIR
