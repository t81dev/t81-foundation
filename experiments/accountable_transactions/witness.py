"""Trusted ledger owner. CLI/admin paths are test orchestration, not producer capabilities."""
import argparse
import hashlib
import json
import os
import sqlite3
from pathlib import Path
import uuid
from time import perf_counter_ns as now
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from verify import (DOMAIN, canonical, database, fields, integer, load, request,
                    transaction_id, checked)


def provision(root):
    root = Path(root)
    root.mkdir(mode=0o700, parents=True, exist_ok=False)
    key = Ed25519PrivateKey.generate()
    private = root / 'key'
    with private.open('xb') as stream:
        os.chmod(private, 0o600)
        stream.write(key.private_bytes_raw())
    public = key.public_key().public_bytes_raw().hex()
    trust = dict(protocol='witnessed-po-v1', public_key=public,
                 witness=hashlib.sha256(bytes.fromhex(public)).hexdigest(),
                 ledger=str(uuid.uuid4()), policies={'1': 50000, '2': 10000})
    conn = database(root / 'ledger.db', create=True)
    try:
        conn.execute('BEGIN IMMEDIATE')
        conn.execute('CREATE TABLE config (pin BLOB NOT NULL)')
        conn.execute('INSERT INTO config VALUES (?)', (canonical(trust),))
        conn.execute('CREATE TABLE policy (singleton INTEGER PRIMARY KEY CHECK(singleton=1), '
                     'version INTEGER NOT NULL, amount_limit INTEGER NOT NULL)')
        conn.execute('INSERT INTO policy VALUES (1,1,50000)')
        conn.execute('CREATE TABLE transactions (id TEXT PRIMARY KEY, request BLOB NOT NULL, '
                     'version INTEGER NOT NULL, state TEXT NOT NULL '
                     "CHECK(state IN ('pending','denied','aborted','committed')), receipt BLOB)")
        conn.execute('CREATE TABLE effects (transaction_id TEXT PRIMARY KEY '
                     'REFERENCES transactions(id), payload BLOB NOT NULL)')
        conn.commit()
    finally:
        conn.close()
    (root / 'trust.json').write_bytes(canonical(trust) + b'\n')
    return trust


