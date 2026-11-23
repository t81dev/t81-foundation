"""
t81 — Python bindings for T81 Foundation.

The Python bindings provide ternary number and tensor types, a compiler and
VM, structured VM audit events, and raw CanonFS storage. The standalone T81 CLI
and MCP server are separate native builds.

Quick start
-----------
>>> import t81
>>> t81.compile_and_run("fn main() -> i32 { return 42; }")
42

>>> big = t81.BigInt(123456789)
>>> (big + t81.BigInt(1)).to_int()
123456790

>>> vm = t81.VM()
>>> prog = t81.compile("@axion_verify\\nfn main() -> i32 { return 7; }")
>>> vm.load_program(prog)
>>> vm.run_to_halt()
>>> vm.get_register(2)
7
>>> "Default policy: allow all" in vm.axion_log
True
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
    StepLimitExceeded,
    AxionEvent,
    VerdictKind,
    make_interpreter_vm,
    # ── CanonFS content-addressed storage ────────────────────────────────────
    ObjectType,
    CanonRef,
    CanonDriver,
    make_in_memory_driver,
    make_persistent_driver,
    # ── v2.1.0 Dual-Rail, Axion Policy, & SHA3-512 primitives ────────────────
    sha3_512_hex,
    validate_policy,
    dualrail_pack,
    dualrail_dot,
    dualrail_diff_check,
    dualrail_adapter_apply,
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
    "StepLimitExceeded",
    "AxionEvent",
    "VerdictKind",
    "make_interpreter_vm",
    # CanonFS
    "ObjectType",
    "CanonRef",
    "CanonDriver",
    "make_in_memory_driver",
    "make_persistent_driver",
    # v2.1.0 Primitives
    "sha3_512_hex",
    "validate_policy",
    "dualrail_pack",
    "dualrail_dot",
    "dualrail_diff_check",
    "dualrail_adapter_apply",
]
