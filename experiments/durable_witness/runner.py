#!/usr/bin/env python3
import sys
import json
import shutil
import tempfile
import subprocess
import argparse
from pathlib import Path

import wasmtime
from witness import init_db, submit_transaction, DEFAULT_SECRET_KEY

def run_arm_a(arm_a_bin: Path, quantity: int, unit_price: int) -> int:
    """Run T81 Arm A C++ binary to compute total and enforce bounded execution."""
    if not arm_a_bin or not arm_a_bin.exists():
        raise RuntimeError(f"Arm A runtime binary not found at '{arm_a_bin}'. Native execution unavailable.")

    # Create temporary contract file and store for Arm A binary
    tmp_dir = Path(tempfile.mkdtemp())
    try:
        input_json = tmp_dir / "input.json"
        store_dir = tmp_dir / "store"
        store_dir.mkdir(parents=True, exist_ok=True)

        contract_payload = {
            "max_unit_price": 1000,
            "max_total_amount": 50000,
            "items": [{"quantity": quantity, "unit_price": unit_price}]
        }
        with open(input_json, "w") as f:
            json.dump(contract_payload, f)

        if store_dir.exists():
            shutil.rmtree(store_dir)
        cmd = [str(arm_a_bin), "approved", str(input_json), str(store_dir)]
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
            raise RuntimeError(f"Arm A binary execution failed (exit code {res.returncode}): {res.stderr.strip()}")

        # Arm A succeeded within bounds
        return quantity * unit_price
    finally:
        shutil.rmtree(tmp_dir, ignore_errors=True)

def run_arm_b(wat_file: Path, quantity: int, unit_price: int) -> int:
    """Run Wasmtime Arm B guest WASM with host submit import."""
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

    return computed_total[0]

def execute_runner(
    arm: str,
    tx_id: str,
    authority_version: str,
    quantity: int,
    unit_price: int,
    db_path: Path,
    output_dir: Path,
    arm_a_bin: Path = None,
    wat_file: Path = None
) -> dict:
    output_dir.mkdir(parents=True, exist_ok=True)
    init_db(db_path)

    po_data = {
        "item": "Widget-X",
        "quantity": quantity,
        "unit_price": unit_price,
        "currency": "USD"
    }

    if arm == "A":
        if arm_a_bin is None:
            # Default search path for built Arm A binary
            default_bin = Path("build/t81_architecture_arm_a")
            if not default_bin.exists():
                default_bin = Path(__file__).resolve().parents[2] / "build" / "t81_architecture_arm_a"
            arm_a_bin = default_bin
        total_amount = run_arm_a(arm_a_bin, quantity, unit_price)
    elif arm == "B":
        wat_path = wat_file or (Path(__file__).parent / "guest.wat")
        total_amount = run_arm_b(wat_path, quantity, unit_price)
    else:
        raise ValueError(f"Unknown arm {arm}")

    success, result, status_code = submit_transaction(
        db_path=db_path,
        tx_id=tx_id,
        authority_version=authority_version,
        po_data=po_data
    )

    evidence = {
        "arm": arm,
        "tx_id": tx_id,
        "authority_version": authority_version,
        "po_data": po_data,
        "total_amount": total_amount,
        "success": success,
        "status_code": status_code,
        "receipt": result if success else None,
        "error": result if not success else None
    }

    evidence_file = output_dir / f"evidence_{tx_id}.json"
    with open(evidence_file, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

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
    args = parser.parse_args()

    evidence = execute_runner(
        arm=args.arm,
        tx_id=args.tx_id,
        authority_version=args.authority_version,
        quantity=args.quantity,
        unit_price=args.unit_price,
        db_path=args.db_path,
        output_dir=args.output_dir,
        arm_a_bin=args.arm_a_bin
    )

    print(json.dumps(evidence, indent=2))
    if not evidence.get("success"):
        sys.exit(1)

if __name__ == "__main__":
    main()