class Witness:
    def __init__(self, root):
        root = Path(root)
        self.conn = database(root / 'ledger.db')
        try:
            self.conn.execute('PRAGMA foreign_keys=ON')
            self.trust = load(root / 'trust.json')
            self.key = Ed25519PrivateKey.from_private_bytes((root / 'key').read_bytes())
            if self.key.public_key().public_bytes_raw().hex() != self.trust['public_key']:
                raise ValueError('witness key mismatch')
            if self.conn.execute('SELECT pin FROM config').fetchall() != [(canonical(self.trust),)]:
                raise ValueError('ledger pin mismatch')
        except BaseException:
            self.conn.close()
            raise

    def close(self):
        self.conn.close()

    def reserve(self, expected):
        start = now()
        fields(expected, 'transaction request policy_version')
        transaction_id(expected['transaction'])
        request(expected['request'])
        integer(expected['policy_version'], 1, 2**31 - 1)
        self.conn.execute('BEGIN IMMEDIATE')
        try:
            version, limit = self.conn.execute('SELECT version,amount_limit FROM policy').fetchone()
            allow = version == expected['policy_version'] and (
                expected['request']['units'] * expected['request']['price'] <= limit)
            try:
                self.conn.execute('INSERT INTO transactions VALUES (?,?,?,?,NULL)',
                                  (expected['transaction'], canonical(expected['request']),
                                   expected['policy_version'], 'pending' if allow else 'denied'))
            except sqlite3.IntegrityError as error:
                raise ValueError('duplicate transaction') from error
            self.conn.commit()
        finally:
            if self.conn.in_transaction:
                self.conn.rollback()
        return dict(status='pending' if allow else 'denied', authorization_ns=now() - start)

    def revoke(self):
        # Admin-only fixed experiment policy transition; shares the commit lock.
        self.conn.execute('BEGIN IMMEDIATE')
        try:
            if self.conn.execute('SELECT version FROM policy').fetchone() != (1,):
                raise ValueError('policy already advanced')
            self.conn.execute('UPDATE policy SET version=2,amount_limit=10000 WHERE singleton=1')
            self.conn.commit()
        finally:
            if self.conn.in_transaction:
                self.conn.rollback()
        return dict(status='revoked', policy_version=2)

    def commit(self, txid, proposed, checkpoint=None):
        transaction_id(txid)
        integer(proposed, 1, 1000000)
        start = now()
        self.conn.execute('BEGIN IMMEDIATE')
        try:
            row = self.conn.execute('SELECT request,version,state FROM transactions WHERE id=?',
                                    (txid,)).fetchone()
            if row is None or row[2] != 'pending':
                raise ValueError('transaction not pending')
            raw, version, _ = row
            req = json.loads(raw)
            current, limit = self.conn.execute('SELECT version,amount_limit FROM policy').fetchone()
            auth_start = now()
            allow = version == current and proposed == req['units'] * req['price'] and proposed <= limit
            authorization_ns = now() - auth_start
            if not allow:
                self.conn.execute("UPDATE transactions SET state='denied' WHERE id=?", (txid,))
                self.conn.commit()
                return dict(status='denied', authorization_ns=authorization_ns,
                            commit_ns=now() - start)
            if checkpoint == 'before-effect':
                os._exit(75)
            payload = f'8491 {proposed}\n'.encode('ascii')
            self.conn.execute('INSERT INTO effects VALUES (?,?)', (txid, payload))
            if checkpoint == 'after-insert':
                os._exit(75)  # Uncommitted SQL effect must roll back; no published receipt.
            body = dict(protocol='witnessed-po-v1', witness=self.trust['witness'],
                        ledger=self.trust['ledger'], transaction=txid, request=req,
                        policy_version=version, limit=limit, status='committed',
                        effect=dict(payload=payload.decode('ascii'),
                                    sha256=hashlib.sha256(payload).hexdigest()))
            signing_start = now()
            receipt = dict(body=body, signature=self.key.sign(DOMAIN + canonical(body)).hex())
            signing_ns = now() - signing_start
            self.conn.execute("UPDATE transactions SET state='committed',receipt=? WHERE id=?",
                              (canonical(receipt), txid))
            self.conn.commit()
            if checkpoint == 'after-commit':
                os._exit(75)  # Durable outbox exists; publication was lost.
        finally:
            if self.conn.in_transaction:
                self.conn.rollback()
        return dict(status='committed', receipt=receipt, authorization_ns=authorization_ns,
                    signing_ns=signing_ns, commit_ns=now() - start)

    def recover(self, txid):
        transaction_id(txid)
        self.conn.execute('BEGIN IMMEDIATE')
        try:
            self.conn.execute("UPDATE transactions SET state='aborted' WHERE id=? AND state='pending'",
                              (txid,))
            row = self.conn.execute('SELECT state,receipt,request,version FROM transactions WHERE id=?', (txid,)).fetchone()
            effect = self.conn.execute('SELECT payload FROM effects WHERE transaction_id=?', (txid,)).fetchone()
            if row is None:
                raise ValueError('unknown transaction; no success inferred')
            state, raw, req, version = row
            if (state == 'committed') != (raw is not None and effect is not None):
                raise ValueError('inconsistent ledger; operator investigation required')
            if state != 'committed' and effect is not None:
                raise ValueError('unexpected effect; operator investigation required')
            if state == 'committed':
                receipt = json.loads(raw)
                checked(receipt, self.trust, dict(transaction=txid, request=json.loads(req),
                                                  policy_version=version))
                if effect[0] != receipt['body']['effect']['payload'].encode('ascii'):
                    raise ValueError('effect/outbox mismatch; operator investigation required')
            self.conn.commit()
        finally:
            if self.conn.in_transaction:
                self.conn.rollback()
        return dict(status=state, receipt=json.loads(raw) if raw else None)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('operation', choices=['init', 'reserve', 'commit', 'recover', 'revoke'])
    parser.add_argument('--expected', type=Path)
    parser.add_argument('--transaction')
    parser.add_argument('--amount', type=int)
    parser.add_argument('--checkpoint', choices=['before-effect', 'after-insert', 'after-commit'])
    args = parser.parse_args()
    owner = None
    try:
        if args.operation == 'init':
            result = provision(args.root)
        else:
            owner = Witness(args.root)
            if args.operation == 'reserve':
                result = owner.reserve(load(args.expected))
            elif args.operation == 'commit':
                result = owner.commit(args.transaction, args.amount, args.checkpoint)
            elif args.operation == 'recover':
                result = owner.recover(args.transaction)
            else:
                result = owner.revoke()
        print(json.dumps(result))
    except Exception as error:
        print(json.dumps(dict(status='error', error=type(error).__name__, detail=str(error))))
        raise SystemExit(2)
    finally:
        if owner:
            owner.close()
