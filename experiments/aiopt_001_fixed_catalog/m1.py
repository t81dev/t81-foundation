"""T81-AIOPT-001 M1: isolated, correctness-only exhaustive runner.

Run: python3 experiments/aiopt_001_fixed_catalog/m1.py
No external dependencies. No timing or optimization claims.
"""
import hashlib
import itertools
import json
import platform

TRITS = (-1, 0, 1)
CATALOG = tuple(itertools.product(TRITS, repeat=4))[:40]


class ContractError(ValueError):
    def __init__(self, code):
        self.code = code
        super().__init__(code)


# Independent semantic oracle: no encoding or adapter helper calls.
def oracle(values, catalog=CATALOG):
    catalog = check_catalog(catalog)
    if type(values) not in (tuple, list) or len(values) != 4:
        raise ContractError("INVALID_LENGTH")
    if any(type(v) is not int or v not in (-1, 0, 1) for v in values):
        raise ContractError("INVALID_TRIT")
    matches = []
    for index, pattern in enumerate(catalog):
        equal = True
        for j in range(4):
            if values[j] != pattern[j]:
                equal = False
        if equal:
            matches.append(index)
    return matches


def check_catalog(catalog):
    # Validate every row before evaluation; Python bool/float equality must not
    # admit non-trits, and one-shot iterables must not be consumed silently.
    if type(catalog) not in (tuple, list) or len(catalog) != 40:
        raise ContractError("INVALID_CATALOG")
    normalized = []
    for pattern in catalog:
        if type(pattern) not in (tuple, list) or len(pattern) != 4:
            raise ContractError("INVALID_CATALOG")
        if any(type(v) is not int or v not in TRITS for v in pattern):
            raise ContractError("INVALID_CATALOG")
        normalized.append(tuple(pattern))
    normalized = tuple(normalized)
    if normalized != CATALOG:
        raise ContractError("INVALID_CATALOG")
    return normalized


def check_trits(values):
    if type(values) not in (tuple, list) or len(values) != 4:
        raise ContractError("INVALID_LENGTH")
    if any(type(v) is not int or v not in TRITS for v in values):
        raise ContractError("INVALID_TRIT")


def encode_bytes(values):
    check_trits(values)
    return bytes(v + 1 for v in values)


def decode_bytes(payload):
    if type(payload) not in (bytes, bytearray, tuple, list):
        raise ContractError("INVALID_ENCODING")
    if len(payload) != 4:
        raise ContractError("INVALID_LENGTH")
    if any(type(v) is not int or v not in (0, 1, 2) for v in payload):
        raise ContractError("INVALID_ENCODING")
    return tuple(v - 1 for v in payload)


def filter_bytes(payload, catalog=CATALOG):
    catalog = check_catalog(catalog)
    decode_bytes(payload)
    result = []
    for i, pattern in enumerate(catalog):
        if all(payload[j] == pattern[j] + 1 for j in range(4)):
            result.append(i)
    return result


def encode_swar(values):
    check_trits(values)
    packed = 0
    for j, v in enumerate(values):
        packed |= (v + 1) << (2*j)
    return packed


def decode_swar(payload):
    if type(payload) is not int or not 0 <= payload <= 255:
        raise ContractError("INVALID_ENCODING")
    codes = tuple((payload >> (2*j)) & 3 for j in range(4))
    if 3 in codes:
        raise ContractError("INVALID_ENCODING")
    return tuple(v - 1 for v in codes)


def filter_swar(payload, catalog=CATALOG):
    catalog = check_catalog(catalog)
    decode_swar(payload)
    result = []
    for i, pattern in enumerate(catalog):
        target = 0
        for j, v in enumerate(pattern):
            target |= (v + 1) << (2*j)
        if ((payload ^ target) & 255) == 0:
            result.append(i)
    return result


def require(condition, detail=None):
    """Always-active correctness assertion, including under python -O."""
    if not condition:
        raise AssertionError(detail)


def expect_error(code, func, *args):
    try:
        func(*args)
    except ContractError as exc:
        require(exc.code == code, (code, exc.code))
        return 1
    raise AssertionError("Expected " + code)


def run():
    domain = tuple(itertools.product(TRITS, repeat=4))
    require(len(domain) == 81 and CATALOG == domain[:40])
    require(len(set(CATALOG)) == 40)
    observations = []
    bytes_seen, swar_seen = set(), set()
    decisions = 0
    for values in domain:
        expected = oracle(values)
        b = encode_bytes(values)
        s = encode_swar(values)
        require(decode_bytes(b) == values)
        require(decode_swar(s) == values)
        require(filter_bytes(b) == expected)
        require(filter_swar(s) == expected)
        for i, pattern in enumerate(CATALOG):
            require((i in expected) == (values == pattern))
            decisions += 1
        bytes_seen.add(b)
        swar_seen.add(s)
        observations.append({"input": values, "matches": expected, "byte": list(b), "swar": s})
    require(len(bytes_seen) == len(swar_seen) == 81)
    malformed = 0
    for position in range(4):
        for code in (3, 7, 255):
            b = [0, 0, 0, 0]
            b[position] = code
            malformed += expect_error("INVALID_ENCODING", decode_bytes, b)
            malformed += expect_error("INVALID_ENCODING", filter_bytes, b)
        for payload in range(256):
            if ((payload >> (position*2)) & 3) == 3:
                malformed += expect_error("INVALID_ENCODING", decode_swar, payload)
                malformed += expect_error("INVALID_ENCODING", filter_swar, payload)
    for values in ((), (-1,), (-1, 0, 1, 0, 1)):
        for func in (oracle, encode_bytes, encode_swar, decode_bytes, filter_bytes):
            malformed += expect_error("INVALID_LENGTH", func, values)
    for position in range(4):
        for bad in (-2, 2, True):
            values = [0, 0, 0, 0]
            values[position] = bad
            for func in (oracle, encode_bytes, encode_swar):
                malformed += expect_error("INVALID_TRIT", func, values)
    for catalog in (CATALOG[:-1], tuple(reversed(CATALOG)), CATALOG[:-1] + (CATALOG[0],)):
        malformed += expect_error("INVALID_CATALOG", oracle, (0, 0, 0, 0), catalog)
        malformed += expect_error("INVALID_CATALOG", filter_bytes, b"\x01"*4, catalog)
        malformed += expect_error("INVALID_CATALOG", filter_swar, 85, catalog)
    for bad in (-1, 256, 3.0, True):
        malformed += expect_error("INVALID_ENCODING", decode_swar, bad)
        malformed += expect_error("INVALID_ENCODING", filter_swar, bad)
    raw = json.dumps(observations, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return {
        "protocol": "T81-AIOPT-001-M1-v0.1.1",
        "inputs": 81,
        "catalog_size": 40,
        "predicate_decisions": decisions,
        "adapter_oracle_filter_comparisons": 162,
        "roundtrips": 162,
        "malformed_assertions": malformed,
        "sha256_results": hashlib.sha256(raw).hexdigest(),
        "python": platform.python_version(),
        "platform": platform.platform(),
        "status": "LOCAL_TESTS_PASS",
    }


if __name__ == "__main__":
    print(json.dumps(run(), sort_keys=True, separators=(",", ":")))
