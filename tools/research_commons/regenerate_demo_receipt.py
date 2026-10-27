#!/usr/bin/env python3
"""Regenerate the implementation-author Research Commons demonstration receipt.

This does not independently verify scientific results or historical audit claims.
It reads the pinned, offline Git evidence through Commons and writes a derived
discovery-response digest receipt to the explicitly requested output path.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "docs/research/commons"
spec = importlib.util.spec_from_file_location("commons", ROOT / "tools/research_commons/commons.py")
commons = importlib.util.module_from_spec(spec)
spec.loader.exec_module(commons)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True,
                        help="Explicit output JSON path; no implicit file mutation")
    args = parser.parse_args()
    template = json.loads((DATA / "demo-observation.json").read_text(encoding="utf-8"))
    index = commons.Commons()
    template["manifest_sha256"] = index.digest
    for entry in template["observations"]:
        response = index.dispatch(entry["request"])
        entry["response_sha256"] = hashlib.sha256(
            commons.canonical(response).encode("utf-8")).hexdigest()
        entry["result_type"] = "list" if isinstance(response["result"], list) else "dict"
    args.output.write_text(json.dumps(template, indent=2, ensure_ascii=True) + "\n",
                           encoding="utf-8")
    print(json.dumps({"output": str(args.output), "manifest_sha256": index.digest,
                      "observations": len(template["observations"]),
                      "status": "implementation_author_receipt_only"}, sort_keys=True))


if __name__ == "__main__":
    main()
