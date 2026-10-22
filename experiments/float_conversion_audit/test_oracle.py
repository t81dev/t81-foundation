"""Exact reference tests; do not equate audit success with production correctness."""
from fractions import Fraction
import json
from pathlib import Path
import unittest

from oracle import binary64, decode, power3, quantize, reference, trits


class ExactReference(unittest.TestCase):
    def test_binary64_decomposition(self):
        self.assertEqual(binary64("0000000000000001")[1], Fraction(1, 2**1074))
        self.assertEqual(binary64("0010000000000000")[1], Fraction(1, 2**1022))
        self.assertEqual(binary64("7fefffffffffffff")[1], (2**53 - 1) * 2**971)
        self.assertEqual(binary64("bfe0000000000000")[1], Fraction(-1, 2))
        for b in ("0000000000000000", "8000000000000000"):
            self.assertEqual(reference(b), {"status": "exact", "candidate": "+0E0"})
        self.assertEqual(reference("3ff0000000000000")["status"], "exact")
        self.assertEqual(reference("3fe0000000000000")["status"], "inexact")

    def test_invalid_values_and_encodings(self):
        for b in ("7ff0000000000000", "fff0000000000000", "7ff8000000000000", "7ff0000000000001"):
            self.assertEqual(reference(b), {"status": "invalid", "candidate": None})
        for b in ("", "0", "0x00000000000000", "000000000000000g", "10000000000000000", -1, None):
            with self.assertRaises(ValueError):
                reference(b)
        for text in ("+Inf", "NaE", "+" + "0"*27 + "E1", "+" + "+"*27 + "E9841"):
            with self.assertRaises(ValueError):
                decode(text)

    def test_range_errors_and_subnormal(self):
        # Reduced exponent field exercises actual destination range boundaries;
        # every finite binary64 input is within the range of <27,9>.
        small = power3(-40 - 3)
        self.assertEqual(quantize(small, 4, 4)["status"], "exact")
        self.assertEqual(quantize(small / 2, 4, 4)["status"], "underflow")
        maximum = 40 * power3(39 - 3)
        self.assertEqual(quantize(maximum, 4, 4)["status"], "exact")
        self.assertEqual(quantize(maximum + 1, 4, 4)["status"], "overflow")
        self.assertEqual(quantize(-maximum - 1, 4, 4)["status"], "overflow")

    def test_against_exhaustive_small_format(self):
        # A second, exhaustive enumeration checks the nearest search, including
        # changing spacing across binades and parity tie breaking.
        encodings = [(Fraction(0), 0, 0, 0)]
        for e in range(-40, 40):
            for m in range(1 if e == -40 else 14, 41):
                encodings.append((m * power3(e - 3), m % 2, e, m))
        inputs = [Fraction(i, 16) for i in range(1, 200)]
        inputs += [Fraction(27, 2), Fraction(81, 2), Fraction(3**10, 2)]
        for value in inputs:
            with self.subTest(value=value):
                _, _, e, m = min(encodings, key=lambda row: (abs(value - row[0]), row[1], row[2], row[3]))
                expected = "+0E0" if not m else "+" + trits(m, 4) + f"E{e}"
                actual = quantize(value, 4, 4)
                self.assertEqual(actual["candidate"], expected)
                self.assertEqual(quantize(-value, 4, 4)["candidate"], "-" + expected[1:])
        # Exact tie at this scale: prefer the even mantissa 28, not 27.
        self.assertEqual(quantize(Fraction(55, 2), 4, 4)["candidate"], "+" + trits(28, 4) + "E3")

    def test_corpus_reference_is_not_macos_golden(self):
        corpus = json.loads((Path(__file__).parent / "corpus.json").read_text())
        for name, bits in corpus.items():
            r = reference(bits)
            kind, value = binary64(bits)
            if kind == "finite" and value:
                self.assertIn(r["status"], ("exact", "inexact"))
                self.assertNotEqual(r["candidate"], "+0E0", name)
                self.assertGreater(decode(r["candidate"]) * value, 0)
            if r["status"] == "exact":
                self.assertEqual(decode(r["candidate"]), value)


if __name__ == "__main__":
    unittest.main()
