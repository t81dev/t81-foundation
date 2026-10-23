"""Defined physical source inventories; no unsupported total-stack LOC estimate."""
import argparse
import importlib.metadata
import json
from pathlib import Path
import re
import subprocess
from verify import canonical, sha

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def source_inventory(paths):
    rows = {}
    for p in sorted(set(paths)):
        data = p.read_bytes()
        rows[str(p.relative_to(ROOT)) if p.is_relative_to(ROOT) else str(p)] = dict(
            bytes=len(data), physical_lines=len(data.splitlines()),
            nonblank_lines=sum(bool(v.strip()) for v in data.splitlines()), sha256=sha(data))
    return dict(files=rows, total_bytes=sum(v['bytes'] for v in rows.values()),
                total_physical_lines=sum(v['physical_lines'] for v in rows.values()),
                total_nonblank_lines=sum(v['nonblank_lines'] for v in rows.values()))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    commands = subprocess.check_output(['ninja','-C',str(args.build),'-t','commands','t81_architecture_arm_a'],text=True)
    deps = subprocess.check_output(['ninja','-C',str(args.build),'-t','deps'],text=True)
    closure = {Path(v) for v in re.findall(r' -c (\S+)', commands)}
    closure.update(Path(line.strip()) for line in deps.splitlines() if line.startswith('    ')
                   and line.strip().startswith(str(ROOT)))
    closure = {v for v in closure if v.is_file() and v.is_relative_to(ROOT)}
    distributions = {}
    for name in ('wasmtime','importlib_resources'):
        dist = importlib.metadata.distribution(name)
        files = [Path(dist.locate_file(v)).resolve() for v in dist.files]
        files = [v for v in files if v.is_file()]
        distributions[name] = dict(version=dist.version, installed_bytes=sum(v.stat().st_size for v in files),
                                   file_count=len(files), python_source=source_inventory([v for v in files if v.suffix=='.py']))
    report = dict(rules='Count every physical/nonblank line, including comments; bytes are uncompressed file bytes. No generated corpus, docs, build outputs or vendored source in adapter counts. Native closure is actual Ninja compile inputs plus repo headers for this otherwise single-target build. Dependency wheel sizes include metadata/native libraries, exclude Python interpreter/stdlib/system shared libraries. Installed sizes are not resident memory or full TCB size.',
                  adapters={arm:source_inventory([HERE/name for name in names]) for arm,names in {
                      'A':['arm_a.cpp'], 'B':['arm_b.py','guest.wat'],
                      'shared':['run.py','verify.py','audit_corpus.py','audit_archives.py'],
                      'validation':['test_challenge.py','attacks.py','count.py']}.items()},
                  native_compiled_source_closure=source_inventory(closure),
                  native_executable_bytes=(args.build/'t81_architecture_arm_a').stat().st_size,
                  python_dependencies=distributions,
                  unknowns=['Wasmtime Rust/C implementation source LOC not present in wheel',
                            'Compiler/linker, Python, OS/filesystem and system library source/maintenance cost',
                            'No apples-to-apples total dependency LOC or resident-memory comparison is established'])
    args.output.write_bytes(canonical(report)+b'\n')

if __name__ == '__main__':
    main()
