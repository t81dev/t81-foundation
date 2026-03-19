#!/usr/bin/env python3
"""Execute marked CLI examples from the current CLI user manual."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


SMOKE_MARKER = "# docs-smoke"


def extract_smoke_commands(markdown: str) -> list[str]:
    commands: list[str] = []
    in_bash_block = False
    for raw in markdown.splitlines():
        line = raw.rstrip()
        stripped = line.strip()
        if stripped.startswith("```"):
            fence = stripped[3:].strip().lower()
            if in_bash_block:
                in_bash_block = False
            else:
                in_bash_block = fence in {"bash", "sh", "shell"}
            continue
        if not in_bash_block:
            continue
        if SMOKE_MARKER not in line:
            continue
        cmd = line.split(SMOKE_MARKER, 1)[0].strip()
        if cmd:
            commands.append(cmd)
    return commands


def run_command(command: str, cwd: Path, timeout_sec: int) -> tuple[int, str, str]:
    proc = subprocess.run(
        command,
        cwd=cwd,
        shell=True,
        text=True,
        capture_output=True,
        timeout=timeout_sec,
        check=False,
    )
    return proc.returncode, proc.stdout, proc.stderr


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--manual",
        type=Path,
        default=Path("docs/user-guide/reference/cli-user-manual.md"),
        help="Path to CLI manual markdown file.",
    )
    parser.add_argument(
        "--cwd",
        type=Path,
        default=Path("."),
        help="Working directory for command execution.",
    )
    parser.add_argument(
        "--timeout-sec",
        type=int,
        default=20,
        help="Per-command timeout in seconds (default: 20).",
    )
    args = parser.parse_args()

    manual = args.manual
    if not manual.exists():
        print(f"error: missing CLI manual: {manual}")
        return 1

    text = manual.read_text(encoding="utf-8")
    commands = extract_smoke_commands(text)
    if not commands:
        print("cli docs smoke: FAILED")
        print(f"  - no commands found with marker '{SMOKE_MARKER}'")
        return 1

    failures: list[str] = []
    for command in commands:
        try:
            rc, out, err = run_command(command, args.cwd, args.timeout_sec)
        except subprocess.TimeoutExpired:
            failures.append(f"timeout: {command}")
            continue
        if rc != 0:
            summary = (err.strip() or out.strip()).splitlines()
            snippet = summary[0] if summary else "no output"
            failures.append(f"exit={rc}: {command} | {snippet}")

    if failures:
        print("cli docs smoke: FAILED")
        for item in failures:
            print(f"  - {item}")
        return 1

    print(f"cli docs smoke: ok (commands={len(commands)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
