#!/usr/bin/env python3
"""Independent reference implementation and verification script for PCG64 PRNG.

Verifies Protocol EXP-T81-COMP3-01 Section 6 PCG64 outputs.
"""

import sys

def verify_pcg64():
    mult = (0x2360ED051FC65DA4 << 64) | 0x4385C511560B53A9
    inc = 3
    seed = (0x5438315F434D5033 << 64) | 0x5F53454544303131

    # State initialization procedure
    state = 0
    state = (state * mult + inc) & ((1 << 128) - 1)
    state = (state + seed) & ((1 << 128) - 1)
    state = (state * mult + inc) & ((1 << 128) - 1)

    expected = [
        0x854CAFBD0A540778,
        0x3D174C3776108E7D,
        0xA8398F10D0F2976C,
        0x722D0D72D6D3EA65
    ]

    for i, exp in enumerate(expected):
        old = state
        state = (old * mult + inc) & ((1 << 128) - 1)
        x = ((old >> 64) ^ old) & ((1 << 64) - 1)
        rot = old >> 122
        output = ((x >> rot) | (x << ((-rot) & 63))) & ((1 << 64) - 1)

        print(f"Output {i}: 0x{output:016X} (Expected: 0x{exp:016X})")
        assert output == exp, f"Mismatch at output {i}: got 0x{output:016X}, expected 0x{exp:016X}"

    print("PCG64_INDEPENDENT_VERIFICATION_SUCCESS")

if __name__ == "__main__":
    verify_pcg64()
