#!/usr/bin/env python3
"""Executable audit, independent finite enumeration, and reversible packing tests."""
import argparse
from fractions import Fraction
import itertools
import json
from pathlib import Path
import random
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from experiments.h04_r02_adaptive_representation.oracle import IndependentOracle, generate_workloads
from experiments.h04_r02_adaptive_representation import packing

parser = argparse.ArgumentParser()
parser.add_argument('--bin-path', required=True, type=Path)
args, rest = parser.parse_known_args()
BINARY = args.bin_path.resolve()


def exhaustive(trace, ternary=False, ratio=1, metadata=2, transition=1, conversion=Fraction(1,2), block=1, bits=8):
    """Enumerate every feasible block schedule; no production/oracle tier helpers or DP.

    Return exact rational cost, all path crossover candidates, and optimal schedules.
    """
    widths = [6,11,21,41] if ternary else [bits,16,32,64]
    groups = [trace[i:i+block] for i in range(0,len(trace),block)]
    paths = []
    for modes in itertools.product(range(4), repeat=len(groups)):
        valid = True
        for mode, values in zip(modes,groups):
            w = widths[mode]
            low, high = (-(3**w-1)//2, (3**w-1)//2) if ternary else (-(2**(w-1)), 2**(w-1)-1)
            if any(v < low or v > high for v in values):
                valid = False
                break
        if valid:
            slots = sum(widths[t]*len(g) for t,g in zip(modes,groups))
            overhead = metadata*len(groups)+conversion*len(trace)+transition*sum(abs(a-b) for a,b in zip(modes,modes[1:]))
            paths.append((Fraction(slots)*ratio+overhead, slots, overhead, modes))
    return min(x[0] for x in paths), paths


class Audit(unittest.TestCase):
    def run_cpp(self, trace=None, **params):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/'result.json'
            command = [str(BINARY),'--json-out',str(path)]
            if trace is not None: command += ['--trace',','.join(map(str,trace))]
            for key,value in params.items(): command += ['--'+key.replace('_','-'),str(value)]
            subprocess.run(command, check=True, capture_output=True, text=True)
            return json.loads(path.read_text())

    def test_default_ledgers_and_current_artifact(self):
        data = self.run_cpp()
        self.assertEqual(data, json.loads((ROOT/'experiments/h04_r02_adaptive_representation/results.json').read_text()))
        oracle = IndependentOracle()
        for name,trace in generate_workloads().items():
            for arm in ('FB','AB','FT','AT'):
                expected = oracle.evaluate(arm,trace)
                for key, value in data['workloads'][name][arm].items():
                    self.assertEqual(value,expected[key], (name,arm,key))
        self.assertAlmostEqual(data['workloads']['constant_small']['total_break_even_c3_c2'],4/3)
        self.assertAlmostEqual(data['workloads']['boundary_oscillation']['total_break_even_c3_c2'],2.165)

    def test_exhaustive_small_traces(self):
        # Every length-three trace over four representational boundary classes.
        for trace in itertools.product([0,128,88574,2**40],repeat=3):
            for trans in [0,100]:
                row = self.run_cpp(trace,ratio=1.6,transition_cost=trans)['workloads']['custom']
                oracle = IndependentOracle(symbol_cost_ratio=1.6,transition_cost_per_level=trans)
                abcost,_ = exhaustive(trace,transition=trans)
                _, paths = exhaustive(trace,ternary=True,transition=trans)
                exact_crossing = max((abcost-o)/s for _,s,o,_ in paths)
                self.assertAlmostEqual(row['total_break_even_c3_c2'],float(exact_crossing),places=8)
                for arm in ['AB','AT']:
                    cost,_ = exhaustive(trace,ternary=arm=='AT',ratio=Fraction(8,5) if arm=='AT' else 1,transition=trans)
                    self.assertAlmostEqual(row[arm]['total_cost'],float(cost),places=8)
                    self.assertAlmostEqual(oracle.evaluate(arm,trace)['total_cost'],float(cost),places=8)

    def test_blocks_metadata_and_comparator(self):
        trace = [127,128,-364,364,0]
        for bits,block,ratio in itertools.product([8,10],[1,2,4],[1,1.6,2]):
            row = self.run_cpp(trace,binary_tier0_bits=bits,block_size=block,ratio=ratio)['workloads']['custom']
            for arm in ['AB','AT']:
                cost,_ = exhaustive(trace,ternary=arm=='AT',ratio=Fraction(str(ratio)) if arm=='AT' else 1,block=block,bits=bits)
                self.assertAlmostEqual(row[arm]['total_cost'],float(cost))
                self.assertEqual(row[arm]['metadata_cost'],2*((len(trace)+block-1)//block))
                seq = row[arm]['tier_sequence']
                self.assertTrue(all(len(set(seq[i:i+block]))==1 for i in range(0,len(seq),block)))
        boundary = self.run_cpp([127,128]*50,binary_tier0_bits=10,ratio=1.6)['workloads']['custom']
        self.assertEqual(boundary['AB']['total_cost'],1250)
        self.assertEqual(boundary['AT']['total_cost'],1210)
        self.assertAlmostEqual(boundary['total_break_even_c3_c2'],5/3)

    def test_schedule_change_at_crossover(self):
        trace = [0,128,0]
        row = self.run_cpp(trace,transition_cost=5)['workloads']['custom']
        ab,_ = exhaustive(trace,transition=5)
        _,paths = exhaustive(trace,ternary=True,transition=5)
        self.assertAlmostEqual(row['total_break_even_c3_c2'],float(max((ab-o)/s for _,s,o,_ in paths)))
        # A trace where AT changes its optimal schedule between r=1 and crossing.
        trace = [0,88574,0]
        row = self.run_cpp(trace,transition_cost=7)['workloads']['custom']
        at = row['AT']
        frozen = (row['AB']['total_cost']-at['metadata_cost']-at['transition_cost']-1.5)/at['symbol_slots']
        self.assertNotAlmostEqual(row['total_break_even_c3_c2'],frozen)

    def test_boundaries_empty_and_large_finite_cost(self):
        trace = [-(2**63),2**63-1,-88574,-88573,88573,88574,-364,364,-365,365]
        data = self.run_cpp(trace,transition_cost=1e20)['workloads']['custom']
        for arm in ['AB','AT']:
            self.assertEqual(data[arm]['transition_count'],0)
        row = self.run_cpp([])['workloads']['custom']
        self.assertEqual(row['AT']['total_cost'],0)
        self.assertEqual(row['total_break_even_c3_c2'],0)
        for n in [6,11,21,41]:
            values = [sum(t*3**i for i,t in enumerate(digits)) for digits in itertools.product([-1,0,1],repeat=n)] if n==6 else None
            if values:
                self.assertEqual(len(set(values)),3**n)
                self.assertEqual((min(values),max(values)),(-(3**n-1)//2,(3**n-1)//2))
        self.assertEqual((3**41-1)//2,18236498188585393201)
        # Prove minimal *signed two's-complement* coverage, not just trit range.
        # The 41-trit maximum fits signed 65 bits, not signed 64 or 66-minimum.
        for trits, bits in [(6,10),(11,18),(21,34),(41,65)]:
            maximum = (3**trits - 1)//2
            self.assertLessEqual(maximum, 2**(bits-1)-1)
            self.assertGreater(maximum, 2**(bits-2)-1)

    def test_shared_overhead_cancels_before_optimization(self):
        # Includes the exact review reproducer, conversion-only losses of
        # significance, partial shared blocks, and large transition/ratio scales.
        cases = [
            ([0], 1, 8, 1, 1),
            ([0], 1, 10, 1, 1),
            ([0,128,0], 1, 8, 5, 1.6),
            ([0,88574,0], 1, 8, 7, 1),
            ([0,128,0], 2, 10, 100, 1.6),
            ([0,88574,0], 1, 8, 1e20, 1e-200),
            ([0,88574,0], 1, 8, 1e-200, 1e200),
        ]
        overheads = [(0,0), (1e-300,1e-300), (1e20,0), (0,1e20),
                     (1e100,1e-100), (1e-100,1e100), (1e300,1e300)]
        for trace,block,bits,transition,ratio in cases:
            baseline = None
            for metadata,conversion in overheads:
                with self.subTest(trace=trace,block=block,bits=bits,
                                  transition=transition,ratio=ratio,
                                  metadata=metadata,conversion=conversion):
                    row = self.run_cpp(trace, block_size=block, binary_tier0_bits=bits,
                                       transition_cost=transition, ratio=ratio,
                                       metadata_bits=metadata, conversion_cost=conversion)['workloads']['custom']
                    oracle = IndependentOracle(symbol_cost_ratio=ratio, block_size=block,
                                               binary_tier0_bits=bits, transition_cost_per_level=transition,
                                               metadata_bits=metadata, conversion_cost=conversion)
                    # Rational enumeration includes the full shared costs, so
                    # this independently checks analytical cancellation too.
                    kw = dict(block=block,bits=bits,transition=Fraction(transition),
                              metadata=Fraction(metadata),conversion=Fraction(conversion))
                    abcost,_ = exhaustive(trace,**kw)
                    _,paths = exhaustive(trace,ternary=True,**kw)
                    exact_crossing = max((abcost-overhead)/slots for _,slots,overhead,_ in paths)
                    self.assertAlmostEqual(row['total_break_even_c3_c2'],float(exact_crossing),places=12)
                    self.assertAlmostEqual(float(oracle.crossover(trace)),float(exact_crossing),places=12)
                    signature = (row['total_break_even_c3_c2'],
                                 row['AB']['tier_sequence'],row['AT']['tier_sequence'])
                    if baseline is None: baseline = signature
                    self.assertEqual(signature,baseline)
                    for arm in ['AB','AT']:
                        cost,_ = exhaustive(trace,ternary=arm=='AT',
                                            ratio=Fraction(ratio) if arm=='AT' else 1,**kw)
                        expected = oracle.evaluate(arm,trace,exact=True)
                        self.assertEqual(expected['total_cost'],cost)
                        ledger = row[arm]
                        # Inspect actual emitted events in exact arithmetic,
                        # rather than subtracting a rounded full total.
                        actual = (ledger['symbol_slots']*(Fraction(ratio) if arm=='AT' else 1)
                                  + ledger['transition_level_delta']*Fraction(transition)
                                  + ((len(trace)+block-1)//block)*Fraction(metadata)
                                  + len(trace)*Fraction(conversion))
                        self.assertEqual(actual,cost)
                        self.assertEqual(ledger['symbol_slots'],expected['symbol_slots'])
                        self.assertEqual(ledger['transition_level_delta'],expected['transition_level_delta'])
                        self.assertEqual(ledger['total_cost'],oracle.evaluate(arm,trace)['total_cost'])

    def test_invalid_inputs_fail(self):
        for key,value in [('block-size','0'),('block-size','-1'),('ratio','nan'),('ratio','inf'),('ratio','-1'),('transition-cost','-1'),('binary-tier0-bits','9'),('trace',str(2**63)),('trace','1,'),('unknown','x')]:
            result = subprocess.run([str(BINARY),'--'+key,value],capture_output=True)
            self.assertNotEqual(result.returncode,0,(key,value))
        result = subprocess.run([str(BINARY),'--json-out','/no-such-directory/h04.json'],capture_output=True)
        self.assertNotEqual(result.returncode,0)
        with self.assertRaises(ValueError): IndependentOracle().evaluate('AT',[2**63])

    def test_packing_round_trips(self):
        for v in range(-364,365):
            self.assertEqual(packing.decode(packing.encode([v])),[v])
        rng = random.Random(243)
        for n in range(151):
            values = [rng.randrange(-364,365) for _ in range(n)]
            for encode,decode,expected in [(packing.encode,packing.decode,(6*n+4)//5),(packing.encode_binary10,packing.decode_binary10,(10*n+7)//8)]:
                blob = encode(values)
                self.assertEqual(len(blob),8+expected)
                self.assertEqual(decode(blob),values)
        self.assertEqual(packing.experiment(),json.loads((ROOT/'experiments/h04_r02_adaptive_representation/packing_results.json').read_text()))
        for v in range(-512,512):
            self.assertEqual(packing.decode_binary10(packing.encode_binary10([v])),[v])

    def test_packing_rejects_noncanonical_data(self):
        valid = packing.encode([0])
        for blob in [b'',valid[:-1],valid+b'\0',valid[:8]+b'\xff'+valid[9:],valid[:-1]+bytes([valid[-1]+3])]:
            with self.assertRaises(ValueError): packing.decode(blob)
        for values in [[365],[-365],[0.5]]:
            with self.assertRaises(ValueError): packing.encode(values)
        with self.assertRaises(ValueError): packing.decode_binary10(packing.encode_binary10([0])[:-1]+b'\xff')


if __name__ == '__main__':
    unittest.main(argv=[sys.argv[0],*rest])
