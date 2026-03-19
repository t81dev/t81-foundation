# RFC-0034 Ternary Inference Evidence Snapshot

Status: Active
Date: 2026-03-18
Owner: @t81dev

## Scope

Evidence refresh for RFC-0034 implementation closure after the CanonFS-backed
`.t81w` execution path was wired through the CLI and exercised against the
existing ternary inference fixture surface.

## Verification

Commands run locally on Darwin ARM64:

```sh
cmake --build build --target t81 t81_cli_contract_test t81_vm_rfc0034_ternary_native_test -j4
./build/t81_vm_rfc0034_ternary_native_test
./build/t81_cli_contract_test ./build/t81
```

Observed results:

- `t81_vm_rfc0034_ternary_native_test`: pass
- `t81_cli_contract_test`: pass
- The CLI contract suite now includes a CanonFS-backed weights-model flow:
  - save deterministic `mat_a`/`mat_b` weights to a real `.t81w`
  - store that artifact in CanonFS via `canonfs put-file`
  - execute `code run tests/fixtures/t81lang_std_tensor/03_matmul_weights.t81`
    with `--weights-model sha3-256:...`
  - observe the expected `<tensor#1>` output

Interpretation:

- RFC-0034 no longer has an in-repo implementation gap around the
  `.t81w/ternary` plus CanonFS path.
- The CLI execution bridge now preserves attached weights models across the
  temporary `.t81` → `.tisc` compile/run handoff, which was required for the
  CanonFS-backed fixture to execute successfully.
- Remaining RFC-0034 closure work is promotion evidence, not code: the
  reference-platform acceptance criteria still need the corresponding
  cross-platform CI refresh.
