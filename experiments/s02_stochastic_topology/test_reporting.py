#!/usr/bin/env python3
"""Independent reductions check every published table cell and preservation gate."""
import hashlib
import itertools
import json
import math
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parent
class Reporting(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data=json.loads((ROOT/'results.json').read_text(encoding="utf-8"));cls.analysis=json.loads((ROOT/'analysis.json').read_text(encoding="utf-8"));cls.report=(ROOT/'RESEARCH_REPORT.md').read_text(encoding="utf-8")
    def test_every_published_numeric_cell_matches_raw_rows(self):
        arms=[('Model A','Binary','Fixed','Stochastic'),('Model B','Ternary','Fixed','Stochastic'),('Model C','Binary','AdaptiveLocal','Stochastic'),('Model D','Ternary','AdaptiveLocal','Stochastic'),('Central heuristic','Binary','CentralizedAdaptive','Stochastic'),('Random control','Binary','RandomAdaptive','Stochastic'),('Zero-T control','Binary','AdaptiveLocal','Deterministic')]
        for name,element,topology,dynamics in arms:
            selected=[r for r in self.data['runs'] if (r['num_nodes'],r['temperature'],r['cost_topology_update'],r['element_type'],r['topology_type'],r['dynamics_type'])==(16,.5,.5,element,topology,dynamics)]
            high=[r for r in self.data['runs'] if (r['num_nodes'],r['temperature'],r['cost_topology_update'],r['element_type'],r['topology_type'],r['dynamics_type'])==(16,.5,5,element,topology,dynamics)]
            values=[math.fsum(r[k] for r in selected)/5 for k in ['energy_error','comm_ops','topology_update_ops','total_cost']]+[math.fsum(r['total_cost'] for r in high)/5]
            cells=next(line for line in self.report.splitlines() if line.startswith('| '+name+' |')).split('|')[2:-1]
            for index,(actual,value) in enumerate(zip(cells,values)):
                with self.subTest(arm=name,column=index):self.assertAlmostEqual(float(actual.strip()),value,places=5 if index==0 else 2)
    def test_repricing_not_extra_replication(self):
        groups={}
        for row in self.data['runs']:groups.setdefault(row['trajectory_id'],[]).append(row)
        self.assertEqual(len(groups),315)
        for rows in groups.values():
            self.assertEqual({r['cost_topology_update'] for r in rows},{0,.5,1,5})
            normalized=[{k:v for k,v in r.items() if k not in {'cost_topology_update','total_cost'}} for r in rows]
            self.assertTrue(all(r==normalized[0] for r in normalized))
    def test_scope_and_missing_capacity_are_explicit(self):
        scope=self.analysis['scope'];self.assertEqual(scope['planned_vertex_matched_rows'],10*4*4*3*6*7)
        self.assertEqual(scope['executed_priced_rows'],len(self.data['runs']))
        self.assertEqual(scope['unexecuted_rows'],18900)
        self.assertEqual(self.analysis['hypotheses']['H2']['status'],'UNTESTED')
        self.assertEqual(self.analysis['decision'],'REVISE')
        self.assertIn('**H2 — UNTESTED**',self.report)
    def test_original_evidence_exact_and_superseded(self):
        historic=ROOT/'historical/pr582';manifest=json.loads((historic/'MANIFEST.json').read_text(encoding="utf-8"))
        self.assertEqual(manifest['status'],'SUPERSEDED_INVALID_SCIENTIFIC_EVIDENCE')
        self.assertEqual(len(manifest['files']),11)
        for name,digest in manifest['files'].items():self.assertEqual(hashlib.sha256((historic/name).read_bytes()).hexdigest(),digest,name)
    def test_frozen_rubric_unchanged(self):
        expected='b220e94331fa59e205929890be1f68f1a876e3d728b32c3535a3012970d87353'
        self.assertEqual(hashlib.sha256((ROOT/'evidence/S02_EVALUATION_RUBRIC.frozen.md').read_bytes()).hexdigest(),expected)
if __name__=='__main__':unittest.main(verbosity=2)
