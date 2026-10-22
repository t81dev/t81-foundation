"""RFC-00D5 draft reference only. No production imports or host float arithmetic.

This is an executable proposal, not a replacement for T81Float conversion.
"""
from fractions import Fraction
import re

MODES = ("exact_only", "nearest_even")


def result(value=None, status="error", error=None, flags=()):
    return {"status": status, "value": value, "error": error, "flags": list(flags)}


def validate(m, e, mode):
    if type(m) is not int or type(e) is not int or not 4 <= m <= 72 or not 4 <= e <= 9:
        return "unsupported_format"
    if mode not in MODES:
        return "unsupported_mode"
    return None


def radix_power(radix, exponent):
    return Fraction(radix**exponent) if exponent >= 0 else Fraction(1, radix**-exponent)


def decode_binary64(bits):
    if not isinstance(bits, str) or re.fullmatch(r"[0-9a-f]{16}", bits) is None:
        return None, False, "invalid_encoding"
    raw = int(bits, 16)
    negative, e, fraction = raw >> 63, (raw >> 52) & 2047, raw & ((1 << 52) - 1)
    if e == 2047:
        return None, False, "nonfinite_input"
    significand = fraction if e == 0 else fraction + (1 << 52)
    value = significand * radix_power(2, -1074 if e == 0 else e - 1075)
    return -value if negative else value, bool(negative and not significand), None


def trit_text(magnitude, width):
    digits = []
    for _ in range(width):
        magnitude, digit = divmod(magnitude, 3)
        if digit == 2:
            magnitude += 1
            digit = -1
        digits.append({-1: "-", 0: "0", 1: "+"}[digit])
    if magnitude:
        raise ValueError("unrepresentable mantissa")
    return "".join(reversed(digits))


def decode_ternary(text, m, e):
    if text in ("NaE", "+Inf", "-Inf"):
        return None, "nonfinite_input"
    if text == "+0E0":
        return Fraction(0), None
    if text == "-0E0":
        return None, "noncanonical_encoding"
    if not isinstance(text, str):
        return None, "invalid_encoding"
    match = re.fullmatch(r"([+-])([+0-]{" + str(m) + r"})E(-?[0-9]+)", text)
    if not match:
        return None, "invalid_encoding"
    sign, digits, exponent = match.groups()
    # Compare bounded spelling before parsing arbitrary-length untrusted digits.
    if len(exponent) > 6:
        return None, "invalid_encoding"
    exp = int(exponent)
    if str(exp) != exponent:
        return None, "noncanonical_encoding"
    bias = (3**e - 1) // 2
    if not -bias <= exp < bias:
        return None, "invalid_encoding"
    magnitude = 0
    for digit in digits:
        magnitude = 3 * magnitude + {"+": 1, "0": 0, "-": -1}[digit]
    if magnitude <= 0 or (exp != -bias and digits[0] != "+"):
        return None, "noncanonical_encoding"
    value = magnitude * radix_power(3, exp - m + 1)
    return -value if sign == "-" else value, None


def quantize_rational(value, m, e, mode="exact_only", negative_zero=False):
    """Mathematical helper for corpus tests; not an additional public API."""
    error = validate(m, e, mode)
    if error:
        return result(error=error)
    value = Fraction(value)
    if not value:
        return result("+0E0", "exact", flags=("negative_zero_normalized",) if negative_zero else ())
    x = abs(value)
    bias, maximum = (3**e - 1) // 2, (3**m - 1) // 2
    low, high = -bias, bias - 1
    if x < radix_power(3, low - m + 1):
        return result(error="underflow")
    if x > maximum * radix_power(3, high - m + 1):
        return result(error="overflow")
    minimum = (3**(m - 1) + 1) // 2
    exp = 0
    while x > maximum * radix_power(3, exp - m + 1):
        exp += 1
    while x <= maximum * radix_power(3, exp - m):
        exp -= 1
    candidates = []
    for exponent in range(max(low, exp - 1), min(high, max(low, exp) + 1) + 1):
        unit = radix_power(3, exponent - m + 1)
        scaled = x / unit
        floor = scaled.numerator // scaled.denominator
        lower = 1 if exponent == low else minimum
        for mantissa in {lower, maximum, max(lower, min(maximum, floor)), max(lower, min(maximum, floor + 1))}:
            candidates.append((abs(x - mantissa * unit), mantissa % 2, exponent, mantissa))
    distance, _, exponent, mantissa = min(candidates)
    if distance and mode == "exact_only":
        return result(error="inexact")
    text = ("-" if value < 0 else "+") + trit_text(mantissa, m) + f"E{exponent}"
    return result(text, "rounded" if distance else "exact")


def from_binary64(bits, m=27, e=9, mode="exact_only"):
    error = validate(m, e, mode)
    if error:
        return result(error=error)
    value, negative_zero, error = decode_binary64(bits)
    if error:
        return result(error=error)
    return quantize_rational(value, m, e, mode, negative_zero)


def to_binary64(text, m=27, e=9, mode="exact_only"):
    error = validate(m, e, mode)
    if error:
        return result(error=error)
    value, error = decode_ternary(text, m, e)
    if error:
        return result(error=error)
    return binary64_from_rational(value, mode)


def binary64_from_rational(value, mode="exact_only"):
    """Integer quotient/remainder rounding to IEEE binary64 encodings."""
    if mode not in MODES:
        return result(error="unsupported_mode")
    value = Fraction(value)
    if not value:
        return result("0000000000000000", "exact")
    x = abs(value)
    if x < Fraction(1, 2**1074):
        return result(error="underflow")
    if x > (2**53 - 1) * 2**971:
        return result(error="overflow")
    exponent = x.numerator.bit_length() - x.denominator.bit_length()
    if x < radix_power(2, exponent):
        exponent -= 1
    subnormal = exponent < -1022
    unit = radix_power(2, -1074 if subnormal else exponent - 52)
    scaled = x / unit
    q, remainder = divmod(scaled.numerator, scaled.denominator)
    if remainder and mode == "exact_only":
        return result(error="inexact")
    if 2 * remainder > scaled.denominator or (2 * remainder == scaled.denominator and q % 2):
        q += 1
    if subnormal:
        raw = q  # q == 2**52 naturally encodes the minimum normal.
    else:
        if q == 2**53:
            exponent += 1
            q //= 2
        raw = ((exponent + 1023) << 52) | (q - 2**52)
    if value < 0:
        raw |= 1 << 63
    return result(f"{raw:016x}", "rounded" if remainder else "exact")


def evaluate(case):
    """Strictly evaluate one machine-readable conformance request."""
    op = case["operation"]
    m, e, mode = case.get("M", 27), case.get("E", 9), case.get("mode", "exact_only")
    if op == "from_binary64":
        return from_binary64(case["input"], m, e, mode)
    if op == "to_binary64":
        return to_binary64(case["input"], m, e, mode)
    if op == "quantize_rational":
        return quantize_rational(Fraction(int(case["numerator"]), int(case["denominator"])), m, e, mode)
    raise ValueError("unsupported reference operation")
