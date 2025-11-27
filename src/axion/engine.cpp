#include "t81/axion/engine.hpp"

namespace t81::axion {
class AllowAllEngine : public Engine {
 public:
  Verdict evaluate(const SyscallContext&) override {
    return {VerdictKind::Allow, "Default policy: allow all"};
  }
};

class InstructionCountingEngine : public Engine {
 public:
  explicit InstructionCountingEngine(std::size_t max_instructions)
      : _max_instructions(max_instructions), _instruction_count(0) {}

  Verdict evaluate(const SyscallContext&) override {
    _instruction_count++;
    if (_instruction_count > _max_instructions) {
      return {VerdictKind::Deny, "Instruction limit exceeded"};
    }
    return {VerdictKind::Allow, "Instruction count within limit"};
  }

 private:
  std::size_t _max_instructions;
  std::size_t _instruction_count;
};

std::unique_ptr<Engine> make_allow_all_engine() {
  return std::make_unique<AllowAllEngine>();
}

std::unique_ptr<Engine> make_instruction_counting_engine(
    std::size_t max_instructions) {
  return std::make_unique<InstructionCountingEngine>(max_instructions);
}
}  // namespace t81::axion
