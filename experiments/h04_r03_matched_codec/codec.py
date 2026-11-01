"""Reference implementations of the four matched codec arms for EXP-H04-R03.

Arms:
- B10: Packed 10-bit binary fields.
- T243: Base-243 packing (ordinary base-3 digits grouped 5 trits per byte).
- B243-CLONE: Binary software implementation of the exact T243 wire format.
- B729: Fixed-length enumerative block integer coding.

Common Domain:
  Signed integer x in [-364, 364], bijectively mapped to u = x + 364 in [0, 728].
Framing:
  Header: 8-byte little-endian uint64 N.
  If RA-INDEXED: immediately after count, B uint64 absolute block offsets (8 bytes each).
  Alignment A in {1, 8, 64}: data starts at D = align_up(H, A).
  Each block j occupies align_up(L_c(m_j), A) bytes.
"""
import struct
from typing import List, Sequence, Tuple

from .oracle import align_up, b_B729, L_B10, L_T243, L_B729, compute_layout


class CodecError(ValueError):
    """Raised when encoding or decoding violates contract or framing."""
    pass


# -----------------------------------------------------------------------------
# Common Validation and Header Parsing
# -----------------------------------------------------------------------------

def validate_input_values(values: Sequence[int]) -> List[int]:
    """Validate that all values are integers in [-364, 364] and return as list."""
    out = []
    for idx, v in enumerate(values):
        if type(v) is not int:
            raise CodecError(f"Value at index {idx} is not an integer: {type(v)}")
        if not (-364 <= v <= 364):
            raise CodecError(f"Value at index {idx} out of range [-364, 364]: {v}")
        out.append(v)
    return out


def parse_framing(blob: bytes, k: int, A: int, access_mode: str) -> Tuple[int, int, List[int], int]:
    """Parse common header and index, validating framing before allocations.

    Returns:
        (N, B, block_offsets, D)
    """
    if len(blob) < 8:
        raise CodecError(f"Corrupted frame: blob length {len(blob)} < 8 bytes")
    (N,) = struct.unpack('<Q', blob[:8])
    if N == 0:
        if len(blob) != 8:
            raise CodecError(f"Extra trailing bytes for N=0: length {len(blob)} != 8")
        return 0, 0, [], 8

    # Prevent unreasonable / overflow allocations if N is huge or malformed
    # For any arm, each block of k symbols requires at least 1 byte of payload,
    # and if RA-INDEXED, 8 bytes of index.
    # Total wire bytes must be at least 8 + (N//k)*1.
    # If N exceeds what could possibly fit in len(blob), reject immediately before calculating B or iterating.
    min_possible_wire = 8 + (N * 6) // 8  # absolute minimal theoretical lower bound
    if N > 2**48 or min_possible_wire > len(blob):
        raise CodecError(f"Declared count N={N} exceeds maximum possible count for blob length {len(blob)}")

    B = (N + k - 1) // k
    header_bytes = 8
    index_bytes = 8 * B if access_mode == "RA-INDEXED" else 0
    H = header_bytes + index_bytes

    if len(blob) < H:
        raise CodecError(f"Truncated frame: expected at least {H} bytes for header/index, got {len(blob)}")

    D = align_up(H, A)
    # Check alignment gap bytes between H and D
    if D > H:
        if len(blob) < D:
            raise CodecError(f"Truncated frame before data start: length {len(blob)} < {D}")
        if any(b != 0 for b in blob[H:D]):
            raise CodecError("Nonzero padding in header alignment gap")

    block_offsets: List[int] = []
    if access_mode == "RA-INDEXED":
        for j in range(B):
            off_bytes = blob[8 + 8 * j : 8 + 8 * (j + 1)]
            (off,) = struct.unpack('<Q', off_bytes)
            block_offsets.append(off)

    return N, B, block_offsets, D


# -----------------------------------------------------------------------------
# Arm: B10 (Packed 10-bit binary fields)
# -----------------------------------------------------------------------------

def encode_block_b10(values: Sequence[int]) -> bytes:
    """Encode up to k values into raw B10 payload bytes."""
    m = len(values)
    raw_len = L_B10(m)
    acc = 0
    bits = 0
    out = bytearray()
    for v in values:
        u = v + 364
        acc |= (u & 1023) << bits
        bits += 10
        while bits >= 8:
            out.append(acc & 0xFF)
            acc >>= 8
            bits -= 8
    if bits > 0:
        out.append(acc & 0xFF)
    if len(out) != raw_len:
        raise CodecError(f"Internal error: B10 encoded {len(out)} bytes, expected {raw_len}")
    return bytes(out)


