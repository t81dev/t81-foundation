#include <cassert>
#include <cmath>
#include <string>
#include <t81/lang/parser.hpp>
#include <t81/lang/compiler.hpp>
#include <t81/vm/vm.hpp>
#include <t81/vm/state.hpp>

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

  {
    const std::string src =
        "fn main() -> T81Float { "
        "let base: T81Float = 2t81; "
        "return base + 1.20t81; }";
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
    const double expected = 3.0 + (2.0 / 81.0);
    assert(std::fabs(vm->state().floats[handle - 1] - expected) < 1e-9);
  }

  {
    const std::string src =
        "fn main() -> T81Fraction { "
        "let base: T81Fraction = 2t81; "
        "return base + 1/2t81; }";
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
    const auto& frac = vm->state().fractions[handle - 1];
    assert(frac.num.to_int64() == 5);
    assert(frac.den.to_int64() == 2);
  }

  {
    const std::string src =
        "fn main() -> T81Int { "
        "return 1t81 < 1.20t81; }";
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
        "fn take(a: T81Float) -> T81Float { return a; }"
        "fn main() -> T81Float { return take(5t81); }";
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
    assert(std::fabs(vm->state().floats[handle - 1] - 5.0) < 1e-9);
  }

  {
    const std::string src =
        "fn main() -> T81Int { "
        "return 10 / 3; }";
    auto mod_res = lang::parse_module(src);
    assert(mod_res.has_value());
    lang::Compiler comp;
    auto prog_res = comp.compile(mod_res.value());
    assert(prog_res.has_value());
    auto vm = vm::make_interpreter_vm();
    vm->load_program(prog_res.value());
    auto run = vm->run_to_halt();
    assert(run.has_value());
    assert(vm->state().registers[0] == 3);
  }

  {
    const std::string src =
        "fn main() -> T81Int { "
        "return 10 % 3; }";
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
        "fn main() -> T81Float { "
        "return 4t81 / 2.00t81; }";
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
    assert(std::fabs(vm->state().floats[handle - 1] - 2.0) < 1e-9);
  }

  {
    const std::string src =
        "fn main() -> T81Fraction { "
        "let a: T81Fraction = 1/2t81; "
        "let b: T81Fraction = 3/4t81; "
        "return b / a; }";
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
    const auto& frac = vm->state().fractions[handle - 1];
    assert(frac.num.to_int64() == 3);
    assert(frac.den.to_int64() == 2);
  }

  {
    const std::string src =
        "fn main() -> Option[T81Int] { "
        "return Some(5t81); }";
    auto mod_res = lang::parse_module(src);
    assert(mod_res.has_value());
    lang::Compiler comp;
    auto prog_res = comp.compile(mod_res.value());
    assert(prog_res.has_value());
    auto vm = vm::make_interpreter_vm();
    vm->load_program(prog_res.value());
    auto run = vm->run_to_halt();
    assert(run.has_value());
    assert(vm->state().register_tags[0] == t81::vm::ValueTag::OptionHandle);
    auto handle = vm->state().registers[0];
    assert(handle > 0);
    const auto& opt = vm->state().options[static_cast<std::size_t>(handle - 1)];
    assert(opt.has_value);
    assert(opt.payload_tag == t81::vm::ValueTag::Int);
    assert(opt.payload == 5);
  }

  {
    const std::string src =
        "fn main() -> Option[T81Int] { "
        "let empty: Option[T81Int] = None; "
        "return empty; }";
    auto mod_res = lang::parse_module(src);
    assert(mod_res.has_value());
    lang::Compiler comp;
    auto prog_res = comp.compile(mod_res.value());
    assert(prog_res.has_value());
    auto vm = vm::make_interpreter_vm();
    vm->load_program(prog_res.value());
    auto run = vm->run_to_halt();
    assert(run.has_value());
    assert(vm->state().register_tags[0] == t81::vm::ValueTag::OptionHandle);
    auto handle = vm->state().registers[0];
    assert(handle > 0);
    const auto& opt = vm->state().options[static_cast<std::size_t>(handle - 1)];
    assert(!opt.has_value);
  }

  {
    const std::string src =
        "fn main() -> Result[T81Int, Symbol] { "
        "return Err(:fail); }";
    auto mod_res = lang::parse_module(src);
    assert(mod_res.has_value());
    lang::Compiler comp;
    auto prog_res = comp.compile(mod_res.value());
    assert(prog_res.has_value());
    auto vm = vm::make_interpreter_vm();
    vm->load_program(prog_res.value());
    auto run = vm->run_to_halt();
    assert(run.has_value());
    assert(vm->state().register_tags[0] == t81::vm::ValueTag::ResultHandle);
    auto handle = vm->state().registers[0];
    assert(handle > 0);
    const auto& res = vm->state().results[static_cast<std::size_t>(handle - 1)];
    assert(!res.is_ok);
    assert(res.payload_tag == t81::vm::ValueTag::SymbolHandle);
    auto sym_handle = res.payload;
    assert(sym_handle > 0);
    const auto& sym =
        vm->state().symbols[static_cast<std::size_t>(sym_handle - 1)];
    assert(sym == "fail");
  }

  {
    const std::string src =
        "fn never() -> T81Int { return never(); }"
        "fn main() -> T81Int { "
        "let guard: T81Int = 0t81; "
        "if (guard && never()) { return 1t81; } "
        "return 0t81; }";
    auto mod_res = lang::parse_module(src);
    assert(mod_res.has_value());
    lang::Compiler comp;
    auto prog_res = comp.compile(mod_res.value());
    assert(prog_res.has_value());
    auto vm = vm::make_interpreter_vm();
    vm->load_program(prog_res.value());
    auto run = vm->run_to_halt(512);
    assert(run.has_value());
    assert(vm->state().halted);
    assert(vm->state().registers[0] == 0);
  }

  {
    const std::string src =
        "fn never() -> T81Int { return never(); }"
        "fn main() -> T81Int { "
        "let guard: T81Int = 1t81; "
        "if (guard || never()) { return 1t81; } "
        "return 0t81; }";
    auto mod_res = lang::parse_module(src);
    assert(mod_res.has_value());
    lang::Compiler comp;
    auto prog_res = comp.compile(mod_res.value());
    assert(prog_res.has_value());
    auto vm = vm::make_interpreter_vm();
    vm->load_program(prog_res.value());
    auto run = vm->run_to_halt(512);
    assert(run.has_value());
    assert(vm->state().halted);
    assert(vm->state().registers[0] == 1);
  }

  {
    const std::string src =
        "fn main() -> T81Int { "
        "let base: Symbol = :core; "
        "let same: Symbol = :core; "
        "let other: Symbol = :shell; "
        "if ((base == same) && (base != other)) { return 1t81; } "
        "return 0t81; }";
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
