#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]

STDLIB_DIR = REPO_ROOT / "lang/stdlib/std"
STDLIB_DOC = REPO_ROOT / "docs/standards/standard-library.md"

EXPECTED_MODULES = {
    "agent": "lang/stdlib/std/agent.t81",
    "async": "lang/stdlib/std/async.t81",
    "bytes": "lang/stdlib/std/bytes.t81",
    "collections": "lang/stdlib/std/collections.t81",
    "core": "lang/stdlib/std/core.t81",
    "io": "lang/stdlib/std/io.t81",
    "math": "lang/stdlib/std/math.t81",
    "polynomial": "lang/stdlib/std/polynomial.t81",
    "symbol": "lang/stdlib/std/symbol.t81",
    "symbolic": "lang/stdlib/std/symbolic.t81",
    "sys": "lang/stdlib/std/sys.t81",
    "tensor": "lang/stdlib/std/tensor.t81",
    "text": "lang/stdlib/std/text.t81",
}

REQUIRED_STD_FIXTURES = [
    "tests/fixtures/t81lang_std_core",
    "tests/fixtures/t81lang_std_math",
    "tests/fixtures/t81lang_std_bytes",
    "tests/fixtures/t81lang_std_collections",
    "tests/fixtures/t81lang_std_polynomial",
    "tests/fixtures/t81lang_std_runtime",
    "tests/fixtures/t81lang_std_symbol",
    "tests/fixtures/t81lang_std_symbolic",
    "tests/fixtures/t81lang_std_tensor",
    "tests/fixtures/t81lang_std_text",
    "tests/fixtures/t81lang_std_io",
    "tests/fixtures/t81lang_std_sys",
    "tests/fixtures/t81lang_std_async",
]

REQUIRED_STD_TESTS = [
    "tests/cpp/cli_stdlib_fixtures_test.cpp",
]

# Collection determinism tests that must exist and pass
COLLECTION_DETERMINISM_TESTS = [
    "tests/fixtures/t81lang_std_collections/13_list_determinism_comprehensive.t81",
    "tests/fixtures/t81lang_std_collections/14_map_determinism_comprehensive.t81", 
    "tests/fixtures/t81lang_std_collections/15_set_determinism_comprehensive.t81",
    "tests/fixtures/t81lang_std_collections/16_tree_determinism_comprehensive.t81",
]

def check_collection_determinism() -> list[str]:
    """Validate collection determinism tests exist and can run."""
    issues = []
    
    # Check test files exist
    for test_file in COLLECTION_DETERMINISM_TESTS:
        test_path = REPO_ROOT / test_file
        if not test_path.exists():
            issues.append(f"missing collection determinism test: {test_file}")
            continue
            
        # Check corresponding output file exists
        output_file = test_file.replace('.t81', '.out')
        output_path = REPO_ROOT / output_file
        if not output_path.exists():
            issues.append(f"missing output file for collection test: {output_file}")
    
    # Try to run collection determinism tests if t81 CLI is available
    t81_cli = REPO_ROOT / "build" / "t81"
    if t81_cli.exists():
        for test_file in COLLECTION_DETERMINISM_TESTS:
            test_path = REPO_ROOT / test_file
            if test_path.exists():
                try:
                    result = subprocess.run(
                        [str(t81_cli), "run", str(test_path)],
                        capture_output=True,
                        text=True,
                        timeout=30
                    )
                    if result.returncode != 0:
                        issues.append(f"collection determinism test failed: {test_file}")
                        issues.append(f"  stderr: {result.stderr.strip()}")
                except subprocess.TimeoutExpired:
                    issues.append(f"collection determinism test timed out: {test_file}")
                except Exception as e:
                    issues.append(f"error running collection test {test_file}: {e}")
    
    return issues


def main() -> int:
    issues: list[str] = []

    if not STDLIB_DIR.exists():
        issues.append("missing stdlib directory: lang/stdlib/std")
    else:
        actual = sorted(p.name for p in STDLIB_DIR.glob("*.t81"))
        expected = sorted(Path(p).name for p in EXPECTED_MODULES.values())
        if actual != expected:
            issues.append(
                "stdlib module set drift detected: "
                f"expected={expected}, actual={actual}"
            )

    for module, rel_path in sorted(EXPECTED_MODULES.items()):
        path = REPO_ROOT / rel_path
        if not path.exists():
            issues.append(f"missing stdlib module file: {rel_path}")
            continue
        heading = f"### `std.{module}`"
        if not STDLIB_DOC.exists():
            issues.append("missing stdlib documentation: docs/standards/standard-library.md")
            continue
        doc_text = STDLIB_DOC.read_text(encoding="utf-8")
        if heading not in doc_text:
            issues.append(
                f"missing stdlib module heading in docs/standards/standard-library.md: {heading}"
            )

    for rel in REQUIRED_STD_FIXTURES + REQUIRED_STD_TESTS:
        if not (REPO_ROOT / rel).exists():
            issues.append(f"missing stdlib evidence artifact: {rel}")

    # Check collection determinism tests (BG-06)
    collection_issues = check_collection_determinism()
    issues.extend(collection_issues)

    if STDLIB_DOC.exists():
        text = STDLIB_DOC.read_text(encoding="utf-8")
        headings = set(re.findall(r"^### `std\.([a-z0-9_]+)`", text, flags=re.MULTILINE))
        unknown = sorted(headings - set(EXPECTED_MODULES.keys()))
        if unknown:
            issues.append(
                "unknown stdlib module headings in docs/standards/standard-library.md: "
                + ", ".join(f"std.{m}" for m in unknown)
            )

    if issues:
        print("stdlib surface baseline check FAILED")
        for issue in issues:
            print(f"- {issue}")
        return 1

    print("stdlib surface baseline check PASSED")
    print(f"- modules validated: {len(EXPECTED_MODULES)}")
    print(f"- fixture directories validated: {len(REQUIRED_STD_FIXTURES)}")
    print(f"- fixture tests validated: {len(REQUIRED_STD_TESTS)}")
    print(f"- collection determinism tests validated: {len(COLLECTION_DETERMINISM_TESTS)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
