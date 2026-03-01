# C2 Month-Close Runbook (2026-03-31)

Status: Scheduled
Owner: @t81dev
Window: 2026-03-31 (UTC)
Prepared On: 2026-02-25

## Purpose

Provide an executable, auditable C2 month-close procedure for finalizing the
March governance review without changing authority, freeze, or determinism
boundaries.

## Scope

This runbook covers only finalization mechanics for:

- `docs/records/audits/2026-03-governance-review.md`
- `docs/governance/MONTHLY_GOVERNANCE_REVIEW_CHECKLIST.md`
- supporting status/governance/product link and hygiene checks

## Preconditions

1. Working tree is clean for governance/status artifacts.
2. No pending unresolved governance exceptions for March close.
3. Promotion-gate snapshot is rerun within the close window.

## Execution Steps (Run in Order)

1. Governance hygiene check:
   - Command:
     - `python3 scripts/governance/check_docs_governance_hygiene.py`
   - Record: pass/fail and key output lines.
2. Promotion gate snapshot refresh:
   - Command:
     - `python3 scripts/governance/t81lang_promotion_gate_snapshot.py`
   - Record: snapshot result (`READY`/`NOT_READY`) and file timestamp for
     `docs/status/T81LANG_PROMOTION_GATE_SNAPSHOT.md`.
3. Markdown link-target sweep:
   - Command:
     - `python3 - <<'PY'\nimport pathlib,re,sys\nroot=pathlib.Path('.').resolve()\nscan=[root/'docs/status',root/'docs/governance',root/'docs/product',root/'docs/records/audits']\npat=re.compile(r'\\[[^\\]]+\\]\\(([^)#]+)')\nmissing=[]\nfor base in scan:\n  for md in base.rglob('*.md'):\n    txt=md.read_text(encoding='utf-8')\n    for m in pat.finditer(txt):\n      link=m.group(1).strip()\n      if not link or '://' in link or link.startswith('#'):\n        continue\n      tgt=(md.parent/link).resolve()\n      if not tgt.exists():\n        missing.append((md.relative_to(root),link))\nif missing:\n  for md,link in missing:\n    print(f'MISSING {md} -> {link}')\n  sys.exit(1)\nprint('link-target sweep passed')\nPY`
   - Record: pass/fail and any missing targets.
   - Consolidated option (runs steps 1-3 and writes report):
     - `python3 scripts/governance/c2_month_close_check.py`
   - Default report output:
     - `docs/status/C2_MONTH_CLOSE_CHECK_2026-03-31.md`
   - One-command preflight (recommended before final close day execution):
     - `python3 scripts/governance/c2_month_close_preflight.py`
   - Default preflight report output:
     - `docs/status/C2_MONTH_CLOSE_PREFLIGHT_2026-03-31.md`
4. Checklist reconfirmation:
   - Source:
     - `docs/governance/MONTHLY_GOVERNANCE_REVIEW_CHECKLIST.md`
   - Record: final pass/fail by section, plus exceptions/remediation if any.
5. Final stamp update:
   - Update `docs/records/audits/2026-03-governance-review.md` fields:
     - `Status: Final`
     - `Finalized Date (UTC): YYYY-MM-DD`
     - `Finalized By: @t81dev`
   - Add final C2 outcome paragraph with command outputs summarized.

## Evidence Capture Template

- Hygiene check: `PASS|FAIL` + output summary
- Promotion snapshot: `READY|NOT_READY` + snapshot timestamp
- Link-target sweep: `PASS|FAIL` + missing list (if any)
- Checklist result: `PASS|PASS WITH EXCEPTIONS|FAIL`
- Final outcome: `C2 CLOSED|C2 BLOCKED` + blockers/remediation owner/due date

## Cross-References

- `docs/status/EXECUTION_PLAN_2026-03.md`
- `docs/status/PROJECT_CONTROL_CENTER.md`
- `docs/records/audits/2026-03-governance-review.md`
- `docs/governance/MONTHLY_GOVERNANCE_REVIEW_CHECKLIST.md`
- `docs/status/T81LANG_PROMOTION_GATE_SNAPSHOT.md`
- `docs/status/C2_MONTH_CLOSE_PREFLIGHT_2026-03-31.md`

## Versioning Statement

Operational runbook only. Authority hierarchy remains:
/spec > docs/architecture/OVERVIEW.md > /docs > /book.
