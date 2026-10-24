#!/usr/bin/env python3
import sys
import os
import json
import tempfile
import subprocess
import argparse
from pathlib import Path
from urllib.parse import quote

from witness import init_db, submit_transaction

class RuntimeFailure(RuntimeError):
    def __init__(self, code, message, details=None):
        super().__init__(message)
        self.code = code
        self.details = details


def run_arm_a(arm_a_bin: Path, quantity: int, unit_price: int) -> dict:
    """Return native computation/admission evidence, including expected denials.

    The native CanonFS commit is a temporary local artifact, not the durable
    witness transaction or proof of an external effect.
    """
    if not arm_a_bin or not arm_a_bin.is_file():
        raise RuntimeFailure("RUNTIME_UNAVAILABLE", f"Native binary unavailable: {arm_a_bin}")
    with tempfile.TemporaryDirectory() as tmp:
        tmp_dir = Path(tmp)
        input_txt = tmp_dir / "input.txt"
        input_txt.write_text(f"{quantity} {unit_price}\n", encoding="utf-8")
        cmd = [str(arm_a_bin.resolve()), "approved", str(input_txt), str(tmp_dir / "store")]
        try:
            res = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        except FileNotFoundError as exc:
            raise RuntimeFailure("RUNTIME_UNAVAILABLE", str(exc)) from exc
        except (OSError, subprocess.TimeoutExpired) as exc:
            raise RuntimeFailure("RUNTIME_FAILURE", str(exc)) from exc
        details = {"returncode": res.returncode, "stdout": res.stdout, "stderr": res.stderr}
        if res.returncode != 0:
            raise RuntimeFailure("RUNTIME_FAILURE", "Native runtime failed", details)
        try:
            data = json.loads(res.stdout)
        except (json.JSONDecodeError, ValueError) as exc:
            raise RuntimeFailure("MALFORMED_RUNTIME_RESPONSE", "Native stdout is not JSON", details) from exc
        if (not isinstance(data, dict) or data.get("status") not in ("committed", "denied", "malformed")
                or type(data.get("amount")) is not int or data["amount"] < 0
                or not isinstance(data.get("events"), list)
                or not all(isinstance(e, str) for e in data["events"])):
            raise RuntimeFailure("MALFORMED_RUNTIME_RESPONSE", "Invalid native response contract", details)
        if ((data["status"] == "committed" and "write.commit" not in data["events"])
                or (data["status"] == "denied" and
                    ("authorize.deny" not in data["events"] or "write.commit" in data["events"]))):
            raise RuntimeFailure("MALFORMED_RUNTIME_RESPONSE", "Contradictory native admission evidence", details)
        # execute_runner only invokes native execution for valid bounded input.
        if data["status"] == "malformed":
            raise RuntimeFailure("MALFORMED_RUNTIME_RESPONSE", "Native rejected valid input", details)
        return data

def run_arm_b(wat_file: Path, quantity: int, unit_price: int) -> int:
    """Run Wasmtime Arm B guest WASM with host submit import."""
    try:
        import wasmtime
    except ImportError as exc:
        raise RuntimeFailure("RUNTIME_UNAVAILABLE", "Wasmtime dependency unavailable") from exc
    engine = wasmtime.Engine()
    store = wasmtime.Store(engine)
    module = wasmtime.Module.from_file(engine, str(wat_file))

    computed_total = [None]

    def host_submit(total: int) -> int:
        computed_total[0] = total
        return 0

    func_type = wasmtime.FuncType([wasmtime.ValType.i32()], [wasmtime.ValType.i32()])
    host_func = wasmtime.Func(store, func_type, host_submit)
    instance = wasmtime.Instance(store, module, [host_func])

    run_export = instance.exports(store)["run"]
    run_export(store, quantity, unit_price)

    if computed_total[0] is None:
        raise RuntimeError("Arm B Wasm execution failed to call host submit or compute result.")

    return computed_total[0]