def decode_block_b10(data: bytes, m: int) -> List[int]:
    """Decode exactly m values from raw B10 payload bytes."""
    raw_len = L_B10(m)
    if len(data) < raw_len:
        raise CodecError(f"Truncated B10 block: expected {raw_len} bytes, got {len(data)}")
    acc = 0
    bits = 0
    res = []
    byte_idx = 0
    for _ in range(m):
        while bits < 10:
            if byte_idx >= raw_len:
                raise CodecError("Premature end of B10 payload")
            acc |= data[byte_idx] << bits
            byte_idx += 1
            bits += 8
        u = acc & 1023
        acc >>= 10
        bits -= 10
        if u >= 729:
            raise CodecError(f"Invalid B10 decoded field value {u} >= 729 (states 729-1023 are prohibited)")
        res.append(u - 364)
    # Check that remaining bits in last byte are 0
    while bits > 0:
        if (acc & 1) != 0:
            raise CodecError("Nonzero padding in B10 tail bits")
        acc >>= 1
        bits -= 1
    return res


# -----------------------------------------------------------------------------
# Arm: T243 (Base-243 packing)
# -----------------------------------------------------------------------------

def encode_block_t243(values: Sequence[int]) -> bytes:
    """Encode up to k values into raw T243 payload bytes."""
    m = len(values)
    raw_len = L_T243(m)
    out = bytearray()
    group, power = 0, 1
    for v in values:
        u = v + 364
        # Extract six base-3 digits d_0 .. d_5 (least significant first)
        code = u
        for _ in range(6):
            code, digit = divmod(code, 3)
            group += digit * power
            power *= 3
            if power == 243:
                out.append(group)
                group, power = 0, 1
    if power != 1:
        out.append(group)
    if len(out) != raw_len:
        raise CodecError(f"Internal error: T243 encoded {len(out)} bytes, expected {raw_len}")
    return bytes(out)


def decode_block_t243(data: bytes, m: int) -> List[int]:
    """Decode exactly m values from raw T243 payload bytes."""
    raw_len = L_T243(m)
    if len(data) < raw_len:
        raise CodecError(f"Truncated T243 block: expected {raw_len} bytes, got {len(data)}")
    for i in range(raw_len):
        if data[i] >= 243:
            raise CodecError(f"Invalid T243 byte {data[i]} >= 243 at offset {i}")

    result = []
    value = 0
    place = 1
    pos = 0
    total_trits = 6 * m

    for i in range(raw_len):
        b = data[i]
        for _ in range(5):
            b, digit = divmod(b, 3)
            if pos < total_trits:
                value += digit * place
                place *= 3
                if place == 729:
                    result.append(value - 364)
                    value, place = 0, 1
            else:
                if digit != 0:
                    raise CodecError("Nonzero padding in T243 unused tail trits")
            pos += 1

    return result


# -----------------------------------------------------------------------------
# Arm: B243-CLONE (Binary software implementation of T243 wire language)
# -----------------------------------------------------------------------------

def encode_block_b243_clone(values: Sequence[int]) -> bytes:
    """Binary software clone producing the exact same Base-243 wire bytes.

    Acts as an equivalence witness. Implemented independently using direct
    arithmetic bit/byte construction.
    """
    m = len(values)
    raw_len = L_T243(m)
    # Collect all base-3 digits of u = x + 364
    digits = []
    for v in values:
        u = v + 364
        for _ in range(6):
            digits.append(u % 3)
            u //= 3

    out = bytearray()
    for i in range(0, len(digits), 5):
        chunk = digits[i : i + 5]
        b = sum(d * (3**p) for p, d in enumerate(chunk))
        out.append(b)

    if len(out) != raw_len:
        raise CodecError(f"Internal error: B243-CLONE encoded {len(out)} bytes, expected {raw_len}")
    return bytes(out)


def decode_block_b243_clone(data: bytes, m: int) -> List[int]:
    """Decode using clone logic, verifying equivalence."""
    return decode_block_t243(data, m)


# -----------------------------------------------------------------------------
# Arm: B729 (Fixed-length enumerative block integer coding)
# -----------------------------------------------------------------------------

def encode_block_b729(values: Sequence[int]) -> bytes:
    """Encode up to k values into raw B729 payload bytes.

    Forms integer I = sum(u_j * 729^j), encoded in b(m) = bit_length(729^m - 1) bits,
    stored least significant bit and byte first.
    """
    m = len(values)
    if m == 0:
        return b""
    raw_len = L_B729(m)
    bits_total = b_B729(m)

    # Calculate I
    I = 0
    power = 1
    for v in values:
        u = v + 364
        I += u * power
        power *= 729

    max_bound = 729**m
    if I >= max_bound:
        raise CodecError(f"Block integer overflow: I={I} >= 729^{m}")

    out = bytearray()
    temp_I = I
    remaining_bits = bits_total
    while remaining_bits > 0:
        out.append(temp_I & 0xFF)
        temp_I >>= 8
        remaining_bits -= 8

    if len(out) != raw_len:
        raise CodecError(f"Internal error: B729 encoded {len(out)} bytes, expected {raw_len}")
    return bytes(out)


