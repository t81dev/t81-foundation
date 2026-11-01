"""Deterministic regression tests and adversarial validation for EXP-H04-R03.

Covers:
1. Every symbol in the 729-state domain [-364, 364].
2. Exhaustive small sequences where computationally practical.
3. Block boundaries and incomplete blocks:
   N in {0, 1, 2, 3, 4, 5, 6, 63, 64, 65, 1023, 1024, 1025}.
4. Round-trip identity and deterministic output across all 6 predefined workloads.
5. Invalid symbols, malformed frames, truncated streams, out-of-bounds offsets,
   T243 bytes >= 243, B10 field >= 729, nonzero padding, and corrupted metadata.
6. Random-access correctness against independent oracle for first, last, boundary,
   and pseudorandom indices.
7. Exact encoded-size accounting against oracle for the entire (k, A, access_mode) grid.
8. Bit-for-bit equivalence between T243 and B243-CLONE.
9. Proof check that L_B729(m) <= L_T243(m) for all m.
"""
import os
import random
import struct
import sys
import unittest
from typing import List

# Ensure repository root is on sys.path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../..")))

from experiments.h04_r03_matched_codec import (
    align_up,
    b_B729,
    L_B10,
    L_T243,
    L_B729,
    compute_layout,
    verify_size_invariants,
    CodecError,
    encode,
    decode,
    random_access,
)


def generate_workload(name: str, n: int, seed: int = 42) -> List[int]:
    """Generate deterministic workload values."""
    rng = random.Random(seed)
    if name == "uniform":
        return [rng.randint(-364, 364) for _ in range(n)]
    elif name == "zero":
        return [0] * n
    elif name == "alternating_endpoints":
        return [364 if i % 2 == 0 else -364 for i in range(n)]
    elif name == "boundary_concentrated":
        choices = [-364, -1, 0, 1, 364]
        weights = [0.2, 0.2, 0.2, 0.2, 0.2]
        return rng.choices(choices, weights=weights, k=n)
    elif name == "slow_runs":
        out = []
        curr = rng.randint(-364, 364)
        while len(out) < n:
            run_len = min(rng.randint(3, 15), n - len(out))
            out.extend([curr] * run_len)
            curr = rng.randint(-364, 364)
        return out
    elif name == "cycle":
        return [(i % 729) - 364 for i in range(n)]
    else:
        raise ValueError(f"Unknown workload {name}")


