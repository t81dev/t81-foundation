#!/usr/bin/env python3
"""
check_kernelcall_abi_freeze.py — RFC-00BD KernelCall ABI ordinal freeze gate.

Parses kernel_abi.hpp to extract the KernelCallKind enum, parses
kernel_abi_wire.hpp for the wire constants, and verifies that every entry in
kernelcall_abi_manifest.txt is present at the expected ordinal.

Exits 0 on success, 1 on any drift detected.

Usage:
    python3 scripts/ci/check_kernelcall_abi_freeze.py [repo-root]

If repo-root is omitted, the current working directory is used.
"""

import re
import sys
from pathlib import Path

# ── Paths ─────────────────────────────────────────────────────────────────────

def repo_root() -> Path:
    if len(sys.argv) > 1:
        return Path(sys.argv[1])
    return Path.cwd()

ROOT          = repo_root()
ENUM_FILE     = ROOT / "userland/experimental/kernel/kernel_abi.hpp"
WIRE_FILE     = ROOT / "userland/experimental/kernel/kernel_abi_wire.hpp"
MANIFEST_FILE = ROOT / "spec/rfcs/kernelcall_abi_manifest.txt"

# ── Parser: KernelCallKind enum ────────────────────────────────────────────────

def parse_enum(path: Path) -> dict[str, int]:
    """Return {name: ordinal} for every KernelCallKind entry in kernel_abi.hpp."""
    text = path.read_text()
    # Extract the enum body.
    m = re.search(
        r'enum class KernelCallKind\s*:\s*\w+\s*\{(.*?)\};',
        text, re.DOTALL
    )
    if not m:
        raise ValueError(f"KernelCallKind enum not found in {path}")

    body = m.group(1)
    result: dict[str, int] = {}
    ordinal = 0
    for line in body.splitlines():
        # Strip comments and whitespace.
        line = re.sub(r'//.*', '', line).strip().rstrip(',')
        if not line or line.startswith('#'):
            continue
        # Assignment: Name = N
        assign = re.match(r'^(\w+)\s*=\s*(-?\d+)$', line)
        if assign:
            name = assign.group(1)
            ordinal = int(assign.group(2))
            result[name] = ordinal
            ordinal += 1
            continue
        # Plain name (implicit ordinal).
        plain = re.match(r'^(\w+)$', line)
        if plain:
            result[plain.group(1)] = ordinal
            ordinal += 1
    return result

# ── Parser: wire constants ─────────────────────────────────────────────────────

def parse_wire_constants(path: Path) -> dict[str, int]:
    """Return the three wire constants needed by the manifest [wire] section."""
    text = path.read_text()
    constants: dict[str, int] = {}
    patterns = {
        'request_magic':  r'kKernelAbiWireRequestMagic\s*=\s*(0x[0-9A-Fa-f]+)',
        'response_magic': r'kKernelAbiWireResponseMagic\s*=\s*(0x[0-9A-Fa-f]+)',
        'version':        r'kKernelAbiWireVersion\s*=\s*(\d+)',
    }
    for key, pat in patterns.items():
        m = re.search(pat, text)
        if not m:
            raise ValueError(f"{key} not found in {path}")
        constants[key] = int(m.group(1), 0)
    return constants

# ── Parser: manifest ──────────────────────────────────────────────────────────

def parse_manifest(path: Path) -> tuple[dict[str, dict], dict[str, int]]:
    """
    Return (ordinal_entries, wire_section).
    ordinal_entries: {name: {'ordinal': int, 'conditional': str|None, 'retired': str|None}}
    wire_section:    {key: int}
    """
    text = path.read_text()
    section = None
    ordinals: dict[str, dict] = {}
    wire: dict[str, int] = {}

    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        if line == '[ordinals]':
            section = 'ordinals'
            continue
        if line == '[wire]':
            section = 'wire'
            continue

        if section == 'ordinals':
            # NAME=ORDINAL [conditional:SYM] [retired:RFC]
            parts = line.split()
            kv = parts[0].split('=', 1)
            if len(kv) != 2:
                continue
            name, ordinal_str = kv
            entry: dict = {
                'ordinal': int(ordinal_str),
                'conditional': None,
                'retired': None,
            }
            for flag in parts[1:]:
                if flag.startswith('conditional:'):
                    entry['conditional'] = flag[len('conditional:'):]
                elif flag.startswith('retired:'):
                    entry['retired'] = flag[len('retired:'):]
            ordinals[name] = entry

        elif section == 'wire':
            kv = line.split('=', 1)
            if len(kv) == 2:
                wire[kv[0].strip()] = int(kv[1].strip(), 0)

    return ordinals, wire

