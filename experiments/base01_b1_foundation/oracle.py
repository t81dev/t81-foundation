"""B0 independent positional oracle, retained verbatim from accepted 91e3a7d7.

Only digest, canonical and verify_codec are copied; no runner or measurement code.
"""
import hashlib
import json


def digest(data):
    return hashlib.sha256(data).hexdigest()


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode()


def verify_codec(rows):
    expected = [-(2**63), -(2**63)+1, 2**63-2, 2**63-1, 0, 1, -1]
    seed = 0x424153453031
    for _ in range(1024):
        seed = (seed*6364136223846793005 + 1442695040888963407) % (2**64)
        magnitude = seed & (2**63-1)
        expected.append(-magnitude if seed >> 63 else magnitude)
    if [r[0] for r in rows] != expected:
        raise ValueError("codec input identity mismatch")
    for x, back, digits in rows:
        # Independent arbitrary-precision positional oracle, not candidate decode.
        if (not digits or any(d not in (-1, 0, 1) for d in digits) or
                (len(digits)>1 and digits[-1]==0) or
                sum(d * 3**i for i, d in enumerate(digits)) != x or back != x):
            raise ValueError("codec independent oracle failed")
    return digest(canonical(expected))
