"""
t81 — Python bindings for T81 Foundation.

T81 is a deterministic virtual machine, ISA, and compiler stack built on
balanced ternary arithmetic {-1, 0, +1}.  This package exposes the full
stack from native ternary types through the compiler pipeline, VM execution,
Axion governance audit trail, and CanonFS content-addressed storage.

Quick start
-----------
>>> import t81
>>> t81.compile_and_run("fn main() -> i32 { return 42; }")
42

>>> big = t81.BigInt(123456789)
>>> big + t81.BigInt(1)
123456790

>>> vm = t81.VM()
>>> prog = t81.compile("fn main() -> i32 { return 7; }")
>>> vm.load_program(prog)
>>> vm.run_to_halt()
>>> vm.get_register(0)
7
>>> vm.axion_log   # Axion governance audit trail
[]
"""

from __future__ import annotations

from t81._t81 import (  # noqa: F401  (re-exported as public API)
    # ── Ternary numeric types ─────────────────────────────────────────────────
    T81Int,
    BigInt,
    Float,
    Fraction,
    # ── Tensor types ──────────────────────────────────────────────────────────
    Tensor1D3,
    Tensor2D33,
    T729DynamicTensor,
    T729IntTensor,
    # ── Compiler pipeline ─────────────────────────────────────────────────────
    Program,
    compile,
    compile_and_run,
    # ── Virtual machine ───────────────────────────────────────────────────────
    HanoiVM,
    make_interpreter_vm,
    # ── CanonFS content-addressed storage ────────────────────────────────────
    ObjectType,
    CanonRef,
    CanonDriver,
    make_in_memory_driver,
    make_persistent_driver,
)

# Friendly alias: `t81.VM()` instead of `t81.make_interpreter_vm()`
VM = make_interpreter_vm

__all__ = [
    # Numeric types
    "T81Int",
    "BigInt",
    "Float",
    "Fraction",
    # Tensors
    "Tensor1D3",
    "Tensor2D33",
    "T729DynamicTensor",
    "T729IntTensor",
    # Compiler
    "Program",
    "compile",
    "compile_and_run",
    # VM
    "VM",
    "HanoiVM",
    "make_interpreter_vm",
    # CanonFS
    "ObjectType",
    "CanonRef",
    "CanonDriver",
    "make_in_memory_driver",
    "make_persistent_driver",
]
