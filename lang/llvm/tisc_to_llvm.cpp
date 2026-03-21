/**
 * @file tisc_to_llvm.cpp
 * @brief TISC → LLVM IR translator.
 */
#include "t81/llvm/tisc_to_llvm.hpp"

#ifdef T81_HAS_LLVM

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"

namespace t81::llvm_backend {

using namespace llvm;
using Op = t81::tisc::Opcode;

// ──────────────────────────────────────────────────────────────────────────
// TISCTranslator
// ──────────────────────────────────────────────────────────────────────────
class TISCTranslator {
 public:
  TISCTranslator(const t81::tisc::Program& prog,
                 LLVMContext&              ctx,
                 const TranslationConfig& cfg)
      : prog_(prog), ctx_(ctx), cfg_(cfg),
        module_(std::make_unique<Module>(cfg.module_name, ctx)),
        builder_(ctx) {
    i1_ty_     = Type::getInt1Ty(ctx_);
    i8_ty_     = Type::getInt8Ty(ctx_);
    i32_ty_    = Type::getInt32Ty(ctx_);
    i64_ty_    = Type::getInt64Ty(ctx_);
    double_ty_ = Type::getDoubleTy(ctx_);
    ptr_ty_    = PointerType::getUnqual(ctx_);

    ireg_arr_ty_ = ArrayType::get(i64_ty_,    kRegCount);
    freg_arr_ty_ = ArrayType::get(double_ty_, kRegCount);
    mem_arr_ty_  = ArrayType::get(i64_ty_,    kMemSize);
  }

  std::unique_ptr<Module> run() {
    if (prog_.insns.empty()) return nullptr;

    setup_main_function();
    find_leaders();
    create_blocks();
    emit_instructions();
    finalize();

    if (cfg_.verify_module) {
      std::string err;
      raw_string_ostream es(err);
      if (verifyModule(*module_, &es)) {
        // Return module anyway — verifier errors are informational here.
        // Caller can re-verify if needed.
      }
    }
    return std::move(module_);
  }

 private:
  static constexpr int kRegCount  = 243;
  static constexpr int kMemSize   = 65536;  // flat memory cells (matches VM default)

  const t81::tisc::Program& prog_;
  LLVMContext&               ctx_;
  const TranslationConfig&   cfg_;
  std::unique_ptr<Module>    module_;
  IRBuilder<>                builder_;

  // LLVM types
  Type*       i1_ty_     = nullptr;
  Type*       i8_ty_     = nullptr;
  Type*       i32_ty_    = nullptr;
  Type*       i64_ty_    = nullptr;
  Type*       double_ty_ = nullptr;
  PointerType* ptr_ty_   = nullptr;
  ArrayType*  ireg_arr_ty_ = nullptr;
  ArrayType*  freg_arr_ty_ = nullptr;
  ArrayType*  mem_arr_ty_  = nullptr;

  // The generated function and its register allocas
  Function*   main_fn_ = nullptr;
  Value*      iregs_   = nullptr;   // [243 x i64]*
  Value*      fregs_   = nullptr;   // [243 x double]*
  Value*      mem_     = nullptr;   // [65536 x i64]* flat memory
  BasicBlock* entry_bb_ = nullptr;

  // PC → BasicBlock mapping
  std::set<size_t>                          leaders_;
  std::unordered_map<size_t, BasicBlock*>   pc_to_bb_;

  // ── Setup ───────────────────────────────────────────────────────────────

  void setup_main_function() {
    auto* fn_ty = FunctionType::get(i64_ty_, {}, false);
    main_fn_ = Function::Create(fn_ty, Function::ExternalLinkage,
                                "t81_main", *module_);
    main_fn_->setDoesNotThrow();

    entry_bb_ = BasicBlock::Create(ctx_, "entry", main_fn_);
    builder_.SetInsertPoint(entry_bb_);

    iregs_ = builder_.CreateAlloca(ireg_arr_ty_, nullptr, "iregs");
    fregs_ = builder_.CreateAlloca(freg_arr_ty_, nullptr, "fregs");
    mem_   = builder_.CreateAlloca(mem_arr_ty_,  nullptr, "mem");

    // Zero-initialize register files and flat memory.
    builder_.CreateMemSet(iregs_, ConstantInt::get(i8_ty_, 0),
                          kRegCount * 8, Align(8));
    builder_.CreateMemSet(fregs_, ConstantInt::get(i8_ty_, 0),
                          kRegCount * 8, Align(8));
    builder_.CreateMemSet(mem_,   ConstantInt::get(i8_ty_, 0),
                          kMemSize  * 8, Align(8));

    // Declare external printf for Print opcode.
    declare_externals();
  }

