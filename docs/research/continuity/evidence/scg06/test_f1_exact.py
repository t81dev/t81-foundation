"""
Exhaustive verification of F1: T81Limb27 Booth table scaling against exact integers.
"""
import oracle

def build_booth27_table_ref():
    table = {}
    for b0 in range(-13, 14):
        for b1 in range(-13, 14):
            mul = 0
            shift = 0
            pattern = b0 + 3 * b1
            if pattern in (1, 3):
                mul = 1
            elif pattern in (2, 4):
                mul = 1
                shift = 1
            elif pattern in (-1, -3):
                mul = -1
            elif pattern in (-2, -4):
                mul = -1
                shift = 1
            table[(b0, b1)] = (mul, shift, pattern)
    return table

def analyze():
    table = build_booth27_table_ref()
    # Check what the table does:
    # A window pair (b0, b1) has mathematical value b0 + 27*b1 in trytes, or if it is overlapping:
    # In Booth27, a tryte is in -13..13. The pattern b0 + 3*b1 represents trits:
    # b0 is the current tryte, but shifted trytes means 3 trits = 27^1.
    # If shift = 1 tryte, the operand is shifted by 3 trits (factor of 27).
    # When pattern == 2, table sets mul=1, shift=1 (tryte shift 1 = *27).
    # But pattern == 2 has mathematical value 2, NOT 27!
    # Even if shift was meant to be 1 trit (factor of 3), 3 != 2.
    # Therefore, any pattern with shift=1 multiplies by 27 instead of 2 or 4.

    anomalies = []
    for (b0, b1), (mul, shift, pattern) in table.items():
        if shift == 1:
            anomalies.append({
                "b0": b0, "b1": b1, "pattern": pattern,
                "mul": mul, "shift_trytes": shift,
                "implied_weight": mul * (27 ** shift),
                "expected_pattern": pattern
            })
    print(f"Total entries in table: {len(table)}")
    print(f"Entries with shift=1: {len(anomalies)}")
    print(f"Sample anomaly: pattern={anomalies[0]['pattern']} gives mul={anomalies[0]['mul']} shift_trytes={anomalies[0]['shift_trytes']} -> value {anomalies[0]['implied_weight']}")

if __name__ == "__main__":
    analyze()
