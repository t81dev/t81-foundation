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
    if tuple(catalog) != CATALOG:
        raise ContractError("INVALID_CATALOG")
    if len(values) != 4:
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
    if tuple(catalog) != CATALOG:
        raise ContractError("INVALID_CATALOG")


def check_trits(values):
    if len(values) != 4:
        raise ContractError("INVALID_LENGTH")
    if any(type(v) is not int or v not in TRITS for v in values):
        raise ContractError("INVALID_TRIT")


def encode_bytes(values):
    check_trits(values)
    return bytes(v + 1 for v in values)


def decode_bytes(payload):
    if len(payload) != 4:
        raise ContractError("INVALID_LENGTH")
    if any(type(v) is not int or v not in (0, 1, 2) for v in payload):
        raise ContractError("INVALID_ENCODING")
    return tuple(v - 1 for v in payload)


def filter_bytes(payload, catalog=CATALOG):
    check_catalog(catalog)
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
    check_catalog(catalog)
    decode_swar(payload)
    result = []
    for i, pattern in enumerate(catalog):
        target = 0
        for j, v in enumerate(pattern):
            target |= (v + 1) << (2*j)
        if ((payload ^ target) & 255) == 0:
            result.append(i)
    return result


def expect_error(code, func, *args):
    try:
        func(*args)
    except ContractError as exc:
        assert exc.code == code, (code, exc.code)
        return
    raise AssertionError("Expected " + code)


def run():
    domain = tuple(itertools.product(TRITS, repeat=4))
    assert len(domain) == 81 and CATALOG == domain[:40]
    assert len(set(CATALOG)) == 40
    observations = []
    bytes_seen, swar_seen = set(), set()
    decisions = 0
    for values in domain:
        expected = oracle(values)
        b = encode_bytes(values)
        s = encode_swar(values)
        assert decode_bytes(b) == values
        assert decode_swar(s) == values
        assert filter_bytes(b) == expected
        assert filter_swar(s) == expected
        for i, pattern in enumerate(CATALOG):
            assert (i in expected) == (values == pattern)
            decisions += 1
        bytes_seen.add(b)
        swar_seen.add(s)
        observations.append({"input": values, "matches": expected, "byte": list(b), "swar": s})
    assert len(bytes_seen) == len(swar_seen) == 81
    malformed = 0
    for position in range(4):
        for code in (3, 7, 255):
            b = [0, 0, 0, 0]
            b[position] = code
            expect_error("INVALID_ENCODING", decode_bytes, b)
            expect_error("INVALID_ENCODING", filter_bytes, b)
            malformed += 2
        for payload in range(256):
            if ((payload >> (position*2)) & 3) == 3:
                expect_error("INVALID_ENCODING", decode_swar, payload)
                expect_error("INVALID_ENCODING", filter_swar, payload)
                malformed += 2
    for values in ((), (-1,), (-1, 0, 1, 0, 1)):
        for func in (oracle, encode_bytes, encode_swar, decode_bytes, filter_bytes):
            expect_error("INVALID_LENGTH", func, values)
            malformed += 1
    for position in range(4):
        for bad in (-2, 2, True):
            values = [0, 0, 0, 0]
            values[position] = bad
            for func in (oracle, encode_bytes, encode_swar):
                expect_error("INVALID_TRIT", func, values)
                malformed += 1
    for catalog in (CATALOG[:-1], tuple(reversed(CATALOG)), CATALOG[:-1] + (CATALOG[0],)):
        expect_error("INVALID_CATALOG", oracle, (0, 0, 0, 0), catalog)
        expect_error("INVALID_CATALOG", filter_bytes, b"\x01"*4, catalog)
        expect_error("INVALID_CATALOG", filter_swar, 85, catalog)
        malformed += 3
    for bad in (-1, 256, 3.0, True):
        expect_error("INVALID_ENCODING", decode_swar, bad)
        expect_error("INVALID_ENCODING", filter_swar, bad)
        malformed += 2
    raw = json.dumps(observations, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return {
        "protocol": "T81-AIOPT-001-M1-v0.1.0",
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
