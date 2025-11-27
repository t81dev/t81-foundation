#pragma once

#include <memory>
#include "t81/axion/context.hpp"
#include "t81/axion/verdict.hpp"

namespace t81::axion {
class Engine {
 public:
  virtual ~Engine() = default;
  virtual Verdict evaluate(const SyscallContext& context) = 0;
};

std::unique_ptr<Engine> make_allow_all_engine();
std::unique_ptr<Engine> make_instruction_counting_engine(
    std::size_t max_instructions);
}  // namespace t81::axion

