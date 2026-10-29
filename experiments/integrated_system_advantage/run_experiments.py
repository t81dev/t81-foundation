#!/usr/bin/env python3
"""Run the full bounded parity gate before any new observations; never overwrite history."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import tempfile

from ablation_runner import AblationHarness

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', default=os.environ.get('T81_BUILD_DIR', str(HERE.parents[1]/'build')))
    parser.add_argument('--work-dir', default=str(HERE/'work'))
    parser.add_argument('--out', required=True, help='new output file; existing files are refused')
    parser.add_argument('--iterations', type=int, default=30)
    parser.add_argument('--seed', type=int, default=602)
    args = parser.parse_args()
    output = Path(args.out)
    if output.exists():
        parser.error('refusing to overwrite previous observations')
    if args.iterations < 1:
        parser.error('iterations must be positive')
    work = Path(args.work_dir); work.mkdir(parents=True, exist_ok=True)
    os.environ['T81_CANONFS_READ_VERIFY'] = '1'
    invocation = Path(tempfile.mkdtemp(prefix='run-', dir=work))
    report = invocation/'parity.json'
    env = dict(os.environ, T81_CANONFS_READ_VERIFY='1', T81_BUILD_DIR=str(Path(args.build_dir).resolve()), T81_PARITY_REPORT=str(report))
    gate = subprocess.run([sys.executable, str(HERE/'test_differential_parity.py')], env=env,
                          capture_output=True, text=True)
    (invocation/'parity.log').write_text(gate.stdout+gate.stderr)
    data = dict(investigation='08', timestamp=datetime.now(timezone.utc).isoformat(),
                architectural_performance_disposition='INCONCLUSIVE',
                assurance_superiority_disposition='INCONCLUSIVE',
                scope='bounded pure-Python implementation versus production C++ assess-fixed path; not general model equivalence',
                gate_exit=gate.returncode, gate_report=json.loads(report.read_text()) if report.exists() else None,
                gate_log=str(invocation/'parity.log'),
                environment=dict(canonfs_read_verify='1',platform=platform.platform(),python=sys.version,machine=platform.machine()),
                source_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=HERE,text=True).strip(),
                source_files_sha256={str(f.relative_to(HERE)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(HERE.glob('*')) if f.suffix in ('.py','.cpp')},
                binaries_sha256={name:hashlib.sha256((Path(args.build_dir)/(name+('.exe' if os.name=='nt' else ''))).read_bytes()).hexdigest() for name in ('t81','t81_make_assess_fixed_demo','t81_investigation08_probe')},
                source_diff_sha256=hashlib.sha256(subprocess.check_output(['git','diff','HEAD'],cwd=HERE)).hexdigest(),
                observations=None)
    if gate.returncode == 0:
        harness = AblationHarness(HERE.parents[1], invocation, args.build_dir)
        fixtures = harness.prepare_fixtures()
        data['fixture_model_hash'] = fixtures['model_hash']
        data['observations'] = harness.run_paired_benchmarks(fixtures,'greet hello',args.iterations,args.seed)
        data['disposition'] = 'BOUNDED PARITY PASSED; IMPLEMENTATION OBSERVATIONS ONLY; investigation remains open'
    else:
        data['disposition'] = 'PARITY GATE FAILED; NO MEASUREMENTS AUTHORIZED'
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open('x') as f:
        json.dump(data,f,indent=2,ensure_ascii=False); f.write('\n')
    print(data['disposition'])
    return 0 if gate.returncode == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
