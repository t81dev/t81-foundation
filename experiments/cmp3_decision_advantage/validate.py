"""Run from any directory: python3 validate.py --compiler clang++ --out /tmp/comp3.
Builds every experiment TU with strict warnings, no LTO, assembly, and ASan/UBSan.
"""
import argparse
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent

def run(args, expected=0):
    print('+', ' '.join(map(str, args)), flush=True)
    result = subprocess.run(list(map(str, args)))
    if result.returncode != expected:
        raise SystemExit(f'FAIL exit={result.returncode}, expected={expected}')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--compiler', required=True)
    parser.add_argument('--out', type=pathlib.Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    flags = ['-std=c++20', '-O3', '-fomit-frame-pointer', '-fno-lto', '-Wall', '-Wextra',
             '-Wpedantic', '-Wconversion', '-Wshadow', '-Werror']
    sources = ['benchmark.cpp', 'candidate.cpp', 'baseline1.cpp', 'baseline2.cpp',
               'baseline3.cpp', 'baseline4.cpp', 'payload.cpp']
    run([args.compiler, '--version'])
    for sanitized in (False, True):
        out = args.out / ('sanitized' if sanitized else 'optimized')
        out.mkdir(exist_ok=True)
        extra = ['-O1', '-g', '-fno-omit-frame-pointer', '-fsanitize=address,undefined',
                 '-fno-sanitize-recover=all'] if sanitized else []
        for source in sources + ['analyze.cpp', 'test_classification.cpp']:
            src = ROOT / source
            run([args.compiler, *flags, *extra, '-c', src, '-o', out / (src.stem + '.o')])
            if not sanitized:
                run([args.compiler, *flags, '-S', '-fverbose-asm', src, '-o', out / (src.stem + '.s')])
        for executable, objects in [('benchmark', sources), ('analyze', ['analyze.cpp']),
                                    ('test_classification', ['test_classification.cpp'])]:
            run([args.compiler, *extra, '-fno-lto',
                 *[out / (pathlib.Path(s).stem + '.o') for s in objects], '-o', out / executable])
        run([out / 'benchmark', '--selftest'])
        run([out / 'test_classification'])
        run([sys.executable, ROOT / 'verify_pcg64.py'])
        run([sys.executable, ROOT / 'test_corpus.py', out / 'benchmark'])
        run([sys.executable, ROOT / 'test_analysis.py', out / 'analyze'])
        # This host intentionally cannot collect Linux PMU counters.
        if sys.platform != 'linux':
            run([out / 'benchmark', '--smoke'], expected=6)
    print('VALIDATION_PASS')

if __name__ == '__main__':
    main()
