"""Offline audit of archived observations; no producer, T81 or Wasmtime imports.

The archive is evidence of consistency only. Its observer must be trusted out of
band if any claim about actual file existence is to follow.
"""
import argparse
import json
from pathlib import Path
from verify import CASES, canonical, canon_address, expected, expected_inventory, sha, unique, verify


def audit(path):
    rows = [json.loads(line, object_pairs_hook=unique) for line in Path(path).read_text().splitlines()]
    if not rows:
        raise ValueError('empty corpus')
    seen, pairs, stable = set(), {}, {}
    for row in rows:
        arm, case, rep = row['arm'], row['case'], row['repetition']
        if arm not in ('A', 'B') or case not in CASES or type(rep) is not int or rep < 0:
            raise ValueError('invalid observation identity')
        key = (arm, case, rep)
        if key in seen:
            raise ValueError('duplicate observation')
        seen.add(key)
        try:
            verify(row['bundle'], case, row['protected_files'], arm)
        except ValueError:
            if case != 'snapshot-control':
                raise
            control = expected(case)
            control['events'] = ['compute', 'precheck.allow', 'policy.reduce.10000', 'authorize.allow', 'write.commit']
            control['status'] = 'committed'
            control['payload_sha256'] = sha(b'8491 25000\n')
            if canonical(row['bundle']) != canonical({'record':control, 'sha256':sha(canonical(control))}):
                raise ValueError('invalid negative control evidence')
            if row['protected_files'] != expected_inventory(control, arm):
                raise ValueError('negative control storage mismatch')
        else:
            if case == 'snapshot-control':
                raise ValueError('negative control was not rejected')
        data = canonical(row['bundle'])+b'\n'
        name = 'objects/'+canon_address(data)+'.blk' if arm == 'A' else sha(data)
        if row['evidence_files'] != {name:data.hex()}:
            raise ValueError('evidence archive mismatch')
        pair_key = (case, rep)
        if pair_key in pairs and pairs[pair_key] != data:
            raise ValueError('arms disagree at protocol boundary')
        pairs[pair_key] = data
        if (arm, case) in stable and stable[(arm, case)] != data:
            raise ValueError('within-arm protocol drift')
        stable[(arm, case)] = data
    repetitions = {rep for _, _, rep in seen}
    if repetitions != set(range(len(repetitions))) or seen != {
            (arm, case, rep) for arm in ('A','B') for case in CASES for rep in repetitions}:
        raise ValueError('incomplete comparison')
    return dict(observations=len(rows), paired_protocol_equality=True,
                snapshot_control_rejections=sum(r['case']=='snapshot-control' for r in rows),
                attestation=False)

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('corpus', type=Path)
    args = p.parse_args()
    print(json.dumps(audit(args.corpus)))
