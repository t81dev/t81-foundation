"""Preserve adversarial bundles and observed outcomes, including accepted lies."""
import argparse
import copy
import json
from pathlib import Path
from verify import canonical, expected, expected_inventory, sha, verify


def exercise(output):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=False)
    record = expected('approved')
    cases = []
    def add(name, candidate, pinned, observed, arm, should_accept, gap=None):
        bundle = dict(record=candidate, sha256=sha(canonical(candidate)))
        try:
            result = verify(bundle, pinned, observed, arm)
            accepted = True
        except (ValueError, KeyError, TypeError) as error:
            accepted, result = False, str(error)
        row = dict(name=name, bundle=bundle, externally_pinned_case=pinned,
                   alleged_observation=observed, arm=arm, accepted=accepted,
                   expected_acceptance=should_accept, result=result, security_gap=gap)
        cases.append(row)
        if accepted != should_accept:
            raise AssertionError(name)
    for arm in ('A', 'B'):
        add('rehash-false-limit', dict(expected('revoke-after-check'), limit=50000),
            'revoke-after-check', None, arm, False)
        add('substitute-approved-story-for-revoked-case', record, 'revoke-after-check', None, arm, False)
        add('tamper-storage', record, 'approved', {'fake': b'false'.hex()}, arm, False)
        add('invent-correct-story-without-executing', record, 'approved', None, arm, True,
            'No authenticated external execution/commit witness; detached acceptance cannot establish occurrence')
        add('invent-story-and-observer-inventory', record, 'approved', expected_inventory(record, arm), arm, True,
            'Untrusted inventory is not an independent observation')
        for index in (1, 2):
            add(f'replay-valid-bundle-{index}', record, 'approved', None, arm, True,
                'Stateless offline verification has no trusted freshness/one-shot transaction registry')
    (output/'attacks.json').write_bytes(canonical(cases)+b'\n')
    return cases

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', required=True)
    args = p.parse_args()
    rows = exercise(args.output)
    print(json.dumps(dict(cases=len(rows), accepted_security_gaps=sum(bool(r['security_gap']) for r in rows))))
