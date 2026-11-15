"""Standalone consumptive verifier: stdlib + cryptography, no producer/runtime imports."""
import argparse
import hashlib
import json
from pathlib import Path
import sqlite3
import uuid
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PublicKey

DOMAIN = b'T81-WITNESSED-PO-v1\0'


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'),
                      ensure_ascii=True, allow_nan=False).encode('ascii')


def unique(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('duplicate JSON key')
        result[key] = value
    return result


def load(path):
    def invalid(value):
        raise ValueError('non-finite number: ' + value)
    return json.loads(Path(path).read_bytes(), object_pairs_hook=unique,
                      parse_constant=invalid)


def fields(value, names):
    if type(value) is not dict or set(value) != set(names.split()):
        raise ValueError('unexpected fields')


def integer(value, low, high):
    if type(value) is not int or not low <= value <= high:
        raise ValueError('integer outside contract')


def hexbytes(value, length):
    if type(value) is not str or len(value) != length * 2:
        raise ValueError('invalid hex length')
    raw = bytes.fromhex(value)
    if raw.hex() != value:
        raise ValueError('noncanonical hex')
    return raw


def transaction_id(value):
    if type(value) is not str or str(uuid.UUID(value)) != value:
        raise ValueError('canonical UUID required')


def request(value):
    fields(value, 'order units price')
    if value['order'] != '8491':
        raise ValueError('unsupported order')
    integer(value['units'], 1, 1000)
    integer(value['price'], 1, 1000)


def checked(receipt, trust, expected):
    fields(trust, 'protocol public_key witness ledger policies')
    fields(expected, 'transaction request policy_version')
    fields(receipt, 'body signature')
    body = receipt['body']
    fields(body, 'protocol witness ledger transaction request policy_version limit effect status')
    if trust['protocol'] != 'witnessed-po-v1' or body['protocol'] != trust['protocol']:
        raise ValueError('protocol mismatch')
    key = hexbytes(trust['public_key'], 32)
    if trust['witness'] != hashlib.sha256(key).hexdigest():
        raise ValueError('invalid witness pin')
    transaction_id(trust['ledger'])
    if body['witness'] != trust['witness'] or body['ledger'] != trust['ledger']:
        raise ValueError('untrusted witness or ledger')
    transaction_id(body['transaction'])
    transaction_id(expected['transaction'])
    request(body['request'])
    request(expected['request'])
    integer(body['policy_version'], 1, 2**31 - 1)
    integer(expected['policy_version'], 1, 2**31 - 1)
    integer(body['limit'], 0, 1000000)
    if (body['transaction'] != expected['transaction'] or
            body['request'] != expected['request'] or
            body['policy_version'] != expected['policy_version']):
        raise ValueError('request/authority binding mismatch')
    if type(trust['policies']) is not dict or not trust['policies']:
        raise ValueError('policies required')
    for version, limit in trust['policies'].items():
        if not version.isascii() or not version.isdecimal() or str(int(version)) != version:
            raise ValueError('invalid policy version pin')
        integer(int(version), 1, 2**31 - 1)
        integer(limit, 0, 1000000)
    if trust['policies'].get(str(body['policy_version'])) != body['limit']:
        raise ValueError('authority not pinned')
    amount = body['request']['units'] * body['request']['price']
    payload = f'8491 {amount}\n'
    fields(body['effect'], 'payload sha256')
    if (body['status'] != 'committed' or amount > body['limit'] or
            body['effect']['payload'] != payload or
            body['effect']['sha256'] != hashlib.sha256(payload.encode('ascii')).hexdigest()):
        raise ValueError('invalid committed effect')
    Ed25519PublicKey.from_public_bytes(key).verify(
        hexbytes(receipt['signature'], 64), DOMAIN + canonical(body))
    return body


def database(path, create=False):
    path = Path(path).resolve()
    if create:
        # Explicit provisioning only. Never silently recreate lost replay state.
        with path.open('xb'):
            pass
    conn = sqlite3.connect(path.as_uri() + '?mode=rw', uri=True, timeout=10,
                           isolation_level=None)
    try:
        conn.execute('PRAGMA synchronous=EXTRA')
        conn.execute('PRAGMA fullfsync=ON')
        if conn.execute('PRAGMA journal_mode').fetchone()[0] != 'delete':
            raise ValueError('rollback journal required')
        if conn.execute('PRAGMA synchronous').fetchone()[0] != 3:
            raise ValueError('synchronous EXTRA required')
        if conn.execute('PRAGMA integrity_check').fetchall() != [('ok',)]:
            raise ValueError('corrupt durable state')
        return conn
    except BaseException:
        conn.close()
        raise


def init_state(path, trust):
    conn = database(path, create=True)
    try:
        conn.execute('BEGIN IMMEDIATE')
        conn.execute('CREATE TABLE config (pin BLOB NOT NULL)')
        conn.execute('INSERT INTO config VALUES (?)', (canonical(trust),))
        conn.execute('CREATE TABLE consumed (witness TEXT, ledger TEXT, transaction_id TEXT, '
                     'receipt_hash TEXT, PRIMARY KEY(witness, ledger, transaction_id))')
        conn.commit()
    finally:
        conn.close()


def verify(receipt, trust, expected, state):
    body = checked(receipt, trust, expected)
    conn = database(state)
    try:
        conn.execute('BEGIN IMMEDIATE')
        if conn.execute('SELECT pin FROM config').fetchall() != [(canonical(trust),)]:
            raise ValueError('verifier state trust pin mismatch')
        try:
            conn.execute('INSERT INTO consumed VALUES (?,?,?,?)',
                         (body['witness'], body['ledger'], body['transaction'],
                          hashlib.sha256(canonical(receipt)).hexdigest()))
        except sqlite3.IntegrityError as error:
            raise ValueError('receipt replay') from error
        conn.commit()
    finally:
        conn.close()
    return dict(accepted=True, transaction=body['transaction'],
                claim='trusted witness committed local ledger effect; consumed once here')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--trust', required=True, type=Path)
    parser.add_argument('--state', required=True, type=Path)
    parser.add_argument('--init-state', action='store_true')
    parser.add_argument('--receipt', type=Path)
    parser.add_argument('--expected', type=Path)
    args = parser.parse_args()
    try:
        trust = load(args.trust)
        if args.init_state:
            init_state(args.state, trust)
            print('{"initialized":true}')
        else:
            if not args.receipt or not args.expected:
                raise ValueError('receipt and expected request required')
            print(json.dumps(verify(load(args.receipt), trust, load(args.expected), args.state)))
    except Exception as error:
        print(json.dumps(dict(accepted=False, error=type(error).__name__, detail=str(error))))
        raise SystemExit(2)