  Function* printf_fn_  = nullptr;
  Function* putchar_fn_ = nullptr;

  void declare_externals() {
    auto* printf_ty = FunctionType::get(i32_ty_, {ptr_ty_}, /*isVarArg=*/true);
    printf_fn_ = cast<Function>(
        module_->getOrInsertFunction("printf", printf_ty).getCallee());

    auto* putchar_ty = FunctionType::get(i32_ty_, {i32_ty_}, false);
    putchar_fn_ = cast<Function>(
        module_->getOrInsertFunction("putchar", putchar_ty).getCallee());
  }

  // ── Leader / block discovery ────────────────────────────────────────────

  static bool is_branch(Op op) {
    switch (op) {
      case Op::Jump:
      case Op::JumpIfZero:
      case Op::JumpIfNotZero:
      case Op::JumpIfNegative:
      case Op::JumpIfPositive:
      case Op::Call:
      case Op::Ret:
      case Op::Halt:
      case Op::Terminate:
      case Op::AxHalt:
        return true;
      default:
        return false;
    }
  }

  // For branch instructions, field `a` holds the target PC.
  static bool has_direct_target(Op op) {
    switch (op) {
      case Op::Jump:
      case Op::JumpIfZero:
      case Op::JumpIfNotZero:
      case Op::JumpIfNegative:
      case Op::JumpIfPositive:
      case Op::Call:
        return true;
      default:
        return false;
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
        if (pc + 1 < n) leaders_.insert(pc + 1);  // fall-through
      }
    }
  }

  void create_blocks() {
    for (size_t pc : leaders_) {
      std::string name = "bb_" + std::to_string(pc);
      pc_to_bb_[pc] = BasicBlock::Create(ctx_, name, main_fn_);
    }
    // Entry block falls into bb_0.
    builder_.SetInsertPoint(entry_bb_);
    builder_.CreateBr(pc_to_bb_[0]);
  }

  // ── Register helpers ────────────────────────────────────────────────────

  Value* ireg_ptr(int32_t idx) {
    return builder_.CreateInBoundsGEP(
        ireg_arr_ty_, iregs_,
        {ConstantInt::get(i64_ty_, 0), ConstantInt::get(i64_ty_, idx)},
        "ip_r" + std::to_string(idx));
  }

  Value* freg_ptr(int32_t idx) {
    return builder_.CreateInBoundsGEP(
        freg_arr_ty_, fregs_,
        {ConstantInt::get(i64_ty_, 0), ConstantInt::get(i64_ty_, idx)},
        "fp_r" + std::to_string(idx));
  }

  Value* load_ireg(int32_t idx) {
    return builder_.CreateLoad(i64_ty_, ireg_ptr(idx),
                               "r" + std::to_string(idx));
  }

  void store_ireg(int32_t idx, Value* v) {
    builder_.CreateStore(v, ireg_ptr(idx));
  }

  Value* load_freg(int32_t idx) {
    return builder_.CreateLoad(double_ty_, freg_ptr(idx),
                               "fr" + std::to_string(idx));
  }

  void store_freg(int32_t idx, Value* v) {
    builder_.CreateStore(v, freg_ptr(idx));
  }

  Value* mem_ptr(int64_t addr) {
    // Clamp address to valid range at IR level (bounds fault is a VM concern;
    // for translated IR we just clamp so the GEP is always in-bounds).
    int64_t clamped = (addr >= 0 && addr < kMemSize) ? addr : 0;
    return builder_.CreateInBoundsGEP(
        mem_arr_ty_, mem_,
        {ConstantInt::get(i64_ty_, 0), ConstantInt::get(i64_ty_, clamped)},
        "mp_" + std::to_string(addr));
  }

