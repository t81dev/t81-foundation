# Preparation record — RFC-00D7-F01

Prepared 2026-11-15. These are document-preparation checks, **not A–F experiment results**. No experiment harness, generated fixture, model enumeration, oracle run or results packet was created. No execution authorization is requested or implied by validation.

The checkout remained at `40695b0ba5cea3e43563b6ed2443c987d2738089`. Contract inputs were read from Git objects at candidate `ec22a7588282d37ad027cff61c807bcf159ac2e0`; the candidate RFC SHA-256 is `a2f8d9fee22f6d27c57cab1d1a8a6b70ff3d0da1f0f81a72aded7a0955520aaf`. All nine entries in `source-pins.json` were checked against both `git show <revision>:<path>` byte hashes and `git rev-parse <revision>:<path>` blob identities. All matched.

The preparation environment was macOS 26.5.2 ARM64, Python 3.14.3 and installed `jsonschema` 4.26.0. The existing interpreter used for document validation was `/private/tmp/t81-pr795-parity-venv/bin/python`; its name does not establish any experimental qualification. Initial attempts with the default interpreter and `/private/tmp/t81-continuity-venv/bin/python` could not run Commons discovery because `jsonschema` was unavailable. No dependency installation was performed. An initially incorrect schema-directory lookup and an incorrect register-key lookup were corrected by reading the actual files; neither was an experiment failure.

Successful commands, run from the repository root:

```sh
/private/tmp/t81-pr795-parity-venv/bin/python -B tools/research_commons/commons.py --request '{"protocol":"t81.research-commons.v1","operation":"get_hypothesis","arguments":{"hypothesis_id":"H07"}}'
/private/tmp/t81-pr795-parity-venv/bin/python -B tools/research_commons/commons.py --validate-contribution docs/research/proposals/RFC-00D7-F01/proposal.json
git diff --check
```

Discovery returned the dated canonical H07 record, its historical limitations and unresolved discrepancies. Its snapshot baseline is `3d395cb604ddd49da3354d17f0505f2ed033757e`; this is not a live integration or CI check. The CLI's normalized-JSON manifest digest is `a1fa8d17672d35364cd630ba8deba3498d18432095f3bf079ac7ad800aa0d6da`. The raw file digest pinned in `source-pins.json` is `c6e2642365399b85a98e53727d1ec5a8240c3cb51ea86416565543a0658de911`. The different digests cover different byte representations, as defined by `tools/research_commons/commons.py`.

Proposal validation exited 0 and returned:

```json
{"execution":"unavailable","limitation":"Shape/reference checks establish neither authorization authenticity nor scientific acceptance.","status":"valid_proposal_shape"}
```

Both JSON documents parse. The new draft files were separately checked for trailing whitespace because `git diff --check` does not inspect untracked files. `SHA256SUMS` identifies this preparation packet; it excludes itself. Hashes establish identity, not truth or acceptance.

Only this new proposal directory was written for the current task. Existing untracked PR #798 review evidence was left in place. No production code, normative specifications, gates, Commons manifest, PR #804, branches or commits were changed; no push or merge occurred. Execution remains unrequested and unauthorized. The two proposed future authorization stages and their limits are in `PROTOCOL.md` §10.
