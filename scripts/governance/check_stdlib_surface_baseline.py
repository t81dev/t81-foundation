#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
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
]

REQUIRED_STD_TESTS = [
    "tests/cpp/cli_std_core_fixtures_test.cpp",
    "tests/cpp/cli_std_math_fixtures_test.cpp",
    "tests/cpp/cli_std_bytes_fixtures_test.cpp",
    "tests/cpp/cli_std_collections_fixtures_test.cpp",
    "tests/cpp/cli_std_polynomial_fixtures_test.cpp",
    "tests/cpp/cli_std_runtime_fixtures_test.cpp",
    "tests/cpp/cli_std_symbol_fixtures_test.cpp",
    "tests/cpp/cli_std_symbolic_fixtures_test.cpp",
    "tests/cpp/cli_std_tensor_fixtures_test.cpp",
    "tests/cpp/cli_std_text_fixtures_test.cpp",
]


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
    return 0


if __name__ == "__main__":
    sys.exit(main())
