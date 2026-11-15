"""Pinned XXH64 production wrapper. No silent fallback to another algorithm."""
import hashlib
from importlib.metadata import version
from pathlib import Path
import json

PACKAGE_VERSION = "3.5.0"
LIBRARY_VERSION = "0.8.2"
SEED = 0
# Upstream test_xxh64.py at python-xxhash v3.5.0, plus its README example.
KNOWN_ANSWERS = ((b"a", 0, 15154266338359012955),
                 (b"a", 1, 16051599287423682246),
                 (b"a", (1 << 64) - 1, 6972758980737027682),
                 (b"xxhash", 0, 0x32dd38952c4bc720))


def checked_backend():
    import xxhash
    if version("xxhash") != PACKAGE_VERSION or xxhash.VERSION != PACKAGE_VERSION or xxhash.XXHASH_VERSION != LIBRARY_VERSION:
        raise RuntimeError("xxhash package/library identity mismatch")
    for data, seed, expected in KNOWN_ANSWERS:
        if xxhash.xxh64(data, seed=seed).intdigest() != expected:
            raise RuntimeError("xxHash64 known-answer failure")
    return xxhash


def checksum_factory():
    """Check versions/KATs in preflight, then call the returned factory in Phase 7."""
    backend = checked_backend()
    return lambda: backend.xxh64(seed=SEED)


def witness_record():
    path = Path(__file__).with_name("checksum_witness.v1.json")
    record = json.loads(path.read_text())
    from .workload import CANONICAL_DATASET_SHA256, WORKLOAD_N
    if (record.get("schema") != "t81.h04-r03.checksum-witness.v1" or record.get("seed") != SEED
            or record.get("n") != WORKLOAD_N or record.get("dataset_sha256") != CANONICAL_DATASET_SHA256
            or record.get("output_serialization") != "signed-int64-little-endian-dataset-order"
            or not isinstance(record.get("xxh64"), str) or len(record["xxh64"]) != 16
            or any(c not in "0123456789abcdef" for c in record["xxh64"])):
        raise RuntimeError("checksum witness configuration mismatch")
    source = path.with_name("checksum_witness.py").read_bytes()
    if hashlib.sha256(source).hexdigest() != record["witness_source_sha256"]:
        raise RuntimeError("checksum witness source identity mismatch")
    return record


def backend_identity():
    backend = checked_backend()
    import xxhash._xxhash as extension
    binary = Path(extension.__file__)
    return {"package_version": backend.VERSION, "library_version": backend.XXHASH_VERSION,
            "extension_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
            "seed": SEED, "witness": witness_record()}