# ── Validation ────────────────────────────────────────────────────────────────

def validate(
    enum: dict[str, int],
    wire_consts: dict[str, int],
    manifest_ordinals: dict[str, dict],
    manifest_wire: dict[str, int],
) -> list[str]:
    """Return a list of failure strings; empty list = pass."""
    failures: list[str] = []

    # Check every manifest ordinal entry against the parsed enum.
    for name, entry in manifest_ordinals.items():
        expected = entry['ordinal']
        if entry['retired']:
            # Retired entries: ordinal must still be absent or present at the
            # same value (implementations may keep it as a comment — we just
            # ensure it hasn't been reassigned).
            if name in enum and enum[name] != expected:
                failures.append(
                    f"FAIL: retired ordinal reassigned: "
                    f"{name} manifest={expected}, enum={enum[name]}"
                )
            continue

        if name not in enum:
            failures.append(
                f"FAIL: KernelCallKind.{name} not found in {ENUM_FILE.name} "
                f"(manifest expects ordinal {expected})"
            )
            continue

        actual = enum[name]
        if actual != expected:
            failures.append(
                f"FAIL: KernelCallKind ordinal drift: "
                f"{name}: manifest={expected}, enum={actual}"
            )

    # Check that no new entry was inserted before the manifest's highest
    # non-conditional ordinal.  "Inserted before" means an enum entry has a
    # lower ordinal than the highest frozen entry but is not in the manifest.
    frozen_max = max(
        e['ordinal'] for e in manifest_ordinals.values()
        if not e['retired'] and not e['conditional']
    )
    frozen_names = {n for n, e in manifest_ordinals.items() if not e['retired']}
    for name, ordinal in enum.items():
        if name not in frozen_names and ordinal <= frozen_max:
            failures.append(
                f"FAIL: new KernelCallKind.{name}={ordinal} inserted before "
                f"freeze point (max frozen unconditional ordinal={frozen_max}); "
                f"add it to kernelcall_abi_manifest.txt"
            )

    # Check wire constants.
    for key, expected in manifest_wire.items():
        actual = wire_consts.get(key)
        if actual is None:
            failures.append(f"FAIL: wire constant '{key}' not found in {WIRE_FILE.name}")
        elif actual != expected:
            failures.append(
                f"FAIL: wire constant drift: {key}: "
                f"manifest=0x{expected:08X}, header=0x{actual:08X}"
            )

    return failures

# ── Main ──────────────────────────────────────────────────────────────────────

def main() -> int:
    try:
        enum           = parse_enum(ENUM_FILE)
        wire_consts    = parse_wire_constants(WIRE_FILE)
        m_ordinals, m_wire = parse_manifest(MANIFEST_FILE)
    except (FileNotFoundError, ValueError) as e:
        print(f"FAIL: {e}", file=sys.stderr)
        return 1

    failures = validate(enum, wire_consts, m_ordinals, m_wire)

    if failures:
        for f in failures:
            print(f, file=sys.stderr)
        print(
            f"\nKernelCall ABI ordinal freeze check FAILED "
            f"({len(failures)} violation(s)).",
            file=sys.stderr,
        )
        return 1

    total = len(m_ordinals)
    ver   = m_wire.get('version', '?')
    print(f"PASS: KernelCallKind ordinals match manifest ({total} entries, wire version={ver})")
    return 0


if __name__ == '__main__':
    sys.exit(main())
