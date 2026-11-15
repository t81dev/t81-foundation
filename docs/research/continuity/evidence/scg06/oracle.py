"""
Independent Exact-Integer Mathematical Oracle for Balanced Ternary Operations.
Uses Python's arbitrary-precision integers with zero dependencies on T81 C++ types.
"""

def trits_to_int(trits):
    """Convert a sequence of trits (little-endian, -1, 0, 1) to integer."""
    val = 0
    p = 1
    for t in trits:
        assert t in (-1, 0, 1), f"Invalid trit: {t}"
        val += t * p
        p *= 3
    return val

def trytes_to_int(trytes):
    """Convert a sequence of trytes (little-endian, -13..13) to integer."""
    val = 0
    p = 1
    for ty in trytes:
        assert -13 <= ty <= 13, f"Invalid tryte: {ty}"
        val += ty * p
        p *= 27
    return val

def int_to_trits(val, num_trits):
    """Convert integer to balanced ternary trits (little-endian), returns (trits, overflow)."""
    trits = []
    rem = val
    for _ in range(num_trits):
        m = rem % 3
        if m == 2:
            t = -1
            rem = (rem + 1) // 3
        elif m == 1:
            t = 1
            rem = rem // 3
        else:
            t = 0
            rem = rem // 3
        trits.append(t)
    overflow = (rem != 0)
    return trits, overflow

def int_to_trytes(val, num_trytes):
    """Convert integer to balanced ternary trytes (little-endian), returns (trytes, overflow)."""
    trits, overflow = int_to_trits(val, num_trytes * 3)
    trytes = []
    for i in range(num_trytes):
        chunk = trits[i*3:(i+1)*3]
        ty = chunk[0] + chunk[1]*3 + chunk[2]*9
        trytes.append(ty)
    return trytes, overflow

def exact_mul(val_a, val_b):
    return val_a * val_b

def exact_add(val_a, val_b):
    return val_a + val_b

def exact_sub(val_a, val_b):
    return val_a - val_b

def trits_to_int_allow_nontrits(digits):
    """Compute mathematical value sum(d_i * 3^i) without asserting balanced ternary canonical range."""
    val = 0
    p = 1
    for d in digits:
        val += d * p
        p *= 3
    return val
