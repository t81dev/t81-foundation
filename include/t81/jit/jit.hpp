#pragma once

#include <functional>
#include <span>
#include <vector>
#include "t81/isa/program.hpp"
#include "t81/tracing/canonhash.hpp"
#include "t81/vm/state.hpp"

namespace t81::vm {

/**
 * @class JitTrace
 * @brief A compiled trace of TISC instructions.
 */
class JitTrace {
public:
  using PolicyHook =
      std::function<bool(std::size_t pc, const t81::tisc::Insn& insn, std::size_t executed_so_far)>;

  enum class ExitKind {
    Completed,
    Branch,
    GuardDeopt,
    PolicyDeny,
    /// RFC-0028 §5: Axion-gated opcode encountered (AxRead/AxSet/AxVerify/AxReport).
    /// The trace terminates cleanly; the interpreter resumes and evaluates the
    /// Axion boundary natively via policy_engine.cpp.
    AxionBoundary,
  };

  struct ExecResult {
    std::size_t instructions_executed{0};
    ExitKind exit_kind{ExitKind::Completed};
  };

  virtual ~JitTrace() = default;
  virtual ExecResult execute(State& state, const PolicyHook& policy_hook = {}) = 0;
  virtual std::size_t size() const = 0;

  /// RFC-0028 §4: Read-only view of the compiled instruction sequence.
  /// Used by JitTraceCache::store() to serialise the trace into CanonFS.
  virtual std::span<const t81::tisc::Insn> instructions() const = 0;

  /// RFC-0028 §2: Canonical identity of this trace.
  /// Computed from CanonHash81(serialised TISC instruction sequence).
  /// Zero-valued if the trace was not compiled via JitCompiler::compile().
  const t81::hash::CanonHash81& trace_hash() const noexcept { return trace_hash_; }

protected:
  t81::hash::CanonHash81 trace_hash_{};
};

/**
 * @class JitCompiler
 * @brief Minimal trace recorder and compiler for HanoiVM.
 */
class JitCompiler {
public:
  void start_tracing(std::size_t pc);
  void record_instruction(const t81::tisc::Insn& insn);
  std::unique_ptr<JitTrace> compile();
  std::size_t trace_start_pc() const { return start_pc_; }

  bool is_tracing() const { return tracing_; }

private:
  bool tracing_{false};
  std::size_t start_pc_{0};
  std::vector<t81::tisc::Insn> trace_buffer_;
};

}  // namespace t81::vm
