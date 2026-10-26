#!/usr/bin/env python3
"""
Independent Reference Oracle for T81 Research Experiment S02
Stochastic State-Topology Computation.
"""

import math
from typing import Dict, List, Tuple

class SimplePRNG:
    def __init__(self, seed: int):
        self.state = (seed ^ 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF

    def next(self) -> int:
        self.state = (self.state * 6364136223846793005 + 1442695040888963407) & 0xFFFFFFFFFFFFFFFF
        return self.state

    def uniform_real(self, min_val: float, max_val: float) -> float:
        u = (self.next() >> 11) / 9007199254740992.0
        return min_val + u * (max_val - min_val)

    def uniform_int(self, min_val: int, max_val: int) -> int:
        span = max_val - min_val + 1
        return min_val + (self.next() % span)


class IsingOracle:
    def __init__(self, num_nodes: int, coupling_scale: float = 1.0, seed: int = 42):
        self.num_nodes = num_nodes
        self.coupling_scale = coupling_scale
        self.seed = seed
        self.h_fields, self.J_couplings = self._generate_problem_instance()

    def _generate_problem_instance(self) -> Tuple[List[float], List[List[float]]]:
        prng = SimplePRNG(self.seed)
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
                if abs(self.J_couplings[i][j]) > 1e-9:
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
        N = self.num_nodes
        min_E = float('inf')
        best_spins = []
        total_states = 3 ** N
        for state in range(total_states):
            spins = []
            tmp = state
            for _ in range(N):
                digit = tmp % 3
                spins.append(-1 if digit == 0 else (0 if digit == 1 else 1))
                tmp //= 3
            E = self.compute_energy_ternary(spins)
            if E < min_E:
                min_E = E
                best_spins = spins
        return min_E, best_spins

    def exact_partition_function_binary(self, temperature: float) -> float:
        N = self.num_nodes
        Z = 0.0
        for mask in range(1 << N):
            spins = [1 if ((mask >> i) & 1) else -1 for i in range(N)]
            E = self.compute_energy_binary(spins)
            Z += math.exp(-E / temperature)
        return Z

    def glauber_transition_probability(self, dE: float, temperature: float) -> float:
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