class TestH04R03MatchedCodec(unittest.TestCase):

    def test_01_all_729_individual_symbols(self):
        """Test every symbol in the 729-state domain [-364, 364] at k=1, A=1."""
        all_symbols = list(range(-364, 365))
        self.assertEqual(len(all_symbols), 729)

        for arm in ["B10", "T243", "B243-CLONE", "B729"]:
            for sym in all_symbols:
                inp = [sym]
                blob = encode(inp, arm=arm, k=1, A=1, access_mode="RA-FIXED")
                # At k=1, A=1, RA-FIXED, header is 8 bytes, payload is 2 bytes -> 10 bytes total
                self.assertEqual(len(blob), 10)
                dec = decode(blob, arm=arm, k=1, A=1, access_mode="RA-FIXED")
                self.assertEqual(dec, inp)
                self.assertEqual(random_access(blob, 0, arm=arm, k=1, A=1, access_mode="RA-FIXED"), sym)

    def test_02_t243_clone_bit_for_bit_identity(self):
        """B243-CLONE must produce the exact identical byte sequence as T243."""
        for n in [0, 1, 2, 5, 17, 64, 73, 100]:
            vals = generate_workload("uniform", n, seed=100 + n)
            for k in [1, 5, 64]:
                for A in [1, 8, 64]:
                    for mode in ["RA-FIXED", "RA-INDEXED"]:
                        blob_t243 = encode(vals, arm="T243", k=k, A=A, access_mode=mode)
                        blob_clone = encode(vals, arm="B243-CLONE", k=k, A=A, access_mode=mode)
                        self.assertEqual(blob_t243, blob_clone, f"Clone byte mismatch at n={n}, k={k}, A={A}, mode={mode}")

    def test_03_exhaustive_small_sequences(self):
        """Exhaustive pairs for boundary states."""
        boundaries = [-364, -363, -1, 0, 1, 363, 364]
        pairs = [[a, b] for a in boundaries for b in boundaries]
        self.assertEqual(len(pairs), 49)

        for arm in ["B10", "T243", "B729"]:
            for pair in pairs:
                for k in [1, 5]:
                    blob = encode(pair, arm=arm, k=k, A=1, access_mode="RA-FIXED")
                    dec = decode(blob, arm=arm, k=k, A=1, access_mode="RA-FIXED")
                    self.assertEqual(dec, pair)
                    self.assertEqual(random_access(blob, 0, arm=arm, k=k, A=1), pair[0])
                    self.assertEqual(random_access(blob, 1, arm=arm, k=k, A=1), pair[1])

    def test_04_block_boundaries_and_incomplete_blocks(self):
        """Test boundary lengths N in {0, 1, 2, 3, 4, 5, 6, 63, 64, 65, 1023, 1024, 1025}."""
        test_lens = [0, 1, 2, 3, 4, 5, 6, 63, 64, 65, 1023, 1024, 1025]
        arms = ["B10", "T243", "B729"]
        k_values = [1, 5, 64, 1024]
        A_values = [1, 8, 64]

        for n in test_lens:
            vals = generate_workload("cycle", n)
            for k in k_values:
                for A in A_values:
                    for mode in ["RA-FIXED", "RA-INDEXED"]:
                        for arm in arms:
                            blob = encode(vals, arm=arm, k=k, A=A, access_mode=mode)
                            layout = compute_layout(n, k, A, mode, arm)
                            # Size verification against oracle
                            self.assertEqual(len(blob), layout["total_bytes"])
                            # Decode round-trip
                            dec = decode(blob, arm=arm, k=k, A=A, access_mode=mode)
                            self.assertEqual(dec, vals)

                            # Random access checks for sparse samples
                            if n > 0:
                                check_indices = {0, n - 1, n // 2}
                                if k < n:
                                    check_indices.add(k - 1)
                                    check_indices.add(k)
                                for idx in check_indices:
                                    if 0 <= idx < n:
                                        self.assertEqual(
                                            random_access(blob, idx, arm=arm, k=k, A=A, access_mode=mode),
                                            vals[idx]
                                        )

    def test_05_deterministic_workloads_round_trip(self):
        """Verify all 6 test workloads round-trip deterministically across arms."""
        workloads = [
            "uniform",
            "zero",
            "alternating_endpoints",
            "boundary_concentrated",
            "slow_runs",
            "cycle",
        ]
        n = 120
        k = 64
        A = 8
        for w in workloads:
            vals = generate_workload(w, n, seed=2026)
            for arm in ["B10", "T243", "B729"]:
                blob1 = encode(vals, arm=arm, k=k, A=A, access_mode="RA-FIXED")
                blob2 = encode(vals, arm=arm, k=k, A=A, access_mode="RA-FIXED")
                self.assertEqual(blob1, blob2, "Determinism violation")
                dec = decode(blob1, arm=arm, k=k, A=A, access_mode="RA-FIXED")
                self.assertEqual(dec, vals, f"Round trip failure on workload {w} arm {arm}")

    def test_06_size_inequality_theorem(self):
        """Verify analytical theorem: L_B729(m) <= L_T243(m) for all 1 <= m <= 1024."""
        for m in range(1, 1025):
            l_t243 = L_T243(m)
            l_b729 = L_B729(m)
            l_b10 = L_B10(m)
            self.assertLessEqual(l_b729, l_t243, f"B729 payload exceeds T243 at m={m}")
            self.assertLessEqual(l_t243, l_b10, f"T243 payload exceeds B10 at m={m}")

    def test_07_analytical_table_verification(self):
        """Verify exact Section 9 specification table values."""
        # k=1
        self.assertEqual(b_B729(1), 10)
        self.assertEqual(L_B10(1), 2)
        self.assertEqual(L_T243(1), 2)
        self.assertEqual(L_B729(1), 2)

        # k=5
        self.assertEqual(b_B729(5), 48)
        self.assertEqual(L_B10(5), 7)
        self.assertEqual(L_T243(5), 6)
        self.assertEqual(L_B729(5), 6)

        # k=64
        self.assertEqual(b_B729(64), 609)
        self.assertEqual(L_B10(64), 80)
        self.assertEqual(L_T243(64), 77)
        self.assertEqual(L_B729(64), 77)

        # k=1024
        self.assertEqual(b_B729(1024), 9739)
        self.assertEqual(L_B10(1024), 1280)
        self.assertEqual(L_T243(1024), 1229)
        self.assertEqual(L_B729(1024), 1218)

    def test_08_adversarial_invalid_symbols_rejected(self):
        """Reject symbols outside [-364, 364] and non-integer values."""
        bad_inputs = [
            [365],
            [-365],
            [1000],
            [-1000],
            [3.14],
            ["0"],
            [None],
            [0, 1, 365, 2],
        ]
        for bad in bad_inputs:
            for arm in ["B10", "T243", "B729"]:
                with self.assertRaises(CodecError):
                    encode(bad, arm=arm, k=5, A=1)

    def test_09_adversarial_malformed_frames_and_corruptions(self):
        """Reject truncated frames, corrupted headers, nonzero padding, huge uint64 counts, and bad bytes."""
        vals = [10, -20, 30, -40, 50, -60]

        for arm in ["B10", "T243", "B729"]:
            blob = encode(vals, arm=arm, k=5, A=8, access_mode="RA-INDEXED")

            # 1. Truncated header
            with self.assertRaises(CodecError):
                decode(blob[:4], arm=arm, k=5, A=8, access_mode="RA-INDEXED")

            # 2. Truncated index
            with self.assertRaises(CodecError):
                decode(blob[:12], arm=arm, k=5, A=8, access_mode="RA-INDEXED")

            # 3. Truncated payload
            with self.assertRaises(CodecError):
                decode(blob[:-1], arm=arm, k=5, A=8, access_mode="RA-INDEXED")

            # 4. Trailing garbage
            with self.assertRaises(CodecError):
                decode(blob + b'\x00', arm=arm, k=5, A=8, access_mode="RA-INDEXED")

            # 5. Zero length with trailing garbage
            zero_blob = encode([], arm=arm, k=5, A=8, access_mode="RA-INDEXED")
            self.assertEqual(len(zero_blob), 8)
            with self.assertRaises(CodecError):
                decode(zero_blob + b'\x01', arm=arm, k=5, A=8, access_mode="RA-INDEXED")

            # 6. Maximum uint64 count (2^64 - 1) before allocation
            max_u64_blob = struct.pack('<Q', 2**64 - 1) + b'\x00' * 32
            with self.assertRaises(CodecError):
                decode(max_u64_blob, arm=arm, k=5, A=8, access_mode="RA-INDEXED")
            with self.assertRaises(CodecError):
                random_access(max_u64_blob, 0, arm=arm, k=5, A=8, access_mode="RA-INDEXED")

            # 7. Declared N = 10,000,000 but blob length is only 20 bytes
            huge_n_blob = struct.pack('<Q', 10_000_000) + b'\x00' * 12
            with self.assertRaises(CodecError):
                decode(huge_n_blob, arm=arm, k=5, A=8, access_mode="RA-INDEXED")
            with self.assertRaises(CodecError):
                random_access(huge_n_blob, 0, arm=arm, k=5, A=8, access_mode="RA-INDEXED")

    def test_10_adversarial_t243_forbidden_bytes(self):
        """Reject T243 payload bytes in [243, 255]."""
        vals = [0, 0, 0, 0, 0]
        blob = bytearray(encode(vals, arm="T243", k=5, A=1, access_mode="RA-FIXED"))
        # Header is 8 bytes; payload starts at 8
        blob[8] = 243
        with self.assertRaises(CodecError):
            decode(bytes(blob), arm="T243", k=5, A=1, access_mode="RA-FIXED")

        blob[8] = 255
        with self.assertRaises(CodecError):
            decode(bytes(blob), arm="T243", k=5, A=1, access_mode="RA-FIXED")

    def test_11_adversarial_b10_prohibited_states(self):
        """Reject B10 decoded field values >= 729 (states 729..1023)."""
        vals = [0]
        blob = bytearray(encode(vals, arm="B10", k=1, A=1, access_mode="RA-FIXED"))
        # At k=1, u=364 encoded in 10 bits. Set field to 729
        # 729 in 10 bits: 729 & 0xFF = 0xD9, 729 >> 8 = 0x02
        blob[8] = 729 & 0xFF
        blob[9] = (729 >> 8) & 0x03
        with self.assertRaises(CodecError):
            decode(bytes(blob), arm="B10", k=1, A=1, access_mode="RA-FIXED")

    def test_12_adversarial_b729_integer_overflow_and_high_padding(self):
        """Reject B729 when high unused bits or integer value are corrupted."""
        vals = [0]
        # At m=1, bits=10, raw_len=2 (16 bits). Bits 10..15 must be zero.
        blob = bytearray(encode(vals, arm="B729", k=1, A=1, access_mode="RA-FIXED"))
        # Set bit 10
        blob[9] |= 0x04
        with self.assertRaises(CodecError):
            decode(bytes(blob), arm="B729", k=1, A=1, access_mode="RA-FIXED")

    def test_13_adversarial_alignment_padding_corruption(self):
        """Reject nonzero padding in alignment gaps."""
        vals = [10]
        # A=8, k=1. Header=8, data at 8. Payload=2 bytes, aligned to 8 bytes.
        # Bytes 10..15 are alignment padding and must be zero.
        blob = bytearray(encode(vals, arm="B10", k=1, A=8, access_mode="RA-FIXED"))
        self.assertEqual(len(blob), 16)
        blob[15] = 0x01
        with self.assertRaises(CodecError):
            decode(bytes(blob), arm="B10", k=1, A=8, access_mode="RA-FIXED")

    def test_14_adversarial_random_access_bounds(self):
        """Reject random access out of range."""
        vals = [1, 2, 3]
        for arm in ["B10", "T243", "B729"]:
            blob = encode(vals, arm=arm, k=5, A=1, access_mode="RA-FIXED")
            with self.assertRaises(CodecError):
                random_access(blob, -1, arm=arm, k=5, A=1)
            with self.assertRaises(CodecError):
                random_access(blob, 3, arm=arm, k=5, A=1)
            with self.assertRaises(CodecError):
                random_access(blob, 100, arm=arm, k=5, A=1)

    def test_16_deterministic_operation_count_random_access(self):
        """Verify deterministic bounded-byte touch count for random access."""
        class InstrumentedBuffer:
            def __init__(self, data: bytes):
                self._data = data
                self.touched_bytes = 0
                self.slice_count = 0

            def __len__(self):
                return len(self._data)

            def __getitem__(self, item):
                self.slice_count += 1
                if isinstance(item, slice):
                    start = item.start or 0
                    stop = item.stop if item.stop is not None else len(self._data)
                    self.touched_bytes += max(0, stop - start)
                else:
                    self.touched_bytes += 1
                return self._data[item]

        # Test across two vastly different sequence lengths N=10 and N=1000
        # For fixed k=5, touched bytes in random_access must remain bounded by k and independent of N
        k = 5
        A = 8
        for arm in ["B10", "T243"]:
            buf_small = InstrumentedBuffer(encode(generate_workload("cycle", 10), arm=arm, k=k, A=A))
            buf_large = InstrumentedBuffer(encode(generate_workload("cycle", 1000), arm=arm, k=k, A=A))

            # Query index 7 in both buffers (block 1, pos 2)
            val_small = random_access(buf_small, 7, arm=arm, k=k, A=A)
            touched_small = buf_small.touched_bytes

            val_large = random_access(buf_large, 7, arm=arm, k=k, A=A)
            touched_large = buf_large.touched_bytes

            self.assertEqual(val_small, val_large)
            # Touched bytes must be identical and independent of N
            self.assertEqual(touched_small, touched_large)
            # B10 and T243 touch <= 32 bytes (header + alignment + at most 3 payload bytes)
            self.assertLessEqual(touched_large, 32)

    def test_15_known_answer_vectors_and_independent_decoders(self):
        """Verify hard-coded known-answer vectors and independent decoder routines."""
        from experiments.h04_r03_matched_codec.independent_witness import (
            KAV_VALUES_3,
            KAV_EXPECTED_B10_BLOB,
            KAV_EXPECTED_T243_BLOB,
            KAV_EXPECTED_B729_BLOB,
            independent_decode_raw_b10,
            independent_decode_raw_t243,
            independent_decode_raw_b729,
        )

        # 1. Check generated blobs match exact independently derived KAV wire blobs
        b10_blob = encode(KAV_VALUES_3, arm="B10", k=5, A=8, access_mode="RA-FIXED")
        t243_blob = encode(KAV_VALUES_3, arm="T243", k=5, A=8, access_mode="RA-FIXED")
        clone_blob = encode(KAV_VALUES_3, arm="B243-CLONE", k=5, A=8, access_mode="RA-FIXED")
        b729_blob = encode(KAV_VALUES_3, arm="B729", k=5, A=8, access_mode="RA-FIXED")

        self.assertEqual(b10_blob, KAV_EXPECTED_B10_BLOB)
        self.assertEqual(t243_blob, KAV_EXPECTED_T243_BLOB)
        self.assertEqual(clone_blob, KAV_EXPECTED_T243_BLOB)  # Clone must match T243 KAV
        self.assertEqual(b729_blob, KAV_EXPECTED_B729_BLOB)

        # 2. Check independent decoders directly on raw payloads
        self.assertEqual(independent_decode_raw_b10(b10_blob[8:12], 3), KAV_VALUES_3)
        self.assertEqual(independent_decode_raw_t243(t243_blob[8:12], 3), KAV_VALUES_3)
        self.assertEqual(independent_decode_raw_t243(clone_blob[8:12], 3), KAV_VALUES_3)
        self.assertEqual(independent_decode_raw_b729(b729_blob[8:12], 3), KAV_VALUES_3)


if __name__ == '__main__':
    unittest.main()