def execute_runner(
    arm: str,
    tx_id: str,
    authority_version: str,
    quantity: int,
    unit_price: int,
    db_path: Path,
    output_dir: Path,
    secret_key: bytes,
    arm_a_bin: Path = None,
    wat_file: Path = None
) -> dict:
    output_dir.mkdir(parents=True, exist_ok=True)
    init_db(db_path).close()

    po_data = {
        "item": "Widget-X",
        "quantity": quantity,
        "unit_price": unit_price,
        "currency": "USD"
    }

    if arm not in ("A", "B"):
        raise ValueError(f"Unknown arm {arm}")
    total_amount = None
    runtime = None
    success = False
    if not all(type(v) is int and 1 <= v <= 1000 for v in (quantity, unit_price)):
        status_code = "MALFORMED_INPUT"
        result = {"error": status_code, "message": "quantity and unit_price must be integers in [1, 1000]"}
    else:
        try:
            if arm == "A":
                if arm_a_bin is None:
                    arm_a_bin = Path(os.environ.get("T81_ARM_A_BIN", str(
                        Path(__file__).resolve().parents[2] / "build" / "t81_architecture_arm_a")))
                runtime = run_arm_a(arm_a_bin, quantity, unit_price)
                total_amount = runtime["amount"]
            else:
                wat_path = wat_file or (Path(__file__).parent / "guest.wat")
                try:
                    total_amount = run_arm_b(wat_path, quantity, unit_price)
                except RuntimeFailure:
                    raise
                except Exception as exc:
                    raise RuntimeFailure("RUNTIME_FAILURE", str(exc)) from exc
                runtime = {"status": "computed", "amount": total_amount}
            success, result, status_code = submit_transaction(
                db_path=db_path, tx_id=tx_id, authority_version=authority_version,
                po_data=po_data, secret_key=secret_key, computed_amount=total_amount,
                runtime_allowed=runtime["status"] != "denied"
            )
        except RuntimeFailure as exc:
            status_code = exc.code
            result = {"error": status_code, "message": str(exc), "details": exc.details}

    evidence = {
        "arm": arm,
        "tx_id": tx_id,
        "authority_version": authority_version,
        "po_data": po_data,
        "total_amount": total_amount,
        "runtime": runtime,
        "success": success,
        "status_code": status_code,
        "receipt": result if success else None,
        "error": result if not success else None
    }

    evidence_name = quote(tx_id, safe="")
    evidence_file = output_dir / f"evidence_{evidence_name}.json"
    attempt = 1
    while True:
        try:
            with evidence_file.open("x", encoding="utf-8") as f:
                json.dump(evidence, f, indent=2)
            break
        except FileExistsError:
            evidence_file = output_dir / f"evidence_{evidence_name}.attempt-{attempt}.json"
            attempt += 1

    return evidence

def main():
    parser = argparse.ArgumentParser(description="Durable Witness Workload Runner")
    parser.add_argument("--arm", choices=["A", "B"], required=True, help="Execution Arm")
    parser.add_argument("--tx-id", required=True, help="Transaction ID")
    parser.add_argument("--authority-version", default="v2", help="Authority Policy Version")
    parser.add_argument("--quantity", type=int, default=10, help="Item quantity")
    parser.add_argument("--unit-price", type=int, default=100, help="Unit price")
    parser.add_argument("--db-path", type=Path, required=True, help="Path to witness SQLite DB")
    parser.add_argument("--output-dir", type=Path, required=True, help="Output evidence directory")
    parser.add_argument("--arm-a-bin", type=Path, default=None, help="Path to Arm A binary")
    parser.add_argument("--secret-key", type=str, required=True, help="Signing secret key for witness HMAC")
    args = parser.parse_args()

    secret_key_bytes = args.secret_key.encode("utf-8")

    evidence = execute_runner(
        arm=args.arm,
        tx_id=args.tx_id,
        authority_version=args.authority_version,
        quantity=args.quantity,
        unit_price=args.unit_price,
        db_path=args.db_path,
        output_dir=args.output_dir,
        secret_key=secret_key_bytes,
        arm_a_bin=args.arm_a_bin
    )

    print(json.dumps(evidence, indent=2))
    if not evidence.get("success"):
        sys.exit(1)

if __name__ == "__main__":
    main()
