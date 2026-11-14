# Dual-witness transition — no workflow or compiler change now

Historical witness H: existing saved18 compiler gate at its pinned source, fixtures,
build/configuration and byte format. Preserve expected aggregate
`ea52b9edf85a7392f2fb49814253d4f2972a79ee205a8be8246f1c40356dff71`, original bytes,
negative outcomes, commands and raw hashes. H is not proof of a new compiler.

New witness N: proposed saved-program-container-v1 namespace, approved descriptor,
codec, caps and exact qualified implementation build. Its expected hash is **not
known**; it must be generated from actual reviewed cross-platform runs, not this
documentation vector. New namespace and required-check names need owner review.

Stages / acceptance:

1. **Package only (now):** retain H untouched; specify N and its qualification plan.
   Required workflows are unchanged. No runtime/output switching or new golden.
2. **Approved implementation, no default switch:** add bounded reader/writer and
   explicit test-only N production path after both freeze reviews. Keep H's historical
   lane independently reproducible at its pinned code; retain a clearly labeled
   current legacy-format emission comparison while that format is supported for
   testing. Do not expose a legacy execution/admission bypass in installed APIs.
3. **Parallel qualification:** H and N both have explicit non-overlapping identities.
   N must compare two emissions per fixture, cross-platform complete container bytes,
   descriptor/build association, payload/component identities and negative admission.
   H proves historical continuity only; N proves the exact new tested surface.
   Test-only compatibility emission is not an approval to execute unbound files.
4. **Reviewed gate transition:** approve exact required contexts, fixture ownership
   and status at the output-switch SHA. Existing required context cannot silently
   become optional, green from a skipped test, or point to a different expected hash.
   If a same-current-source legacy emission check cannot remain meaningful after the
   switch, retire/replace its *current-output role* only through explicit governance
   review, retaining the historical lane/evidence and adding N as a required proof.
5. **Separately authorized switch:** only after approved context configuration and
   fresh evidence may the compiler default change. This package does not authorize
   that switch. New expected N hash requires review with raw files, not blind update.

Retain exact source/build/fixture revisions, complete output SHA-256s, internal
container digest, descriptor and caps identities, commands, environments, failures,
skips and approvals. Internal digest is not the complete file's SHA-256. Optional
payload witness cannot replace whole-container identity or authenticate provenance.

Target Linux GCC/Clang x86_64/ARM64, macOS ARM64/x86_64, Windows MSVC/clang-cl;
Release-active assertions and failure injection; available ASan/UBSan and allocation
budget checks. Mark unavailable lanes, unknown identities and unqualified variants
as blocked, not passing. No new cross-platform run or N witness exists in this package.

Historical raw13, saved18 fixtures, CPA traces and Commons discrepancy pins remain
byte-identical. Referenced CPA source paths absent from pinned main require locating
actual pinned evidence; local untracked copies do not count as authenticated history.
