#include <cassert>
#include <string>
#include <t81/lang/parser.hpp>
#include <t81/lang/compiler.hpp>

int main() {
  using namespace t81;

  // Undeclared identifier should fail
  {
    auto mod = lang::parse_module("fn main() -> T81Int { return y; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::UndeclaredIdentifier);
  }

  // Missing return should fail
  {
    auto mod = lang::parse_module("fn main() -> T81Int { let x: T81Int = 1; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::MissingReturn);
  }

  // If without else return should fail
  {
    auto mod = lang::parse_module("fn main() -> T81Int { if (1) { return 1; } }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::MissingReturn);
  }

  // Calling unknown function should fail
  {
    auto mod = lang::parse_module("fn main() -> T81Int { return foo(); }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::UnknownFunction);
  }

  // Calling with wrong arity should fail
  {
    auto mod = lang::parse_module(
        "fn helper(a: T81Int) -> T81Int { return a; }"
        "fn main() -> T81Int { return helper(1, 2); }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::InvalidCall);
  }

  // Base-81 literal compiles
  {
    auto mod = lang::parse_module("fn main() -> T81Int { return 7A3t81; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(res.has_value());
  }

  // Float literal compiles
  {
    auto mod = lang::parse_module("fn main() -> T81Float { return 1.20t81; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(res.has_value());
  }

  // Fraction literal compiles
  {
    auto mod = lang::parse_module("fn main() -> T81Fraction { return 22/7t81; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(res.has_value());
  }

  // Symbol literal compiles
  {
    auto mod = lang::parse_module("fn main() -> Symbol { return :graph; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(res.has_value());
  }

  // Type mismatch should fail
  {
    auto mod = lang::parse_module("fn main() -> T81Float { return :oops; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::UnsupportedType);
  }

  // Integer comparison compiles
  {
    auto mod = lang::parse_module("fn main() -> T81Int { return 1t81 == 2t81; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(res.has_value());
  }

  // Float comparison compiles
  {
    auto mod = lang::parse_module("fn main() -> T81Int { "
                                  "let a: T81Float = 1.00t81; "
                                  "let b: T81Float = 2.00t81; "
                                  "return a < b; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(res.has_value());
  }

  // Comparison type mismatch fails
  {
    auto mod = lang::parse_module("fn main() -> T81Int { return 1t81 == :bad; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::UnsupportedType);
  }

  return 0;
}
