"""Audit public native artifacts with only the independent verifier, never either producer."""
import argparse
import hashlib
import json
from pathlib import Path
from verify import canonical, checked, load


def audit(root):
    root = Path(root)
    result = load(root / 'tests' / 'results.json')
    if result['failures'] or len(result['observations']) != 48:
        raise ValueError('native tests failed or incomplete')
    receipts = mismatches = bypasses = 0
    for row in result['observations']:
        folder = root / 'tests' / (row['arm'] + '-' + row['case'])
        if load(folder / 'outcome.json') != row or not row['passed']:
            raise ValueError('test evidence mismatch')
        expected, trust = load(folder / 'expected.json'), load(folder / 'trust.json')
        if load(folder / 'producer.json')['amount'] != 25000:
            raise ValueError('unexpected producer proposal')
        ledger = row.get('ledger', {})
        signed = {}
        for receipt in ledger.get('receipts', []):
            body = checked(receipt, trust, expected)
            signed[body['transaction']] = body['effect']['payload'].encode('ascii').hex()
            receipts += 1
        effect_ids = {effect['transaction'] for effect in ledger.get('effects', [])}
        committed_ids = {txid for txid, state in ledger.get('transactions', []) if state == 'committed'}
        if set(signed) != committed_ids or not set(signed).issubset(effect_ids):
            raise ValueError('signed outbox lacks corresponding committed observed effect')
        for effect in ledger.get('effects', []):
            if signed.get(effect['transaction']) != effect['payload']:
                # Expected, explicitly preserved fault/permission negative controls.
                if row['case'] == 'excluded-same-uid-bypass' and row['category'] == 'excluded-negative-control':
                    bypasses += 1
                elif row['case'] == 'inconsistent-recovery' and row['observed']['outcome'] == 'inconsistent-recovery-rejected':
                    mismatches += 1
                else:
                    raise ValueError('observed effect lacks matching independently verifiable receipt')
    if (bypasses, mismatches) != (2, 2):
        raise ValueError('lost expected negative evidence')
    measurements = load(root / 'measurements' / 'measurements.json')
    if any(row.get('returncode', 0) != 0 for row in measurements['samples']):
        raise ValueError('measurement subprocess failed')
    return dict(checks=len(result['observations']), signed_receipts_checked=receipts,
                explicit_unreceipted_raw_bypasses=bypasses,
                explicit_corrupt_payload_recovery_rejections=mismatches,
                excluded_controls=result['accepted_excluded_negative_controls'],
                platform=measurements['platform'], machine=measurements['machine'],
                sqlite=measurements['sqlite'], source_revision=measurements['source_revision'],
                source_sha256=measurements['source_sha256'],
                result_sha256=hashlib.sha256(canonical(result)).hexdigest(),
                warning='Audit uses test fixture pins; real trust anchors must arrive out of band')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    args = parser.parse_args()
    print(json.dumps(audit(args.root), indent=2))
