"""Exact audit reference, deliberately independent of T81 and host libm.

Profile and unresolved normative/API conflict are documented in README.md.
An inexact/invalid/range result is an error, not numerical success. A nearest
candidate is returned for diagnosis only. All computation uses Python integers
and Fraction after decomposing the caller's exact 64-bit encoding.
"""
from fractions import Fraction
import re


def binary64(hex_bits):
    if not isinstance(hex_bits, str) or re.fullmatch(r"[0-9a-fA-F]{16}", hex_bits) is None:
        raise ValueError("expected exactly 16 hexadecimal digits")
    bits = int(hex_bits, 16)
    sign, exponent, fraction = bits >> 63, (bits >> 52) & 2047, bits & ((1 << 52) - 1)
    if exponent == 2047:
        return ("nan" if fraction else "infinity"), None
    significand = fraction if exponent == 0 else (1 << 52) + fraction
    shift = -1074 if exponent == 0 else exponent - 1023 - 52
    magnitude = Fraction(significand << shift) if shift >= 0 else Fraction(significand, 1 << -shift)
    return "finite", -magnitude if sign else magnitude


def power3(exponent):
    return Fraction(3**exponent) if exponent >= 0 else Fraction(1, 3**-exponent)


def trits(integer, width):
    if integer < 0 or integer > (3**width - 1) // 2:
        raise ValueError("mantissa outside balanced storage")
    digits = []
    for _ in range(width):
        integer, digit = divmod(integer, 3)
        if digit == 2:
            digit, integer = -1, integer + 1
        digits.append({-1: "-", 0: "0", 1: "+"}[digit])
    if integer:
        raise ValueError("mantissa overflow")
    return "".join(reversed(digits))


def quantize(value, mantissa=27, exponent_trits=9):
    """Nearest finite balanced representation; ties prefer even mantissa.

    Finite normals have leading positive mantissa trit. Gradual subnormals use
    exponent -bias. Out-of-range nonzero inputs are errors, never silent zero or
    infinity. This checked audit profile does not amend the production API.
    """
    if type(mantissa) is not int or type(exponent_trits) is not int or not 4 <= mantissa <= 72 or not 4 <= exponent_trits <= 9:
        raise ValueError("unsupported reference format")
    value = Fraction(value)
    if not value:
        return {"status": "exact", "candidate": "+0E0"}
    negative, x = value < 0, abs(value)
    bias = (3**exponent_trits - 1) // 2
    low, high = -bias, bias - 1
    maximum = (3**mantissa - 1) // 2
    minimum_normal = (3**(mantissa - 1) + 1) // 2
    if x < power3(low - mantissa + 1):
        return {"status": "underflow", "candidate": None}
    if x > maximum * power3(high - mantissa + 1):
        return {"status": "overflow", "candidate": None}
    # Find the first exponent whose maximum mantissa covers x. No log/floor/libm.
    exponent = 0
    while x > maximum * power3(exponent - mantissa + 1):
        exponent += 1
    while x <= maximum * power3(exponent - mantissa):
        exponent -= 1
    candidates = []
    for e in range(max(low, exponent - 1), min(high, max(low, exponent) + 1) + 1):
        unit = power3(e - mantissa + 1)
        scaled = x / unit
        floor = scaled.numerator // scaled.denominator
        lower = 1 if e == low else minimum_normal
        # Include adjacent binade endpoints: spacing changes across them.
        for m in {lower, maximum, max(lower, min(maximum, floor)), max(lower, min(maximum, floor + 1))}:
            candidates.append((abs(x - m * unit), m % 2, e, m))
    error, _, e, m = min(candidates)
    return {"status": "exact" if not error else "inexact",
            "candidate": ("-" if negative else "+") + trits(m, mantissa) + f"E{e}"}


def reference(hex_bits):
    kind, value = binary64(hex_bits)
    if kind != "finite":
        return {"status": "invalid", "candidate": None}
    return quantize(value)


def decode(text, width=27, exponent_trits=9):
    """Parse a finite production encoding to its exact rational value."""
    if text == "+0E0":
        return Fraction(0)
    match = re.fullmatch(r"([+-])([+0-]{" + str(width) + r"})E(-?[0-9]+)", text)
    if not match:
        raise ValueError("not a finite canonical encoding")
    sign, digits, exp = match.groups()
    exponent = int(exp)
    bias = (3**exponent_trits - 1) // 2
    if not -bias <= exponent < bias or (digits[0] != "+" and exponent != -bias):
        raise ValueError("not normalized")
    integer = 0
    for digit in digits:
        integer = integer * 3 + {"+": 1, "0": 0, "-": -1}[digit]
    if integer <= 0:
        raise ValueError("nonpositive mantissa")
    return (-1 if sign == "-" else 1) * integer * power3(exponent - width + 1)