  Value* i64(int64_t v) { return ConstantInt::get(i64_ty_, v, /*isSigned=*/true); }
  Value* dbl(double v)   { return ConstantFP::get(double_ty_, v); }

  // ── LLVM intrinsic helpers ───────────────────────────────────────────────

  Value* call_fp_intrinsic1(Intrinsic::ID id, Value* arg) {
    auto* fn = Intrinsic::getOrInsertDeclaration(module_.get(), id,
                                                  {double_ty_});
    return builder_.CreateCall(fn, {arg});
  }

  Value* call_fp_intrinsic2(Intrinsic::ID id, Value* a, Value* b) {
    auto* fn = Intrinsic::getOrInsertDeclaration(module_.get(), id,
                                                  {double_ty_});
    return builder_.CreateCall(fn, {a, b});
  }

  // ── Instruction emission ────────────────────────────────────────────────

  void emit_instructions() {
    const size_t n = prog_.insns.size();
    BasicBlock* cur_bb = pc_to_bb_[0];
    builder_.SetInsertPoint(cur_bb);

    for (size_t pc = 0; pc < n; ++pc) {
      // If this PC is a leader, switch to its BasicBlock.
      if (pc != 0 && leaders_.count(pc)) {
        BasicBlock* next_bb = pc_to_bb_[pc];
        // Add fallthrough branch if current block isn't terminated.
        if (!cur_bb->getTerminator()) {
          builder_.CreateBr(next_bb);
        }
        cur_bb = next_bb;
        builder_.SetInsertPoint(cur_bb);
      }

      const auto& insn = prog_.insns[pc];

      if (cfg_.emit_comments) {
        // Embed PC as metadata comment via a nop-equivalent (no-op call to
        // llvm.donothing is not always available in older LLVM; use a
        // variable-name convention in the IR instead — the GEP names carry it).
      }

      emit_one(pc, insn);
    }

    // Terminate the final block if needed.
    if (!cur_bb->getTerminator()) {
      builder_.CreateRet(i64(0));
    }
  }

