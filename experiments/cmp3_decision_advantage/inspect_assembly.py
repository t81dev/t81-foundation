"""Static instruction inventory for emitted assembly; never dynamic PMU counts.
Usage: python3 inspect_assembly.py <optimized-output-directory>.
"""
import pathlib
import re
import sys


def functions(path):
    result = []
    body = None
    for line in path.read_text().splitlines():
        if line.startswith('\t.cfi_startproc'):
            body = []
        elif line.startswith('\t.cfi_endproc'):
            result.append(body)
            body = None
        elif body is not None and re.match(r'^\t[a-z][a-z0-9.]*\s', line):
            body.append(line.split(';')[0].split('#APP')[0].strip())
    return result


def main():
    directory = pathlib.Path(sys.argv[1])
    bodies = {}
    for name in ('candidate', 'baseline1', 'baseline2', 'baseline3', 'baseline4', 'payload'):
        bodies[name] = functions(directory / (name + '.s'))
        print(f'{name}: static_instruction_totals={[len(b) for b in bodies[name]]}')
    # Exactly two no-inline dispatch wrappers and three external payload handlers.
    assert all(len(bodies[name]) == 2 for name in bodies if name != 'payload')
    assert len(bodies['payload']) == 3
    for name in ('candidate', 'baseline2'):
        for body in bodies[name]:
            assert any('targets' in line for line in body), name
            assert any(re.match(r'(br\s+x[0-9]+|jmp\s+\*)', line) for line in body), name
    for name in ('baseline1', 'baseline3', 'baseline4'):
        for body in bodies[name]:
            for target in ('target_less', 'target_equal', 'target_greater'):
                assert any(target in line for line in body), (name, target)
    equal = bodies['candidate'][0] == bodies['baseline2'][0]
    print(f'AC_candidate_baseline2_identical_instructions={equal}')
    print('Static totals include all basic blocks, including mutually exclusive branches.')
    print('These are compiler lowering observations, not ternary advantage evidence.')

if __name__ == '__main__':
    main()
