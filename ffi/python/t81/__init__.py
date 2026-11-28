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

import os
from pathlib import Path
import shutil

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
    # ── v2.3.0 M3 Governed Provider & Native Ed25519 DAG primitives ──────────
    ProviderGovernor,
    T81VmProvider,
    ControlledProvider,
    ed25519_generate_keypair,
    sign_rfc00d4_payload,
    verify_dag,
)

# Friendly alias: `t81.VM()` instead of `t81.make_interpreter_vm()`
VM = make_interpreter_vm


def cli_path() -> Path:
    """Return the path to the bundled native ``t81`` CLI binary.

    Resolution order:
      1. ``T81_BIN`` environment variable (explicit override)
      2. Bundled ``t81/bin/t81`` (or ``t81.exe`` on Windows) inside this package
      3. ``t81`` on ``PATH``
    """
    env_bin = os.environ.get("T81_BIN")
    if env_bin:
        candidate = Path(env_bin)
        if candidate.is_file():
            return candidate
    exe_name = "t81.exe" if os.name == "nt" else "t81"
    bundled = Path(__file__).resolve().parent / "bin" / exe_name
    if bundled.is_file():
        return bundled
    found = shutil.which("t81")
    if found:
        return Path(found)
    raise FileNotFoundError(
        f"Native t81 CLI binary not found at {bundled} or on PATH. "
        "Reinstall the t81 wheel or set T81_BIN=/path/to/t81."
    )


__all__ = [
    "cli_path",
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
    # v2.3.0 M3 Provider & Ed25519
    "ProviderGovernor",
    "T81VmProvider",
    "ControlledProvider",
    "ed25519_generate_keypair",
    "sign_rfc00d4_payload",
    "verify_dag",
]
