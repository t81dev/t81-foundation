# t81/codec — Base-243 Codec Surface

A small, stable API for encoding/decoding **base-243** digit vectors. The current
implementation performs canonical base-256 ↔ base-243 conversion (big-endian),
plus a textual codec for `T81BigInt`.

## Files

- `base243.hpp`
  - `Base243::encode_bytes_be(std::vector<uint8_t>) -> std::vector<digit_t>`
  - `Base243::decode_bytes_be(std::vector<digit_t>) -> std::vector<uint8_t>`
  - `Base243::encode_ascii(std::string)`
  - `Base243::decode_ascii(std::vector<digit_t>)`
  - `Base243::encode_bigint(const T81BigInt&) -> std::string`
  - `Base243::decode_bigint(std::string_view, T81BigInt&) -> bool`
- `trit_packing.hpp` (New)
  - Provides fixed-width packing for **PT-5** and **Base-81**.
  - Safe conversion between canonical PT-5 and symbolic Base-81 digits/strings.

## Notes

- `digit_t` is `uint8_t` with range **[0..242]**; digits are MSB-first.
- Bytes are interpreted as a big-endian base-256 integer; digits are canonical
  base-243 representation.
- Bigints are rendered as dot-separated base-243 digits with optional leading
  `-`; parsing rejects out-of-range digits and malformed input.

## Fixed-Width Packing (PT-5 & Base-81)

The `trit_packing.hpp` module provides deterministic, fixed-width packing of trits into bytes:

1.  **PT-5 (Canonical):** Packs **5 trits** into 1 byte (value range 0..242).
    - Authoritative on-disk and in-memory representation.
    - Used for hashing and stable storage.
2.  **Base-81 (Symbolic):** Packs **4 trits** into 1 digit (value range 0..80).
    - Used for I/O, human-readable strings, and tooling.
    - Interoperable with PT-5 via direct conversion layer.

### Padding and Termination

- All packing and conversion operations are governed by an authoritative **trit_count**.
- If a sequence of trits does not fill a whole byte (PT-5) or digit (Base-81), it is padded with **zero-trits**.
- Metadata (trit_count) MUST be preserved alongside the packed bytes to ensure correct unpacking.

### Provenance Rule

- **Hashing:** Hashing and canonical identification are performed over the authoritative **PT-5 bytes** and associated metadata.
- **Base-81 Serialization:** Base-81 digits or strings are strictly for I/O and symbolic representation; they MUST NOT be used as the primary hash source.
- **Interoperability:** Base-81 representations must always be capable of round-tripping back to bit-identical PT-5 bytes given the same trit count.

## Example

```cpp
#include <t81/codec/base243.hpp>
using namespace t81::codec;

std::vector<uint8_t> bytes = {0x01, 0xFE, 0xA5};
auto digits = Base243::encode_bytes_be(bytes);
auto round  = Base243::decode_bytes_be(digits);
```
