# CanonHash-81 Formal Test Vectors

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [CanonHash-81 Formal Test Vectors](#canonhash-81-formal-test-vectors)
  - [Base-81 Vectors](#base-81-vectors)
  - [Test Execution](#test-execution)

<!-- T81-TOC:END -->


This document contains the official Reference Vectors for `CanonHash-81`, the 256-bit truncated SHA3-512 digest structurally encoded into the 41-43 character Base-81 alphabet natively used by the T81 Architecture.

These vectors allow downstream interoperability implementers to formally verify their CanonHash-81 algorithms against expected exact match outputs for deterministic parsing.

## Base-81 Vectors

- **Empty String**:
  - `3aY2<G>−fI∞Y5m0oL3π7τoqwπ0iyh>ocΓk1π1wpq7`

- **Single Null Byte (0x00)**:
  - `2RzMoFItZ6τNgKqD6XyYI63H5i83≠cπ8≈7cΓZbpwc`

- **Single Byte (0x01)**:
  - `45VF−QZBFpWQ−Cv<UCYIsWi2e6DuCA7ZKII20xz×s`

- **String 'hello'**:
  - `2Zms9p3oVτ≠YQμi≠Y∞eWbh6∞DzZe÷QcPj>>-Hμ4Rs`

- **String 'world'**:
  - `2NzLSbmL5P−xLlVp×π4G<9d0ωbτ3yVλ2≤yμμλr3>Y`

- **String 'T81CanonHash'**:
  - `w≥T∞4−aωμJμσHfDJOA×0≈Oo8snIg∞VksπFω-F4∞b`

- **CanonBlock All-Zeros (729 bytes)**:
  - `3>i≥≈zμU7z9sKIXgmfJ1Eog9L0uhgτlGVRYK∞λbΓr`

- **CanonBlock All-Ones (729 bytes 0x01)**:
  - `4πFk−U61k5P−3noQB<CNzB0fJu6≥NσP29Uωy8T6wx`

- **String 'RFC-0000-reference-vector'**:
  - `2RTmsHJX8Sbm≠rvPsCYZnMΓDcEO≈T-cK<÷IμB6d8I`

## Test Execution
You can identically reproduce these exact vectors on your target CPU architecture using the `t81_canonhash81_reference_vectors_test` executable.