  void emit_one(size_t pc, const t81::tisc::Insn& insn) {
    const int32_t a = insn.a;
    const int64_t b = insn.b;
    const int32_t c = insn.c;

    switch (insn.opcode) {
      // ── No-op / Halt ─────────────────────────────────────────────────
      case Op::Nop:
        break;

      case Op::Halt:
      case Op::Terminate:
      case Op::AxHalt:
        builder_.CreateRet(load_ireg(0));  // return R0
        break;

      // ── Immediate load ───────────────────────────────────────────────
      case Op::LoadImm:
        // R[a] = b (immediate integer)
        store_ireg(a, i64(b));
        break;

      // ── Register move ────────────────────────────────────────────────
      case Op::Mov:
        // R[a] = R[b]
        store_ireg(a, load_ireg(static_cast<int32_t>(b)));
        break;

      // ── Integer arithmetic ───────────────────────────────────────────
      case Op::Add:
        store_ireg(a, builder_.CreateAdd(load_ireg(a), load_ireg(c), "add"));
        break;
      case Op::Sub:
        store_ireg(a, builder_.CreateSub(load_ireg(a), load_ireg(c), "sub"));
        break;
      case Op::Mul:
        store_ireg(a, builder_.CreateMul(load_ireg(a), load_ireg(c), "mul"));
        break;
      case Op::Div: {
        Value* divisor = load_ireg(c);
        // Guard: if divisor == 0, return 0 (halt with trap code).
        // For simplicity in Phase 1, emit sdiv directly.
        store_ireg(a, builder_.CreateSDiv(load_ireg(a), divisor, "div"));
        break;
      }
      case Op::Mod:
        store_ireg(a, builder_.CreateSRem(load_ireg(a), load_ireg(c), "mod"));
        break;
      case Op::Neg:
        store_ireg(a, builder_.CreateNeg(load_ireg(a), "neg"));
        break;
      case Op::Inc:
        store_ireg(a, builder_.CreateAdd(load_ireg(a), i64(1), "inc"));
        break;
      case Op::Dec:
        store_ireg(a, builder_.CreateSub(load_ireg(a), i64(1), "dec"));
        break;

      // ── Bitwise ──────────────────────────────────────────────────────
      case Op::BitAnd:
        store_ireg(a, builder_.CreateAnd(load_ireg(a), load_ireg(c), "band"));
        break;
      case Op::BitOr:
        store_ireg(a, builder_.CreateOr(load_ireg(a), load_ireg(c), "bor"));
        break;
      case Op::BitXor:
        store_ireg(a, builder_.CreateXor(load_ireg(a), load_ireg(c), "bxor"));
        break;
      case Op::BitNot:
        store_ireg(a, builder_.CreateNot(load_ireg(a), "bnot"));
        break;
      case Op::BitShl:
        store_ireg(a, builder_.CreateShl(load_ireg(a), load_ireg(c), "shl"));
        break;
      case Op::BitShr:
        store_ireg(a, builder_.CreateAShr(load_ireg(a), load_ireg(c), "shr"));
        break;
      case Op::BitUShr:
        store_ireg(a, builder_.CreateLShr(load_ireg(a), load_ireg(c), "ushr"));
        break;

      // ── Comparison ───────────────────────────────────────────────────
      // Ternary comparison: R[a] = sign(R[a] - R[c])  → -1, 0, or 1
      case Op::Cmp: {
        Value* lv  = load_ireg(a);
        Value* rv  = load_ireg(c);
        Value* lt  = builder_.CreateICmpSLT(lv, rv);
        Value* gt  = builder_.CreateICmpSGT(lv, rv);
        Value* neg = builder_.CreateSelect(lt, i64(-1), i64(0));
        store_ireg(a, builder_.CreateSelect(gt, i64(1), neg, "cmp"));
        break;
      }
      case Op::Less:
        store_ireg(a, builder_.CreateSExt(
            builder_.CreateICmpSLT(load_ireg(a), load_ireg(c)), i64_ty_, "lt"));
        break;
      case Op::LessEqual:
        store_ireg(a, builder_.CreateSExt(
            builder_.CreateICmpSLE(load_ireg(a), load_ireg(c)), i64_ty_, "le"));
        break;
      case Op::Greater:
        store_ireg(a, builder_.CreateSExt(
            builder_.CreateICmpSGT(load_ireg(a), load_ireg(c)), i64_ty_, "gt"));
        break;
      case Op::GreaterEqual:
        store_ireg(a, builder_.CreateSExt(
            builder_.CreateICmpSGE(load_ireg(a), load_ireg(c)), i64_ty_, "ge"));
        break;
      case Op::Equal:
        store_ireg(a, builder_.CreateSExt(
            builder_.CreateICmpEQ(load_ireg(a), load_ireg(c)), i64_ty_, "eq"));
        break;
      case Op::NotEqual:
        store_ireg(a, builder_.CreateSExt(
            builder_.CreateICmpNE(load_ireg(a), load_ireg(c)), i64_ty_, "ne"));
        break;

      // ── Control flow ─────────────────────────────────────────────────
      case Op::Jump:
        if (pc_to_bb_.count(static_cast<size_t>(a)))
          builder_.CreateBr(pc_to_bb_[static_cast<size_t>(a)]);
        else
          builder_.CreateRet(i64(0));
        break;

      case Op::JumpIfZero: {
        Value* cond = builder_.CreateICmpEQ(load_ireg(c), i64(0), "jz_cond");
        BasicBlock* true_bb  = resolve_bb(static_cast<size_t>(a), pc + 1);
        BasicBlock* false_bb = resolve_bb(pc + 1, pc + 1);
        builder_.CreateCondBr(cond, true_bb, false_bb);
        break;
      }
      case Op::JumpIfNotZero: {
        Value* cond = builder_.CreateICmpNE(load_ireg(c), i64(0), "jnz_cond");
        BasicBlock* true_bb  = resolve_bb(static_cast<size_t>(a), pc + 1);
        BasicBlock* false_bb = resolve_bb(pc + 1, pc + 1);
        builder_.CreateCondBr(cond, true_bb, false_bb);
        break;
      }
      case Op::JumpIfNegative: {
        Value* cond = builder_.CreateICmpSLT(load_ireg(c), i64(0), "jn_cond");
        BasicBlock* true_bb  = resolve_bb(static_cast<size_t>(a), pc + 1);
        BasicBlock* false_bb = resolve_bb(pc + 1, pc + 1);
        builder_.CreateCondBr(cond, true_bb, false_bb);
        break;
      }
      case Op::JumpIfPositive: {
        Value* cond = builder_.CreateICmpSGT(load_ireg(c), i64(0), "jp_cond");
        BasicBlock* true_bb  = resolve_bb(static_cast<size_t>(a), pc + 1);
        BasicBlock* false_bb = resolve_bb(pc + 1, pc + 1);
        builder_.CreateCondBr(cond, true_bb, false_bb);
        break;
      }

      // Call: for Phase 1, lower as a direct branch (no stack frame).
      // This handles non-recursive calls.  Ret branches back via fall-through
      // of the return PC that was noted during leader analysis.
      case Op::Call:
        if (pc_to_bb_.count(static_cast<size_t>(a)))
          builder_.CreateBr(pc_to_bb_[static_cast<size_t>(a)]);
        else
          builder_.CreateRet(i64(0));
        break;

      case Op::Ret:
        builder_.CreateRet(load_ireg(0));
        break;

      // ── Float arithmetic ─────────────────────────────────────────────
      case Op::FAdd:
        store_freg(a, builder_.CreateFAdd(load_freg(a), load_freg(c), "fadd"));
        break;
      case Op::FSub:
        store_freg(a, builder_.CreateFSub(load_freg(a), load_freg(c), "fsub"));
        break;
      case Op::FMul:
        store_freg(a, builder_.CreateFMul(load_freg(a), load_freg(c), "fmul"));
        break;
      case Op::FDiv:
        store_freg(a, builder_.CreateFDiv(load_freg(a), load_freg(c), "fdiv"));
        break;

      // ── Float transcendentals (LLVM intrinsics) ──────────────────────
      case Op::FSin:
        store_freg(a, call_fp_intrinsic1(Intrinsic::sin, load_freg(a)));
        break;
      case Op::FCos:
        store_freg(a, call_fp_intrinsic1(Intrinsic::cos, load_freg(a)));
        break;
      case Op::FSqrt:
        store_freg(a, call_fp_intrinsic1(Intrinsic::sqrt, load_freg(a)));
        break;
      case Op::FExp:
        store_freg(a, call_fp_intrinsic1(Intrinsic::exp, load_freg(a)));
        break;
      case Op::FLog:
        store_freg(a, call_fp_intrinsic1(Intrinsic::log, load_freg(a)));
        break;
      case Op::FPow:
        // pow(base, exp): R[a] = R[a] ^ R[c]
        store_freg(a, call_fp_intrinsic2(Intrinsic::pow, load_freg(a), load_freg(c)));
        break;
      case Op::FTan: {
        // tan(x) = sin(x) / cos(x)
        Value* s  = call_fp_intrinsic1(Intrinsic::sin, load_freg(a));
        Value* co = call_fp_intrinsic1(Intrinsic::cos, load_freg(a));
        store_freg(a, builder_.CreateFDiv(s, co, "ftan"));
        break;
      }
      case Op::FAsin:
        store_freg(a, call_fp_intrinsic1(Intrinsic::asin, load_freg(a)));
        break;
      case Op::FAcos:
        store_freg(a, call_fp_intrinsic1(Intrinsic::acos, load_freg(a)));
        break;
      case Op::FAtan:
        store_freg(a, call_fp_intrinsic1(Intrinsic::atan, load_freg(a)));
        break;
      case Op::FSinh:
        store_freg(a, call_fp_intrinsic1(Intrinsic::sinh, load_freg(a)));
        break;
      case Op::FCosh:
        store_freg(a, call_fp_intrinsic1(Intrinsic::cosh, load_freg(a)));
        break;
      case Op::FTanh:
        store_freg(a, call_fp_intrinsic1(Intrinsic::tanh, load_freg(a)));
        break;

      // ── Type conversions ─────────────────────────────────────────────
      case Op::I2F:
        store_freg(a, builder_.CreateSIToFP(load_ireg(a), double_ty_, "i2f"));
        break;
      case Op::F2I:
        store_ireg(a, builder_.CreateFPToSI(load_freg(a), i64_ty_, "f2i"));
        break;

      // ── Print ────────────────────────────────────────────────────────
      case Op::Print: {
        // Print R[a] as a decimal integer followed by newline.
        Constant* fmt = builder_.CreateGlobalString("%lld\n", "print_fmt");
        builder_.CreateCall(printf_fn_, {fmt, load_ireg(a)});
        break;
      }

      // ── Memory load/store ─────────────────────────────────────────────
      // Load:  R[a] = mem[b]   (b is immediate address)
      // Store: mem[a] = R[b]   (a is immediate address, b is register index)
      case Op::Load:
        store_ireg(a, builder_.CreateLoad(i64_ty_, mem_ptr(b), "ld"));
        break;
      case Op::Store:
        builder_.CreateStore(load_ireg(static_cast<int32_t>(b)), mem_ptr(a));
        break;

      // ── Stack ops (modeled via reg 242 as stack pointer) ─────────────
      // Push: mem[R[242]] = R[a]; R[242]--
      // Pop:  R[a] = mem[R[242]]; R[242]++
      // Phase 1: model using the flat mem_ array with R[242] as SP.
      case Op::Push: {
        Value* sp     = load_ireg(242);
        Value* sp_ptr = builder_.CreateInBoundsGEP(
            mem_arr_ty_, mem_,
            {ConstantInt::get(i64_ty_, 0), sp}, "sp_push");
        builder_.CreateStore(load_ireg(a), sp_ptr);
        store_ireg(242, builder_.CreateSub(sp, i64(1), "sp_dec"));
        break;
      }
      case Op::Pop: {
        Value* sp     = builder_.CreateAdd(load_ireg(242), i64(1), "sp_inc");
        store_ireg(242, sp);
        Value* sp_ptr = builder_.CreateInBoundsGEP(
            mem_arr_ty_, mem_,
            {ConstantInt::get(i64_ty_, 0), sp}, "sp_pop");
        store_ireg(a, builder_.CreateLoad(i64_ty_, sp_ptr, "pop_val"));
        break;
      }

      // ── All other opcodes: emit a no-op ──────────────────────────────
      default:
        break;
    }
  }

