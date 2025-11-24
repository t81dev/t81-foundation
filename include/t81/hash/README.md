# t81/hash — Base-81 & CanonHash stubs

This directory provides a stable API surface for hashing/encoding so other
modules can compile before the real codec is wired.

## Files

- `base81.hpp`

  - `encode_base81(bytes) -> std::string`
  - `decode_base81(string) -> std::vector<uint8_t>`
  - **Stub:** currently emits/accepts a `"b81:"` + hex fallback for determinism.

- `canonhash.hpp`

  - `make_canonhash81_base81stub(void* data, size_t len) -> CanonHash81`
  - **Stub:** encodes bytes via the Base-81 stub and truncates/pads to 81 bytes.
    Replace with a real digest path (e.g., BLAKE3 -> Base-81) in production.

## Migration Notes

1. Keep the function signatures stable.
2. When the real codec lands:
   - Swap `encode_base81`/`decode_base81` with canonical Base-81.
   - Update `make_canonhash81_base81stub` to compute a digest before encoding.
3. `CanonHash81.text` remains a fixed 81-byte buffer (zero-padded).

## Safety

These stubs are **non-cryptographic** and intended only for testing and wiring.
Do not use in production or for security-sensitive features.
