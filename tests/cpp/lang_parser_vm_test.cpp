#include <cassert>
#include <cmath>
#include <string>
#include <t81/lang/parser.hpp>
#include <t81/lang/compiler.hpp>
#include <t81/vm/vm.hpp>

int main() {
  using namespace t81;
  {
    const std::string src =
        "fn triple(v: T81Int) -> T81Int { return add(v, v) + v; }"
        "fn add(a: T81Int, b: T81Int) -> T81Int { return a + b; }"
        "fn main() -> T81Int { let start: T81Int = 2t81; triple(start); "
        "let agg: T81Int = triple(add(1t81, 2t81)); return agg + add(3t81, 4t81); }";
    auto mod_res = lang::parse_module(src);
    assert(mod_res.has_value());
    lang::Compiler comp;
    auto prog_res = comp.compile(mod_res.value());
    assert(prog_res.has_value());

    auto vm = vm::make_interpreter_vm();
    vm->load_program(prog_res.value());
    auto run = vm->run_to_halt();
    assert(run.has_value());
    assert(vm->state().registers[0] == 16);
  }

  {
    const std::string src =
        "fn blend(a: T81Float, b: T81Float) -> T81Float { "
        "let sum: T81Float = a + b; "
        "return sum * a; }"
        "fn main() -> T81Float { "
        "let left: T81Float = 1.20t81; "
        "let right: T81Float = 2.00t81; "
        "return blend(left, right); }";
    auto mod_res = lang::parse_module(src);
    assert(mod_res.has_value());
    lang::Compiler comp;
    auto prog_res = comp.compile(mod_res.value());
    assert(prog_res.has_value());

    auto vm = vm::make_interpreter_vm();
    vm->load_program(prog_res.value());
    auto run = vm->run_to_halt();
    assert(run.has_value());
    double left = 1.0 + (2.0 / 81.0);
    double right = 2.0;
    double expected = (left + right) * left;
    int handle = static_cast<int>(vm->state().registers[0]);
    assert(handle > 0);
    assert(static_cast<std::size_t>(handle) <= vm->state().floats.size());
    assert(std::fabs(vm->state().floats[handle - 1] - expected) < 1e-9);
  }

  {
    const std::string src =
        "fn mix(a: T81Fraction, b: T81Fraction) -> T81Fraction { "
        "let prod: T81Fraction = a * b; "
        "return prod + a; }"
        "fn main() -> T81Fraction { "
        "let first: T81Fraction = 1/2t81; "
        "let second: T81Fraction = 2/3t81; "
        "return mix(first, second); }";
    auto mod_res = lang::parse_module(src);
    assert(mod_res.has_value());
    lang::Compiler comp;
    auto prog_res = comp.compile(mod_res.value());
    assert(prog_res.has_value());

    auto vm = vm::make_interpreter_vm();
    vm->load_program(prog_res.value());
    auto run = vm->run_to_halt();
    assert(run.has_value());
    int handle = static_cast<int>(vm->state().registers[0]);
    assert(handle > 0);
    assert(static_cast<std::size_t>(handle) <= vm->state().fractions.size());
    const auto& frac = vm->state().fractions[handle - 1];
    assert(frac.num.to_int64() == 5);
    assert(frac.den.to_int64() == 6);
  }

  {
    const std::string src =
        "fn main() -> T81Int { let a: T81Int = 1t81; let b: T81Int = 3t81; "
        "if (a < b) { return 1t81; } else { return 0t81; } }";
    auto mod_res = lang::parse_module(src);
    assert(mod_res.has_value());
    lang::Compiler comp;
    auto prog_res = comp.compile(mod_res.value());
    assert(prog_res.has_value());
    auto vm = vm::make_interpreter_vm();
    vm->load_program(prog_res.value());
    auto run = vm->run_to_halt();
    assert(run.has_value());
    assert(vm->state().registers[0] == 1);
  }

  {
    const std::string src =
        "fn main() -> T81Int { "
        "let x: T81Float = 1.20t81; "
        "let y: T81Float = 1.20t81; "
        "return x == y; }";
    auto mod_res = lang::parse_module(src);
    assert(mod_res.has_value());
    lang::Compiler comp;
    auto prog_res = comp.compile(mod_res.value());
    assert(prog_res.has_value());
    auto vm = vm::make_interpreter_vm();
    vm->load_program(prog_res.value());
    auto run = vm->run_to_halt();
    assert(run.has_value());
    assert(vm->state().registers[0] == 1);
  }

  {
    const std::string src =
        "fn main() -> T81Int { "
        "let a: T81Fraction = 1/2t81; "
        "let b: T81Fraction = 3/4t81; "
        "return b >= a; }";
    auto mod_res = lang::parse_module(src);
    assert(mod_res.has_value());
    lang::Compiler comp;
    auto prog_res = comp.compile(mod_res.value());
    assert(prog_res.has_value());
    auto vm = vm::make_interpreter_vm();
    vm->load_program(prog_res.value());
    auto run = vm->run_to_halt();
    assert(run.has_value());
    assert(vm->state().registers[0] == 1);
  }

  return 0;
}
