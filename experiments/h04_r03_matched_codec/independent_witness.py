"""Independent reference decoders and known-answer vectors (KAV) for EXP-H04-R03.

Written completely independently of codec.py to serve as an authoritative
correctness witness across all four arms (B10, T243, B243-CLONE, B729).
"""
import struct
from typing import List, Tuple

# Exact Known-Answer Vectors for a canonical sequence of values:
# values: [-364, 0, 364]
# mapped u: [0, 364, 728]
# k=5, A=8, access_mode="RA-FIXED"
# Layout derivation:
# N=3 -> B=1 block. H=8 bytes header, A=8 -> D=8.
# m_1 = 3 values.
#
# B10 payload for 3 values:
# u_0 = 0 (10 bits: 0000000000)
# u_1 = 364 (10 bits: 0101101100 -> bit0..9 = 0011011010)
# u_2 = 728 (10 bits: 1011011000 -> bit0..9 = 0001101101)
# Total bits = 30 bits -> 4 bytes (L_B10(3) = 4 bytes).
# Let's derive exact byte values for B10:
# Bit layout:
# acc = (u_0) | (u_1 << 10) | (u_2 << 20)
# u_0 = 0, u_1 = 364 (0x16C), u_2 = 728 (0x2D8)
# acc = 0 | (364 << 10) | (728 << 20) = 372736 | 763363328 = 763736064 (0x2D85B000)
# Little-endian bytes:
# Byte 0: 0x00
# Byte 1: 0xB0
# Byte 2: 0x85
# Byte 3: 0x2D
# Aligned to A=8: 4 bytes payload + 4 bytes alignment padding = 8 bytes.
# Total blob = 8 (header) + 8 = 16 bytes.
KAV_VALUES_3 = [-364, 0, 364]

KAV_EXPECTED_B10_BLOB = (
    struct.pack('<Q', 3) +
    bytes([0x00, 0xB0, 0x85, 0x2D, 0x00, 0x00, 0x00, 0x00])
)

# T243 / B243-CLONE payload for 3 values:
# u_0 = 0 -> six trits: [0, 0, 0, 0, 0, 0]
# u_1 = 364 -> ordinary base-3 digits:
#   364 / 3 = 121 r 1
#   121 / 3 = 40 r 1
#   40 / 3 = 13 r 1
#   13 / 3 = 4 r 1
#   4 / 3 = 1 r 1
#   1 / 3 = 0 r 1
#   -> six trits: [1, 1, 1, 1, 1, 1]
# u_2 = 728 -> 728 = 2*364 -> ordinary base-3 digits:
#   728 / 3 = 242 r 2
#   242 / 3 = 80 r 2
#   80 / 3 = 26 r 2
#   26 / 3 = 8 r 2
#   8 / 3 = 2 r 2
#   2 / 3 = 0 r 2
#   -> six trits: [2, 2, 2, 2, 2, 2]
# Concatenated 18 trits:
# [0, 0, 0, 0, 0,  0, 1, 1, 1, 1,  1, 1, 2, 2, 2,  2, 2, 2]
# Group 0 (5 trits): [0, 0, 0, 0, 0] -> sum = 0 -> byte 0x00
# Group 1 (5 trits): [0, 1, 1, 1, 1] -> 0*1 + 1*3 + 1*9 + 1*27 + 1*81 = 120 -> byte 0x78
# Group 2 (5 trits): [1, 1, 2, 2, 2] -> 1*1 + 1*3 + 2*9 + 2*27 + 2*81 = 1 + 3 + 18 + 54 + 162 = 238 -> byte 0xEE
# Group 3 (3 trits + 2 zero pad): [2, 2, 2, 0, 0] -> 2*1 + 2*3 + 2*9 = 2 + 6 + 18 = 26 -> byte 0x1A
# Raw payload: [0x00, 0x78, 0xEE, 0x1A] (4 bytes = L_T243(3)).
# Aligned to A=8: 4 bytes payload + 4 bytes alignment padding = 8 bytes.
KAV_EXPECTED_T243_BLOB = (
    struct.pack('<Q', 3) +
    bytes([0x00, 0x78, 0xEE, 0x1A, 0x00, 0x00, 0x00, 0x00])
)

# B729 payload for 3 values:
# I = u_0 + u_1 * 729 + u_2 * 729^2
#   = 0 + 364 * 729 + 728 * 531441
#   = 265356 + 386889048 = 387154404
# b_B729(3) = bit_length(729^3 - 1) = bit_length(387420488) = 29 bits.
# L_B729(3) = ceil(29/8) = 4 bytes.
# 387154404 in hex:
# 387154404 = 0x171381E4
# Little-endian bytes: [0xE4, 0x81, 0x13, 0x17]
# Aligned to A=8: 4 bytes payload + 4 bytes alignment padding = 8 bytes.
KAV_EXPECTED_B729_BLOB = (
    struct.pack('<Q', 3) +
    bytes([0xE4, 0x81, 0x13, 0x17, 0x00, 0x00, 0x00, 0x00])
)


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
