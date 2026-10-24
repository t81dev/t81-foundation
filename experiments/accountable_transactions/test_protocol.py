"""Adversarial two-arm experiment; preserve every outcome, including excluded attacks."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import shutil
import sqlite3
import subprocess
import sys
import uuid
from time import perf_counter_ns as now
from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from verify import DOMAIN, canonical, checked, init_state, load, verify
from witness import Witness, provision

HERE = Path(__file__).resolve().parent
ASSUMPTIONS = ['honest isolated witness and OS', 'no durable-state rollback',
               'externally pinned key/epoch/request/policy', 'local ledger effect only']
SURFACE = 'Key theft, witness compromise, rollback, availability and external business truth remain outside claim.'


def exercise(runner, output):
    output.mkdir(parents=True, exist_ok=False)
    rows, failures, raw = [], [], []

    def cli(root, operation, **kwargs):
        command = [sys.executable, str(HERE / 'witness.py'), str(root), operation]
        for name, value in kwargs.items():
            command.extend(['--' + name.replace('_', '-'), str(value)])
        start = now()
        result = subprocess.run(command, capture_output=True, text=True, timeout=30)
        raw.append(dict(command=command, returncode=result.returncode,
                        stdout=result.stdout, stderr=result.stderr, process_ns=now() - start))
        return result

    def calculate(arm, folder):
        command = [str(runner), '100', '250'] if arm == 'A' else [
            sys.executable, str(HERE / 'arm_b.py'), '100', '250']
        result = subprocess.run(command, capture_output=True, text=True, timeout=30)
        raw.append(dict(command=command, returncode=result.returncode,
                        stdout=result.stdout, stderr=result.stderr))
        if result.returncode:
            raise RuntimeError('calculator failed: ' + result.stderr)
        proposal = json.loads(result.stdout)
        (folder / 'producer.json').write_bytes(canonical(proposal) + b'\n')
        assert proposal['amount'] == 25000
        return proposal['amount']

    for arm in ('A', 'B'):
        def case(name, callback, expected, category='in-model'):
            folder = output / (arm + '-' + name)
            folder.mkdir()
            root = folder / 'witness'
            trust = provision(root)
            exp = dict(transaction=str(uuid.uuid4()), request=dict(order='8491', units=100, price=250),
                       policy_version=1)
            expected_file = folder / 'expected.json'
            expected_file.write_bytes(canonical(exp) + b'\n')
            amount = calculate(arm, folder)
            state = folder / 'verifier.db'
            init_state(state, trust)
            owner = Witness(root)
            try:
                observed = callback(owner, root, exp, expected_file, trust, state, amount, folder)
                passed = observed['outcome'] == expected
                row = dict(arm=arm, case=name, category=category, expected=expected,
                           observed=observed, passed=passed, trust_assumptions=ASSUMPTIONS,
                           remaining_attack_surface=SURFACE)
                if not passed:
                    failures.append(arm + ':' + name)
            except Exception as error:
                row = dict(arm=arm, case=name, expected=expected, passed=False,
                           error=type(error).__name__, detail=str(error))
                failures.append(arm + ':' + name)
            finally:
                owner.close()
            # Preserve observable ledger and receipts; never commit private keys.
            try:
                with sqlite3.connect(root / 'ledger.db') as db:
                    row['ledger'] = dict(
                        transactions=db.execute('SELECT id,state FROM transactions').fetchall(),
                        effects=[dict(transaction=x, payload=bytes(y).hex()) for x, y in
                                 db.execute('SELECT transaction_id,payload FROM effects')])
            except sqlite3.Error as error:
                row['ledger_error'] = str(error)
            (folder / 'trust.json').write_bytes(canonical(trust) + b'\n')
            (folder / 'outcome.json').write_bytes(canonical(row) + b'\n')
            rows.append(row)

        def committed(o, r, e, f, t, s, a, d):
            assert o.reserve(e)['status'] == 'pending'
            response = o.commit(e['transaction'], a)
            assert response['status'] == 'committed'
            receipt = response['receipt']
            (d / 'receipt.json').write_bytes(canonical(receipt) + b'\n')
            assert verify(receipt, t, e, s)['accepted']
            return dict(outcome='committed', response=response)
        case('valid', committed, 'committed')

        def before(o, r, e, f, t, s, a, d):
            o.reserve(e)
            assert cli(r, 'revoke').returncode == 0
            response = o.commit(e['transaction'], a)
            assert o.recover(e['transaction'])['receipt'] is None
            assert o.conn.execute('SELECT count(*) FROM effects').fetchone() == (0,)
            return dict(outcome=response['status'], response=response)
        case('revoke-before-admission', before, 'denied')

        def after(o, r, e, f, t, s, a, d):
            o.reserve(e)
            response = o.commit(e['transaction'], a)
            o.revoke()
            verify(response['receipt'], t, e, s)
            assert o.recover(e['transaction'])['receipt'] == response['receipt']
            return dict(outcome='committed-before-revocation', receipt=response['receipt'])
        case('revoke-after-commit', after, 'committed-before-revocation')

        def duplicate(o, r, e, f, t, s, a, d, restart=False):
            o.reserve(e)
            o.commit(e['transaction'], a)
            if restart:
                first = cli(r, 'reserve', expected=f)
                second = cli(r, 'commit', transaction=e['transaction'], amount=a)
                assert first.returncode == second.returncode == 2
            else:
                for action in (lambda: o.reserve(e), lambda: o.commit(e['transaction'], a)):
                    try:
                        action()
                    except ValueError:
                        pass
                    else:
                        raise AssertionError('duplicate accepted')
            assert o.conn.execute('SELECT count(*) FROM effects').fetchone() == (1,)
            return dict(outcome='duplicate-rejected')
        case('duplicate-in-process', duplicate, 'duplicate-rejected')
        case('duplicate-after-restart', lambda *x: duplicate(*x, restart=True), 'duplicate-rejected')

        def replay(o, r, e, f, t, s, a, d):
            o.reserve(e)
            receipt = o.commit(e['transaction'], a)['receipt']
            (d / 'receipt.json').write_bytes(canonical(receipt) + b'\n')
            verify(receipt, t, e, s)
            try:
                verify(receipt, t, e, s)
            except ValueError as error:
                assert str(error) == 'receipt replay'
            else:
                raise AssertionError('replay accepted')
            command = [sys.executable, str(HERE / 'verify.py'), '--trust', str(r / 'trust.json'),
                       '--state', str(s), '--receipt', str(d / 'receipt.json'), '--expected', str(f)]
            proc = subprocess.run(command, capture_output=True, text=True, timeout=30)
            raw.append(dict(command=command, returncode=proc.returncode, stdout=proc.stdout, stderr=proc.stderr))
            assert proc.returncode == 2 and 'receipt replay' in proc.stdout
            return dict(outcome='replay-rejected-after-restart', receipt=receipt)
        case('receipt-replay', replay, 'replay-rejected-after-restart')

        def rejected(action):
            try:
                action()
            except (ValueError, TypeError, sqlite3.Error) as error:
                return type(error).__name__ + ': ' + str(error)
            except InvalidSignature:
                return 'InvalidSignature'
            raise AssertionError('attack accepted')

        def fabrication(o, r, e, f, t, s, a, d, identity=False):
            # No reserve, producer execution history or effect: invented fully consistent body.
            payload = '8491 25000\n'
            body = dict(protocol='witnessed-po-v1', witness=t['witness'], ledger=t['ledger'],
                        transaction=e['transaction'], request=e['request'], policy_version=1,
                        limit=50000, status='committed', effect=dict(payload=payload,
                        sha256=hashlib.sha256(payload.encode()).hexdigest()))
            attacker = Ed25519PrivateKey.generate()
            if identity:
                body['witness'] = hashlib.sha256(attacker.public_key().public_bytes_raw()).hexdigest()
            receipt = dict(body=body, signature=attacker.sign(DOMAIN + canonical(body)).hex())
            error = rejected(lambda: verify(receipt, t, e, s))
            assert o.conn.execute('SELECT count(*) FROM effects').fetchone() == (0,)
            return dict(outcome='fabrication-rejected', candidate=receipt, error=error,
                        internally_consistent_sha256=hashlib.sha256(canonical(body)).hexdigest())
        case('fabricated-history', fabrication, 'fabrication-rejected')
        case('forged-witness-identity', lambda *x: fabrication(*x, identity=True), 'fabrication-rejected')

        def versions(o, r, e, f, t, s, a, d):
            o.reserve(e)
            receipt = o.commit(e['transaction'], a)['receipt']
            changed = dict(e, policy_version=2)
            error = rejected(lambda: verify(receipt, t, changed, s))
            candidate = copy.deepcopy(receipt)
            candidate['body']['policy_version'] = 2
            error2 = rejected(lambda: verify(candidate, t, changed, s))
            return dict(outcome='version-mismatch-rejected', errors=[error, error2], candidate=candidate)
        case('authority-version', versions, 'version-mismatch-rejected')

        def interrupted(o, r, e, f, t, s, a, d, checkpoint):
            o.reserve(e)
            proc = cli(r, 'commit', transaction=e['transaction'], amount=a, checkpoint=checkpoint)
            assert proc.returncode == 75 and not proc.stdout
            proc = cli(r, 'recover', transaction=e['transaction'])
            assert proc.returncode == 0, proc.stdout
            response = json.loads(proc.stdout)
            if checkpoint == 'after-commit':
                assert response['status'] == 'committed'
                verify(response['receipt'], t, e, s)
                assert o.recover(e['transaction']) == response
            else:
                assert response == dict(status='aborted', receipt=None)
            assert cli(r, 'commit', transaction=e['transaction'], amount=a).returncode == 2
            assert o.conn.execute('SELECT count(*) FROM effects').fetchone() == ((1 if checkpoint == 'after-commit' else 0),)
            return dict(outcome=response['status'], recovered=response, crash_exit=75)
        for checkpoint in ('before-effect', 'after-insert', 'after-commit'):
            case('interrupted-' + checkpoint, lambda *x, cp=checkpoint: interrupted(*x, checkpoint=cp),
                 'committed' if checkpoint == 'after-commit' else 'aborted')

        def pending(o, r, e, f, t, s, a, d):
            o.reserve(e)
            proc = cli(r, 'recover', transaction=e['transaction'])
            assert proc.returncode == 0
            result = json.loads(proc.stdout)
            assert result['receipt'] is None
            return dict(outcome=result['status'])
        case('restart-pending-recovery', pending, 'aborted')

        def concurrent(o, r, e, f, t, s, a, d):
            def race(operation, flags):
                command = [sys.executable, str(HERE / 'witness.py'), str(r), operation, *flags]
                processes = [subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                               text=True) for _ in range(2)]
                results = []
                for proc in processes:
                    stdout, stderr = proc.communicate(timeout=30)
                    raw.append(dict(command=command, returncode=proc.returncode, stdout=stdout, stderr=stderr))
                    results.append(proc.returncode)
                assert sorted(results) == [0, 2]
                return results
            reserve = race('reserve', ['--expected', str(f)])
            commit = race('commit', ['--transaction', e['transaction'], '--amount', str(a)])
            assert o.conn.execute('SELECT count(*) FROM effects').fetchone() == (1,)
            return dict(outcome='one-effect', reserve_exit_codes=reserve, commit_exit_codes=commit)
        case('concurrent-same-transaction', concurrent, 'one-effect')

        def storage(o, r, e, f, t, s, a, d, corrupt):
            o.reserve(e)
            if corrupt:
                o.close()
                (r / 'ledger.db').write_bytes(b'damaged SQLite header' + b'\0' * 4096)
            else:
                # Close before rename for Windows portability; missing store must not recreate.
                o.close()
                (r / 'ledger.db').rename(r / 'unavailable.db')
            result = cli(r, 'commit', transaction=e['transaction'], amount=a)
            assert result.returncode == 2
            if not corrupt:
                assert not (r / 'ledger.db').exists()
                (r / 'unavailable.db').rename(r / 'ledger.db')
            return dict(outcome='storage-fail-closed', detail=json.loads(result.stdout))
        case('corrupt-witness-storage', lambda *x: storage(*x, corrupt=True), 'storage-fail-closed')
        case('unavailable-witness-storage', lambda *x: storage(*x, corrupt=False), 'storage-fail-closed')

        def bypass(o, r, e, f, t, s, a, d):
            error = rejected(lambda: o.commit(e['transaction'], a))
            (d / 'producer-claimed-effect.txt').write_bytes(b'8491 25000\n')
            assert o.conn.execute('SELECT count(*) FROM effects').fetchone() == (0,)
            return dict(outcome='unreserved-effect-rejected', error=error)
        case('direct-api-bypass', bypass, 'unreserved-effect-rejected')

        def malformed(o, r, e, f, t, s, a, d):
            o.reserve(e)
            receipt = o.commit(e['transaction'], a)['receipt']
            errors = []
            for change in ('bool', 'extra', 'signature', 'effect'):
                candidate = copy.deepcopy(receipt)
                if change == 'bool': candidate['body']['policy_version'] = True
                if change == 'extra': candidate['body']['history'] = []
                if change == 'signature': candidate['signature'] = '00' * 64
                if change == 'effect': candidate['body']['effect']['payload'] = '8491 1\n'
                errors.append(rejected(lambda: verify(candidate, t, e, s)))
            malformed_file = d / 'bad.json'
            for data in ('{"body":1,"body":2}', '{"body":NaN}'):
                malformed_file.write_text(data)
                errors.append(rejected(lambda: load(malformed_file)))
            verify(receipt, t, e, s)  # Rejections must not consume legitimate receipt.
            return dict(outcome='malformed-rejected', errors=errors)
        case('malformed-receipts', malformed, 'malformed-rejected')

        def mismatch(o, r, e, f, t, s, a, d):
            o.reserve(e)
            response = o.commit(e['transaction'], 1)
            return dict(outcome=response['status'])
        case('forged-amount', mismatch, 'denied')

        def lost_state(o, r, e, f, t, s, a, d):
            o.reserve(e)
            receipt = o.commit(e['transaction'], a)['receipt']
            s.unlink()
            error = rejected(lambda: verify(receipt, t, e, s))
            assert not s.exists()
            return dict(outcome='missing-verifier-state-rejected', error=error)
        case('missing-verifier-state', lost_state, 'missing-verifier-state-rejected')

        def verifier_storage(o, r, e, f, t, s, a, d):
            o.reserve(e)
            receipt = o.commit(e['transaction'], a)['receipt']
            wrong_pin = dict(t, ledger=str(uuid.uuid4()))
            error = rejected(lambda: verify(receipt, wrong_pin, e, s))
            s.write_bytes(b'broken database')
            corrupt = rejected(lambda: verify(receipt, t, e, s))
            return dict(outcome='verifier-storage-rejected', errors=[error, corrupt])
        case('corrupt-verifier-storage', verifier_storage, 'verifier-storage-rejected')

        def uncertain(o, r, e, f, t, s, a, d):
            unknown = rejected(lambda: o.recover(e['transaction']))
            o.reserve(e)
            response = o.commit(e['transaction'], a)
            o.conn.execute('UPDATE effects SET payload=? WHERE transaction_id=?',
                           (b'corrupt payload', e['transaction']))
            mismatch = rejected(lambda: o.recover(e['transaction']))
            return dict(outcome='inconsistent-recovery-rejected', errors=[unknown, mismatch],
                        receipt=response['receipt'])
        case('inconsistent-recovery', uncertain, 'inconsistent-recovery-rejected')

        def excluded(o, r, e, f, t, s, a, d, rollback=False):
            if rollback:
                o.reserve(e)
                receipt = o.commit(e['transaction'], a)['receipt']
                snapshot = d / 'old-state.db'
                shutil.copyfile(s, snapshot)
                verify(receipt, t, e, s)
                shutil.copyfile(snapshot, s)
                verify(receipt, t, e, s)
                return dict(outcome='rollback-replay-accepted', severity='critical if rollback is in threat model')
            # Same UID can write witness DB and steal key. This demonstrates missing OS isolation.
            o.reserve(e)
            with sqlite3.connect(r / 'ledger.db') as db:
                db.execute('INSERT INTO effects VALUES (?,?)', (e['transaction'], b'8491 25000\n'))
            assert o.conn.execute('SELECT count(*) FROM effects').fetchone() == (1,)
            return dict(outcome='raw-filesystem-bypass-accepted', severity='critical without OS isolation',
                        unreceipted=True, note='No sandbox claim; producer with DB permissions bypasses owner API')
        case('excluded-same-uid-bypass', excluded, 'raw-filesystem-bypass-accepted', 'excluded-negative-control')
        case('excluded-state-rollback', lambda *x: excluded(*x, rollback=True),
             'rollback-replay-accepted', 'excluded-negative-control')

    (output / 'results.json').write_bytes(canonical(dict(
        observations=rows, failures=failures,
        accepted_in_model_attacks=0 if not failures else None,
        accepted_excluded_negative_controls=sum(r.get('category') == 'excluded-negative-control' and r['passed'] for r in rows))) + b'\n')
    (output / 'raw.json').write_bytes(canonical(raw) + b'\n')
    print(json.dumps(dict(cases=len(rows), failures=failures)))
    return not failures


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runner', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    raise SystemExit(0 if exercise(args.runner.resolve(), args.output.resolve()) else 1)
