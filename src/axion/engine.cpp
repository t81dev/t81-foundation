#include "t81/axion/engine.hpp"

namespace t81::axion {
class AllowAllEngine : public Engine {
 public:
  Verdict evaluate(const SyscallContext&) override {
    // TODO: Implement full Axion policy (spec/axion-kernel.md).
    return {VerdictKind::Allow, "TODO: full Axion policy"};
  }
};

std::unique_ptr<Engine> make_allow_all_engine() { return std::make_unique<AllowAllEngine>(); }
}  // namespace t81::axion

