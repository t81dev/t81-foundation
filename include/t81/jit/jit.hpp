#pragma once

#include <functional>
#include <vector>
#include "t81/isa/program.hpp"
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
  };

  struct ExecResult {
    std::size_t instructions_executed{0};
    ExitKind exit_kind{ExitKind::Completed};
  };

  virtual ~JitTrace() = default;
  virtual ExecResult execute(State& state, const PolicyHook& policy_hook = {}) = 0;
  virtual std::size_t size() const = 0;
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
