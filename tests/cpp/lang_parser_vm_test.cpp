#include <cassert>
#include <string>
#include <t81/lang/parser.hpp>
#include <t81/lang/compiler.hpp>
#include <t81/vm/vm.hpp>

int main() {
  using namespace t81;
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
  return 0;
}
