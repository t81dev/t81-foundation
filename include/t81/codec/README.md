# t81/codec — Base-243 Codec Surface

A small, stable API for encoding/decoding **base-243** digit vectors. The current
implementation performs canonical base-256 ↔ base-243 conversion (big-endian),
plus a textual codec for `T243BigInt`.

## Files
- `base243.hpp`
  - `Base243::encode_bytes_be(std::vector<uint8_t>) -> std::vector<digit_t>`
  - `Base243::decode_bytes_be(std::vector<digit_t>) -> std::vector<uint8_t>`
  - `Base243::encode_ascii(std::string)`
  - `Base243::decode_ascii(std::vector<digit_t>)`
  - `Base243::encode_bigint(const T243BigInt&) -> std::string`
  - `Base243::decode_bigint(std::string_view, T243BigInt&) -> bool`

## Notes
- `digit_t` is `uint8_t` with range **[0..242]**; digits are MSB-first.
- Bytes are interpreted as a big-endian base-256 integer; digits are canonical
  base-243 representation.
- Bigints are rendered as dot-separated base-243 digits with optional leading
  `-`; parsing rejects out-of-range digits and malformed input.

## Example
```cpp
#include <t81/codec/base243.hpp>
using namespace t81::codec;

std::vector<uint8_t> bytes = {0x01, 0xFE, 0xA5};
auto digits = Base243::encode_bytes_be(bytes);
auto round  = Base243::decode_bytes_be(digits);
