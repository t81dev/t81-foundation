#!/usr/bin/env python3
"""Keep ARCHITECTURE target table in sync with CMake target declarations."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ARCH_TABLE_ROW = re.compile(r"^\|\s*`([^`]+)`(?:\s+\([^|`]*\))?\s*\|")
# This is a declaration inventory, including optional/platform branches, not
# CMake configuration or a proof of target kinds, linkage, or enabled features.
CMAKE_LITERAL = re.compile(
    r'(?P<comment>\#\[(?P<ceq>=*)\[.*?\](?P=ceq)\]|\#[^\n]*)'
    r'|(?P<bracket>\[(?P<beq>=*)\[.*?\](?P=beq)\])'
    r'|(?P<quoted>"(?:\\.|[^"\\])*")',
    re.DOTALL,
)
CMAKE_COMMAND = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")
TARGET_COMMANDS = {"add_library", "add_executable", "pybind11_add_module"}


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def architecture_targets(architecture_md: Path) -> set[str]:
    targets: set[str] = set()
    for line in read_text(architecture_md).splitlines():
        match = ARCH_TABLE_ROW.match(line)
        if match:
            targets.add(match.group(1))
    return targets


def cmake_commands(text: str):
    """Read literal command/first-argument pairs without evaluating CMake.

    Skip comments and strings while finding balanced command boundaries so
    examples inside message()/quoted/bracket text cannot declare fake targets.
    Function/macro bodies are not executed or credited as target declarations.
    """
    code = CMAKE_LITERAL.sub(lambda m: re.sub(r"[^\n]", " ", m[0]), text)
    position = definitions = 0
    while match := CMAKE_COMMAND.search(code, position):
        begin = position = match.end()
        depth = 1
        while position < len(code) and depth:
            depth += (code[position] == "(") - (code[position] == ")")
            position += 1
        if depth:
            raise ValueError(f"unclosed CMake command {match[1]}")
        name = match[1].lower()
        if name in {"function", "macro"}:
            definitions += 1
            continue
        if name in {"endfunction", "endmacro"}:
            definitions -= 1
            if definitions < 0:
                raise ValueError("unmatched CMake function/macro end")
            continue
        if definitions:
            continue
        arguments = CMAKE_LITERAL.sub(
            lambda m: " " if m["comment"] is not None else m[0],
            text[begin:position - 1],
        ).lstrip()
        literal = CMAKE_LITERAL.match(arguments)
        if literal and literal["quoted"] is not None:
            first = re.sub(r"\\(.)", r"\1", literal[0][1:-1])
        elif literal and literal["bracket"] is not None:
            equals = literal["beq"]
            first = literal[0][len(equals) + 2:-(len(equals) + 2)].lstrip("\n")
        else:
            first = arguments.split(maxsplit=1)[0] if arguments else ""
        yield name, first
    if definitions:
        raise ValueError("unclosed CMake function/macro")


def cmake_targets(cmake_files: list[Path]) -> set[str]:
    return {
        first
        for cmake in cmake_files
        for command, first in cmake_commands(read_text(cmake))
        if command in TARGET_COMMANDS and re.fullmatch(r"[A-Za-z0-9_+-]+", first)
    }


def cmake_source_files(root: Path) -> list[Path]:
    """Follow literal local add_subdirectory edges, never scan stray build trees.

    All conditional branches are inventoried, preserving the existing optional
    target contract. Variable-generated paths/targets and include-loaded target
    factories are not evaluated. A documented name needs a literal declaration
    in the inspected source tree; dynamic declarations cannot satisfy it.
    """
    root = root.resolve()
    pending = [root / "CMakeLists.txt"]
    visited: set[Path] = set()
    while pending:
        cmake = pending.pop().resolve()
        if not cmake.is_relative_to(root):
            raise ValueError(f"CMake source escapes repository: {cmake}")
        if cmake in visited:
            continue
        visited.add(cmake)
        for command, directory in cmake_commands(read_text(cmake)):
            if command == "add_subdirectory" and directory and "$" not in directory:
                pending.append(cmake.parent / directory / "CMakeLists.txt")
    return sorted(visited)


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    architecture_md = root / "docs/explanation/ARCHITECTURE.md"
    required_files = [root / "CMakeLists.txt", root / "benchmarks/CMakeLists.txt"]

    if not architecture_md.exists():
        print(f"error: missing {architecture_md}")
        return 1
    for cmake in required_files:
        if not cmake.exists():
            print(f"error: missing {cmake}")
            return 1

    arch_targets = architecture_targets(architecture_md)
    if not arch_targets:
        print("error: ARCHITECTURE.md has no documented targets")
        return 1
    try:
        cmake_files = cmake_source_files(root)
        declared_targets = cmake_targets(cmake_files)
    except (OSError, ValueError) as error:
        print(f"error: cannot inventory CMake declarations: {error}")
        return 1

    missing = sorted(t for t in arch_targets if t not in declared_targets)
    if missing:
        print("ARCHITECTURE.md references targets not declared in CMake:")
        for target in missing:
            print(f"  - {target}")
        return 1

    print(f"architecture target sync: ok ({len(arch_targets)} documented targets, "
          f"{len(cmake_files)} reachable CMakeLists.txt files)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
