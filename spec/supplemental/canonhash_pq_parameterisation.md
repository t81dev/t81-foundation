# Post-Quantum CanonHash Parameterisation

**Status:** Stable
**Published:** 2026-03-08

This document defines the normative parameterisation for CanonHash-81 to achieve post-quantum resistance, fulfilling the requirement established in RFC-0000.

## 1. Objective

CanonHash-81 currently relies on SHA3-512 truncated to 256 bits for its canonical identifier generation. While SHA3-512 provides significant resistance against classical attacks, the advent of sufficiently large quantum computers could weaken 256-bit collision resistance via Grover's algorithm. To maintain long-term security properties (e.g., AGI containment and non-self-modification guarantees), CanonHash-81 must define a post-quantum parameterisation.

## 2. Parameterisation Strategy

To provide 256-bit post-quantum collision resistance, the digest size must be expanded. The parameterisation defined here utilizes a 512-bit state size to ensure that even under Grover's algorithm, the effective collision resistance remains $\ge 256$ bits.

The post-quantum version of CanonHash-81 (denoted **CanonHash-81-PQ**) uses the full 512-bit output of the SHA3-512 permutation without truncation.

## 3. Encoding

The canonical Base-81 encoding scheme must be extended to accommodate a 512-bit (64-byte) digest.
Since $\log_2(81) \approx 6.33985$ bits per Base-81 character, a 512-bit digest requires:
$$ \lceil 512 / 6.33985 \rceil = 81 \text{ characters} $$

Thus, **CanonHash-81-PQ** produces exactly an 81-character Base-81 string. This aligns perfectly with the Base-81 architecture of the T81 foundation (as 81 is $3^4$).

## 4. Implementation Details

1.  **Digest Generation:** The hash function remains SHA3-512. The `t81::crypto::sha3_512` function is used.
2.  **Output Size:** The output array `std::array<std::uint8_t, 64>` is used entirely.
3.  **Base-81 Conversion:** The 64 bytes are encoded using the standard `t81::hash::encode_base81` function, yielding an 81-character string.

This parameterisation requires no novel cryptographic primitives and relies on the standardized SHA3 family, fulfilling the post-quantum parameterisation requirement of RFC-0000 while remaining compatible with the T81 execution environment.
