"""Exact rational binary32 oracle; never calls native/production conversion."""
from fractions import Fraction
import struct

SCALE = 3**80


def rational(bits):
    value = struct.unpack('>f', bits.to_bytes(4, 'big'))[0]
    # Every binary32 value is exactly representable in the Python binary64 value.
    if (bits >> 23) & 255 == 255:
        return None
    numerator, denominator = value.as_integer_ratio()
    return Fraction(numerator, denominator)


def convert(bits):
    value = rational(bits)
    if value is None:
        return dict(status='nonfinite', packed=None, raw_integer=None)
    scaled = value * SCALE
    magnitude = abs(scaled.numerator) // scaled.denominator
    q = -magnitude if scaled < 0 else magnitude
    require = (3**192-1)//2
    if abs(q) > require:
        raise ValueError('Oracle range exceeded')
    n, digits = q, []
    for _ in range(192):
        n, r = divmod(n, 3)
        if r == 2:
            r = -1
            n += 1
        digits.append(r)
    if n:
        raise ValueError('Unrepresented high digits')
    raw = bytes(sum((digits[i+j]+1) << (2*j) for j in range(4)) for i in range(0,192,4))
    # Independent decoding and quantization-cell check, not a native self-report.
    if sum(t*3**i for i,t in enumerate(digits)) != q:
        raise ValueError('Oracle encoding check')
    if not abs(Fraction(q,SCALE)-value) < Fraction(1,SCALE):
        raise ValueError('Quantization error bound')
    return dict(status='ok', packed=raw.hex(), raw_integer=str(q))
