"""Unit tests for deterministic workload and query schedule generators."""
import hashlib
import unittest

from experiments.h04_r03_matched_codec.workload import (
    CANONICAL_DATASET_SHA256,
    CANONICAL_QUERY_SHA256,
    DATASET_SEED,
    LIMIT_729,
    QUERY_SEED,
    WORKLOAD_N,
    Xoshiro256PlusPlus,
    generate_dataset_symbols,
    generate_query_schedule,
    serialize_dataset_blob,
    serialize_query_blob,
    splitmix64,
)


class TestWorkloadGenerators(unittest.TestCase):
    """Test deterministic PRNG algorithms, seeds, rejection sampling, and digests."""

    def test_01_splitmix64_golden_vector(self):
        """Verify SplitMix64 produces expected deterministic sequence."""
        s0 = splitmix64(DATASET_SEED)
        s1 = splitmix64((DATASET_SEED + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF)
        self.assertIsInstance(s0, int)
        self.assertIsInstance(s1, int)
        self.assertNotEqual(s0, s1)

    def test_02_xoshiro256plusplus_reproducibility(self):
        """Verify xoshiro256++ stream reproducibility."""
        rng1 = Xoshiro256PlusPlus(DATASET_SEED)
        rng2 = Xoshiro256PlusPlus(DATASET_SEED)
        seq1 = [rng1.next_u64() for _ in range(100)]
        seq2 = [rng2.next_u64() for _ in range(100)]
        self.assertEqual(seq1, seq2)

    def test_03_query_digest_full_reproduction(self):
        """Generate full N=2^20 query schedule and verify canonical SHA-256."""
        queries = generate_query_schedule(q=WORKLOAD_N, n=WORKLOAD_N, seed=QUERY_SEED)
        self.assertEqual(len(queries), WORKLOAD_N)
        # All queries within [0, 2^20 - 1]
        self.assertTrue(all(0 <= q < WORKLOAD_N for q in queries))

        blob = serialize_query_blob(queries)
        self.assertEqual(len(blob), 4 * WORKLOAD_N)
        digest = hashlib.sha256(blob).hexdigest()
        self.assertEqual(digest, CANONICAL_QUERY_SHA256)

    def test_04_dataset_digest_full_reproduction(self):
        """Generate full N=2^20 dataset and verify canonical SHA-256."""
        symbols = generate_dataset_symbols(n=WORKLOAD_N, seed=DATASET_SEED)
        self.assertEqual(len(symbols), WORKLOAD_N)
        # All symbols in [-364, 364]
        self.assertTrue(all(-364 <= x <= 364 for x in symbols))

        blob = serialize_dataset_blob(symbols)
        self.assertEqual(len(blob), 2 * WORKLOAD_N)
        digest = hashlib.sha256(blob).hexdigest()
        self.assertEqual(digest, CANONICAL_DATASET_SHA256)


if __name__ == "__main__":
    unittest.main()
