"""Independent mathematical byte-count and structure oracle for EXP-H04-R03.

Defines exact analytical byte counts, block strides, alignment padding,
and index sizes according to experiments/h04_r03_matched_codec/SPECIFICATION.md.
"""
from typing import Dict, Any


def align_up(val: int, a: int) -> int:
    """Align val up to the nearest multiple of a."""
    if a <= 0:
        raise ValueError(f"Alignment must be positive, got {a}")
    return ((val + a - 1) // a) * a


def b_B729(m: int) -> int:
    """Bit length for m symbols in 729-state domain: bit_length(729^m - 1)."""
    if m <= 0:
        return 0
    return (729**m - 1).bit_length()


def L_B10(m: int) -> int:
    """Payload bytes for m symbols under B10: ceil(10*m / 8)."""
    if m <= 0:
        return 0
    return (10 * m + 7) // 8


def L_T243(m: int) -> int:
    """Payload bytes for m symbols under T243 / B243-CLONE: ceil(6*m / 5)."""
    if m <= 0:
        return 0
    return (6 * m + 4) // 5


def L_B729(m: int) -> int:
    """Payload bytes for m symbols under B729: ceil(b_B729(m) / 8)."""
    if m <= 0:
        return 0
    return (b_B729(m) + 7) // 8


def compute_layout(N: int, k: int, A: int, access_mode: str, arm: str) -> Dict[str, Any]:
    """Compute exact canonical layout breakdown.

    Args:
        N: number of logical symbols in [-364, 364].
        k: block size (k in {1, 5, 64, 1024}).
        A: alignment in bytes (A in {1, 8, 64}).
        access_mode: 'RA-FIXED' or 'RA-INDEXED'.
        arm: 'B10', 'T243', 'B243-CLONE', or 'B729'.
    """
    if access_mode not in ("RA-FIXED", "RA-INDEXED"):
        raise ValueError(f"Unknown access_mode: {access_mode}")
    if arm not in ("B10", "T243", "B243-CLONE", "B729"):
        raise ValueError(f"Unknown arm: {arm}")
    if k <= 0 or A <= 0 or N < 0:
        raise ValueError("Invalid parameters")

    if N == 0:
        return {
            "N": 0,
            "k": k,
            "A": A,
            "access_mode": access_mode,
            "arm": arm,
            "blocks": 0,
            "header_bytes": 8,
            "index_bytes": 0,
            "data_offset": 8,
            "payload_bytes": 0,
            "alignment_bytes": 0,
            "total_bytes": 8,
            "block_offsets": [],
        }

    B = (N + k - 1) // k
    header_bytes = 8
    index_bytes = 8 * B if access_mode == "RA-INDEXED" else 0
    H = header_bytes + index_bytes
    D = align_up(H, A)

    payload_func = {
        "B10": L_B10,
        "T243": L_T243,
        "B243-CLONE": L_T243,
        "B729": L_B729,
    }[arm]

    payload_bytes = 0
    block_offsets = []
    curr_offset = D

    for j in range(B):
        m_j = min(k, N - j * k)
        block_offsets.append(curr_offset)
        raw_l = payload_func(m_j)
        aligned_l = align_up(raw_l, A)
        payload_bytes += raw_l
        curr_offset += aligned_l

    total_bytes = curr_offset
    alignment_bytes = (D - H) + (total_bytes - D - payload_bytes)

    return {
        "N": N,
        "k": k,
        "A": A,
        "access_mode": access_mode,
        "arm": arm,
        "blocks": B,
        "header_bytes": header_bytes,
        "index_bytes": index_bytes,
        "data_offset": D,
        "payload_bytes": payload_bytes,
        "alignment_bytes": alignment_bytes,
        "total_bytes": total_bytes,
        "block_offsets": block_offsets,
    }


def verify_size_invariants(N: int, k: int, A: int, access_mode: str = "RA-FIXED") -> None:
    """Verify analytical theorem: L_B729(m) <= L_T243(m) for all blocks."""
    layout_t243 = compute_layout(N, k, A, access_mode, "T243")
    layout_b243 = compute_layout(N, k, A, access_mode, "B243-CLONE")
    layout_b729 = compute_layout(N, k, A, access_mode, "B729")
    layout_b10 = compute_layout(N, k, A, access_mode, "B10")

    assert layout_t243["total_bytes"] == layout_b243["total_bytes"], "Clone total bytes must match T243"
    assert layout_b729["payload_bytes"] <= layout_t243["payload_bytes"], "B729 payload must be <= T243"
    assert layout_b729["total_bytes"] <= layout_t243["total_bytes"], "B729 total must be <= T243"
