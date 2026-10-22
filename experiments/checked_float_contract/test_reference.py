"""Tests of the draft contract, not claims about production T81Float."""
from fractions import Fraction
import json
from pathlib import Path
import unittest

from reference import (binary64_from_rational, decode_ternary, evaluate,
                       from_binary64, quantize_rational, to_binary64)

HERE = Path(__file__).parent


def independent_value(text, m):
    if text == "+0E0":
        return Fraction(0)
    body, exponent = text[1:].split("E")
    mantissa = sum({"+": 1, "0": 0, "-": -1}[d] * 3**i for i, d in enumerate(reversed(body)))
    power = int(exponent) - m + 1
    magnitude = Fraction(mantissa * 3**power) if power >= 0 else Fraction(mantissa, 3**-power)
    return magnitude if text[0] == "+" else -magnitude


def independent_binary(bits):
    raw = int(bits, 16)
    exponent = ((raw >> 52) & 2047)
    significand = (raw & ((1 << 52) - 1)) + ((1 << 52) if exponent else 0)
    shift = exponent - 1075 if exponent else -1074
    value = Fraction(significand * 2**shift) if shift >= 0 else Fraction(significand, 2**-shift)
    return -value if raw >> 63 else value


class ContractReference(unittest.TestCase):
    def test_static_corpus(self):
        corpus = json.loads((HERE / "corpus.json").read_text())
        self.assertEqual(corpus["schema"], "t81.checked-float-conversion.draft1")
        self.assertEqual(len(corpus["cases"]), 104)
        ids = set()
        for case in corpus["cases"]:
            with self.subTest(case=case["id"]):
                self.assertNotIn(case["id"], ids)
                ids.add(case["id"])
                self.assertEqual(evaluate(case), case["expected"])
                expected = case["expected"]
                self.assertEqual(set(expected), {"value", "status", "error", "flags"})
                if expected["status"] == "error":
                    self.assertIsNone(expected["value"])
                    self.assertIsNotNone(expected["error"])
                else:
                    self.assertIsNone(expected["error"])

    def test_rounded_corpus_has_independent_neighbor_certificates(self):
        # Prove each static rounded expected value is no farther from the exact
        # source than either adjacent representable value; no conversion call.
        for case in json.loads((HERE / "corpus.json").read_text())["cases"]:
            if case["expected"]["status"] != "rounded":
                continue
            with self.subTest(case=case["id"]):
                op, output = case["operation"], case["expected"]["value"]
                m, e = case.get("M", 27), case.get("E", 9)
                if op == "to_binary64":
                    signed = independent_value(case["input"], m)
                    self.assertEqual(int(output, 16) >> 63, int(signed < 0))
                    x = abs(signed)
                    bits = int(output, 16) & ((1 << 63) - 1)
                    value = independent_binary(f"{bits:016x}")
                    neighbors = [independent_binary(f"{b:016x}") for b in (bits-1, bits+1)
                                 if 0 <= b <= 0x7fefffffffffffff]
                    self.assertTrue(all(abs(x-value) <= abs(x-n) for n in neighbors))
                    if any(abs(x-value) == abs(x-n) for n in neighbors):
                        self.assertEqual(bits % 2, 0)
                else:
                    signed = independent_binary(case["input"]) if op == "from_binary64" else Fraction(int(case["numerator"]), int(case["denominator"]))
                    self.assertEqual(output[0], "-" if signed < 0 else "+")
                    x = abs(signed)
                    mantissa_text, exponent_text = output[1:].split("E")
                    exponent = int(exponent_text)
                    mantissa = sum({"+":1,"0":0,"-":-1}[d]*3**i for i,d in enumerate(reversed(mantissa_text)))
                    bias, upper, lower = (3**e-1)//2, (3**m-1)//2, (3**(m-1)+1)//2
                    self.assertTrue(-bias <= exponent < bias)
                    self.assertTrue((1 if exponent == -bias else lower) <= mantissa <= upper)
                    candidates = [(mantissa, exponent)]
                    if mantissa > (1 if exponent == -bias else lower):
                        candidates.append((mantissa-1, exponent))
                    elif exponent > -bias:
                        candidates.append((upper, exponent-1))
                    if mantissa < upper:
                        candidates.append((mantissa+1, exponent))
                    elif exponent < bias-1:
                        candidates.append((lower, exponent+1))
                    def rank(pair):
                        n, exp = pair
                        unit = Fraction(3**(exp-m+1)) if exp-m+1 >= 0 else Fraction(1,3**(m-1-exp))
                        return abs(x-n*unit), n % 2, exp
                    self.assertEqual(min(candidates, key=rank), (mantissa, exponent))

    def test_exhaustive_small_lattice(self):
        # Enumerate every finite <4,4> magnitude independently of exponent search.
        encodings = []
        for exponent in range(-40, 40):
            unit = Fraction(3**(exponent-3)) if exponent >= 3 else Fraction(1,3**(3-exponent))
            for mantissa in range(1 if exponent == -40 else 14, 41):
                encodings.append((mantissa*unit, mantissa % 2, exponent, mantissa))
        inputs = [Fraction(i,8) for i in range(1,100)]
        for exponent in (-40,-39,-1,0,1,38):
            unit = Fraction(3**(exponent-3)) if exponent >= 3 else Fraction(1,3**(3-exponent))
            inputs += [Fraction(n,2)*unit for n in (79,80,81,82,83,84)]
        inputs = [x for x in inputs if encodings[0][0] <= x <= encodings[-1][0]]
        for x in inputs:
            value, _, exponent, mantissa = min(encodings, key=lambda row:(abs(x-row[0]),row[1],row[2]))
            output = quantize_rational(x,4,4,"nearest_even")
            actual = independent_value(output["value"],4)
            self.assertEqual(actual,value)
            self.assertEqual(quantize_rational(x,4,4)["status"], "exact" if x == value else "error")
            negative = quantize_rational(-x,4,4,"nearest_even")
            self.assertEqual(independent_value(negative["value"],4),-value)

    def test_binary_rounding_subnormal_and_carry(self):
        unit = Fraction(1,2**1074)
        # Nearest-even at subnormal precision and carry into the first normal.
        for value, bits in ((Fraction(3,2)*unit,"0000000000000002"),
                            (Fraction(5,2)*unit,"0000000000000002"),
                            (Fraction(2**53-1,2)*unit,"0010000000000000")):
            self.assertEqual(binary64_from_rational(value,"nearest_even")["value"], bits)
            self.assertEqual(binary64_from_rational(value)["error"], "inexact")
        self.assertEqual(binary64_from_rational(unit/2,"nearest_even")["error"], "underflow")
        self.assertEqual(binary64_from_rational((2**53-1)*2**971+1,"nearest_even")["error"], "overflow")

    def test_rounding_model_comparison_and_binade_ambiguity(self):
        # Model comparison at the same exact rational; away and truncation are
        # deliberate alternatives, not hidden host rounding choices.
        for value, even, away, truncation in ((Fraction(57,2),28,29,28),
                                              (Fraction(55,2),28,28,27),
                                              (Fraction(41),40,42,40)):
            self.assertEqual(independent_value(quantize_rational(value,4,4,"nearest_even")["value"],4), even)
            lattice = (27,28,29,40,42)
            self.assertEqual(min(lattice, key=lambda v:(abs(value-v),-v)),away)
            self.assertEqual(max(v for v in lattice if v <= value),truncation)
        # At odd radix binade ties, both endpoint mantissas have SAME parity.
        # Additional lower-exponent rule is necessary; "ties to even" is not enough.
        for m, value in ((4,41),(5,122)):
            upper, lower = (3**m-1)//2,(3**(m-1)+1)//2
            self.assertEqual(upper % 2,lower % 2)
            self.assertEqual(independent_value(quantize_rational(value,m,4,"nearest_even")["value"],m),upper)

    def test_encoding_validation_and_error_precedence(self):
        self.assertEqual(from_binary64("junk",3,9,"invalid")["error"],"unsupported_format")
        self.assertEqual(from_binary64("junk",27,9,"invalid")["error"],"unsupported_mode")
        for text in (None,{},[],"", "-0E0", "+0000E0", "+0++0E0", "+----E0",
                     "++000E-0", "++000E01", "++000E40", "++000E-41", "++000E"+"9"*10000):
            self.assertIsNotNone(to_binary64(text,4,4)["error"])
        # Subnormal values may have leading zero trits; reserved exponent may not.
        self.assertEqual(decode_ternary("+000+E-40",4,4)[0],Fraction(1,3**43))
        self.assertEqual(to_binary64("+000+E40",4,4)["error"],"invalid_encoding")
        for m,e in ((False,9),(27,True),(4,3),(73,9),(27,10)):
            self.assertEqual(from_binary64("0000000000000000",m,e)["error"],"unsupported_format")

    def test_checked_result_cannot_hide_loss(self):
        self.assertEqual(from_binary64("3fe0000000000000")["error"],"inexact")
        self.assertEqual(from_binary64("3fe0000000000000",mode="nearest_even")["status"],"rounded")
        negative_zero = from_binary64("8000000000000000")
        self.assertEqual(negative_zero["flags"],["negative_zero_normalized"])
        for bits in ("7ff0000000000000","7ff0000000000001","fff8000000000001"):
            self.assertEqual(from_binary64(bits)["error"],"nonfinite_input")
        # No "encode(decode(x))" promise through an inexact binary conversion.
        third = to_binary64("++00000000000000000000000000E-1",mode="nearest_even")
        self.assertEqual(third["status"],"rounded")
        self.assertEqual(from_binary64(third["value"])["error"],"inexact")


if __name__ == "__main__":
    unittest.main()
