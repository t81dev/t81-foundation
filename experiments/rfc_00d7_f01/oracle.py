"""Reference equations/decoders. No candidate/domain/accounting imports."""

import hashlib
import json

from jsonschema import Draft202012Validator


def commitment_bound(case, visit=lambda: None):
    # Count unit advances in the relevant prefix, not checkpoint arithmetic.
    advances = 0
    for counter in range(case['halt']):
        visit()
        if counter >= case['commit']:
            advances += 1
    return case['halt'] >= case['commit'] and advances <= case['k']


def serial_publication(case):
    sequence = list(case['order'])
    publish_position = sequence.index('publish')
    revoke_position = sequence.index('commit')
    effective = case['initial_revoked'] or (
        case['applicable'] and revoke_position < publish_position)
    return not effective


def future_permission(history, capability=1, visit=lambda: None):
    denied = set()
    for op, subject in history:
        visit()
        if op == 'revoke':
            denied.add(subject)
    return capability not in denied


def canonical_trace(payload):
    # The bounded ASCII-only domain permits a separate literal projection.
    selected = {}
    for name in sorted(payload):
        if name == 'non_deterministic_host_telemetry':
            continue
        if name == 'integrity_digest_canon_hash81':
            continue
        selected[name] = payload[name]
    return json.dumps(selected, sort_keys=True, separators=(',', ':'),
                      ensure_ascii=True).encode('ascii')


def live_allocation_bound(case, visit=lambda: None):
    # Include emergency bytes explicitly; candidate instead subtracts them.
    ledger = {f'allocation-{i}': size for i, size in enumerate(case['sizes'])}
    ledger['emergency-slots'] = 512 * case['slots']
    total = 0
    for identity in sorted(ledger):
        visit()
        total += ledger[identity]
    return total <= case['capacity'], total


def conserved_steps(case, visit=lambda: None):
    all_charges = []
    for purpose, demand in enumerate(case['charges']):
        for _ in range(demand):
            visit()
            all_charges.append(purpose)
    allowed = case['limit'] - case['reserve']
    return len(all_charges[:allowed])


def capacity_bound(case):
    if case['queued'] >= case['queue_limit']:
        return False
    available_records = case['slots'] - case['active']
    return available_records >= case['queue_limit']


def record_valid(raw):
    if len(raw) != 512 or raw[:8] != b'T81EFv1\x00':
        return False
    if int.from_bytes(raw[8:10], 'little') != 1:
        return False
    if raw[12:16] != bytes(4) or raw[51:56] != bytes(5):
        return False
    covered = b't81:ms:emergency:v1:' + bytes(raw[index] for index in range(480))
    expected = hashlib.sha3_512(covered).digest()[0:32]
    return raw[480:512] == expected


def durable_ack_valid(snapshot):
    stable = bytes.fromhex(snapshot['durable'])
    if not snapshot['ack']:
        return True
    return (record_valid(stable) and bool(stable[10] & 0x01)
            and stable == bytes.fromhex(snapshot['record']))


def immutable_record(case):
    # Literal independent packer for a control, no candidate packing routine.
    parts = [b'T81EFv1\x00', b'\x01\x00', b'\x01\x00', bytes(4)]
    parts.extend(number.to_bytes(8, 'little')
                 for number in (case['request'], 1, 1, case['step']))
    parts.extend((b'\x01\x02\x03', bytes(5),
                  case['reason'].encode('ascii').ljust(424, b'\x00')))
    raw = b''.join(parts)
    return raw + hashlib.sha3_512(b't81:ms:emergency:v1:' + raw).digest()[:32]


def manifest_reference(case, schema):
    errors = sorted(Draft202012Validator(schema).iter_errors(case['manifest']),
                    key=lambda error: (str(list(error.path)), error.message))
    impossible = (schema.get('additionalProperties') is False
                  and 'model_canon_hash81' not in schema['properties'])
    return {'schema': not errors, 'binding_satisfiable': not impossible,
            'errors': [{'instance': list(error.path),
                        'schema': list(error.schema_path)} for error in errors]}
