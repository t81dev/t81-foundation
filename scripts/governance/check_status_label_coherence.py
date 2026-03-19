#!/usr/bin/env python3
"""Check component maturity/spec-authority coherence against canonical policy."""

from __future__ import annotations

import re
import sys
import json
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]

README = REPO_ROOT / "README.md"
MATRIX = REPO_ROOT / "docs/status/IMPLEMENTATION_MATRIX.md"
SYSTEM_STATUS = REPO_ROOT / "docs/status/SYSTEM_STATUS.md"
PROMOTION_GATE = REPO_ROOT / "docs/status/T81LANG_PROMOTION_GATE.md"
CANONICAL = REPO_ROOT / "docs/governance/MATURITY_CANONICAL_STATUS.json"


def _normalize(label: str) -> str:
    cleaned = label.lower()
    cleaned = re.sub(r"[*_`]", "", cleaned)
    cleaned = re.sub(r":[^:]+:", "", cleaned)
    cleaned = cleaned.replace("⚠️", "").replace("🚧", "").replace("✅", "").replace("🧪", "")
    cleaned = " ".join(cleaned.split())
    if "beta" in cleaned:
        return "beta"
    if "alpha" in cleaned:
        return "alpha"
    if "stable" in cleaned:
        return "stable"
    if "experimental" in cleaned:
        return "experimental"
    if "concept" in cleaned:
        return "concept"
    if "partial" in cleaned:
        return "partial"
    return cleaned


def _parse_markdown_table(path: Path) -> list[list[str]]:
    rows: list[list[str]] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.startswith("|"):
            continue
        if set(line.replace("|", "").strip()) <= {":", "-", " "}:
            continue
        parts = [p.strip() for p in line.split("|")[1:-1]]
        if parts:
            rows.append(parts)
    return rows


def _component_labels_from_readme(path: Path) -> dict[str, str]:
    labels: dict[str, str] = {}
    for row in _parse_markdown_table(path):
        if len(row) < 2:
            continue
        comp = re.sub(r"[*`]", "", row[0]).strip().lower()
        maturity = row[1]
        if comp in {"t81vm", "t81lang"}:
            labels[comp] = _normalize(maturity)
        elif comp in {"axion", "axion governance kernel", "axion kernel"}:
            labels["axion"] = _normalize(maturity)
    return labels


def _component_labels_from_system_status(path: Path) -> dict[str, str]:
    labels: dict[str, str] = {}
    for row in _parse_markdown_table(path):
        if len(row) < 2:
            continue
        comp = re.sub(r"[*`]", "", row[0]).strip().lower()
        if comp in {"t81vm", "t81lang", "axion kernel", "axion governance kernel"}:
            key = "axion" if comp in {"axion kernel", "axion governance kernel"} else comp
            labels[key] = _normalize(row[1])
    return labels


def _component_labels_from_matrix(path: Path) -> dict[str, str]:
    labels: dict[str, str] = {}
    rows = _parse_markdown_table(path)
    if not rows:
        return labels
    header = [h.strip().lower() for h in rows[0]]
    component_idx = header.index("subsystem") if "subsystem" in header else 0
    maturity_idx = (
        header.index("implementation maturity")
        if "implementation maturity" in header
        else min(3, len(header) - 1)
    )
    for row in rows[1:]:
        if len(row) <= max(component_idx, maturity_idx):
            continue
        comp = re.sub(r"[*`]", "", row[component_idx]).strip().lower()
        maturity = row[maturity_idx]
        if comp in {"t81vm", "t81lang", "axion kernel", "axion governance kernel"}:
            key = "axion" if comp in {"axion kernel", "axion governance kernel"} else comp
            labels[key] = _normalize(maturity)
    return labels


def _load_canonical() -> dict[str, dict[str, str]]:
    payload = json.loads(CANONICAL.read_text(encoding="utf-8"))
    components = payload.get("components", {})
    if not isinstance(components, dict):
        raise RuntimeError("canonical status file missing 'components' object")
    normalized: dict[str, dict[str, str]] = {}
    for key, value in components.items():
        if not isinstance(value, dict):
            raise RuntimeError(f"component '{key}' entry must be an object")
        maturity = str(value.get("maturity", "")).strip()
        spec_authority = str(value.get("spec_authority", "")).strip()
        spec_path = str(value.get("spec_path", "")).strip()
        if not maturity or not spec_authority or not spec_path:
            raise RuntimeError(
                f"component '{key}' must define maturity, spec_authority, and spec_path"
            )
        normalized[key] = {
            "maturity": _normalize(maturity),
            "spec_authority": _normalize(spec_authority),
            "spec_path": spec_path,
            "promotion_gate_classification": str(
                value.get("promotion_gate_classification", "")
            ).strip(),
        }
    return normalized


