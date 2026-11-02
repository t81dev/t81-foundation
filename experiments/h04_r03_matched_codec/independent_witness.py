"""Independent reference decoders and known-answer vectors (KAV) for EXP-H04-R03.

Written completely independently of codec.py to serve as an authoritative
correctness witness across all four arms (B10, T243, B243-CLONE, B729).
Includes asymmetric and non-palindromic six-trit patterns to strictly catch
endianness or digit-order reversals.
"""
import struct
from typing import List

# -----------------------------------------------------------------------------
# Asymmetric / Non-Palindromic Known-Answer Vector 1:
# Single asymmetric value: x = -363 -> u = 1.
# Six base-3 digits (least significant first): [1, 0, 0, 0, 0, 0].
# Palindromic reverse would be: [0, 0, 0, 0, 0, 1] -> u = 243 -> x = -121.
# k=1, A=1, RA-FIXED.
# N=1 -> B=1. Header=8 bytes. A=1 -> D=8.
# B10 payload (1 value): 10 bits -> 2 bytes. LSB first: u=1 -> bytes: [0x01, 0x00].
# Total blob = 8 + 2 = 10 bytes.
# -----------------------------------------------------------------------------
KAV_ASYMMETRIC_VAL_1 = [-363]

KAV_ASYMMETRIC_B10_BLOB = (
    struct.pack('<Q', 1) +
    bytes([0x01, 0x00])
)

# T243 payload (1 value = 6 trits):
# Group 0 (first 5 trits): [1, 0, 0, 0, 0] -> 1*1 = 1 -> byte 0x01
# Group 1 (6th trit + 4 zero pads): [0, 0, 0, 0, 0] -> 0 -> byte 0x00
# Total bytes = 2 bytes (L_T243(1) = 2).
KAV_ASYMMETRIC_T243_BLOB = (
    struct.pack('<Q', 1) +
    bytes([0x01, 0x00])
)

# B729 payload (1 value): I = 1.
# b_B729(1) = bit_length(729 - 1) = 10 bits -> 2 bytes.
# Bytes: [0x01, 0x00].
KAV_ASYMMETRIC_B729_BLOB = (
    struct.pack('<Q', 1) +
    bytes([0x01, 0x00])
)

# -----------------------------------------------------------------------------
# Asymmetric / Multi-Symbol Known-Answer Vector 2:
# values: [-364, 0, 364] -> u: [0, 364, 728].
# k=5, A=8, access_mode="RA-FIXED"
# -----------------------------------------------------------------------------
KAV_VALUES_3 = [-364, 0, 364]

KAV_EXPECTED_B10_BLOB = (
    struct.pack('<Q', 3) +
    bytes([0x00, 0xB0, 0x85, 0x2D, 0x00, 0x00, 0x00, 0x00])
)

KAV_EXPECTED_T243_BLOB = (
    struct.pack('<Q', 3) +
    bytes([0x00, 0x78, 0xEE, 0x1A, 0x00, 0x00, 0x00, 0x00])
)

KAV_EXPECTED_B729_BLOB = (
    struct.pack('<Q', 3) +
    bytes([0xE4, 0x81, 0x13, 0x17, 0x00, 0x00, 0x00, 0x00])
)


# -----------------------------------------------------------------------------
# Asymmetric / Multi-Symbol Known-Answer Vector 3:
# values: [-363, 10] -> u: [1, 374].
# u_0 = 1 -> digits: [1, 0, 0, 0, 0, 0]
# u_1 = 374 -> 374 = 2*1 + 1*3 + 2*9 + 1*27 + 1*81 + 1*243 -> digits: [2, 1, 2, 1, 1, 1]
# k=5, A=1, RA-FIXED.
# N=2 -> B=1. Header=8 bytes. A=1 -> D=8.
# B10 payload (2 values = 20 bits -> 3 bytes):
# acc = 1 | (374 << 10) = 1 | 382976 = 382977 (0x5D801)
# Little-endian bytes: [0x01, 0xD8, 0x05]
# T243 payload (2 values = 12 trits -> 3 bytes):
# Trits: [1, 0, 0, 0, 0,  0, 2, 1, 1, 1,  1, 1, 0, 0, 0]
# Group 0: [1, 0, 0, 0, 0] -> 1 -> byte 0x01
# Group 1: [0, 2, 1, 2, 1] -> 0 + 2*3 + 1*9 + 2*27 + 1*81 = 6 + 9 + 54 + 81 = 150 -> byte 0x96
# Group 2: [1, 1, 0, 0, 0] -> 1 + 1*3 = 4 -> byte 0x04
# Bytes: [0x01, 0x96, 0x04]
# B729 payload: I = 1 + 374 * 729 = 1 + 272646 = 272647 (0x42907).
# b_B729(2) = bit_length(729^2 - 1) = bit_length(531440) = 20 bits -> 3 bytes.
# Bytes: [0x07, 0x29, 0x04]
# -----------------------------------------------------------------------------
KAV_ASYMMETRIC_VALS_2 = [-363, 10]

KAV_ASYMMETRIC_2_B10_BLOB = (
    struct.pack('<Q', 2) +
    bytes([0x01, 0xD8, 0x05])
)

KAV_ASYMMETRIC_2_T243_BLOB = (
    struct.pack('<Q', 2) +
    bytes([0x01, 0x96, 0x04])
)

KAV_ASYMMETRIC_2_B729_BLOB = (
    struct.pack('<Q', 2) +
    bytes([0x07, 0x29, 0x04])
)


# -----------------------------------------------------------------------------
# Standalone Independent Decoders
# -----------------------------------------------------------------------------

def independent_decode_raw_b10(payload: bytes, m: int) -> List[int]:
    """Independent decoder for B10 payload bytes."""
    bits_str = ""
    for b in payload:
        bits_str += bin(b)[2:].zfill(8)[::-1]  # LSB first
    vals = []
    for i in range(m):
        field_bits = bits_str[i * 10 : (i + 1) * 10]
        u = int(field_bits[::-1], 2)
        if u >= 729:
            raise ValueError(f"Invalid B10 value {u}")
        vals.append(u - 364)
    return vals


def independent_decode_raw_t243(payload: bytes, m: int) -> List[int]:
    """Independent decoder for T243 payload bytes."""
    trits = []
    for b in payload:
        if b >= 243:
            raise ValueError("Invalid Base-243 byte")
        curr = b
        for _ in range(5):
            curr, d = divmod(curr, 3)
            trits.append(d)
    vals = []
    for i in range(m):
        sym_trits = trits[i * 6 : (i + 1) * 6]
        u = sum(d * (3**p) for p, d in enumerate(sym_trits))
        vals.append(u - 364)
    return vals


def independent_decode_raw_b729(payload: bytes, m: int) -> List[int]:
    """Independent decoder for B729 payload bytes."""
    if m == 0:
        return []
    I = int.from_bytes(payload, byteorder='little')
    vals = []
    for _ in range(m):
        I, u = divmod(I, 729)
        vals.append(u - 364)
    return vals
