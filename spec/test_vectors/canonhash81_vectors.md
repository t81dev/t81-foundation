# CanonHash-81 Reference Test Vectors

**Status:** Stable
**Published:** 2026-03-08

This document establishes the normative reference test vectors for CanonHash-81 as required by RFC-0000. All compliant implementations MUST produce these exact Base-81 string digests for the given inputs.

## Test Vectors

| Vector ID | Input Description | Base-81 Digest Output |
| :--- | :--- | :--- |
| `V01` | Empty string (`""`) | `3aY2<G>−fI∞Y5m0oL3π7τoqwπ0iyh>ocΓk1π1wpq7` |
| `V02_00` | Single byte: `0x00` | `2RzMoFItZ6τNgKqD6XyYI63H5i83≠cπ8≈7cΓZbpwc` |
| `V02_01` | Single byte: `0x01` | `45VF−QZBFpWQ−Cv<UCYIsWi2e6DuCA7ZKII20xz×s` |
| `V03_hello` | ASCII string: `"hello"` | `2Zms9p3oVτ≠YQμi≠Y∞eWbh6∞DzZe÷QcPj>>-Hμ4Rs` |
| `V03_world` | ASCII string: `"world"` | `2NzLSbmL5P−xLlVp×π4G<9d0ωbτ3yVλ2≤yμμλr3>Y` |
| `V03_t81` | ASCII string: `"T81CanonHash"` | `w≥T∞4−aωμJμσHfDJOA×0≈Oo8snIg∞VksπFω-F4∞b` |
| `V04` | `CanonBlock` (all zeros, 729 bytes of `0x00`) | `3>i≥≈zμU7z9sKIXgmfJ1Eog9L0uhgτlGVRYK∞λbΓr` |
| `V05` | `CanonBlock` (all ones, 729 bytes of `0x01`) | `4πFk−U61k5P−3noQB<CNzB0fJu6≥NσP29Uωy8T6wx` |
| `V06` | `CanonBlock` (roundtrip pattern: `i % 81` for `i` in `0..728`) | `22vH<B×kqhRE98IO≠HDPGdLσ7iBnHMμ≤sCLλIdv2σ` |
| `V08` | ASCII string: `"RFC-0000-reference-vector"` | `2RTmsHJX8Sbm≠rvPsCYZnMΓDcEO≈T-cK<÷IμB6d8I` |

## Reproduction

These vectors correspond directly to the test cases in `tests/cpp/canonhash81_reference_vectors_test.cpp`.

To verify locally:
```bash
ctest --test-dir build -R canonhash81_reference_vectors_test
```
