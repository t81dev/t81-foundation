#!/usr/bin/env python3
"""Exact finite-state storage counts under an idealized per-element cost model."""
from fractions import Fraction
import json
import math


def ceil_digits(states, radix):
    digits, capacity = 0, 1
    while capacity < states:
        digits += 1
        capacity *= radix
    return digits


rows = []
for states in [2, 3, 8, 81, 256, 2**64, 2**256]:
    binary = ceil_digits(states, 2)
    ternary = ceil_digits(states, 3)
    ratio = Fraction(binary, ternary)
    rows.append({"states": str(states), "binary_elements": binary,
                 "ternary_elements": ternary,
                 "break_even_C3_over_C2": str(ratio),
                 "ternary_cost_at_C3_over_C2_3_over_2": str(Fraction(3, 2)*ternary),
                 "binary_cost_at_C2_1": binary,
                 "ternary_wins_at_ratio_3_over_2": Fraction(3, 2)*ternary < binary})
assert rows[4]["binary_elements"] == 8 and rows[4]["ternary_elements"] == 6
assert rows[4]["ternary_wins_at_ratio_3_over_2"] is False
assert (1.5 < math.log2(3))
print(json.dumps({"evidence_tier": "D", "assumptions":
    "independent symbols, arbitrary injective encoding of M states, equal fixed capacity, zero sensing/ECC/control/conversion overhead; no device measurements",
    "asymptotic_threshold": math.log2(3), "finite_rows": rows}, indent=2))
