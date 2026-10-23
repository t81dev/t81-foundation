"""Adversarial producer and verifier tests, including deliberately accepted forgery."""
import argparse
import contextlib
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import wasmtime
from verify import canonical, expected, inventory, load, sha, verify
from arm_b import run

HERE = Path(__file__).resolve().parent


def envelope(record):
    return dict(record=record, sha256=sha(canonical(record)))


class Challenge(unittest.TestCase):
    def test_two_real_arms_and_negative_controls(self):
        with contextlib.nullcontext(str(TEST_OUTPUT or tempfile.mkdtemp(prefix='t81-challenge-tests-'))) as temp:
            output = Path(temp)/'result'
            subprocess.run([sys.executable, HERE/'run.py', '--runner', RUNNER,
                            '--output', output, '--repetitions', '2'], check=True, timeout=120)
            report = load(output/'report.json')
            self.assertEqual(report['failures'], [])
            rows = [json.loads(line) for line in (output/'corpus.jsonl').read_text().splitlines()]
            self.assertEqual(len(rows), 44)
            from audit_corpus import audit
            self.assertEqual(audit(output/'corpus.jsonl')['observations'], 44)
            bad_rows = copy.deepcopy(rows)
            bad_control = next(r for r in bad_rows if r['case'] == 'snapshot-control')
            # Preserve the original hash to challenge bool/int equality in the
            # special negative-control path, not only the ordinary verifier.
            bad_control['bundle']['record']['version'] = True
            attacked = output/'attacked-corpus.jsonl'
            attacked.write_text(''.join(json.dumps(r)+'\n' for r in bad_rows))
            with self.assertRaises(ValueError):
                audit(attacked)
            self.assertEqual(sum('write.commit' in r['bundle']['record']['events'] for r in rows
                                 if r['case'] == 'snapshot-control'), 4)
            for arm in ('A', 'B'):
                # Evidence archive contains exactly the detached bytes, including after crashes.
                for row in (r for r in rows if r['arm'] == arm):
                    self.assertEqual(list(row['evidence_files'].values()),
                                     [(canonical(row['bundle'])+b'\n').hex()])
                for case in ('interrupted-before', 'interrupted-after'):
                    raw = [json.loads(line) for line in (output/'raw.jsonl').read_text().splitlines()]
                    raw = [r for r in raw if r['label'] == f'{case}-{arm}-0'][0]
                    self.assertEqual(raw['returncode'], 75)
                store = output/f'approved-{arm}-0'/'store'
                next(p for p in store.rglob('*') if p.is_file()).write_bytes(b'tampered')
                with self.assertRaises(ValueError):
                    verify(load(output/f'approved-{arm}-0'/'evidence.json'), 'approved', inventory(store), arm)
                with self.assertRaises(ValueError):
                    verify(envelope(expected('revoke-after-compute')), 'revoke-after-compute',
                           {'extra': b'unauthorized'.hex()}, arm)

    def test_self_consistent_false_policy_and_result(self):
        record = expected('revoke-after-check')
        for field, value in [('amount', 1), ('limit', 50000), ('status', 'committed'),
                             ('events', ['compute', 'authorize.allow', 'write.commit']),
                             ('payload_sha256', sha(b'8491 25000\n')), ('request', '1 1\n'),
                             ('transaction_id', '8492'), ('version', True)]:
            bad = dict(record, **{field: value})
            with self.subTest(field=field), self.assertRaises(ValueError):
                verify(envelope(bad), 'revoke-after-check')
        with self.assertRaises(ValueError):
            verify(envelope(expected('approved')), 'revoke-after-check')
        bad = envelope(expected('approved'))
        bad['record']['amount'] = 2
        with self.assertRaises(ValueError):
            verify(bad, 'approved')

    def test_fabricated_correct_story_is_not_attestation(self):
        # No producer is invoked and no purchase order exists. The liar can
        # manufacture the exact expected event story and every hash.
        forged = envelope(expected('approved'))
        accepted = verify(forged, 'approved')
        self.assertEqual(accepted['producer_identity'], 'UNAUTHENTICATED')
        self.assertEqual(accepted['factual_correctness'], 'UNESTABLISHED')
        with self.assertRaises(ValueError):
            verify(forged, 'approved', {}, 'A')
        with self.assertRaises(ValueError):
            verify(forged, 'approved', {}, 'B')
        # If the producer also supplies the supposed observer's inventory,
        # consistency cannot authenticate it. This is a preserved failure.
        from verify import expected_inventory
        for arm in ('A', 'B'):
            self.assertTrue(verify(forged, 'approved', expected_inventory(forged['record'], arm), arm)['storage_checked'])

    def test_strict_json_and_schema(self):
        with tempfile.TemporaryDirectory() as temp:
            f = Path(temp)/'bad.json'
            for text in ('{"record":1,"record":2}', '{"bad":NaN}'):
                f.write_text(text)
                with self.assertRaises(ValueError):
                    load(f)
        for change in ('missing', 'extra', 'boolean'):
            bad = expected('approved')
            if change == 'missing': del bad['limit']
            if change == 'extra': bad['signed'] = True
            if change == 'boolean': bad['amount'] = True
            with self.subTest(change=change), self.assertRaises(ValueError):
                verify(envelope(bad), 'approved')

    def test_wasm_capability_is_the_enforcement_boundary(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            inp = root/'input.txt'
            inp.write_bytes(b'100 250\n')
            for value in (-1, 1, 25001, 1000000):
                guest = root/'evil.wat'
                guest.write_text('(module (import "host" "submit" (func $s (param i32) (result i32))) '
                                 '(func (export "run") (param i32 i32) (result i32) '
                                 f'i32.const {value} call $s))')
                result = run('approved', inp, root/str(value), guest)
                self.assertEqual(result['status'], 'denied')
                self.assertEqual(inventory(root/str(value)), {})
            guest.write_text('(module (import "wasi_snapshot_preview1" "path_open" (func (param i32) (result i32))) '
                             '(func (export "run") (param i32 i32) (result i32) i32.const 1))')
            with self.assertRaises(wasmtime.WasmtimeError):
                run('approved', inp, root/'no-wasi', guest)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--runner', required=True, type=Path)
    parser.add_argument('--output-root', type=Path)
    args, rest = parser.parse_known_args()
    RUNNER = str(args.runner.resolve())
    TEST_OUTPUT = args.output_root.resolve() if args.output_root else None
    unittest.main(argv=[__file__, *rest])
