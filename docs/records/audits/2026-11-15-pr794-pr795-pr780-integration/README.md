# Combined #794 / #795 / #780 integration candidate

Base main: `c2ab68e582c9f773ce94a9e13bcb805016f07dd4`.
Isolated branch: `candidate/pr794-pr795-pr780-main-integration`.

Accepted component pins:

- #794: `5f5751ae9b9f634f2af2cb020afef6f9e2cd610d`.
- #795: `43747b60675444ab58f693713577d6cd42e3f234`.
- #780: `c08c211bd3475d8b09e35648f84c6b5c0a7e31a2`.

`accepted-file-identities.json` records every component's changed file, Git blob,
mode and SHA-256. All 48 files were copied byte-for-byte from those pins. Neither
the three component deltas nor newer main changes overlap. No substantive
resolution or source rewriting was necessary. Original heads are preserved.

#794's wrapper, skill text, Python harness and top-level CMake registration are
retained with its historical receipts. #795's validator, experiment CMake and
original audit packet are retained. Its separately retained parity evidence is
copied unchanged to `../2026-11-15-pr795-validator-ci-parity/`; that packet still
identifies execution against #795, not the combined candidate. #780's corrected
CPU-time/batch timing documentation, source and Cell correctness checks are
retained, including byte-identical raw measurements and their original label
strings. No production Cell arithmetic is changed and no new BASE-PERF-00
performance campaign is authorized or performed by this integration.

Integration-only CI wiring adds direct positive, negative and JSON-file validator
execution normally and under Python -O to the existing main CI docs/spec job,
with the existing jsonschema==4.26.0 pin and retained log artifact. This closes
the previously reported hosted selection gap without a new standalone workflow
or any schema-engine changes. Hosted validator execution is direct Python;
local CTest execution remains separately identified in the qualification record.

Local correctness and full hosted qualification are pending at assembly time.
Their exact tested source, execution commands and results will be retained
separately from historical measurements. No source PR is merged or closed.

This is Codex-authored engineering integration with prior context, not
independent scientific acceptance, new experiment approval or governance change.
