"""Corrective contracts; T81_ARM_A_BIN must name a real, freshly built binary."""
import copy
from contextlib import closing
import hashlib
import hmac
import json
import os
from pathlib import Path
import sqlite3
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import runner
import witness
import verify

KEY = b'public-corrective-test-fixture-only'
BINARY = Path(os.environ.get('T81_ARM_A_BIN', 'build/t81_architecture_arm_a')).resolve()


def historical_signature(secret_key, tx_id, authority_version, effect_id,
                         payload_hash='', total_amount=0, timestamp='',
                         witness_id='durable-witness-v1'):
    # Historical delimiter encoding. Extra fields are held identical in the attack.
    return hmac.new(secret_key, f'{tx_id}:{authority_version}:{effect_id}'.encode(),
                    hashlib.sha256).hexdigest()


class CorrectiveContracts(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.db = self.root / 'witness.db'
        witness.init_db(self.db).close()

    def execute(self, arm='B', tx='tx', authority='v2', quantity=5, price=100, **kw):
        return runner.execute_runner(arm, tx, authority, quantity, price, self.db,
                                     self.root / 'out', KEY, arm_a_bin=kw.pop('arm_a_bin', BINARY), **kw)

    def assert_empty(self):
        with closing(sqlite3.connect(self.db)) as conn:
            for table in ('transactions', 'effects'):
                self.assertEqual(conn.execute(f'SELECT COUNT(*) FROM {table}').fetchone()[0], 0)

    @unittest.skipUnless(BINARY.is_file(), 'UNAVAILABLE: native T81 binary')
    def test_native_denial_is_evidence(self):
        ev = self.execute('A', quantity=100, price=600)
        self.assertFalse(ev['success'])
        self.assertEqual(ev['status_code'], 'POLICY_LIMIT_EXCEEDED')
        self.assertEqual(ev['runtime']['status'], 'denied')
        self.assertIn('authorize.deny', ev['runtime']['events'])
        self.assertIsNone(ev['receipt'])
        self.assert_empty()

    @unittest.skipUnless(BINARY.is_file(), 'UNAVAILABLE: native T81 binary')
    def test_defined_outcome_parity(self):
        for label, auth, q, p, status in [
            ('approved', 'v2', 5, 100, 'SUCCESS'),
            ('over-limit', 'v2', 100, 600, 'POLICY_LIMIT_EXCEEDED'),
            ('revoked', 'v1', 5, 100, 'REVOCATION_DETECTED'),
            ('revoked-over-limit', 'v1', 100, 600, 'REVOCATION_DETECTED'),
            ('unknown', 'absent', 5, 100, 'INVALID_AUTHORITY_VERSION'),
            ('malformed', 'v2', 0, 100, 'MALFORMED_INPUT'),
        ]:
            for arm in ('A', 'B'):
                with self.subTest(case=label, arm=arm):
                    ev = self.execute(arm, tx=f'{label}-{arm}', authority=auth, quantity=q, price=p)
                    self.assertEqual(ev['status_code'], status)
                    self.assertEqual(ev['success'], status == 'SUCCESS')
                    if ev['success']:
                        self.assertEqual(ev['total_amount'], ev['receipt']['total_amount'])
                        verdict = verify.verify_evidence(self.root / 'out' / f'evidence_{label}-{arm}.json', self.db, KEY)
                        self.assertTrue(verdict['valid'])
                        self.assertFalse(verdict['effect_observed'])
        with closing(sqlite3.connect(self.db)) as conn:
            self.assertEqual(conn.execute('SELECT COUNT(*) FROM transactions').fetchone()[0], 2)
            self.assertEqual(conn.execute('SELECT COUNT(*) FROM effects').fetchone()[0], 2)

    def test_incorrect_runtime_result_rejected_both_arms(self):
        for arm, function, result in [
            ('A', 'run_arm_a', {'status': 'committed', 'amount': 499, 'events': ['write.commit']}),
            ('B', 'run_arm_b', 499),
        ]:
            with self.subTest(arm=arm), patch.object(runner, function, return_value=result):
                ev = self.execute(arm)
                self.assertFalse(ev['success'])
                self.assertEqual(ev['status_code'], 'COMPUTED_AMOUNT_MISMATCH')
                self.assertIsNone(ev['receipt'])
                self.assert_empty()

    def test_witness_requires_computed_amount(self):
        po = {'quantity': 5, 'unit_price': 100}
        for amount in (499, None, '500', True):
            with self.subTest(amount=amount):
                ok, _, status = witness.submit_transaction(self.db, 'bad', 'v2', po, KEY,
                                                           computed_amount=amount)
                self.assertFalse(ok)
                self.assertEqual(status, 'COMPUTED_AMOUNT_MISMATCH')
                self.assert_empty()
        with self.assertRaises(TypeError):
            witness.submit_transaction(self.db, 'missing', 'v2', po, KEY)

    def test_native_malformed_responses_and_runtime_failure(self):
        fixture = self.root / 'mock-native'
        fixture.touch()  # subprocess is explicitly mocked in this contract test
        for stdout in ('not-json', '[]', '{}', '{"status":"committed","amount":"500"}',
                       '{"status":"unexpected","amount":500}',
                       '{"status":"committed","amount":true}',
                       '{"status":"malformed","amount":0,"events":["input.reject"]}'):
            with self.subTest(stdout=stdout), patch.object(runner.subprocess, 'run',
                    return_value=subprocess.CompletedProcess([], 0, stdout, '')):
                ev = self.execute('A', arm_a_bin=fixture)
                self.assertFalse(ev['success'])
                self.assertEqual(ev['status_code'], 'MALFORMED_RUNTIME_RESPONSE')
                self.assert_empty()
        with patch.object(runner.subprocess, 'run', return_value=subprocess.CompletedProcess([], 2, '', 'VM fault')):
            ev = self.execute('A', arm_a_bin=fixture)
            self.assertEqual(ev['status_code'], 'RUNTIME_FAILURE')
            self.assert_empty()

    def test_wasm_unavailable_and_trapped(self):
        with patch.dict('sys.modules', {'wasmtime': None}):
            ev = self.execute()
        self.assertEqual(ev['status_code'], 'RUNTIME_UNAVAILABLE')
        self.assert_empty()
        guest = self.root / 'trap.wat'
        guest.write_text('(module (import "host" "submit" (func (param i32) (result i32))) '
                         '(func (export "run") (param i32 i32) (result i32) unreachable))')
        ev = self.execute(wat_file=guest)
        self.assertEqual(ev['status_code'], 'RUNTIME_FAILURE')
        self.assert_empty()

    def test_unavailable_runtime_has_no_fallback(self):
        with patch.object(runner, 'run_arm_b', side_effect=AssertionError('must not fallback')):
            ev = runner.execute_runner('A', 'unavailable', 'v2', 5, 100, self.db,
                                       self.root / 'out', KEY, arm_a_bin=self.root / 'missing')
        self.assertFalse(ev['success'])
        self.assertEqual(ev['status_code'], 'RUNTIME_UNAVAILABLE')
        self.assert_empty()

    def test_historical_collision_and_substitution(self):
        fields = dict(effect_id='eff1', payload_hash='hash1', total_amount=100,
                      timestamp='2026-10-24T12:00:00Z', witness_id='durable-witness-v1')
        a = dict(tx_id='tx:1', authority_version='v2', **fields)
        b = dict(tx_id='tx', authority_version='1:v2', **fields)
        self.assertEqual(f"{a['tx_id']}:{a['authority_version']}:{a['effect_id']}".encode(),
                         f"{b['tx_id']}:{b['authority_version']}:{b['effect_id']}".encode())
        self.assertEqual(historical_signature(KEY, **a), historical_signature(KEY, **b))
        def canonical(x):
            return json.dumps(dict(v=1, **x), sort_keys=True, separators=(',', ':')).encode()
        self.assertNotEqual(canonical(a), canonical(b))
        self.assertNotEqual(witness.compute_receipt_signature(KEY, **a),
                            witness.compute_receipt_signature(KEY, **b))

        ev = self.execute(tx='tx:1')
        self.assertTrue(ev['success'])
        tampered = copy.deepcopy(ev)
        tampered['receipt'].update(tx_id='tx', authority_version='1:v2')
        path = self.root / 'substituted.json'
        path.write_text(json.dumps(tampered))
        self.assertEqual(verify.verify_evidence(path, None, KEY)['reason'], 'INVALID_RECEIPT_SIGNATURE')

    def test_old_encoding_negative_control_detects_regression(self):
        # Run the actual security assertion with the historical signer restored.
        # A passing nested test would mean our regression is ineffective.
        case = CorrectiveContracts('test_historical_collision_and_substitution')
        result = unittest.TestResult()
        with patch.object(witness, 'compute_receipt_signature', historical_signature), \
             patch.object(verify, 'compute_receipt_signature', historical_signature):
            case.run(result)
        self.assertEqual(len(result.failures), 1)
        self.assertEqual(len(result.errors), 0)

    def test_incorrect_wasm_guest_result(self):
        guest = self.root / 'incorrect.wat'
        guest.write_text('(module (import "host" "submit" (func $submit (param i32) (result i32))) '
                         '(func (export "run") (param i32 i32) (result i32) i32.const 499 call $submit))')
        ev = self.execute(wat_file=guest)
        self.assertEqual(ev['total_amount'], 499)
        self.assertEqual(ev['status_code'], 'COMPUTED_AMOUNT_MISMATCH')
        self.assertFalse(ev['success'])
        self.assert_empty()

    def test_native_denial_cannot_become_success(self):
        with patch.object(runner, 'run_arm_a', return_value={
                'status': 'denied', 'amount': 500, 'events': ['authorize.deny']}):
            ev = self.execute('A')
        self.assertFalse(ev['success'])
        self.assertEqual(ev['status_code'], 'RUNTIME_POLICY_DENIED')
        self.assert_empty()

    def test_verifier_rejects_substituted_runtime_amount(self):
        ev = self.execute()
        ev['total_amount'] = 499
        path = self.root / 'tampered-amount.json'
        path.write_text(json.dumps(ev))
        self.assertEqual(verify.verify_evidence(path, self.db, KEY)['reason'], 'AMOUNT_BINDING_MISMATCH')

    def test_replay_keeps_original_evidence(self):
        self.assertTrue(self.execute()['success'])
        self.assertEqual(self.execute()['status_code'], 'REPLAY_DETECTED')
        self.assertTrue(json.loads((self.root / 'out' / 'evidence_tx.json').read_text())['success'])
        self.assertFalse(json.loads((self.root / 'out' / 'evidence_tx.attempt-1.json').read_text())['success'])

    def test_database_policy_limits_are_enforced(self):
        with closing(sqlite3.connect(self.db)) as conn:
            with conn:
                conn.execute("UPDATE authority_policies SET max_total_amount=400 WHERE authority_version='v2'")
        for arm, function, result in [
            ('A', 'run_arm_a', {'status': 'committed', 'amount': 500, 'events': ['write.commit']}),
            ('B', 'run_arm_b', 500),
        ]:
            with self.subTest(arm=arm), patch.object(runner, function, return_value=result):
                ev = self.execute(arm)
                self.assertEqual(ev['status_code'], 'POLICY_LIMIT_EXCEEDED')
                self.assert_empty()

    def test_partial_write_rolls_back_effect(self):
        with closing(sqlite3.connect(self.db)) as conn:
            conn.execute("CREATE TRIGGER fail_transaction BEFORE INSERT ON transactions BEGIN SELECT RAISE(ABORT, 'injected failure'); END")
        ev = self.execute()
        self.assertFalse(ev['success'])
        self.assertEqual(ev['status_code'], 'INTEGRITY_ERROR')
        self.assert_empty()


if __name__ == '__main__':
    unittest.main(verbosity=2)
