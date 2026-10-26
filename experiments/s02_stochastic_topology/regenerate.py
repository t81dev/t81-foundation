#!/usr/bin/env python3
"""Run, audit and publish the bounded corrected S02 packet locally."""
import argparse
from datetime import datetime,timezone
import gzip
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import tempfile
from test_oracle import verify_dataset
from analyze_results import analyze,render
ROOT=Path(__file__).resolve().parent
SOURCE_FILES=['stochastic_topology.hpp','main.cpp','oracle.py','test_oracle.py','analyze_results.py','regenerate.py']
def main():
    p=argparse.ArgumentParser();p.add_argument('--runner',type=Path,required=True);a=p.parse_args();runner=a.runner.resolve()
    before={f:hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in SOURCE_FILES}
    with tempfile.TemporaryDirectory(prefix='s02-regenerate-') as scratch:
        scratch=Path(scratch);results=scratch/'results.json';trace=scratch/'traces.jsonl'
        cmd=[str(runner),str(results),str(trace)];process=subprocess.run(cmd,capture_output=True,text=True,check=True)
        audit=verify_dataset(results,trace)
        packed=scratch/'results.traces.jsonl.gz'
        with packed.open('wb') as f:
            with gzip.GzipFile(filename='',fileobj=f,mode='wb',mtime=0) as archive:archive.write(trace.read_bytes())
        # Analyze/audit before replacing any canonical output, avoiding a failed
        # generation silently becoming the active scientific packet.
        analysis=analyze(results,packed);report=render(analysis)
        after={f:hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in SOURCE_FILES}
        if before!=after:raise RuntimeError('source changed during generation')
        (ROOT/'results.json').write_bytes(results.read_bytes());(ROOT/'results.traces.jsonl.gz').write_bytes(packed.read_bytes())
        (ROOT/'analysis.json').write_text(json.dumps(analysis,indent=2)+'\n', encoding="utf-8");(ROOT/'RESEARCH_REPORT.md').write_text(report, encoding="utf-8")
        (ROOT/'evidence/after/oracle-audit.json').write_text(json.dumps(audit,indent=2)+'\n', encoding="utf-8")
        metadata={'baseline_merge':'551d5075f35355b9ef96d9ce659d71be0891ee27','generated_at_utc':datetime.now(timezone.utc).isoformat(),'source_sha256':before,'runner_sha256':hashlib.sha256(runner.read_bytes()).hexdigest(),'runner_path_recorded':str(runner),'invocation':'RUNNER <temporary-results.json> <temporary-traces.jsonl>','stdout':process.stdout,'stderr':process.stderr,'python':platform.python_version(),'platform':platform.platform(),'result_sha256':hashlib.sha256(results.read_bytes()).hexdigest(),'compressed_trace_sha256':hashlib.sha256(packed.read_bytes()).hexdigest(),'uncompressed_trace_sha256':hashlib.sha256(trace.read_bytes()).hexdigest(),'independent_oracle_audit':audit,'physical_performance_claim':False}
        (ROOT/'evidence/generation.json').write_text(json.dumps(metadata,indent=2)+'\n', encoding="utf-8")
    print(json.dumps({'decision':'REVISE','audited_rows':audit['rows_verified'],'audited_states':audit['states_audited'],'H2':'UNTESTED'}))
if __name__=='__main__':main()
