#include <cassert>
#include <cmath>
#include <functional>
#include <string>
#include <t81/lang/compiler.hpp>
#include <t81/lang/parser.hpp>
#include <t81/vm/vm.hpp>

namespace {
constexpr double expected_base81_float() {
  // 1.20t81 = 1 + 2/81
  return 1.0 + (2.0 / 81.0);
}

void run_and_check(const std::string& src,
                   const std::function<void(const t81::vm::State&)>& verifier) {
  auto mod_res = t81::lang::parse_module(src);
  assert(mod_res.has_value());
  t81::lang::Compiler comp;
  auto prog_res = comp.compile(mod_res.value());
  assert(prog_res.has_value());
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(prog_res.value());
  auto run_res = vm->run_to_halt();
  assert(run_res.has_value());
  verifier(vm->state());
}
}  // namespace

int main() {
  using namespace t81;

  // Verify literals stored in registers map to pools.
  run_and_check(
      "fn main() -> T81Int { "
      "let f: T81Float = 1.20t81; "
      "let q: T81Fraction = 22/7t81; "
      "let s: Symbol = :alpha; "
      "return 0; }",
      [](const vm::State& state) {
        const double expected = expected_base81_float();
        int float_handle = static_cast<int>(state.registers[1]);
        assert(float_handle == 1);
        assert(float_handle <= static_cast<int>(state.floats.size()));
        assert(std::fabs(state.floats[float_handle - 1] - expected) < 1e-9);

        int frac_handle = static_cast<int>(state.registers[2]);
        assert(frac_handle == 1);
        assert(frac_handle <= static_cast<int>(state.fractions.size()));
        const auto& frac = state.fractions[frac_handle - 1];
        assert(frac.num.to_int64() == 164);  // 22 base-81 = 164 decimal
        assert(frac.den.to_int64() == 7);

        int sym_handle = static_cast<int>(state.registers[3]);
        assert(sym_handle == 1);
        assert(sym_handle <= static_cast<int>(state.symbols.size()));
        assert(state.symbols[sym_handle - 1] == "alpha");
      });

  // Returning a float literal loads handle into R0.
  run_and_check("fn main() -> T81Float { return 1.20t81; }",
                [](const vm::State& state) {
                  const double expected = expected_base81_float();
                  int handle = static_cast<int>(state.registers[0]);
                  assert(handle == 1);
                  assert(handle <= static_cast<int>(state.floats.size()));
                  assert(std::fabs(state.floats[handle - 1] - expected) < 1e-9);
                });

  // Returning a fraction literal.
  run_and_check("fn main() -> T81Fraction { return 22/7t81; }",
                [](const vm::State& state) {
                  int handle = static_cast<int>(state.registers[0]);
                  assert(handle == 1);
                  assert(handle <= static_cast<int>(state.fractions.size()));
                  const auto& frac = state.fractions[handle - 1];
                  assert(frac.num.to_int64() == 164);
                  assert(frac.den.to_int64() == 7);
                });

  // Returning a symbol literal.
  run_and_check("fn main() -> Symbol { return :omega; }",
                [](const vm::State& state) {
                  int handle = static_cast<int>(state.registers[0]);
                  assert(handle == 1);
                  assert(handle <= static_cast<int>(state.symbols.size()));
                  assert(state.symbols[handle - 1] == "omega");
                });

  return 0;
}
