/**
 * @file tisc_to_mlir.hpp
 * @brief TISC → MLIR translator public API.
 *
 * Lowers a TISC program to MLIR using standard dialects (arith, math, func,
 * cf, memref). Constant-index register-file memrefs are scalar-replaced and
 * promoted to SSA during the MLIR → LLVM lowering pipeline. Two float modes
 * are available:
 *
 *   FloatMode::Compat  — standard math.* ops (IEEE-754 f64, non-DCP surface)
 *   FloatMode::DCP     — func.call @t81_dmath_* (dmath, DCP-compatible when
 *                        linked against the T81 runtime library)
 *
 * The MLIR text output (.mlir) is compatible with any MLIR toolchain.
 * DCP mode is opt-in and requires the T81 runtime at link time.
 *
 * Build with: cmake -DT81_ENABLE_MLIR=ON -DT81_ENABLE_LLVM=ON
 */
#pragma once

#ifdef T81_HAS_MLIR

#include <filesystem>
#include <memory>
#include <string>

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"

#include "t81/isa/program.hpp"

namespace t81::mlir_frontend {

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

/// Float translation mode — controls how TISC float/transcendental ops are
/// lowered into MLIR.
enum class FloatMode {
  /// Standard math.* dialect ops (IEEE-754 f64, maximum toolchain compat).
  /// This is a non-DCP surface; float results may vary across platforms.
  Compat,

  /// Emit func.call @t81_dmath_* instead of math.* ops.
  /// Preserves T81 determinism guarantees when linked against the T81 runtime.
  /// The generated MLIR is still valid standard MLIR (uses func.call).
  DCP,
};

struct TranslationConfig {
  std::string module_name   = "t81_program";
  FloatMode   float_mode    = FloatMode::Compat;
  bool        use_t81_dialect = false;  ///< emit custom t81.reg_* ops for register access
  bool        emit_comments = true;  ///< embed PC annotations in block names
  bool        verify_module = true;  ///< run MLIR verifier after translation
};

// ---------------------------------------------------------------------------
// Primary translation API
// ---------------------------------------------------------------------------

/// Translate a TISC program into an MLIR ModuleOp.
/// The returned module is owned by the caller; the context must outlive it.
mlir::OwningOpRef<mlir::ModuleOp> translate(
    const t81::tisc::Program& prog,
    mlir::MLIRContext&         ctx,
    const TranslationConfig&   cfg = {});

/// Translate and write MLIR text (.mlir) to @p output_path.
bool emit_mlir_text(
    const t81::tisc::Program&    prog,
    const std::filesystem::path& output_path,
    const TranslationConfig&     cfg = {});

// ---------------------------------------------------------------------------
// Lowering API (MLIR → LLVM IR)
// ---------------------------------------------------------------------------

/// Run the standard dialect-to-LLVM conversion pass pipeline on an already-
/// translated module and write LLVM IR text (.ll) to @p output_path.
/// Consumes the module (runs passes in-place).
bool lower_to_llvm_ir(
    mlir::OwningOpRef<mlir::ModuleOp>& module,
    mlir::MLIRContext&                  ctx,
    const std::filesystem::path&        output_path);

/// Parse an existing .mlir file and lower it to LLVM IR text.
bool lower_mlir_file(
    const std::filesystem::path& mlir_input,
    const std::filesystem::path& output_path);

/// One-shot convenience: TISC → MLIR → LLVM IR.
bool pipeline_to_llvm(
    const t81::tisc::Program&    prog,
    const std::filesystem::path& output_path,
    const TranslationConfig&     cfg = {});

}  // namespace t81::mlir_frontend

#endif  // T81_HAS_MLIR
