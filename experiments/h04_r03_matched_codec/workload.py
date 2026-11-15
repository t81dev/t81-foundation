"""Deterministic dataset and query schedule generators for EXP-H04-R03.

Enforces frozen PRNG algorithms from H04_R03_CANDIDATE_A_PREREGISTRATION.md Section 5:
- 64-bit SplitMix64 state initialization.
- 64-bit xoshiro256++ transition function.
- Dataset Seed: 0x5438315F44415441 ("T81_DATA")
  Unbiased rejection sampling with LIMIT_729 = 18446744073709550922
  Canonical Serialization: little-endian signed 16-bit integers (<h)
  Canonical SHA-256: 1188f12cb887f8ad044d970ca92e7e2f0125e57a5fda012606407da857cc2879
- Query Seed: 0x5438315F51554552 ("T81_QUER")
  Direct masking (next_u64() & 0xFFFFF) for uniform draws with replacement over [0, 2^20 - 1]
  Canonical Serialization: little-endian unsigned 32-bit integers (<I)
  Canonical SHA-256: 02e5f3da8ad331fd8b64197884933ae11211974ce719e0354f2fe3321002938e
"""
import hashlib
import struct
from typing import List, Tuple

# Frozen Seed Constants
DATASET_SEED = 0x5438315F44415441  # ASCII "T81_DATA"
QUERY_SEED = 0x5438315F51554552    # ASCII "T81_QUER"

# Unbiased Rejection Threshold
# 2^64 = 729 * 25302403348724534 + 694
# Rejection threshold = 2^64 - 694 = 18446744073709550922
LIMIT_729 = 18446744073709550922

# Preregistered Workload Sizing
WORKLOAD_N = 1048576  # 2^20
WORKLOAD_K = 64
WORKLOAD_A = 1
WORKLOAD_ACCESS_MODE = "RA-FIXED"

# Expected Canonical SHA-256 Digests
CANONICAL_DATASET_SHA256 = "1188f12cb887f8ad044d970ca92e7e2f0125e57a5fda012606407da857cc2879"
CANONICAL_QUERY_SHA256 = "02e5f3da8ad331fd8b64197884933ae11211974ce719e0354f2fe3321002938e"


def splitmix64(z: int) -> int:
    """Deterministic 64-bit SplitMix64 generator."""
    z = (z + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9 & 0xFFFFFFFFFFFFFFFF
    z = (z ^ (z >> 27)) * 0x94D049BB133111EB & 0xFFFFFFFFFFFFFFFF
    return z ^ (z >> 31)


def rotl64(x: int, k: int) -> int:
    """64-bit rotate-left operation."""
    return ((x << k) | (x >> (64 - k))) & 0xFFFFFFFFFFFFFFFF


class Xoshiro256PlusPlus:
    """Deterministic xoshiro256++ PRNG seeded via SplitMix64."""

    def __init__(self, seed: int):
        sm = seed
        self.s0 = splitmix64(sm)
        sm = (sm + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
        self.s1 = splitmix64(sm)
        sm = (sm + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
        self.s2 = splitmix64(sm)
        sm = (sm + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
        self.s3 = splitmix64(sm)

    def next_u64(self) -> int:
        """Advance PRNG state and return next unsigned 64-bit integer."""
        res = (rotl64((self.s0 + self.s3) & 0xFFFFFFFFFFFFFFFF, 23) + self.s0) & 0xFFFFFFFFFFFFFFFF
        t = (self.s1 << 17) & 0xFFFFFFFFFFFFFFFF
        self.s2 ^= self.s0
        self.s3 ^= self.s1
        self.s1 ^= self.s2
        self.s0 ^= self.s3
        self.s2 ^= t
        self.s3 = rotl64(self.s3, 45)
        return res


def generate_dataset_symbols(n: int = WORKLOAD_N, seed: int = DATASET_SEED) -> List[int]:
    """Generate n discrete uniform symbols x in [-364, 364] using xoshiro256++."""
    rng = Xoshiro256PlusPlus(seed)
    symbols: List[int] = []
    while len(symbols) < n:
        val = rng.next_u64()
        if val < LIMIT_729:
            u = val % 729
            symbols.append(u - 364)
    return symbols


def serialize_dataset_blob(symbols: List[int]) -> bytes:
    """Pack dataset symbols as little-endian signed 16-bit integers (<h)."""
    return struct.pack(f"<{len(symbols)}h", *symbols)


def deserialize_dataset_blob(blob: bytes) -> List[int]:
    """Unpack dataset blob from little-endian signed 16-bit integers (<h)."""
    n = len(blob) // 2
    return list(struct.unpack(f"<{n}h", blob))


def generate_query_schedule(q: int = WORKLOAD_N, n: int = WORKLOAD_N, seed: int = QUERY_SEED) -> List[int]:
    """Generate q discrete uniform indices in [0, n-1] drawn with replacement."""
    if n != 1048576:
        raise ValueError(f"Workload schedule requires N=2^20 (1048576), got {n}")
    rng = Xoshiro256PlusPlus(seed)
    # Masking next_u64() & 0xFFFFF is mathematically unbiased for 2^20
    return [rng.next_u64() & 0xFFFFF for _ in range(q)]


def serialize_query_blob(queries: List[int]) -> bytes:
    """Pack query schedule as little-endian unsigned 32-bit integers (<I)."""
    return struct.pack(f"<{len(queries)}I", *queries)


def deserialize_query_blob(blob: bytes) -> List[int]:
    """Unpack query schedule from little-endian unsigned 32-bit integers (<I)."""
    q = len(blob) // 4
    return list(struct.unpack(f"<{q}I", blob))


def verify_canonical_dataset_digest(symbols: List[int]) -> Tuple[bool, str]:
    """Verify SHA-256 digest of serialized dataset against canonical preregistered digest."""
    blob = serialize_dataset_blob(symbols)
    digest = hashlib.sha256(blob).hexdigest()
    return (digest == CANONICAL_DATASET_SHA256, digest)


def verify_canonical_query_digest(queries: List[int]) -> Tuple[bool, str]:
    """Verify SHA-256 digest of serialized query schedule against canonical preregistered digest."""
    blob = serialize_query_blob(queries)
    digest = hashlib.sha256(blob).hexdigest()
    return (digest == CANONICAL_QUERY_SHA256, digest)
