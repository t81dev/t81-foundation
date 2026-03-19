# RFC-00BA GGUF Bridge Evidence Snapshot

Date: 2026-03-18
Host: macOS ARM64
Build: `Release`, `-DT81_ENABLE_LLAMA_CPP=ON`

## Verified

- `./build-llama/t81_gguf_import_bridge_test`
- `ctest --test-dir build-llama -R 't81_gguf_import_bridge_test' --output-on-failure`
- `./build-llama/t81 weights import tinyllama-1.1b.Q2_K --format gguf -o /tmp/t81-tinyllama-provenance.t81w`

## Observed

- Bridge-backed GGUF import succeeds for the checked-in TinyLlama model.
- `weights import` reports bridge provenance on the import surface:
  - `Format: GGUF(llama.cpp bridge; arch=llama; profile=llama-dense-v1)`
  - `Source SHA3: sha3-512:...`
  - `Bridge Rev: llama.cpp-bridge-v1`
- The resulting native artifact is written successfully as `.t81w`.

## Conclusion

RFC-00BA acceptance criteria are met in-repo:

- narrow internal bridge exists
- build gating is enforced
- metadata enumeration and float export are tested
- bridge-backed `weights import --format gguf` converts a standard non-`T3_K`
  GGUF model into native `.t81w`
- provenance output records the source GGUF and bridge revision
- llama.cpp coupling remains localized to the bridge layer
