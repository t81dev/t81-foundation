"""Bounded subprocess tests: require rejection AND the mandatory diagnostic."""
import subprocess
import sys

for kind in ('value', 'digits', 'sign', 'truncation', 'overflow',
             'binary-zero', 'binary-value', 'budget'):
    result = subprocess.run([sys.argv[1], '--negative', kind],
                            capture_output=True, text=True, timeout=10)
    diagnostic = ('GATE 1 CUMULATIVE TRIAL BUDGET EXCEEDED' if kind == 'budget'
                  else 'INVALID EXPERIMENT (CORRECTNESS DEFECT: ORACLE MISMATCH)')
    if result.returncode != 1 or diagnostic not in result.stderr:
        raise SystemExit(f'{kind}: unexpected status/output: {result}')
    print(f'{kind}: rejected with exit 1 and required diagnostic')
