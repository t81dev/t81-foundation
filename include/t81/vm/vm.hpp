#pragma once

#include <memory>
#include <string_view>
#include <t81/support/expected.hpp>
#include "t81/axion/engine.hpp"
#include "t81/vm/state.hpp"
#include "t81/vm/traps.hpp"
#include "t81/tisc/program.hpp"

namespace t81::vm {
class IVirtualMachine {
 public:
  virtual ~IVirtualMachine() = default;
  virtual void load_program(const t81::tisc::Program& program) = 0;
  virtual std::expected<void, Trap> step() = 0;
  virtual std::expected<void, Trap> run_to_halt(std::size_t max_steps = 100000) = 0;
  virtual const State& state() const = 0;
  virtual std::int64_t load_weights_tensor(std::string_view name) = 0;
  virtual const t81::weights::NativeTensor* weights_tensor(std::int64_t handle) const = 0;
};

// Factory for the in-tree interpreter implementation.
std::unique_ptr<IVirtualMachine> make_interpreter_vm(std::unique_ptr<t81::axion::Engine> engine = nullptr);
}  // namespace t81::vm
