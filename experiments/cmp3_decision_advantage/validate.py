"""Non-performance r1 validation; never executes a benchmark workload."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--objdump', required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    results = []

    def run(label, command, expected=0, contains=None, binary_hash=None, output=None):
        command = [str(x) for x in command]
        print('+', ' '.join(command), flush=True)
        proc = subprocess.run(command, capture_output=True, env={**os.environ, 'PYTHONDONTWRITEBYTECODE': '1'})
        good = proc.returncode == expected
        if contains is not None:
            good = good and contains.encode() in proc.stderr + proc.stdout
        if binary_hash is not None:
            good = good and hashlib.sha256(proc.stdout).hexdigest() == binary_hash
        if output:
            output.write_bytes(proc.stdout)
        elif binary_hash is None:
            print(proc.stdout.decode(errors='replace'), end='')
        print(proc.stderr.decode(errors='replace'), end='', file=sys.stderr)
        results.append({'name': label, 'command': command, 'exit_code': proc.returncode, 'status': 'PASS' if good else 'FAIL'})
        if not good:
            (args.out/'results.json').write_text(json.dumps(results, indent=2)+'\n')
            raise SystemExit('FAIL: '+label)
        return proc

    flags = ['-std=c++20', '-O3', '-fomit-frame-pointer', '-fno-lto', '-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wshadow', '-Werror']
    sources = ['benchmark.cpp', 'candidate.cpp', 'baseline1.cpp', 'baseline2.cpp', 'baseline3.cpp', 'baseline4.cpp', 'payload.cpp']
    run('compiler_version', [args.compiler, '--version'])
    for sanitized in (False, True):
        name = 'sanitized' if sanitized else 'optimized'
        out = args.out/name
        out.mkdir(exist_ok=True)
        extra = ['-O1', '-g', '-fno-omit-frame-pointer', '-fsanitize=address,undefined', '-fno-sanitize-recover=all'] if sanitized else []
        for source in sources+['analyze.cpp']:
            src = ROOT/source
            run(name+'_compile_'+source, [args.compiler, *flags, *extra, '-c', src, '-o', out/(src.stem+'.o')])
            if not sanitized:
                run('assembly_'+source, [args.compiler, *flags, '-S', '-fverbose-asm', src, '-o', out/(src.stem+'.s')])
        for executable, objects in [('benchmark', sources), ('analyze', ['analyze.cpp'])]:
            run(name+'_link_'+executable, [args.compiler, *extra, '-fno-lto', *[out/(Path(s).stem+'.o') for s in objects], '-o', out/executable])
        run(name+'_semantic_selftest', [out/'benchmark', '--selftest'], contains='PASS PCG=4')
        for flag, reason in [('correctness', 'CORRECTNESS_FAILURE'), ('payload', 'PAYLOAD_PARITY')]:
            run(name+'_fault_'+flag, [out/'benchmark', '--selftest', '--test-fault-'+flag], expected=6, contains=reason)
        run(name+'_pcg_reference', [sys.executable, ROOT/'verify_pcg64.py'])
        run(name+'_corpus_differential', [sys.executable, ROOT/'test_corpus.py', out/'benchmark'])
        run(name+'_bootstrap_indices', [out/'benchmark', '--dump-bootstrap-indices'], binary_hash='8c5f235bc6d1df1c1fcdca99903abaaaa8201591d5521fe84ba7e72868770b64')
        run(name+'_legacy_analyzer_disabled', [out/'analyze'], expected=6, contains='LEGACY_ANALYZER_DISABLED')
        run(name+'_workload_authorization_gate', [out/'benchmark'], expected=6, contains='MEASUREMENT_NOT_AUTHORIZED')
    run('protocol_identity_preservation', [sys.executable, ROOT/'test_protocol_identity.py'])
    run('python_r1_regressions', [sys.executable, ROOT/'test_r1.py'])
    run('assembly_inventory', [sys.executable, ROOT/'inspect_assembly.py', args.out/'optimized', '--objdump', args.objdump], output=args.out/'assembly.json')
    if sys.platform != 'linux':
        run('pmu_unavailable_probe', [args.out/'optimized/benchmark', '--probe-pmu'], expected=6, contains='PMU_UNAVAILABLE')
        results.append({'name':'Linux_affinity_governor_boost_native_wait4_PMU', 'status':'SKIP', 'reason':'requires Linux; host is '+sys.platform})
    else:
        probe = subprocess.run([args.out/'optimized/benchmark', '--probe-pmu'], capture_output=True)
        (args.out/'pmu-probe.txt').write_bytes(probe.stderr)
        status = 'PASS' if probe.returncode == 0 else 'SKIP' if probe.returncode == 6 and b'PMU_UNAVAILABLE' in probe.stderr else 'FAIL'
        results.append({'name':'PMU_infrastructure_probe', 'status':status, 'reason':probe.stderr.decode(errors='replace')})
    (args.out/'results.json').write_text(json.dumps(results, indent=2)+'\n')
    counts = {k:sum(r['status']==k for r in results) for k in ('PASS','FAIL','SKIP')}
    print(json.dumps(counts))
    if counts['FAIL']: raise SystemExit(1)


if __name__ == '__main__':
    main()
