#!/usr/bin/env python3
"""
Independent Reference Oracle for T81 Research Experiment S02
Stochastic State-Topology Computation.
"""

import math
import itertools
from typing import Dict, List, Tuple

MASK64 = (1 << 64) - 1
PROBLEM_STREAM = 0x243F6A8885A308D3

class SimplePRNG:
    """SplitMix64 plus exact rejection-based integer range reduction.

    Reference: https://prng.di.unimi.it/splitmix64.c (public domain).
    Deterministic reproducibility, not a physical/cryptographic IID guarantee.
    """
    def __init__(self, seed: int):
        self.state = seed & MASK64
    def next(self) -> int:
        self.state = (self.state + 0x9E3779B97F4A7C15) & MASK64
        z = self.state
        z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & MASK64
        z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & MASK64
        return z ^ (z >> 31)
    def uniform_real(self, min_val: float, max_val: float) -> float:
        return min_val + (self.next() >> 11) / 9007199254740992.0 * (max_val - min_val)
    def uniform_int(self, min_val: int, max_val: int) -> int:
        span = max_val - min_val + 1
        if not 0 < span <= 1 << 64:
            raise ValueError("invalid integer range")
        threshold = ((1 << 64) - span) % span
        while True:
            word = self.next()
            if word >= threshold:
                return min_val + word % span


class IsingOracle:
    def __init__(self, num_nodes: int, coupling_scale: float = 1.0, seed: int = 42):
        self.num_nodes = num_nodes
        self.coupling_scale = coupling_scale
        self.seed = seed
        self.h_fields, self.J_couplings = self._generate_problem_instance()

    def _generate_problem_instance(self) -> Tuple[List[float], List[List[float]]]:
        prng = SimplePRNG(self.seed ^ PROBLEM_STREAM)
        N = self.num_nodes
        h_fields = [prng.uniform_real(-0.5, 0.5) for _ in range(N)]
        J_couplings = [[0.0] * N for _ in range(N)]

        side = int(math.ceil(math.sqrt(N)))
        for i in range(N):
            r = i // side
            c = i % side
            if c + 1 < side:
                j = r * side + (c + 1)
                if j < N:
                    val = (1.0 if prng.uniform_int(0, 1) == 1 else -1.0) * self.coupling_scale
                    J_couplings[i][j] = val
                    J_couplings[j][i] = val
            if r + 1 < side:
                j = (r + 1) * side + c
                if j < N:
                    val = (1.0 if prng.uniform_int(0, 1) == 1 else -1.0) * self.coupling_scale
                    J_couplings[i][j] = val
                    J_couplings[j][i] = val

        return h_fields, J_couplings

    def compute_energy_binary(self, spins: List[int]) -> float:
        E = 0.0
        for i in range(self.num_nodes):
            E -= self.h_fields[i] * spins[i]
            for j in range(i + 1, self.num_nodes):
                if self.J_couplings[i][j] != 0.0:
                    E -= self.J_couplings[i][j] * spins[i] * spins[j]
        return E

    def compute_energy_ternary(self, spins: List[int]) -> float:
        return self.compute_energy_binary(spins)

    def exact_ground_state_binary(self) -> Tuple[float, List[int]]:
        N = self.num_nodes
        min_E = float('inf')
        best_spins = []
        for mask in range(1 << N):
            spins = [1 if ((mask >> i) & 1) else -1 for i in range(N)]
            E = self.compute_energy_binary(spins)
            if E < min_E:
                min_E = E
                best_spins = spins
        return min_E, best_spins

    def exact_ground_state_ternary(self) -> Tuple[float, List[int]]:
        # Multi-affine Hamiltonian: eliminate zeros one coordinate at a time.
        # A minimum exists at binary endpoints. No onsite quadratic terms here.
        return self.exact_ground_state_binary()

    def exhaustive_ground_state_ternary(self) -> Tuple[float, List[int]]:
        """Independent 3^N check for tiny problems, not a production shortcut."""
        return min((self.compute_energy_ternary(list(s)), list(s))
                   for s in itertools.product((-1, 0, 1), repeat=self.num_nodes))

    def exact_partition_function_binary(self, temperature: float) -> float:
        N = self.num_nodes
        Z = 0.0
        for mask in range(1 << N):
            spins = [1 if ((mask >> i) & 1) else -1 for i in range(N)]
            E = self.compute_energy_binary(spins)
            Z += math.exp(-E / temperature)
        return Z

    def metropolis_acceptance_probability(self, dE: float, temperature: float) -> float:
        if dE <= 0.0:
            return 1.0
        return math.exp(-dE / temperature)

if __name__ == "__main__":
    oracle = IsingOracle(num_nodes=8, seed=1)
    min_E_bin, _ = oracle.exact_ground_state_binary()
    min_E_tri, _ = oracle.exact_ground_state_ternary()
    Z_bin = oracle.exact_partition_function_binary(1.0)
    print(f"8-node Ising Oracle Ground State (Binary): {min_E_bin:.6f}")
    print(f"8-node Ising Oracle Ground State (Ternary): {min_E_tri:.6f}")
    print(f"8-node Ising Oracle Partition Function Z (T=1.0): {Z_bin:.6f}")