def _parse_spec_status(path: Path) -> str:
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("Status:"):
            value = line.split(":", 1)[1].strip().rstrip("\\").strip()
            return _normalize(value)
    return ""


def main() -> int:
    issues: list[str] = []

    for p in (README, MATRIX, SYSTEM_STATUS, PROMOTION_GATE, CANONICAL):
        if not p.exists():
            issues.append(f"missing required file: {p.relative_to(REPO_ROOT)}")
    if issues:
        for issue in issues:
            print(f"- {issue}")
        return 1

    try:
        canonical = _load_canonical()
    except Exception as exc:  # pragma: no cover
        print(f"status label coherence check FAILED: {exc}")
        return 1

    readme = _component_labels_from_readme(README)
    matrix = _component_labels_from_matrix(MATRIX)
    system = _component_labels_from_system_status(SYSTEM_STATUS)

    required_components = tuple(canonical.keys())
    for comp in required_components:
        if comp not in readme:
            issues.append(f"README missing component status row for: {comp}")
        if comp not in matrix:
            issues.append(f"implementation matrix missing component row for: {comp}")
        if comp not in system:
            issues.append(f"system status missing component row for: {comp}")

    for comp in required_components:
        if comp not in readme or comp not in matrix or comp not in system:
            continue
        expected = canonical[comp]["maturity"]
        if readme[comp] != expected:
            issues.append(f"README maturity mismatch for {comp}: expected={expected} got={readme[comp]}")
        if matrix[comp] != expected:
            issues.append(
                f"implementation matrix maturity mismatch for {comp}: expected={expected} got={matrix[comp]}"
            )
        if system[comp] != expected:
            issues.append(
                f"system status maturity mismatch for {comp}: expected={expected} got={system[comp]}"
            )
        if not (readme[comp] == matrix[comp] == system[comp] == expected):
            issues.append(
                "label mismatch for "
                f"{comp}: README={readme[comp]} matrix={matrix[comp]} system={system[comp]} expected={expected}"
            )

    matrix_rows = _parse_markdown_table(MATRIX)
    matrix_header = [h.strip().lower() for h in matrix_rows[0]] if matrix_rows else []
    subsystem_idx = matrix_header.index("subsystem") if "subsystem" in matrix_header else 0
    spec_authority_idx = (
        matrix_header.index("spec authority")
        if "spec authority" in matrix_header
        else min(2, len(matrix_header) - 1)
    )
    matrix_spec_surface: dict[str, str] = {}
    for row in matrix_rows[1:]:
        if len(row) <= max(subsystem_idx, spec_authority_idx):
            continue
        comp = re.sub(r"[*`]", "", row[subsystem_idx]).strip().lower()
        if comp in {"axion kernel", "axion governance kernel"}:
            comp = "axion"
        if comp in canonical:
            matrix_spec_surface[comp] = row[spec_authority_idx]

    for comp, rules in canonical.items():
        expected_spec = rules["spec_authority"]
        surface = matrix_spec_surface.get(comp, "")
        surface_norm = _normalize(surface)
        if f"({expected_spec})" not in surface.lower() and surface_norm != expected_spec:
            issues.append(
                f"implementation matrix spec-authority mismatch for {comp}: expected marker ({expected_spec}) in '{surface}'"
            )

    gate_text = PROMOTION_GATE.read_text(encoding="utf-8")
    t81lang_gate = canonical.get("t81lang", {}).get("promotion_gate_classification", "")
    if t81lang_gate and t81lang_gate not in gate_text:
        issues.append("promotion gate classification mismatch for t81lang")

    for comp, rules in canonical.items():
        spec_path = REPO_ROOT / rules["spec_path"]
        if not spec_path.exists():
            issues.append(f"missing spec for {comp}: {rules['spec_path']}")
            continue
        spec_status = _parse_spec_status(spec_path)
        expected = rules["spec_authority"]
        if spec_status != expected:
            issues.append(
                f"spec status mismatch for {comp}: expected={expected} got={spec_status} ({rules['spec_path']})"
            )

    if issues:
        print("status label coherence check FAILED:")
        for issue in issues:
            print(f"- {issue}")
        return 1

    print("status label coherence check PASSED")
    print("- checked README, IMPLEMENTATION_MATRIX, and SYSTEM_STATUS maturity labels against canonical policy")
    print("- checked spec authority markers in IMPLEMENTATION_MATRIX")
    print("- checked spec Status headers against canonical policy")
    return 0


if __name__ == "__main__":
    sys.exit(main())
