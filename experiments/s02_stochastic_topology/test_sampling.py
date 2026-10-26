#!/usr/bin/env python3
"""Independent finite/reference controls; no new experimental sweep."""
import itertools
import math
import unittest
from oracle import SimplePRNG, IsingOracle
class Sampling(unittest.TestCase):
    def test_splitmix_reference_vectors(self):
        p=SimplePRNG(0)
        self.assertEqual([p.next() for _ in range(5)],[0xe220a8397b1dcdaf,0x6e789e6aa1b965f4,0x06c45d188009454f,0xf88bb8a8724c81ec,0x1b39896a51a8749b])
    def test_rejection_discards_incomplete_interval(self):
        class Words(SimplePRNG):
            def __init__(self):self.words=iter([0,5,16]);self.calls=0
            def next(self):self.calls+=1;return next(self.words)
        p=Words();self.assertEqual(p.uniform_int(0,9),6);self.assertEqual(p.calls,3)
    def test_bounded_reduction_full_small_word_space(self):
        # Exhaustively check the same rejection algebra on 4-bit uniform words.
        for span in range(1,16):
            threshold=(16-span)%span;counts=[0]*span
            for word in range(threshold,16):counts[word%span]+=1
            self.assertEqual(len(set(counts)),1)
    def test_range_rejects_invalid_request(self):
        with self.assertRaises(ValueError):SimplePRNG(1).uniform_int(2,1)
        with self.assertRaises(ValueError):SimplePRNG(1).uniform_int(0,1<<64)
    def test_indices_not_forced_to_parity_sublattices(self):
        for size in [8,16]:
            p=SimplePRNG(1);triples=[[p.uniform_int(0,size-1) for _ in range(3)] for _ in range(64)]
            self.assertTrue(any(u%2==v%2 for u,v,w in triples))
            self.assertTrue(any(u%2!=w%2 for u,v,w in triples))
    def test_ternary_endpoint_theorem_by_independent_exhaustion(self):
        for n in range(1,7):
            for seed in [1,2,3]:
                oracle=IsingOracle(n,1,seed)
                self.assertAlmostEqual(oracle.exact_ground_state_binary()[0],oracle.exhaustive_ground_state_ternary()[0],places=11)
    def test_fixed_graph_metropolis_target_and_proposal_symmetry(self):
        for alphabet in [(-1,1),(-1,0,1)]:
            energies=[-.7*x for x in alphabet];T=.5
            weights=[math.exp(-e/T) for e in energies];pi=[w/sum(weights) for w in weights]
            K=[]
            for i,e in enumerate(energies):
                row=[0.0]*len(alphabet)
                for j,f in enumerate(energies):
                    if i!=j:row[j]=min(1.0,math.exp(-(f-e)/T))/(len(alphabet)-1)
                row[i]=1-sum(row);K.append(row)
            for i in range(len(alphabet)):
                self.assertAlmostEqual(sum(pi[k]*K[k][i] for k in range(len(alphabet))),pi[i],places=12)
                for j in range(len(alphabet)):self.assertAlmostEqual(pi[i]*K[i][j],pi[j]*K[j][i],places=12)
    def test_partition_function_known_one_spin(self):
        oracle=IsingOracle(1);h=oracle.h_fields[0]
        self.assertAlmostEqual(oracle.exact_partition_function_binary(.5),2*math.cosh(h/.5),places=12)
if __name__=='__main__':unittest.main(verbosity=2)
