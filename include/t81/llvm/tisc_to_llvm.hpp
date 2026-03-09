/**
 * @file tisc_to_llvm.hpp
 * @brief TISC bytecode → LLVM IR translation.
 *
 * Lowers a compiled t81::tisc::Program to an llvm::Module containing a single
 * entry function `@t81_main() -> i64` that models the TISC program's execution.
 *
 * Register model
 * ──────────────
 * • Integer register file : [243 x i64]   (R0–R242, all TISC int/comparison ops)
 * • Float register file   : [243 x double] (R0–R242, all F* opcodes)
 * Registers are thread-local stack allocas inside @t81_main.
 * I2F / F2I conversion opcodes bridge between the two files.
 *
 * Control flow
 * ────────────
 * A two-pass algorithm identifies all basic-block leaders (jump targets +
 * fall-throughs), creates an llvm::BasicBlock per leader, then lowers
 * instructions sequentially.  Unknown / privileged opcodes are lowered to a
 * no-op so the output always assembles.
 *
 * Requires T81_ENABLE_LLVM=ON at CMake configure time.
 */
#pragma once

#ifdef T81_HAS_LLVM

#include <filesystem>
#include <memory>
#include <string>

// Forward-declare LLVM types so consumers don't have to pull in all of LLVM
// just to use the public API.
namespace llvm {
class LLVMContext;
class Module;
}  // namespace llvm

#include "t81/isa/program.hpp"

namespace t81::llvm_backend {

struct TranslationConfig {
  std::string module_name    = "t81_program";
  bool        emit_comments  = true;   ///< Annotate each instruction in the IR
  bool        verify_module  = true;   ///< Run llvm::verifyModule() before returning
};

/**
 * Translate a TISC program to an LLVM Module.
 * Returns nullptr if translation fails (e.g., empty program).
 */
std::unique_ptr<llvm::Module> translate(
    const t81::tisc::Program& prog,
    llvm::LLVMContext&        ctx,
    const TranslationConfig&  cfg = {});

/**
 * Translate and write human-readable LLVM IR (.ll) to @p output_path.
 * Returns true on success.
 */
bool emit_ir_text(
    const t81::tisc::Program& prog,
    const std::filesystem::path& output_path,
    const TranslationConfig& cfg = {});

/**
 * Translate and write LLVM bitcode (.bc) to @p output_path.
 * Returns true on success.
 */
bool emit_ir_bitcode(
    const t81::tisc::Program& prog,
    const std::filesystem::path& output_path,
    const TranslationConfig& cfg = {});

}  // namespace t81::llvm_backend

#endif  // T81_HAS_LLVM
