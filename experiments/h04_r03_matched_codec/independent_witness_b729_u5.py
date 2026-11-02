"""Independent reference decoder and known-answer vectors (KAV) for B729-µ5.

Written completely independently of experiments/h04_r03_matched_codec/codec.py
to serve as an authoritative correctness witness for Arm 4 (B729-µ5).
Enforces exact little-endian deserialization, bit-length limits, unused high padding bits,
and range constraints.
"""
import struct
from typing import List

# -----------------------------------------------------------------------------
# Microblock bit and byte length specifications (Section 4.3):
# v=1: 10 bits -> 2 bytes (unused high bits: 16 - 10 = 6)
# v=2: 20 bits -> 3 bytes (unused high bits: 24 - 20 = 4)
# v=3: 29 bits -> 4 bytes (unused high bits: 32 - 29 = 3)
# v=4: 39 bits -> 5 bytes (unused high bits: 40 - 39 = 1)
# v=5: 48 bits -> 6 bytes (unused high bits: 48 - 48 = 0)
# -----------------------------------------------------------------------------
U5_BIT_LENGTHS = {
    1: 10,
    2: 20,
    3: 29,
    4: 39,
    5: 48,
}

U5_BYTE_LENGTHS = {
    1: 2,
    2: 3,
    3: 4,
    4: 5,
    5: 6,
}

# -----------------------------------------------------------------------------
# Hard-coded Known-Answer Vectors for B729-µ5
# -----------------------------------------------------------------------------

# KAV 1: Single asymmetric value [-363] -> u = 1.
# N=1, k=1, A=1, RA-FIXED.
# Header: 8 bytes (<Q 1).
# Microblock v=1: I_mu = 1.
# Byte length = 2 bytes: [0x01, 0x00].
# Total: 10 bytes.
KAV_U5_ASYMMETRIC_VAL_1 = [-363]
KAV_U5_ASYMMETRIC_BLOB_1 = struct.pack('<Q', 1) + bytes([0x01, 0x00])

# KAV 2: Two asymmetric values [-363, 10] -> u: [1, 374].
# N=2, k=5, A=1, RA-FIXED.
# Header: 8 bytes (<Q 2).
# Microblock v=2: I_mu = 1 + 374 * 729 = 272647 (0x42907).
# Byte length = 3 bytes: [0x07, 0x29, 0x04].
# Total: 11 bytes.
KAV_U5_ASYMMETRIC_VALS_2 = [-363, 10]
KAV_U5_ASYMMETRIC_BLOB_2 = struct.pack('<Q', 2) + bytes([0x07, 0x29, 0x04])

# KAV 3: Three values [-364, 0, 364] -> u: [0, 364, 728].
# N=3, k=5, A=8, RA-FIXED.
# Header: 8 bytes (<Q 3).
# Microblock v=3: I_mu = 0 + 364 * 729 + 728 * (729^2) = 265356 + 386889048 = 387154404 (0x171381E4).
# Byte length = 4 bytes: [0xE4, 0x81, 0x13, 0x17].
# Alignment A=8: payload 4 bytes padded with 4 zero bytes to 8 bytes.
# Total: 16 bytes.
KAV_U5_VALUES_3 = [-364, 0, 364]
KAV_U5_BLOB_3 = struct.pack('<Q', 3) + bytes([0xE4, 0x81, 0x13, 0x17, 0x00, 0x00, 0x00, 0x00])

# KAV 4: Exactly 5 values spanning full microblock: [-364, -182, 0, 182, 364]
# u = [0, 182, 364, 546, 728].
# I_mu = 0 + 182 * 729 + 364 * 729^2 + 546 * 729^3 + 728 * 729^4
# 182*729 = 132678
# 364*531441 = 193444524
# 546*387420489 = 211531586994
# 728*282429536481 = 205608702558168
# Total I_mu = 205820427722364 (0xBB314D7B5E7C).
# 6 bytes little-endian: [0x7C, 0x5E, 0x7B, 0x4D, 0x31, 0xBB].
# k=5, A=1, RA-FIXED.
# Total blob = 8 + 6 = 14 bytes.
KAV_U5_VALUES_5 = [-364, -182, 0, 182, 364]
KAV_U5_BLOB_5 = struct.pack('<Q', 5) + bytes([0x7C, 0x5E, 0x7B, 0x4D, 0x31, 0xBB])


def independent_decode_raw_b729_u5(payload: bytes, m: int) -> List[int]:
    """Independent decoder for raw B729-µ5 payload bytes without shared codec logic.

    Validates:
    - Microblock partitioning
    - Little-endian integer reconstruction
    - High unused padding bits are strictly zero
    - Microblock integer range bound I_mu < 729^v
    """
    if m == 0:
        if len(payload) != 0:
            raise ValueError(f"Extra payload bytes for m=0: {len(payload)}")
        return []

    values: List[int] = []
    num_microblocks = (m + 4) // 5
    offset = 0

    for g in range(num_microblocks):
        vg = min(5, m - 5 * g)
        l_bytes = U5_BYTE_LENGTHS[vg]
        b_bits = U5_BIT_LENGTHS[vg]

        if offset + l_bytes > len(payload):
            raise ValueError(f"Truncated payload in microblock {g}: need {l_bytes} bytes, remaining {len(payload) - offset}")

        chunk = payload[offset : offset + l_bytes]
        offset += l_bytes

        # Independent manual integer assembly (little-endian)
        I_mu = 0
        for i, byte_val in enumerate(chunk):
            I_mu += byte_val * (256 ** i)

        # Validate unused high bits
        unused_bits = (l_bytes * 8) - b_bits
        if unused_bits > 0:
            high_mask = ((1 << unused_bits) - 1) << b_bits
            if (I_mu & high_mask) != 0:
                raise ValueError(f"Nonzero unused high bits in microblock {g}: value={I_mu:#x}")

        # Validate range bound
        limit = 729 ** vg
        if I_mu >= limit:
            raise ValueError(f"Integer overflow in microblock {g}: {I_mu} >= {limit}")

        # Extract values
        curr = I_mu
        for _ in range(vg):
            curr, u = divmod(curr, 729)
            values.append(u - 364)

    if offset != len(payload):
        raise ValueError(f"Trailing unexpected bytes in payload: parsed {offset}, got {len(payload)}")

    return values
