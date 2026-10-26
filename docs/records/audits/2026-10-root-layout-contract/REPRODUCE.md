# Reproducing this investigation

Use a full-history checkout at exact #578
`f4ffa85bf0bcaf028b51462113bf19c2eebb0091`, separate from any contributor work.
Read-only commands below do not add policy permission:

```sh
git log --all --diff-filter=A --format='%H %as %s' -- scripts/governance/check_root_structure.py
git log --follow -p -- scripts/governance/check_root_structure.py
git blame HEAD -- scripts/governance/check_root_structure.py
git cat-file -e 529a9486^:scripts/governance/check_root_structure.py
git diff-tree --root --no-commit-id --name-status -r 529a9486 -- scripts/governance/check_root_structure.py
git merge-base --is-ancestor 529a9486 01c8501931
git diff 01c8501931^ 01c8501931 -- scripts/governance/check_root_structure.py
git rev-parse 01c8501931:scripts/governance/check_root_structure.py 01c8501931^:scripts/governance/check_root_structure.py
git show cb54c175 -- package.json package-lock.json
git log -L '/### F. New Root File/,+2:docs/governance/SPEC_AUTHORITY_MODEL.md'
```

The `cat-file` failure is intentional evidence of pre-introduction absence, not
a passing validation check. `lineage-and-authority.json` records every argument,
exit and diagnostic, including those intentional failures.

The 27-command argument lists in `inventory.json` can be run independently with
Python `subprocess.run(command, capture_output=True, text=True, timeout=180)`.
Record both output streams and the actual return code for every command. Do not
let the first failure suppress later inventory commands. Verify clean status
before and after; the metrics generator writes ignored outputs. A shallow clone
can change translation-staleness results. This inventory is not a build test.

For the five existing regression programs, run the commands recorded in
`regressions.json`. Supply a **new, absent** scratch path to the failure-propagation
program's required `--output-dir`; the program intentionally refuses an existing
directory. The bare command's exit 2 is preserved separately. Supply-chain tests
use the already available PyYAML 6.0.3. The corrected failure-propagation run
produces a 47-case result and individual logs at the recorded scratch path; those
temporary probe files are not production research artifacts.

After obtaining this audit's proposed data files, run `verify_proposal.py` with
`jsonschema==4.26.0` to reproduce its JSON-schema and Git-observation checks. It
prints verification JSON to stdout; redirect to an external scratch file when
comparing with `proposal-verification.json`. It does not compare current root
permissions or grant approval. All baseline objects must still be available.

No report or proposal file must be placed in the clean baseline checkout during
the baseline inventory. The audit was assembled afterward in the isolated
descendant branch. The non-normative JSON was built by explicitly transcribing
existing allowlist expectations minus the two obsolete manifests and generated
Makefile, plus three active directory candidates, then recording separate
unresolved rows for every other entry. `ENTRY_MATRIX.md` and the design explain
the evidence and decisions for each category; observed types are obtained from
`git ls-tree -z` and are never treated as independent approval.
