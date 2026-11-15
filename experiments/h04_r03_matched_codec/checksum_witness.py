"""Source-separated correctness oracle; never times or executes codec queries.

XXH64 follows the v0.8.2 algorithm specification, using a one-shot formulation
rather than the production C streaming implementation. The dataset generator
below does not import workload.py, codec.py, or the production checksum wrapper.
Same agent authored both paths: source separation is NOT independent authorship
or independent scientific reproduction.
"""
import hashlib
import struct

MASK = (1 << 64) - 1
P1 = 11400714785074694791
P2 = 14029467366897019727
P3 = 1609587929392839161
P4 = 9650029242287828579
P5 = 2870177450012600261


def _rotate(value, shift):
    value &= MASK
    return ((value << shift) | (value >> (64 - shift))) & MASK


def _round(accumulator, lane):
    return (_rotate(accumulator + lane * P2, 31) * P1) & MASK


def reference_xxh64(message, seed=0):
    """Independent one-shot integer digest; all arithmetic modulo 2**64."""
    if type(seed) is not int or not 0 <= seed <= MASK:
        raise ValueError("unsigned 64-bit seed required")
    size, pos = len(message), 0
    if size >= 32:
        lanes = [(seed + P1 + P2) & MASK, (seed + P2) & MASK, seed, (seed - P1) & MASK]
        while pos + 32 <= size:
            for lane in range(4):
                word = int.from_bytes(message[pos:pos + 8], "little")
                lanes[lane] = _round(lanes[lane], word)
                pos += 8
        value = sum(_rotate(lanes[i], shift) for i, shift in enumerate((1, 7, 12, 18))) & MASK
        for lane in lanes:
            value = ((value ^ _round(0, lane)) * P1 + P4) & MASK
    else:
        value = (seed + P5) & MASK
    value = (value + size) & MASK
    while pos + 8 <= size:
        value ^= _round(0, int.from_bytes(message[pos:pos + 8], "little"))
        value = (_rotate(value, 27) * P1 + P4) & MASK
        pos += 8
    if pos + 4 <= size:
        value ^= int.from_bytes(message[pos:pos + 4], "little") * P1 & MASK
        value = (_rotate(value, 23) * P2 + P3) & MASK
        pos += 4
    while pos < size:
        value ^= message[pos] * P5 & MASK
        value = (_rotate(value, 11) * P1) & MASK
        pos += 1
    value ^= value >> 33
    value = value * P2 & MASK
    value ^= value >> 29
    value = value * P3 & MASK
    return (value ^ (value >> 32)) & MASK


def witness_symbols(n=1 << 20):
    """Separate transcription of the frozen SplitMix64/xoshiro256++ equations."""
    cursor = 0x5438315F44415441
    state = []
    for _ in range(4):
        cursor = (cursor + 0x9E3779B97F4A7C15) & MASK
        word = cursor
        word = ((word ^ (word >> 30)) * 0xBF58476D1CE4E5B9) & MASK
        word = ((word ^ (word >> 27)) * 0x94D049BB133111EB) & MASK
        state.append(word ^ (word >> 31))
    produced = 0
    while produced < n:
        word = (_rotate((state[0] + state[3]) & MASK, 23) + state[0]) & MASK
        temp = (state[1] << 17) & MASK
        state[2] ^= state[0]
        state[3] ^= state[1]
        state[1] ^= state[2]
        state[0] ^= state[3]
        state[2] ^= temp
        state[3] = _rotate(state[3], 45)
        if word < 18446744073709550922:
            produced += 1
            yield word % 729 - 364


def derive_witness(n=1 << 20):
    """Untimed correctness preparation; no candidate codec or query loop runs."""
    raw = hashlib.sha256()
    output = bytearray()
    for value in witness_symbols(n):
        raw.update(struct.pack("<h", value))
        output.extend(struct.pack("<q", value))
    return {"n": n, "seed": 0, "output_serialization": "signed-int64-little-endian-dataset-order",
            "dataset_sha256": raw.hexdigest(), "output_sha256": hashlib.sha256(output).hexdigest(),
            "xxh64": f"{reference_xxh64(output):016x}"}
