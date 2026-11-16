"""Reference equations/decoders. No candidate/domain/accounting imports."""

import hashlib
import json
import re

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


def schema_verdict(manifest, schema):
    return Draft202012Validator(schema).is_valid(manifest)


def fixture_reference(manifest, symbolic_digest, schema):
    equal = False
    for name, value in manifest.items():
        if name == 'model_canon_hash81':
            equal = isinstance(value, str) and value == symbolic_digest
    return {'schema_valid': Draft202012Validator(schema).is_valid(manifest),
            'root_model_equality': equal}


def proof_fact(number, context, observed):
    """Independent raw-source projection; no candidate state/constructors."""
    schema = json.loads(context['schema_bytes'])
    required_names = ['schema', 'source_path', 'source_format',
                      'normalized_artifact_type', 'imported_format_summary',
                      'tensor_count', 'parameters', 'trits', 'file_size_bytes',
                      'provenance', 'tensors']
    stage = re.search(r'### 4\.1 [\s\S]*?(?=### 4\.2 )', context['rfc_text'])
    if number in (1, 2):
        source = context['schema_bytes'] if number == 1 else context['rfc_bytes']
        pinned = context['schema_sha256'] if number == 1 else context['rfc_sha256']
        return hashlib.sha256(source).hexdigest() == pinned
    if number == 3:
        return (context['assumptions']['same_root'] == 'accepted'
                and context['assumptions']['ordinary_immutable_json'] == 'accepted')
    if number == 4:
        return (schema.get('type') == 'object' and schema.get('$schema')
                == 'https://json-schema.org/draft/2020-12/schema')
    if number == 5:
        return schema.get('properties', {}).get('schema', {}).get('const') == 't81.model-manifest.v1'
    if number == 6:
        return 'additionalProperties' in schema and schema['additionalProperties'] is False
    if number == 7:
        return not any(name == 'patternProperties' for name in schema)
    if number == 8:
        return sorted(schema.get('properties', {})) == sorted(required_names)
    if number >= 9 and number <= 19:
        name = required_names[number - 9]
        return name != 'model_canon_hash81' and name in schema.get('properties', {})
    if number == 20:
        return (stage is not None and re.search(
            r'manifest\.model_canon_hash81\s*==\s*model_ref\.model_canon_hash81',
            stage.group()) is not None
            and context['assumptions']['root_member_reading'] == 'accepted')
    if number == 21:
        return context['assumptions']['nonnull_symbolic_reference'] == 'accepted'
    if number == 22:
        return not any(observed[n] is not True for n in (6, 7, 8, 9, 10, 11, 12,
                                                        13, 14, 15, 16, 17, 18, 19))
    if number == 23:
        return (all(observed[n] is True for n in (3, 20, 21)) and stage is not None
                and 'RFC-00D3-model-manifest-schema.json' in stage.group()
                and context['assumptions']['pinned_schema_version'] == 'accepted')
    if number == 24:
        return observed[22] is True and observed[23] is True
    raise ValueError('unknown reference fact')
