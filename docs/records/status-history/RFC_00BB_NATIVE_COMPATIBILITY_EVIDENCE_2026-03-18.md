# RFC-00BB Native Compatibility Evidence — 2026-03-18

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [RFC-00BB Native Compatibility Evidence — 2026-03-18](#rfc-00bb-native-compatibility-evidence-—-2026-03-18)
  - [Scope](#scope)
  - [Verified Path](#verified-path)
  - [Commands](#commands)
  - [Result](#result)
  - [Notes](#notes)

<!-- T81-TOC:END -->


This note records the first family-specific native execution evidence for
RFC-00BB.

## Scope

- Source model family: `llama`
- Real artifact: `models/tinyllama-1.1b.Q2_K.gguf`
- Import path: RFC-00BA llama.cpp GGUF bridge
- Native profile: `llama-dense-v1`

## Verified Path

The in-repo `t81_gguf_import_bridge_test` now proves the following end-to-end
sequence on a real TinyLlama tensor:

1. import the real TinyLlama GGUF through `load_gguf(...)`
2. save the native model to `.t81w` via `save_t81w(...)`
3. reload the `.t81w` via `load_t81w(...)`
4. execute a VM program using `WeightsLoad` + `TWEMBED` against the reloaded
   native artifact
5. verify the resulting gathered row against the packed native tensor content

The execution proof uses the real family tensor `blk.0.attn_q.weight`, not a
synthetic fixture.

## Commands

```sh
cmake --build build-llama --target t81_gguf_import_bridge_test -j4
ctest --test-dir build-llama -R 't81_gguf_import_bridge_test' --output-on-failure
```

## Result

- `t81_gguf_import_bridge_test` passed
- Real-family native execution evidence now exists for at least one promoted
  architecture profile, satisfying RFC-00BB's `accepted` threshold

## Notes

- The verified execution path intentionally uses a real TinyLlama projection
  tensor (`blk.0.attn_q.weight`) rather than `token_embd.weight`, because the
  vocabulary embedding row width exceeds the current Tier-5 shape-complexity
  ceiling for `TWEMBED` result allocation in the VM.
- This evidence closes the RFC-00BB transition from `proposed` to `accepted`,
  but does not by itself promote every listed family from
  `experimental_native` to `native_supported`.
