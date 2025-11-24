#include <cassert>
#include <string>
#include <t81/lang/parser.hpp>
#include <t81/lang/compiler.hpp>
#include <t81/vm/vm.hpp>
#include <t81/vm/state.hpp>

int main() {
  using namespace t81;
  {
    const std::string src =
        "fn main() -> T81Int { return !1t81; }";
    auto mod_res = lang::parse_module(src);
    assert(mod_res.has_value());
    lang::Compiler comp;
    auto prog_res = comp.compile(mod_res.value());
    assert(prog_res.has_value());

    auto vm = vm::make_interpreter_vm();
    vm->load_program(prog_res.value());
    auto run = vm->run_to_halt();
    assert(run.has_value());
    assert(vm->state().registers[0] == 0);
  }

  {
    const std::string src =
        "fn main() -> T81Int { return !0t81; }";
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
