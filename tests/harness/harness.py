import json
import os
import subprocess
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent
VECTORS = ROOT / "test_vectors"
CANONICAL = ROOT / "canonical"

VM = "./t81vm"  # Expected compiled VM binary or interpreter

# -------------------------------------------------------------------
# Utility: run program and capture deterministic trace
# -------------------------------------------------------------------

def run_vm(path):
    cmd = [VM, "--trace", str(path)]
    result = subprocess.run(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )
    return {
        "stdout": result.stdout,
        "stderr": result.stderr,
    }


def trace_hash(trace):
    # Deterministic normalization
    return hashlib.sha256(trace.encode("utf-8")).hexdigest()


# -------------------------------------------------------------------
# Determinism test
# -------------------------------------------------------------------

def test_determinism():
    print("[1] Determinism Test")
    for program in VECTORS.glob("*.t81"):
        print(f"  • {program.name}")

        first = run_vm(program)
        second = run_vm(program)

        h1 = trace_hash(first["stdout"])
        h2 = trace_hash(second["stdout"])

        if h1 != h2:
            raise AssertionError(
                f"Trace mismatch in {program.name}: {h1} != {h2}"
            )

    print("✓ Determinism verified.")


# -------------------------------------------------------------------
# Canonicalization tests
# -------------------------------------------------------------------

def load_json(name):
    return json.load(open(CANONICAL / name))


def test_bigint_canonical():
    print("[2] BigInt Canonicalization")

    samples = load_json("bigint.json")
    for raw, expected in samples.items():
        # Use your VM to evaluate canonical form
        # Example: VM --canonical-bigint "value"
        cmd = [VM, "--canonical-bigint", raw]
        out = subprocess.check_output(cmd).decode().strip()

        if out != expected:
            raise AssertionError(f"Canonical form mismatch: {raw} => {out} (expected {expected})")

    print("✓ BigInt canonicalization verified.")


def test_fraction_canonical():
    print("[3] Fraction Canonicalization")

    samples = load_json("fraction.json")
    for raw, expected in samples.items():
        cmd = [VM, "--canonical-fraction", raw]
        out = subprocess.check_output(cmd).decode().strip()
        if out != expected:
            raise AssertionError(f"Fraction mismatch: {raw} => {out}")

    print("✓ Fraction canonicalization verified.")


def test_tensor_canonical():
    print("[4] Tensor Canonicalization")

    samples = load_json("tensor.json")
    for raw, expected in samples.items():
        cmd = [VM, "--canonical-tensor", raw]
        out = subprocess.check_output(cmd).decode().strip()
        if out != expected:
            raise AssertionError(f"Tensor mismatch: {raw} => {out}")

    print("✓ Tensor canonicalization verified.")


# -------------------------------------------------------------------
# Fault Behavior Tests
# -------------------------------------------------------------------

def test_faults():
    print("[5] Fault Tests")

    fault_vectors = [
        "faults.t81",
        "recursion.t81",
        "privileged_ops.t81",
    ]

    for vec in fault_vectors:
        program = VECTORS / vec
        print(f"  • {program.name}")

        output = run_vm(program)
        if "FAULT" not in output["stderr"]:
            raise AssertionError(
                f"No fault detected for {program.name}"
            )

    print("✓ Fault behavior verified.")


# -------------------------------------------------------------------
# Entrypoint
# -------------------------------------------------------------------

def main():
    import sys
    if len(sys.argv) > 1 and sys.argv[1] == "run":
        test_determinism()
        test_bigint_canonical()
        test_fraction_canonical()
        test_tensor_canonical()
        test_faults()
        return
    print("Usage: harness.py run")


if __name__ == "__main__":
    main()

