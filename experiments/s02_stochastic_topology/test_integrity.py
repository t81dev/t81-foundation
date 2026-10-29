#!/usr/bin/env python3
"""Behavioral regressions for the five findings; can run against preserved PR582."""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parent
COMPILED_PROBE=None
PROBE=r'''
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
// Intrusive experiment-only fixture: alter graph/state directly to isolate energy
// semantics without depending on random trajectories. No production T81 API.
#define private public
#include "stochastic_topology.hpp"
#undef private
using namespace t81::s02;
int main(int argc,char** argv) {
 std::cout << std::setprecision(17);
 std::string mode=argc>1?argv[1]:"";
 if(mode=="cache") {
  ProblemInstance p;p.num_nodes=1;p.h_fields={1.0};p.J_couplings={{0.0}};
  auto first=compute_exact_ground_state(p,ElementType::Binary);
  p.h_fields[0]=2.0;
  std::cout<<"["<<first<<","<<compute_exact_ground_state(p,ElementType::Binary)<<"]";
 } else if(mode=="objective") {
  ProblemInstance p;p.num_nodes=3;p.h_fields={0,0,0};p.J_couplings={{0,1,0},{1,0,0},{0,0,0}};
  SimulationConfig cfg;cfg.num_nodes=3;cfg.coupling_scale=2;
  NetworkSimulator sim(p,cfg);sim.states_={1,1,1};
  auto before=sim.calculate_current_energy();sim.active_adj_.assign(3,std::vector<int>(3,0));
  auto removed=sim.calculate_current_energy();sim.active_adj_[0][2]=sim.active_adj_[2][0]=1;
  std::cout<<"["<<before<<","<<removed<<","<<sim.calculate_current_energy()<<"]";
 } else if(mode=="sampling") {
  SimplePRNG p(1);std::cout<<"[";
  for(int i=0;i<64;i++){if(i)std::cout<<",";std::cout<<p.uniform_int(0,1);}std::cout<<"]";
 }
}
'''
class Integrity(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.temp=tempfile.TemporaryDirectory(prefix='s02-integrity-');cls.dir=Path(cls.temp.name)
  if COMPILED_PROBE is not None:
   cls.exe=COMPILED_PROBE.resolve()
  else:
   src=cls.dir/'probe.cpp';src.write_text(PROBE,encoding='utf-8');cls.exe=cls.dir/'probe'
   subprocess.run(['c++','-std=c++17','-O2','-I',str(ROOT),str(src),'-o',str(cls.exe)],check=True,capture_output=True)
  spec=importlib.util.spec_from_file_location('s02_oracle_under_test',ROOT/'oracle.py');cls.module=importlib.util.module_from_spec(spec);spec.loader.exec_module(cls.module)
 @classmethod
 def tearDownClass(cls):cls.temp.cleanup()
 def probe(self,mode):return json.loads(subprocess.check_output([str(self.exe),mode]))
 def test_finding_1_oracle_cache_uses_problem_coefficients(self):
  self.assertEqual(self.probe('cache'),[-1.0,-2.0],'same-size changed field must change cached reference')
 def test_finding_2_task_energy_survives_edge_removal_and_addition(self):
  self.assertEqual(self.probe('objective'),[-1.0,-1.0,-1.0],'computational graph must not redefine task energy')
 def test_finding_3_cpp_and_python_integer_sampling(self):
  prng=self.module.SimplePRNG(1)
  for language,values in [('C++',self.probe('sampling')),('Python',[prng.uniform_int(0,1) for _ in range(64)])]:
   with self.subTest(language=language):self.assertTrue(any(a==b for a,b in zip(values,values[1:])),language+' binary draws are forced alternating')
 def test_finding_4_table_reproduces_actual_data(self):
  data=json.loads((ROOT/'results.json').read_text(encoding="utf-8"));report=(ROOT/'RESEARCH_REPORT.md').read_text(encoding="utf-8")
  row=next(line for line in report.splitlines() if line.startswith('|') and 'Model A' in line and not 'Configuration' in line)
  reported=float(row.split('|')[2].strip())
  values=[r['energy_error'] for r in data['runs'] if r['num_nodes']==16 and r['temperature']==0.5 and r['cost_topology_update']==0.5 and r['element_type']=='Binary' and r['topology_type']=='Fixed' and r['dynamics_type']=='Stochastic']
  self.assertEqual(len(values),5);self.assertAlmostEqual(reported,sum(values)/len(values),places=5)
 def test_finding_5_capacity_hypothesis_not_claimed_tested(self):
  self.assertIn('**H2 — UNTESTED**',(ROOT/'RESEARCH_REPORT.md').read_text(encoding="utf-8"),'vertex-matched observations cannot decide H2')
def main():
 global ROOT,COMPILED_PROBE
 p=argparse.ArgumentParser();p.add_argument('--experiment-dir',type=Path,default=ROOT);p.add_argument('--report',type=Path);p.add_argument('--probe',type=Path);a=p.parse_args();ROOT=a.experiment_dir.resolve();COMPILED_PROBE=a.probe
 result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(Integrity))
 if a.report:a.report.write_text(json.dumps({'experiment_dir':str(ROOT),'tests':result.testsRun,'failures':len(result.failures),'errors':len(result.errors),'details':[{'test':str(t),'traceback':tb} for t,tb in result.failures+result.errors]},indent=2)+'\n', encoding="utf-8")
 return 0 if result.wasSuccessful() else 1
if __name__=='__main__':raise SystemExit(main())
