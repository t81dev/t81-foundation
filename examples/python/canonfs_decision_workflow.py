"""End-to-end Python SDK workflow: deterministic T81Lang decision table + CanonFS.

Demonstrates:
  1. Compiling and running a small deterministic T81Lang ternary decision-table
     program (`-1` = deny, `0` = review, `+1` = approve) via `t81.compile_and_run`.
  2. Storing the canonical serialized result in persistent CanonFS.
  3. Retrieving the exact bytes in a fresh Python process using only the store
     path and the saved 64-character hex digest (`t81.CanonRef.from_hex`).

Scope boundary:
  This example demonstrates deterministic execution and content-addressed
  storage. Axion policy enforcement is outside this example.
"""

from __future__ import annotations

import argparse
import json
import shlex
import sys
from pathlib import Path

import t81

DECISION_LABELS = {
    -1: "deny",
    0: "review",
    1: "approve",
}


def build_decision_program(
    *,
    score: int,
    risk_flag: int,
    approve_threshold: int = 70,
    review_threshold: int = 40,
) -> str:
    """Return a deterministic T81Lang ternary decision-table program."""
    return f"""
fn decide(score: i32, risk_flag: i32) -> i32 {{
    if (risk_flag != 0) {{
        return -1;
    }}
    if (score >= {approve_threshold}) {{
        return 1;
    }}
    if (score >= {review_threshold}) {{
        return 0;
    }}
    return -1;
}}

fn main() -> i32 {{
    return decide({score}, {risk_flag});
}}
""".strip()


def run_and_store(
    store_path: Path,
    *,
    score: int,
    risk_flag: int,
    approve_threshold: int = 70,
    review_threshold: int = 40,
) -> tuple[str, bytes, int]:
    """Execute the T81Lang decision table and persist canonical result bytes."""
    source = build_decision_program(
        score=score,
        risk_flag=risk_flag,
        approve_threshold=approve_threshold,
        review_threshold=review_threshold,
    )
    decision = int(t81.compile_and_run(source))
    record = {
        "approve_threshold": approve_threshold,
        "decision": decision,
        "label": DECISION_LABELS.get(decision, "unknown"),
        "review_threshold": review_threshold,
        "risk_flag": risk_flag,
        "score": score,
    }
    payload = json.dumps(record, sort_keys=True, separators=(",", ":")).encode("utf-8")

    driver = t81.make_persistent_driver(str(store_path))
    ref = driver.write_object(t81.ObjectType.RawBlock, payload)
    return ref.hex(), payload, decision


def read_by_digest(store_path: Path, digest_hex: str) -> bytes:
    """Retrieve exact object bytes from persistent CanonFS using a saved hex digest."""
    driver = t81.make_persistent_driver(str(store_path))
    ref = t81.CanonRef.from_hex(digest_hex)
    return driver.read_object_bytes(ref)


def print_command(label: str, command: str, store: Path, *options: str) -> None:
    argv = [sys.executable, str(Path(__file__).resolve()), command, "--store", str(store.resolve())]
    print(f"{label}: {shlex.join([*argv, *options])}", file=sys.stderr)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Deterministic T81Lang decision-table + persistent CanonFS workflow."
    )
    sub = parser.add_subparsers(dest="command", required=True)

    run_p = sub.add_parser("run", help="Run decision table and store result in CanonFS.")
    run_p.add_argument("--store", required=True, type=Path, help="Persistent CanonFS directory.")
    run_p.add_argument("--score", required=True, type=int, help="Integer input score.")
    run_p.add_argument("--risk-flag", default=0, type=int, help="Non-zero forces deny (-1).")
    run_p.add_argument("--approve-threshold", default=70, type=int, help="Score threshold for +1.")
    run_p.add_argument("--review-threshold", default=40, type=int, help="Score threshold for 0.")
    run_p.add_argument("--compare-to", help="Print a command comparing to this saved digest.")
    run_p.add_argument(
        "--next-approve-threshold", type=int, help="Print a rerun command using this threshold."
    )

    read_p = sub.add_parser("read", help="Read exact bytes from CanonFS by saved hex digest.")
    read_p.add_argument("--store", required=True, type=Path, help="Persistent CanonFS directory.")
    read_p.add_argument("--digest", required=True, type=str, help="64-character hex CanonRef digest.")

    cmp_p = sub.add_parser("compare", help="Compare two stored CanonFS records by digest.")
    cmp_p.add_argument("--store", required=True, type=Path, help="Persistent CanonFS directory.")
    cmp_p.add_argument("--digest-a", required=True, type=str, help="First 64-char hex digest.")
    cmp_p.add_argument("--digest-b", required=True, type=str, help="Second 64-char hex digest.")

    args = parser.parse_args(argv)

    if args.command == "run":
        if args.compare_to is not None:
            try:
                args.compare_to = t81.CanonRef.from_hex(args.compare_to).hex()
            except ValueError as exc:
                run_p.error(f"invalid --compare-to digest: {exc}")
        digest_hex, payload, decision = run_and_store(
            args.store,
            score=args.score,
            risk_flag=args.risk_flag,
            approve_threshold=args.approve_threshold,
            review_threshold=args.review_threshold,
        )
        print(
            json.dumps(
                {
                    "decision": decision,
                    "digest": digest_hex,
                    "payload": payload.decode("utf-8"),
                },
                sort_keys=True,
            )
        )
        print_command("Read", "read", args.store, "--digest", digest_hex)
        if args.compare_to is not None:
            print_command(
                "Compare",
                "compare",
                args.store,
                "--digest-a",
                args.compare_to,
                "--digest-b",
                digest_hex,
            )
        if args.next_approve_threshold is not None:
            print_command(
                "Rerun",
                "run",
                args.store,
                "--score",
                str(args.score),
                "--risk-flag",
                str(args.risk_flag),
                "--approve-threshold",
                str(args.next_approve_threshold),
                "--review-threshold",
                str(args.review_threshold),
                "--compare-to",
                digest_hex,
            )
        return 0

    if args.command == "read":
        raw = read_by_digest(args.store, args.digest)
        sys.stdout.buffer.write(raw + b"\n")
        return 0

    if args.command == "compare":
        raw_a = read_by_digest(args.store, args.digest_a)
        raw_b = read_by_digest(args.store, args.digest_b)
        print(
            json.dumps(
                {
                    "digest_a": args.digest_a.lower(),
                    "digest_b": args.digest_b.lower(),
                    "same_digest": args.digest_a.lower() == args.digest_b.lower(),
                    "same_bytes": raw_a == raw_b,
                    "record_a": json.loads(raw_a.decode("utf-8")),
                    "record_b": json.loads(raw_b.decode("utf-8")),
                },
                sort_keys=True,
            )
        )
        return 0

    return 2


if __name__ == "__main__":
    raise SystemExit(main())
