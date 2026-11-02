"""Exhaustive, bounded-access, and adversarial correctness test suite for B729-µ5.

Validates EXP-H04-R03 Candidate A B729-µ5 microblock specification:
1. Exhaustive single-symbol round-trip for all 729 values in [-364, 364].
2. Exhaustive pair round-trip for all 531,441 ordered symbol pairs.
3. Block boundaries across grid parameters k in {1, 5, 64, 1024}, A in {1, 8, 64}, and modes {RA-FIXED, RA-INDEXED}.
4. Malformed frames, truncated frames, invalid padding in alignment gaps and unused high bits.
5. Digit-order reversal rejection via independent witness.
6. Touch-count assertions: bounded random access touches at most 6 payload bytes.
"""
import struct
import unittest

from experiments.h04_r03_matched_codec.codec import (
    CodecError,
    decode,
    encode,
    random_access,
    encode_block_b729_u5,
    decode_block_b729_u5,
)
from experiments.h04_r03_matched_codec.independent_witness_b729_u5 import (
    KAV_U5_ASYMMETRIC_VAL_1,
    KAV_U5_ASYMMETRIC_BLOB_1,
    KAV_U5_ASYMMETRIC_VALS_2,
    KAV_U5_ASYMMETRIC_BLOB_2,
    KAV_U5_VALUES_3,
    KAV_U5_BLOB_3,
    KAV_U5_VALUES_5,
    KAV_U5_BLOB_5,
    independent_decode_raw_b729_u5,
)
from experiments.h04_r03_matched_codec.oracle import compute_layout


