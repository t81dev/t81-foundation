#include <cassert>
#include <memory>

#include "t81/t81.hpp"

int main() {
  using namespace t81::lang;

  Expr lhs;
  lhs.node = ExprLiteral{2};
  Expr rhs;
  rhs.node = ExprLiteral{3};

  ExprBinary bin;
  bin.op = ExprBinary::Op::Add;
  bin.lhs = std::make_unique<Expr>(lhs);
  bin.rhs = std::make_unique<Expr>(rhs);

  Expr root;
  root.node = bin;

  Function fn;
  fn.name = "main";
  fn.return_type = Type::I64;
  Statement ret;
  ret.node = StatementReturn{root};
  fn.body.push_back(ret);

  Module mod;
  mod.functions.push_back(fn);

  Compiler compiler;
  auto program_res = compiler.compile(mod);
  assert(program_res.has_value());

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program_res.value());
  auto run_res = vm->run_to_halt();
  assert(run_res.has_value());
  assert(vm->state().registers[0] == 5);

  return 0;
}
