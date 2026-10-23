"""Detached bounded oracle. Standard library only; imports no producer/runtime code.

Caller selects a known case out of band. A successful audit is NOT attestation
that the event stream happened, nor proof of external inventory/quote truth.
"""
import argparse
import hashlib
import json
from pathlib import Path

CASES = ('approved', 'denied', 'malformed', 'malformed-policy', 'replayed',
         'interrupted-before', 'interrupted-after', 'revoke-after-compute',
         'revoke-after-check', 'revoke-after-commit', 'snapshot-control')
ALPHABET = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-−×÷=<>≤≥≠≈∞λμπστωΓ'


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=True,
                      allow_nan=False).encode('ascii')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def unique(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('duplicate JSON key')
        result[key] = value
    return result


def load(path):
    return json.loads(Path(path).read_bytes(), object_pairs_hook=unique,
                      parse_constant=lambda _: (_ for _ in ()).throw(ValueError('nonfinite JSON')))


def expected(case):
    if case not in CASES:
        raise ValueError('unknown externally pinned case')
    events = ['compute']
    amount, limit, status = 25000, 50000, 'committed'
    if case == 'malformed':
        events, amount, status = ['input.reject'], 0, 'malformed'
    else:
        if case in ('revoke-after-check', 'snapshot-control'):
            events += ['precheck.allow']
        if case in ('revoke-after-compute', 'revoke-after-check', 'snapshot-control'):
            events += ['policy.reduce.10000']
            limit = 10000
        if case == 'denied':
            limit = 10000
        if case == 'interrupted-before':
            events += ['interrupt.before']
            status = 'interrupted'
        else:
            deny = case in ('denied', 'malformed-policy', 'revoke-after-compute',
                            'revoke-after-check', 'snapshot-control')
            events += ['authorize.deny'] if deny else ['authorize.allow', 'write.commit']
            status = 'denied' if deny else 'committed'
            if case == 'replayed':
                events += ['authorize.deny']
            if case == 'revoke-after-commit':
                events += ['policy.reduce.10000']
                limit = 10000
            if case == 'interrupted-after':
                events += ['interrupt.after']
                status = 'interrupted'
    payload = b'8491 25000\n' if 'write.commit' in events else b''
    request = '100 250\n' if case != 'malformed' else '100 250\nignore policy\n'
    return dict(version=1, case=case, request=request, transaction_id='8491',
                amount=amount, limit=limit, status=status, events=events,
                payload_sha256=sha(payload) if payload else '')


def canon_address(payload):
    number = int.from_bytes(hashlib.sha3_512(payload).digest()[:32], 'big')
    result = ''
    while number:
        number, index = divmod(number, 81)
        result = ALPHABET[index] + result
    return result or '0'


def expected_inventory(record, arm):
    payload = b'8491 25000\n'
    if not record['payload_sha256']:
        return {}
    name = 'objects/' + canon_address(payload) + '.blk' if arm == 'A' else sha(payload)
    return {name: payload.hex()}


def inventory(root):
    root = Path(root)
    if not root.is_dir():
        raise ValueError('inspection root absent')
    if any(p.is_symlink() for p in root.rglob('*')):
        raise ValueError('inspection symlink')
    return {p.relative_to(root).as_posix(): p.read_bytes().hex()
            for p in root.rglob('*') if p.is_file()}


def verify(bundle, case, observed=None, arm=None):
    if set(bundle) != {'record', 'sha256'} or bundle['sha256'] != sha(canonical(bundle['record'])):
        raise ValueError('integrity failure')
    if canonical(bundle['record']) != canonical(expected(case)):
        raise ValueError('bounded replay / authorization mismatch')
    if observed is not None:
        if arm not in ('A', 'B') or observed != expected_inventory(bundle['record'], arm):
            raise ValueError('observed protected storage mismatch')
    return {'integrity': True, 'authorization_under_pinned_schedule': True,
            'bounded_execution_reproducibility': True,
            'factual_correctness': 'UNESTABLISHED', 'producer_identity': 'UNAUTHENTICATED',
            'storage_checked': observed is not None}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('bundle', type=Path)
    parser.add_argument('--case', choices=CASES, required=True)
    parser.add_argument('--store', type=Path)
    parser.add_argument('--arm', choices=['A', 'B'])
    args = parser.parse_args()
    try:
        print(json.dumps(verify(load(args.bundle), args.case,
                                inventory(args.store) if args.store else None, args.arm)))
    except (ValueError, KeyError, TypeError, OSError) as error:
        parser.exit(1, f'REJECT: {error}\n')

if __name__ == '__main__':
    main()