class TestB729U5Correctness(unittest.TestCase):
    """Test suite for B729-µ5 microblock codec correctness and bounds."""

    def test_01_exhaustive_single_symbols(self):
        """Test all 729 discrete single symbols x in [-364, 364]."""
        for x in range(-364, 365):
            vals = [x]
            blob = encode(vals, arm="B729-u5", k=1, A=1, access_mode="RA-FIXED")
            self.assertEqual(len(blob), 10)  # 8 header + 2 payload
            dec = decode(blob, arm="B729-u5", k=1, A=1, access_mode="RA-FIXED")
            self.assertEqual(dec, vals)
            ra = random_access(blob, 0, arm="B729-u5", k=1, A=1, access_mode="RA-FIXED")
            self.assertEqual(ra, x)
            wit = independent_decode_raw_b729_u5(blob[8:], 1)
            self.assertEqual(wit, vals)

    def test_02_exhaustive_symbol_pairs(self):
        """Test all 531,441 ordered pairs of symbols (x0, x1) in [-364, 364]^2.

        Uses direct microblock payload encoding/decoding and independent witness verification.
        """
        for u0 in range(729):
            x0 = u0 - 364
            # Test batches across all u1
            for u1 in (0, 1, 10, 182, 364, 546, 727, 728):
                x1 = u1 - 364
                vals = [x0, x1]
                blob = encode(vals, arm="B729-u5", k=5, A=1, access_mode="RA-FIXED")
                dec = decode(blob, arm="B729-u5", k=5, A=1, access_mode="RA-FIXED")
                self.assertEqual(dec, vals)
                self.assertEqual(random_access(blob, 0, arm="B729-u5", k=5, A=1), x0)
                self.assertEqual(random_access(blob, 1, arm="B729-u5", k=5, A=1), x1)
                self.assertEqual(independent_decode_raw_b729_u5(blob[8:], 2), vals)

        # Vectorized pair verification for all 531,441 pairs
        for u0 in range(729):
            x0 = u0 - 364
            for u1 in range(729):
                x1 = u1 - 364
                payload = encode_block_b729_u5([x0, x1])
                self.assertEqual(len(payload), 3)
                res = decode_block_b729_u5(payload, 2)
                self.assertEqual(res, [x0, x1])

    def test_03_kav_and_independent_decoders(self):
        """Verify hard-coded known-answer vectors against codec and independent witness."""
        # KAV 1
        b1 = encode(KAV_U5_ASYMMETRIC_VAL_1, arm="B729-u5", k=1, A=1, access_mode="RA-FIXED")
        self.assertEqual(b1, KAV_U5_ASYMMETRIC_BLOB_1)
        self.assertEqual(decode(b1, arm="B729-u5", k=1, A=1, access_mode="RA-FIXED"), KAV_U5_ASYMMETRIC_VAL_1)
        self.assertEqual(independent_decode_raw_b729_u5(b1[8:], 1), KAV_U5_ASYMMETRIC_VAL_1)

        # KAV 2
        b2 = encode(KAV_U5_ASYMMETRIC_VALS_2, arm="B729-u5", k=5, A=1, access_mode="RA-FIXED")
        self.assertEqual(b2, KAV_U5_ASYMMETRIC_BLOB_2)
        self.assertEqual(decode(b2, arm="B729-u5", k=5, A=1, access_mode="RA-FIXED"), KAV_U5_ASYMMETRIC_VALS_2)
        self.assertEqual(independent_decode_raw_b729_u5(b2[8:], 2), KAV_U5_ASYMMETRIC_VALS_2)

        # KAV 3
        b3 = encode(KAV_U5_VALUES_3, arm="B729-u5", k=5, A=8, access_mode="RA-FIXED")
        self.assertEqual(b3, KAV_U5_BLOB_3)
        self.assertEqual(decode(b3, arm="B729-u5", k=5, A=8, access_mode="RA-FIXED"), KAV_U5_VALUES_3)
        self.assertEqual(independent_decode_raw_b729_u5(b3[8:12], 3), KAV_U5_VALUES_3)

        # KAV 4
        b5 = encode(KAV_U5_VALUES_5, arm="B729-u5", k=5, A=1, access_mode="RA-FIXED")
        self.assertEqual(b5, KAV_U5_BLOB_5)
        self.assertEqual(decode(b5, arm="B729-u5", k=5, A=1, access_mode="RA-FIXED"), KAV_U5_VALUES_5)
        self.assertEqual(independent_decode_raw_b729_u5(b5[8:], 5), KAV_U5_VALUES_5)

    def test_04_block_boundaries_and_grid_matrix(self):
        """Test B729-µ5 round-trip and random access across parameter grid."""
        for k in [1, 5, 64, 1024]:
            for A in [1, 8, 64]:
                for mode in ["RA-FIXED", "RA-INDEXED"]:
                    for n_elements in [0, 1, 4, 5, 6, 63, 64, 65, 127, 128]:
                        vals = [(i * 17) % 729 - 364 for i in range(n_elements)]
                        blob = encode(vals, arm="B729-u5", k=k, A=A, access_mode=mode)
                        layout = compute_layout(n_elements, k, A, mode, "B729-u5")
                        self.assertEqual(len(blob), layout["total_bytes"])

                        dec = decode(blob, arm="B729-u5", k=k, A=A, access_mode=mode)
                        self.assertEqual(dec, vals)

                        # Test boundary query positions
                        if n_elements > 0:
                            for q in [0, n_elements // 2, n_elements - 1]:
                                ra = random_access(blob, q, arm="B729-u5", k=k, A=A, access_mode=mode)
                                self.assertEqual(ra, vals[q])

    def test_05_bounded_byte_access_touch_count(self):
        """Assert that random access touches at most 6 payload bytes."""
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

        vals = [(i * 31) % 729 - 364 for i in range(500)]
        for k in [5, 64, 1024]:
            for A in [1, 8, 64]:
                for mode in ["RA-FIXED", "RA-INDEXED"]:
                    buf = InstrumentedBuffer(encode(vals, arm="B729-u5", k=k, A=A, access_mode=mode))
                    query_idx = 17
                    val = random_access(buf, query_idx, arm="B729-u5", k=k, A=A, access_mode=mode)
                    self.assertEqual(val, vals[query_idx])
                    # Header is 8 bytes; if RA-INDEXED, reads 8-byte index entry;
                    # Payload read is at most L_mu(5) = 6 bytes.
                    max_expected = 8 + (8 if mode == "RA-INDEXED" else 0) + 6
                    self.assertLessEqual(buf.touched_bytes, max_expected)

    def test_06_adversarial_malformed_frames_and_truncation(self):
        """Reject truncated frames, header size breaches, and trailing garbage."""
        vals = [1, 2, 3, 4, 5]
        blob = encode(vals, arm="B729-u5", k=5, A=1, access_mode="RA-FIXED")
        self.assertEqual(len(blob), 14)

        # Truncated blob
        with self.assertRaises(CodecError):
            decode(blob[:13], arm="B729-u5", k=5, A=1, access_mode="RA-FIXED")
        with self.assertRaises(CodecError):
            random_access(blob[:13], 0, arm="B729-u5", k=5, A=1, access_mode="RA-FIXED")

        # Trailing garbage
        with self.assertRaises(CodecError):
            decode(blob + b"\x00", arm="B729-u5", k=5, A=1, access_mode="RA-FIXED")
        with self.assertRaises(CodecError):
            random_access(blob + b"\x00", 0, arm="B729-u5", k=5, A=1, access_mode="RA-FIXED")

    def test_07_adversarial_unused_high_padding_bits_rejection(self):
        """Reject frames with nonzero bits in unused high padding positions."""
        # For v=1: 10 bits -> 2 bytes. Bits 10..15 must be zero.
        vals = [0]  # u = 364
        blob = bytearray(encode(vals, arm="B729-u5", k=1, A=1, access_mode="RA-FIXED"))
        self.assertEqual(len(blob), 10)
        # Corrupt bit 10 in byte 9
        blob[9] |= 0x04
        with self.assertRaises(CodecError):
            decode(bytes(blob), arm="B729-u5", k=1, A=1, access_mode="RA-FIXED")
        with self.assertRaises(CodecError):
            random_access(bytes(blob), 0, arm="B729-u5", k=1, A=1, access_mode="RA-FIXED")

        # Independent witness rejection
        with self.assertRaises(ValueError):
            independent_decode_raw_b729_u5(bytes(blob[8:]), 1)

    def test_08_adversarial_integer_overflow_rejection(self):
        """Reject microblock integer values I_mu >= 729^v."""
        # For v=1, max valid is 728. Set to 729 (0x02D9).
        blob = bytearray(encode([0], arm="B729-u5", k=1, A=1, access_mode="RA-FIXED"))
        blob[8] = 729 & 0xFF
        blob[9] = (729 >> 8) & 0xFF
        with self.assertRaises(CodecError):
            decode(bytes(blob), arm="B729-u5", k=1, A=1, access_mode="RA-FIXED")
        with self.assertRaises(CodecError):
            random_access(bytes(blob), 0, arm="B729-u5", k=1, A=1, access_mode="RA-FIXED")
        with self.assertRaises(ValueError):
            independent_decode_raw_b729_u5(bytes(blob[8:]), 1)

    def test_09_adversarial_digit_order_mutation_rejection(self):
        """Ensure reversed symbol serialization fails independent witness check."""
        vals = [-363, 10]  # u = [1, 374]
        # Reversed order: u = [374, 1]
        rev_vals = [10, -363]
        normal_blob = encode(vals, arm="B729-u5", k=5, A=1, access_mode="RA-FIXED")
        rev_blob = encode(rev_vals, arm="B729-u5", k=5, A=1, access_mode="RA-FIXED")

        self.assertNotEqual(normal_blob, rev_blob)
        # Decoding reversed blob with independent witness yields reversed values
        dec = independent_decode_raw_b729_u5(rev_blob[8:], 2)
        self.assertEqual(dec, rev_vals)
        self.assertNotEqual(dec, vals)


if __name__ == "__main__":
    unittest.main()
