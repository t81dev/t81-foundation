#pragma once

#include <filesystem>
#include <memory>
#include <string_view>
#include <t81/support/expected.hpp>
#include "t81/axion/engine.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/state.hpp"
#include "t81/vm/traps.hpp"

#include <vector>

namespace t81::axion {
// Forward declaration — avoids pulling state.hpp heavyweight transitive chain
// into every consumer of vm.hpp a second time.
class DeterminismDetector;
}  // namespace t81::axion

namespace t81::vm {

class IVirtualMachine {
public:
  virtual ~IVirtualMachine() = default;
  virtual void load_program(const t81::tisc::Program& program) = 0;
  virtual void set_fault_injections(std::vector<FaultInjection> faults) = 0;
  virtual std::expected<void, Trap> step() = 0;
  virtual std::expected<void, Trap> run_to_halt(std::size_t max_steps = 100000) = 0;
  virtual const State& state() const = 0;
  virtual void set_register(int idx, std::int64_t value, ValueTag tag = ValueTag::Int) = 0;
  /// Write a single word into flat VM memory at `word_index` with ValueTag::Int.
  /// No-op if `word_index` is out of range.  Must be called after load_program().
  virtual void set_memory_word(std::size_t word_index, std::int64_t value) noexcept = 0;

  /// Write a single word with an explicit ValueTag.
  /// No-op if `word_index` is out of range.  Must be called after load_program().
  virtual void set_memory_word_tagged(std::size_t word_index,
                                      std::int64_t value,
                                      ValueTag     tag) noexcept = 0;

  /// Allocate a float in the VM's float handle pool and return the 1-based handle.
  /// Used by the DPE task runner to restore FloatHandle words from delta snapshots
  /// without coupling the snapshot format to transient pool indices.
  virtual std::int64_t intern_float(double value) noexcept = 0;
  virtual std::int64_t load_weights_tensor(std::string_view name) = 0;
  virtual const t81::weights::NativeTensor* weights_tensor(std::int64_t handle) const = 0;

  /// Attach a CanonFS driver rooted at the given path.
  /// Overrides T81_CANONFS_ROOT for this VM instance.
  virtual void set_canonfs_root(const std::filesystem::path& /*root*/) {}

  /// Opt-in hook for CI harnesses: inject a DeterminismDetector to compare
  /// axion_log hash chains across successive runs (AX-M5).
  /// Default no-op — override in concrete implementations.
  virtual void set_determinism_detector(t81::axion::DeterminismDetector* /*detector*/) {}
};

// Factory for the in-tree interpreter implementation.
std::unique_ptr<IVirtualMachine> make_interpreter_vm(
    std::unique_ptr<t81::axion::Engine> engine = nullptr);
}  // namespace t81::vm
