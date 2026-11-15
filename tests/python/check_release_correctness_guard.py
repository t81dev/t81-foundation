"""Require the selected native correctness failure to terminate unsuccessfully."""
import argparse
import json
import os
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("kind", choices=("sha3", "float", "map"))
    parser.add_argument("binary")
    args = parser.parse_args()
    if os.name == "posix":
        import resource
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    result = subprocess.run([args.binary], capture_output=True, text=True, timeout=15)
    expected = ("Test 'release correctness guard' failed!" if args.kind == "sha3"
                else "release correctness guard")
    passed = result.returncode != 0 and expected in result.stderr
    if args.kind != "sha3":
        passed = passed and "assert" in result.stderr.lower()
    print(json.dumps({"kind": args.kind, "returncode": result.returncode,
                      "status": "PASS" if passed else "FAIL",
                      "stdout": result.stdout, "stderr": result.stderr}))
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
