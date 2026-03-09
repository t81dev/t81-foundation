#!/usr/bin/env python3
"""Enforce parity between CLI help output and docs/guides/cli-user-manual.md."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


HELP_TOPICS = [
    "code",
    "project",
    "env",
    "internal",
    "completion",
    "man",
    "feedback",
    "weights",
    "policy",
    "trace",
    "canonfs",
    "determinism",
    "vm",
    "tisc",
    "ir",
    "tier",
    "tensor",
    "axion",
    "c",
    "llvm",
    "mlir",
]

# Flags/features we intentionally do not support in the current shipped CLI.
DENYLIST_TOKENS = [
    "-P ",
    "--trace-guards",
    "--include ",
    "--axion-policy",
    "--axion-policy-text",
    "--profile",
    "--emit-axion-log",
]


def normalize_ws(text: str) -> str:
    return " ".join(text.strip().split())


def run_cmd(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, text=True, capture_output=True, check=False)


def extract_usage_block(help_text: str) -> list[str]:
    lines = help_text.splitlines()
    for idx, line in enumerate(lines):
        if line.startswith("Usage: "):
            usage_lines = [normalize_ws(line)]
            j = idx + 1
            while j < len(lines):
                nxt = lines[j]
                if not nxt.strip():
                    break
                if "t81 " in nxt:
                    usage_lines.append(normalize_ws(nxt))
                    j += 1
                    continue
                break
            return usage_lines
    return []


def extract_manual_cli_lines(manual_text: str) -> set[str]:
    lines: set[str] = set()
    for raw in manual_text.splitlines():
        line = raw.strip().strip("`")
        if "t81 " in line:
            lines.add(normalize_ws(line))
    return lines


def extract_top_level_commands(help_text: str) -> list[str]:
    commands: list[str] = []
    in_commands = False
    for line in help_text.splitlines():
        if line.strip() == "Commands:":
            in_commands = True
            continue
        if in_commands:
            if not line.strip():
                break
            match = re.match(r"^\s{2}([a-z0-9-]+)\s", line)
            if match:
                commands.append(match.group(1))
    return commands


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--t81-bin",
        type=Path,
        default=Path("build/t81"),
        help="Path to t81 binary (default: build/t81).",
    )
    parser.add_argument(
        "--manual",
        type=Path,
        default=Path("docs/guides/cli-user-manual.md"),
        help="Path to CLI manual markdown file.",
    )
    args = parser.parse_args()

    t81_bin = args.t81_bin
    manual = args.manual
    if not t81_bin.exists():
        print(f"error: missing t81 binary: {t81_bin}")
        return 1
    if not manual.exists():
        print(f"error: missing CLI manual: {manual}")
        return 1

    manual_text = manual.read_text(encoding="utf-8")
    manual_lines = extract_manual_cli_lines(manual_text)

    failures: list[str] = []

    # Gate 1: every command usage emitted by help must be represented in manual.
    for topic in HELP_TOPICS:
        proc = run_cmd([str(t81_bin), "help", topic])
        output = proc.stdout + proc.stderr
        if proc.returncode != 0:
            failures.append(f"help topic failed: {topic} (exit={proc.returncode})")
            continue
        usage_lines = extract_usage_block(output)
        if not usage_lines:
            failures.append(f"missing Usage block in help output for topic: {topic}")
            continue
        for usage in usage_lines:
            candidates = {usage}
            if usage.startswith("Usage: "):
                candidates.add(normalize_ws(usage[len("Usage: ") :]))
            if not any(candidate in manual_lines for candidate in candidates):
                failures.append(
                    f"manual missing usage line for '{topic}': {usage}"
                )

    # Gate 2: top-level commands listed in --help must appear in manual.
    top = run_cmd([str(t81_bin), "--help"])
    top_text = top.stdout + top.stderr
    if top.returncode != 0:
        failures.append(f"top-level --help failed (exit={top.returncode})")
    else:
        for command in extract_top_level_commands(top_text):
            if command in {"help", "version"}:
                continue
            if f"`{command}`" not in manual_text and f"t81 {command}" not in manual_text:
                failures.append(
                    f"manual missing documented command section/reference: {command}"
                )

    # Gate 3: stale/unsupported options should not exist in the manual.
    for token in DENYLIST_TOKENS:
        if token in manual_text:
            failures.append(f"manual contains unsupported/stale token: {token.strip()}")

    if failures:
        print("cli docs parity: FAILED")
        for item in failures:
            print(f"  - {item}")
        return 1

    print(
        f"cli docs parity: ok (topics={len(HELP_TOPICS)}, "
        f"manual_lines={len(manual_lines)})"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
