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

  // Symbol equality compiles
  {
    auto mod = lang::parse_module(
        "fn main() -> T81Int { "
        "let a: Symbol = :core; "
        "let b: Symbol = :core; "
        "return a == b; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(res.has_value());
  }

  // Symbol ordering fails
  {
    auto mod = lang::parse_module(
        "fn main() -> T81Int { "
        "let a: Symbol = :core; "
        "let b: Symbol = :shell; "
        "return a < b; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::UnsupportedType);
  }

  // Logical expressions require T81Int operands
  {
    auto mod = lang::parse_module(
        "fn main() -> T81Int { "
        "let a: T81Float = 1.00t81; "
        "return a && 1t81; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::UnsupportedType);
  }

  // Logical expressions compile for integers
  {
    auto mod = lang::parse_module(
        "fn main() -> T81Int { "
        "return (1t81 && 1t81) || (0t81 && 1t81); }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(res.has_value());
  }

  // Integer to float widening works
  {
    auto mod = lang::parse_module("fn main() -> T81Float { let f: T81Float = 2t81; return f; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(res.has_value());
  }

  // Integer to fraction widening works
  {
    auto mod = lang::parse_module("fn main() -> T81Fraction { let f: T81Fraction = 2t81; return f; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(res.has_value());
  }

  // Mixed int/float arithmetic allowed
  {
    auto mod = lang::parse_module("fn main() -> T81Float { return 1t81 + 1.00t81; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(res.has_value());
  }

  // Float/fraction mixing still rejected
  {
    auto mod = lang::parse_module(
        "fn main() -> T81Float { "
        "let a: T81Float = 1.00t81; "
        "let b: T81Fraction = 1/2t81; "
        "return a + b; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::UnsupportedType);
  }

  // Float to int narrowing is rejected
  {
    auto mod = lang::parse_module(
        "fn main() -> T81Int { "
        "let f: T81Float = 1.20t81; "
        "return f; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::UnsupportedType);
  }

  // Option Some/None compile
  {
    auto mod = lang::parse_module(
        "fn main() -> Option[T81Int] { "
        "let value: Option[T81Int] = Some(1t81); "
        "let empty: Option[T81Int] = None; "
        "return value; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(res.has_value());
  }

  // Result Ok/Err compile
  {
    auto mod = lang::parse_module(
        "fn main() -> Result[T81Int, Symbol] { "
        "let ok: Result[T81Int, Symbol] = Ok(1t81); "
        "let err: Result[T81Int, Symbol] = Err(:oops); "
        "return err; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(res.has_value());
  }

  // None without Option context fails
  {
    auto mod = lang::parse_module("fn main() -> T81Int { return None; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::MissingType);
  }

  // Option payload type mismatch
  {
    auto mod = lang::parse_module(
        "fn main() -> Option[T81Int] { "
        "return Some(:bad); }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::UnsupportedType);
  }

  // Result requires matching variant type
  {
    auto mod = lang::parse_module(
        "fn main() -> Result[T81Int, Symbol] { "
        "return Ok(:bad); }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::UnsupportedType);
  }

  // Modulo only valid for integers
  {
    auto mod = lang::parse_module("fn main() -> T81Float { return 1.00t81 % 2.00t81; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::UnsupportedType);
  }

  {
    auto mod = lang::parse_module("fn main() -> T81Float { return 1t81 % 2.00t81; }");
    assert(mod.has_value());
    lang::Compiler comp;
    auto res = comp.compile(mod.value());
    assert(!res.has_value());
    assert(res.error() == lang::CompileError::UnsupportedType);
  }

  return 0;
}
