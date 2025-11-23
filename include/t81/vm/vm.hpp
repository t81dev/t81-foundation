#pragma once

#include <memory>
#include <t81/support/expected.hpp>
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
};

// Factory for the in-tree interpreter implementation.
std::unique_ptr<IVirtualMachine> make_interpreter_vm();
}  // namespace t81::vm