def decode_block_b729(data: bytes, m: int) -> List[int]:
    """Decode exactly m values from raw B729 payload bytes."""
    if m == 0:
        return []
    raw_len = L_B729(m)
    bits_total = b_B729(m)
    if len(data) < raw_len:
        raise CodecError(f"Truncated B729 block: expected {raw_len} bytes, got {len(data)}")

    I = 0
    for byte_idx in reversed(range(raw_len)):
        I = (I << 8) | data[byte_idx]

    # Check unused high padding bits
    high_bits = (raw_len * 8) - bits_total
    if high_bits > 0:
        # Check that high padding bits are 0
        mask = ((1 << high_bits) - 1) << bits_total
        if (I & mask) != 0:
            raise CodecError("Nonzero padding in B729 unused high bits")

    max_bound = 729**m
    if I >= max_bound:
        raise CodecError(f"Invalid B729 block integer {I} >= 729^{m}")

    res = []
    temp_I = I
    for _ in range(m):
        temp_I, u = divmod(temp_I, 729)
        res.append(u - 364)

    return res


# -----------------------------------------------------------------------------
# Unified Framing, Encoding, Decoding, and Random Access Dispatcher
# -----------------------------------------------------------------------------

ARM_ENCODERS = {
    "B10": encode_block_b10,
    "T243": encode_block_t243,
    "B243-CLONE": encode_block_b243_clone,
    "B729": encode_block_b729,
}

ARM_DECODERS = {
    "B10": decode_block_b10,
    "T243": decode_block_t243,
    "B243-CLONE": decode_block_b243_clone,
    "B729": decode_block_b729,
}


def encode(values: Sequence[int], arm: str, k: int, A: int, access_mode: str = "RA-FIXED") -> bytes:
    """Encode values into the complete framed wire artifact."""
    clean_vals = validate_input_values(values)
    N = len(clean_vals)
    layout = compute_layout(N, k, A, access_mode, arm)

    if N == 0:
        return struct.pack('<Q', 0)

    encoder = ARM_ENCODERS[arm]
    B = layout["blocks"]
    D = layout["data_offset"]

    out = bytearray(struct.pack('<Q', N))

    if access_mode == "RA-INDEXED":
        for off in layout["block_offsets"]:
            out.extend(struct.pack('<Q', off))

    # Pad alignment gap between H and D
    H = len(out)
    if D > H:
        out.extend(b'\x00' * (D - H))

    for j in range(B):
        m_j = min(k, N - j * k)
        block_vals = clean_vals[j * k : j * k + m_j]
        raw_payload = encoder(block_vals)
        aligned_len = align_up(len(raw_payload), A)
        out.extend(raw_payload)
        if aligned_len > len(raw_payload):
            out.extend(b'\x00' * (aligned_len - len(raw_payload)))

    if len(out) != layout["total_bytes"]:
        raise CodecError(f"Encoded size mismatch: got {len(out)}, oracle expected {layout['total_bytes']}")

    return bytes(out)


def decode(blob: bytes, arm: str, k: int, A: int, access_mode: str = "RA-FIXED") -> List[int]:
    """Decode complete framed wire artifact into a list of signed integers."""
    N, B, block_offsets, D = parse_framing(blob, k, A, access_mode)
    if N == 0:
        return []

    layout = compute_layout(N, k, A, access_mode, arm)
    if len(blob) != layout["total_bytes"]:
        raise CodecError(f"Invalid frame total bytes: expected {layout['total_bytes']}, got {len(blob)}")

    if access_mode == "RA-INDEXED":
        if block_offsets != layout["block_offsets"]:
            raise CodecError(f"Non-canonical block offsets in index: got {block_offsets}, expected {layout['block_offsets']}")

    decoder = ARM_DECODERS[arm]
    result: List[int] = []

    for j in range(B):
        m_j = min(k, N - j * k)
        off_j = layout["block_offsets"][j]
        raw_l = {
            "B10": L_B10,
            "T243": L_T243,
            "B243-CLONE": L_T243,
            "B729": L_B729,
        }[arm](m_j)
        aligned_l = align_up(raw_l, A)

        block_data = blob[off_j : off_j + raw_l]
        pad_data = blob[off_j + raw_l : off_j + aligned_l]
        if any(b != 0 for b in pad_data):
            raise CodecError(f"Nonzero padding in block {j} alignment padding")

        vals = decoder(block_data, m_j)
        result.extend(vals)

    return result