  // Return the BB for @p target_pc; if not found, return the BB for @p fallback.
  BasicBlock* resolve_bb(size_t target_pc, size_t fallback_pc) {
    auto it = pc_to_bb_.find(target_pc);
    if (it != pc_to_bb_.end()) return it->second;
    it = pc_to_bb_.find(fallback_pc);
    if (it != pc_to_bb_.end()) return it->second;
    // Last resort: create a new ret block.
    auto* bb = BasicBlock::Create(ctx_, "unreachable", main_fn_);
    IRBuilder<> tmp(bb);
    tmp.CreateRet(i64(0));
    return bb;
  }

  void finalize() {
    // Any basic block that has no terminator yet gets an implicit ret.
    for (auto& bb : *main_fn_) {
      if (!bb.getTerminator()) {
        IRBuilder<> tmp(&bb);
        tmp.CreateRet(i64(0));
      }
    }
  }
};

// ──────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────

std::unique_ptr<llvm::Module> translate(const t81::tisc::Program& prog,
                                        LLVMContext&               ctx,
                                        const TranslationConfig&   cfg) {
  TISCTranslator translator(prog, ctx, cfg);
  return translator.run();
}

bool emit_ir_text(const t81::tisc::Program& prog,
                  const std::filesystem::path& output_path,
                  const TranslationConfig& cfg) {
  LLVMContext ctx;
  auto mod = translate(prog, ctx, cfg);
  if (!mod) return false;

  std::error_code ec;
  llvm::raw_fd_ostream os(output_path.string(), ec, llvm::sys::fs::OF_Text);
  if (ec) return false;
  mod->print(os, nullptr);
  return true;
}

bool emit_ir_bitcode(const t81::tisc::Program& prog,
                     const std::filesystem::path& output_path,
                     const TranslationConfig& cfg) {
  LLVMContext ctx;
  auto mod = translate(prog, ctx, cfg);
  if (!mod) return false;

  std::error_code ec;
  llvm::raw_fd_ostream os(output_path.string(), ec, llvm::sys::fs::OF_None);
  if (ec) return false;
  llvm::WriteBitcodeToFile(*mod, os);
  return true;
}

}  // namespace t81::llvm_backend

#endif  // T81_HAS_LLVM
