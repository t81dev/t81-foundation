#!/usr/bin/env python3
"""Non-destructive wrapper probes: fake t81 only, scratch HOME/store, no network.

This collects defects; a zero collector exit does not mean wrappers work.
No production wrapper is changed and no model/program is executed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[4]
BASES = {'ai-tasks':'assess.sh','bundle-creator':'create-bundle.sh',
         'bundle-validator':'validate.sh','canonfs-export':'export.sh',
         'canonfs-import':'import.sh','governance-demo':'demo-governance.sh',
         'model-loader':'load-hf.sh','policy-checker':'check.sh',
         't81-compiler':'build.sh','t81-debugger':'debug.sh',
         't81vm-runner':'run.sh','tensor-ops':'tensor-math.sh'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('output must not exist; preserve prior observations')
    observations = []
    with tempfile.TemporaryDirectory(prefix='t81-skills-audit-') as temporary:
        scratch = Path(temporary)
        fixture = scratch / 'fixture.t81'
        fixture.write_text('fn main() -> i32 { return 0; }\n')
        fake = scratch / 'bin'
        fake.mkdir()
        executable = fake / 't81'
        executable.write_text('#!/bin/bash\nprintf "%s\\0" "$@" >> "$AUDIT_CAPTURE"\nexit 0\n')
        executable.chmod(0o700)
        capture = scratch / 'capture'
        env = dict(os.environ, HOME=str(scratch / 'home'),
                   T81_CANONFS_ROOT=str(scratch / 'store'),
                   T81_POLICY_PATH=str(scratch / 'policies'),
                   AUDIT_CAPTURE=str(capture), PATH=str(fake) + os.pathsep + os.environ['PATH'])
        for variable in ('BASH_ENV', 'ENV', 'SHELLOPTS', 'BASHOPTS', 'CDPATH'):
            env.pop(variable, None)
        (scratch / 'home').mkdir()

        def run(id, argv, kind):
            if capture.exists():
                capture.unlink()
            before = sorted(p.relative_to(scratch).as_posix() for p in scratch.rglob('*'))
            result = subprocess.run(argv, cwd=scratch, env=env, text=True,
                                    capture_output=True, timeout=10)
            called = capture.read_bytes().split(b'\0')[:-1] if capture.exists() else []
            after = sorted(p.relative_to(scratch).as_posix() for p in scratch.rglob('*'))
            observations.append(dict(id=id,kind=kind,argv=argv,exit_code=result.returncode,
                                     stdout=result.stdout,stderr=result.stderr,
                                     fake_cli_arguments=[s.decode() for s in called],
                                     created_paths=sorted(set(after)-set(before))))

        for script in sorted((ROOT / 'skills').glob('*/*.sh')):
            syntax = subprocess.run(['/bin/bash','-n',str(script)],capture_output=True,text=True)
            assert syntax.returncode == 0, (script,syntax.stderr)
            values = ['--iterations','1'] if script.parent.name == 'governance-demo' else [str(fixture)]
            if script.parent.name == 'tensor-ops':
                values = ['add',str(fixture),str(fixture)]
            if script.parent.name == 'policy-checker':
                values = ['import',str(fixture)]
            run(str(script.relative_to(ROOT)), ['/bin/bash',str(script),*values], 'ordinary_unmodified_wrapper')
        for script in sorted((ROOT / 'skills/governance-demo').glob('*.sh')):
            run(str(script.relative_to(ROOT)), ['/bin/bash',str(script)], 'empty_argument_mode_probe_fake_cli')
        # Explicitly different invocation context: sourcing under a conditional
        # suppresses errexit inside the wrapper. Only benign printf expansion is
        # used; fake t81 records argv and never interprets a program or contacts a service.
        literal = scratch / 'input$(printf AUDIT_CANARY)'
        literal.write_text('audit fixture')
        run('wrapper-prepolicy-mkdir', ['/bin/bash',
            str(ROOT/'skills/canonfs-import/import.sh'),str(fixture),
            '--canonfs-root',str(scratch/'prepolicy-store'),
            '--policy',str(scratch/'missing.apl')], 'missing_policy_wrapper_preflight')
        run('eval-ordinary-execution', ['/bin/bash',
            str(ROOT/'skills/canonfs-import/import.sh'),str(literal)],
            'ordinary_wrapper_benign_shell_expansion_probe')
        run('eval-conditional-source', ['/bin/bash','-c',
            'if source "$1" "${@:2}"; then exit 0; else exit $?; fi',
            'import.sh',str(ROOT/'skills/canonfs-import/import.sh'),str(literal)],
            'conditional_source_semantics_not_normal_entrypoint')
    receipt = dict(scope='Wrapper-only fake-CLI probes, not successful T81 operations or security certification',
                   repository_revision=subprocess.check_output(['git','-C',str(ROOT),'rev-parse','HEAD'],text=True).strip(),
                   platform=platform.platform(),python=platform.python_version(),
                   bash=subprocess.check_output(['/bin/bash','--version'],text=True).splitlines()[0],
                   observations=observations,
                   script_sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
                                  for p in sorted((ROOT/'skills').glob('*/*.sh'))})
    args.output.write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(ordinary_probes=66,syntax_checks=66,
                         backend_calls_in_ordinary_probes=sum(bool(o['fake_cli_arguments']) for o in observations if o['kind']=='ordinary_unmodified_wrapper'),
                         receipt=str(args.output))))


if __name__ == '__main__':
    main()