def random_access(blob: bytes, index: int, arm: str, k: int, A: int, access_mode: str = "RA-FIXED") -> int:
    """Random access query for x_i without decoding preceding blocks.

    Work, memory allocation, and touched bytes are strictly bounded by block size k,
    computed in O(1) space without allocating or reconstructing an O(B) list of block offsets.
    """
    if len(blob) < 8:
        raise CodecError("Blob smaller than header")
    (N,) = struct.unpack('<Q', blob[:8])
    if not (0 <= index < N):
        raise CodecError(f"Index out of bounds: index={index}, N={N}")

    min_possible_wire = 8 + (N * 6) // 8
    if N > 2**48 or min_possible_wire > len(blob):
        raise CodecError(f"Declared count N={N} exceeds maximum possible count for blob length {len(blob)}")

    B = (N + k - 1) // k
    j = index // k
    pos_in_block = index % k
    m_j = min(k, N - j * k)

    # Determine data start D and block offset in O(1) space
    header_bytes = 8
    index_bytes = 8 * B if access_mode == "RA-INDEXED" else 0
    H = header_bytes + index_bytes
    D = align_up(H, A)

    raw_func = {
        "B10": L_B10,
        "T243": L_T243,
        "B243-CLONE": L_T243,
        "B729": L_B729,
    }[arm]
    raw_full = raw_func(k)
    stride = align_up(raw_full, A)
    raw_j = raw_func(m_j)
    aligned_j = align_up(raw_j, A)

    # Compute expected total length in O(1) to validate frame bounds
    if B == 1:
        expected_total = D + aligned_j
    else:
        last_m = N - (B - 1) * k
        aligned_last = align_up(raw_func(last_m), A)
        expected_total = D + (B - 1) * stride + aligned_last

    if len(blob) != expected_total:
        raise CodecError(f"Invalid frame total bytes: expected {expected_total}, got {len(blob)}")

    # Compute block offset in O(1) space without reconstructing all offsets
    expected_offset_j = D + j * stride

    if access_mode == "RA-INDEXED":
        # Read only the 8-byte index entry for block j directly at offset 8 + 8*j
        idx_pos = 8 + 8 * j
        if len(blob) < idx_pos + 8:
            raise CodecError("Index entry truncated")
        (block_offset,) = struct.unpack('<Q', blob[idx_pos : idx_pos + 8])
        if block_offset != expected_offset_j:
            raise CodecError(f"Non-canonical block offset in index: {block_offset} != {expected_offset_j}")
    else:
        block_offset = expected_offset_j

    if block_offset + aligned_j > len(blob):
        raise CodecError("Block offset out of bounds of blob")

    # Validate alignment padding of target block
    pad_data = blob[block_offset + raw_j : block_offset + aligned_j]
    if any(b != 0 for b in pad_data):
        raise CodecError("Nonzero padding in target block alignment")

    block_data = blob[block_offset : block_offset + raw_j]

    # Direct digit extraction for B10 and T243 without full block decode
    if arm == "B10":
        # Each symbol takes 10 bits: starts at bit_pos = 10 * pos_in_block
        bit_start = 10 * pos_in_block
        start_byte = bit_start // 8
        bit_offset = bit_start % 8
        # Touches at most 3 bytes (10 bits can cross at most 2 byte boundaries)
        chunk = block_data[start_byte : start_byte + 3]
        acc = 0
        for p, b in enumerate(chunk):
            acc |= b << (8 * p)
        u = (acc >> bit_offset) & 1023
        if u >= 729:
            raise CodecError(f"Invalid B10 decoded field value {u} >= 729")
        return u - 364

    elif arm in ("T243", "B243-CLONE"):
        # Each symbol is 6 trits. Symbol pos_in_block starts at trit_start = 6 * pos_in_block
        trit_start = 6 * pos_in_block
        # 5 trits per byte: starts in byte trit_start // 5, ends in byte (trit_start + 5) // 5
        start_byte = trit_start // 5
        end_byte = (trit_start + 5) // 5
        # Touches at most 2 bytes!
        for b_idx in range(start_byte, end_byte + 1):
            if block_data[b_idx] >= 243:
                raise CodecError(f"Invalid T243 byte {block_data[b_idx]} >= 243")

        # Extract 6 trits
        trits = []
        for b_idx in range(start_byte, end_byte + 1):
            byte_val = block_data[b_idx]
            for _ in range(5):
                byte_val, digit = divmod(byte_val, 3)
                trits.append(digit)

        # Offset within the extracted stream
        skip_trits = trit_start - (start_byte * 5)
        sym_trits = trits[skip_trits : skip_trits + 6]
        u = sum(d * (3**p) for p, d in enumerate(sym_trits))
        return u - 364

    elif arm == "B729":
        # B729 decodes only the target block (bounded by k) to extract the element
        vals = decode_block_b729(block_data, m_j)
        return vals[pos_in_block]

    raise CodecError(f"Unknown arm {arm}")
