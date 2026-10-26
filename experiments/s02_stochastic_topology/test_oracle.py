#!/usr/bin/env python3
"""Independent coefficient/ground-state/trace audit for the corrected S02 pilot."""
import argparse
import gzip
import itertools
import json
import math
from pathlib import Path
import subprocess
import tempfile
from oracle import IsingOracle

def require(condition, message):
    if not condition:raise ValueError(message)

def open_trace(path):return gzip.open(path,'rt') if str(path).endswith('.gz') else path.open()

def verify_dataset(results, traces):
    data=json.loads(Path(results).read_text(encoding="utf-8"));rows=data['runs'];problems=data['problems']
    require(data['schema_version']==2 and len(rows)==data['total_runs']==1260,'wrong scope/count/schema')
    require(data['capacity_comparison']=='VERTEX_MATCHED_ONLY','unsupported capacity claim')
    by_problem={p['problem_id']:p for p in problems};require(len(by_problem)==15,'expected fifteen problem instances')
    with open_trace(Path(traces)) as source:
        trace_list=[json.loads(line) for line in source]
    trace_map={t['trajectory_id']:t for t in trace_list}
    require(len(trace_map)==len(trace_list)==data['simulated_trajectories']==315,'trace identity collision/count')
    arm_keys={('Binary','Fixed','Stochastic'),('Ternary','Fixed','Stochastic'),('Binary','AdaptiveLocal','Stochastic'),('Ternary','AdaptiveLocal','Stochastic'),('Binary','CentralizedAdaptive','Stochastic'),('Binary','RandomAdaptive','Stochastic'),('Binary','AdaptiveLocal','Deterministic')}
    expected={(n,seed,T,c,element,topology,dynamics) for n,seed,T,c,(element,topology,dynamics) in itertools.product([8,12,16],range(1,6),[.1,.5,1.0],[0,.5,1,5],arm_keys)}
    actual=[(x['num_nodes'],x['seed'],x['temperature'],x['cost_topology_update'],x['element_type'],x['topology_type'],x['dynamics_type']) for x in rows]
    require(len(set(actual))==len(actual) and set(actual)==expected,'missing/duplicated sweep cells')
    grouped={}
    for row in rows:grouped.setdefault(row['problem_id'],[]).append(row)
    references={};energy_functions={};max_reference_error=0.0
    for pid,p in by_problem.items():
        group=grouped[pid];seed=group[0]['seed'];n=p['num_nodes']
        require(all(x['seed']==seed and x['num_nodes']==n for x in group),'problem identity mixed')
        oracle=IsingOracle(n,1.0,seed)
        require(oracle.h_fields==p['h_fields'] and oracle.J_couplings==p['J_couplings'],'C++/Python task coefficients differ')
        ground,_=oracle.exact_ground_state_binary()
        references[pid]=ground;energy_functions[pid]=oracle
    per_trajectory={};price_counts={};trajectory_bindings={}
    for row in rows:
        pid=row['problem_id'];tid=row['trajectory_id'];trace=trace_map[tid];oracle=energy_functions[pid];n=row['num_nodes'];steps=row['num_steps'];ground=references[pid]
        normalized={k:v for k,v in row.items() if k not in {'cost_topology_update','total_cost'}}
        if tid in trajectory_bindings:require(trajectory_bindings[tid]==normalized,'one trace assigned inconsistent metrics/arm')
        else:trajectory_bindings[tid]=normalized
        require(trace['problem_id']==pid and len(trace['states'])==steps+1==201,'wrong trace/problem/length')
        require(row['coupling_scale']==1 and row['seed'] in range(1,6) and n in {8,12,16} and row['temperature'] in {.1,.5,1.0},'out-of-scope row')
        require(row['cost_topology_update'] in {0,.5,1,5},'out-of-scope repricing')
        if tid not in per_trajectory:
            allowed={-1,1} if row['element_type']=='Binary' else {-1,0,1}
            for state in trace['states']:require(len(state)==n and set(state)<=allowed,'illegal spin state')
            for previous,current in zip(trace['states'],trace['states'][1:]):require(sum(a!=b for a,b in zip(previous,current))<=1,'not a single-site trajectory')
            energies=[oracle.compute_energy_binary(state) for state in trace['states']]
            hits=[i for i,e in enumerate(energies) if abs(e-ground)<1e-9]
            occupied=sum(abs(e-ground)<1e-9 for e in energies[1:])
            per_trajectory[tid]={'final':energies[-1],'mean':sum(energies[1:])/steps,'first':hits[0] if hits else None,'hits':occupied,'gap':max(0,energies[-1]-ground)}
        audited=per_trajectory[tid]
        difference=abs(row['ground_state_energy']-ground);max_reference_error=max(max_reference_error,difference)
        require(difference<1e-10,'incorrect ground reference')
        for key,value in [('final_energy',audited['final']),('mean_energy',audited['mean']),('energy_error',audited['gap'])]:require(math.isfinite(row[key]) and abs(row[key]-value)<1e-9,key+' differs from immutable-task trace')
        require(audited['final']>=ground-1e-9,'below exact task optimum')
        require(row['steps_to_ground_state']==audited['first'] and row['reached_ground_state']==(audited['first'] is not None),'wrong first hit')
        require(row['ground_state_hits']==audited['hits'] and abs(row['ground_state_occupancy_fraction']-audited['hits']/steps)<1e-12,'wrong occupancy (not ESS)')
        require(row['spin_update_ops']==steps,'wrong proposal count')
        require(abs(row['total_cost']-(steps+row['comm_ops']+row['cost_topology_update']*row['topology_update_ops']))<1e-9,'wrong declared proxy cost')
        price_counts.setdefault(tid,[]).append(row['cost_topology_update'])
    require(all(sorted(v)==[0,.5,1,5] for v in price_counts.values()),'missing/duplicate price cell')
    return {'status':'PASS','rows_verified':len(rows),'trajectories_verified':len(per_trajectory),'states_audited':sum(len(x['states']) for x in trace_list),'distinct_immutable_problems':len(references),'max_cpp_python_reference_difference':max_reference_error,'oracle':'independent Python binary exhaustive enumeration; ternary equality follows multi-affinity and is exhaustively checked separately on tiny cases','hardware_or_architectural_advantage_verified':False}

def main():
    p=argparse.ArgumentParser();p.add_argument('--runner',type=Path);p.add_argument('--results',type=Path);p.add_argument('--traces',type=Path);p.add_argument('--report',type=Path);a=p.parse_args()
    if a.runner:
        with tempfile.TemporaryDirectory(prefix='s02-oracle-') as scratch:
            result_path=Path(scratch)/'results.json';trace_path=Path(scratch)/'traces.jsonl'
            subprocess.run([str(a.runner.resolve()),str(result_path),str(trace_path)],check=True)
            result=verify_dataset(result_path,trace_path)
    else:
        if not a.results or not a.traces:p.error('provide --runner or both --results and --traces')
        result=verify_dataset(a.results,a.traces)
    if a.report:a.report.write_text(json.dumps(result,indent=2)+'\n', encoding="utf-8")
    print(json.dumps(result,indent=2))
if __name__=='__main__':main()
