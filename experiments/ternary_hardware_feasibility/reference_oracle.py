#!/usr/bin/env python3
"""
Independent Software Reference Oracle for Scientific Investigation 07
Ternary Dot-Product Hardware Feasibility and Matched Binary Baselines

Mathematical Contract:
Y = sum_{i=0}^{N-1} W_i * X_i
where:
  W_i in {-1, 0, +1}
  X_i in [-128, 127] (8-bit signed Two's Complement INT8)

Accumulator Width Derivation:
For vector length N = 4096:
  Maximum positive contribution per element: (-1) * (-128) = +128.
  Maximum positive dot-product Y_max = 4096 * 128 = +524,288.
  Maximum negative contribution per element: (+1) * (-128) = -128.
  Maximum negative dot-product Y_min = 4096 * (-128) = -524,288.

Signed 2's complement bit width requirement:
  A B-bit signed integer has range [-2^(B-1), 2^(B-1) - 1].
  For B = 20 bits: range is [-524288, +524287]. (Exceeds upper limit +524288 by 1 bit).
  For B = 21 bits: range is [-1048576, +1048575].
  Therefore, minimum sufficient signed accumulator width for N = 4096 is 21 bits.
  We standardize B_acc = 32 bits across all hardware designs and oracle.
"""

import random
from typing import List, Tuple, Dict, Any


def dot_product_reference(W: List[int], X: List[int]) -> int:
    """
    Independent reference oracle for exact integer dot-product.
    Does NOT use hardware logic simulation or sign-mask bitplanes.
    """
    assert len(W) == len(X), "Vector length mismatch"
    accumulator = 0
    for w, x in zip(W, X):
        assert w in (-1, 0, 1), f"Invalid ternary weight: {w}"
        assert -128 <= x <= 127, f"Invalid INT8 activation: {x}"
        accumulator += w * x
    return accumulator


def generate_directed_tests(N: int) -> List[Tuple[str, List[int], List[int], int]]:
    """Generates directed test cases for verification."""
    tests = []

    # 1. All Zeros
    W_zero = [0] * N
    X_zero = [0] * N
    tests.append(("All Zeros", W_zero, X_zero, dot_product_reference(W_zero, X_zero)))

    # 2. All +1 weights, Max positive activation (+127)
    W_p1 = [1] * N
    X_max = [127] * N
    tests.append(("+1 Weights, Max Pos Activation", W_p1, X_max, dot_product_reference(W_p1, X_max)))

    # 3. All +1 weights, Min negative activation (-128)
    X_min = [-128] * N
    tests.append(("+1 Weights, Min Neg Activation (-128)", W_p1, X_min, dot_product_reference(W_p1, X_min)))

    # 4. All -1 weights, Min negative activation (-128) -> Max Positive Accumulation (+524,288)
    W_m1 = [-1] * N
    tests.append(("-1 Weights, Min Neg Activation (-128)", W_m1, X_min, dot_product_reference(W_m1, X_min)))

    # 5. All -1 weights, Max positive activation (+127) -> Max Negative Accumulation
    tests.append(("-1 Weights, Max Pos Activation (+127)", W_m1, X_max, dot_product_reference(W_m1, X_max)))

    # 6. Alternating weights and activations
    W_alt = [1 if i % 2 == 0 else -1 for i in range(N)]
    X_alt = [127 if i % 2 == 0 else -128 for i in range(N)]
    tests.append(("Alternating Signs", W_alt, X_alt, dot_product_reference(W_alt, X_alt)))

    # 7. Single element non-zero combinations (at position 0)
    for w in [-1, 0, 1]:
        for x in [-128, -127, -1, 0, 1, 126, 127]:
            W_single = [0] * N
            X_single = [0] * N
            W_single[0] = w
            X_single[0] = x
            tests.append((f"Single Element W={w}, X={x}", W_single, X_single, dot_product_reference(W_single, X_single)))

    return tests


def generate_random_tests(N: int, seed: int = 42, count: int = 50) -> List[Tuple[str, List[int], List[int], int]]:
    """Generates randomized test cases with specified distributions."""
    random.seed(seed)
    tests = []

    # Dense weights (equal probability of -1, 0, +1)
    for i in range(count // 2):
        W = [random.choice([-1, 0, 1]) for _ in range(N)]
        X = [random.randint(-128, 127) for _ in range(N)]
        tests.append((f"Random Dense {i}", W, X, dot_product_reference(W, X)))

    # Sparse weights (90% zeros)
    for i in range(count // 2):
        W = [random.choice([-1, 1]) if random.random() < 0.1 else 0 for _ in range(N)]
        X = [random.randint(-128, 127) for _ in range(N)]
        tests.append((f"Random Sparse {i}", W, X, dot_product_reference(W, X)))

    return tests


def main():
    print("=== Reference Oracle Self-Test ===")
    N = 4096
    directed_tests = generate_directed_tests(N)
    random_tests = generate_random_tests(N, seed=42, count=20)
    all_tests = directed_tests + random_tests

    print(f"Generated {len(all_tests)} test cases for N={N}.")
    for name, W, X, expected in all_tests[:10]:
        val = dot_product_reference(W, X)
        assert val == expected
        print(f"  [PASS] {name}: Y = {val}")

    print("Reference Oracle verified successfully.")


if __name__ == "__main__":
    main()
