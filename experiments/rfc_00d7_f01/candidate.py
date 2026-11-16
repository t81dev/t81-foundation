"""Synthetic draft interpretations. Import is inert; never imports the oracle."""

import hashlib


def revocation(case):
    return 0 <= case['halt'] - case['latch'] <= case['k']


def publication(case, broken=False, visit=lambda: None):
    revoked = case['initial_revoked']
    checked = False
    visible = False
    for event in case['order']:
        visit()
        if event == 'commit' and case['applicable']:
            revoked = True
        elif event == 'check':
            checked = not revoked
        elif event == 'publish':
            visible = checked if broken else not revoked
    return visible


def session_projection(history, complete=False, visit=lambda: None):
    result = []
    for seq, (operation, subject) in enumerate(history, 1):
        visit()
        request = subject if operation in ('dispatch', 'cancel') else 99
        row = (seq, operation, request, 5)
        result.append(row + (subject,) if complete else row)
    return tuple(result)


def trace_projection(payload):
    # Literal draft exclusion. Nested LIP-Queue normalization is unspecified.
    return {key: value for key, value in payload.items()
            if key not in ('integrity_digest_canon_hash81',
                           'non_deterministic_host_telemetry')}


def memory_gate(case, complete=False):
    sizes = case['sizes']
    charged = sum(sizes) if complete else sum(sizes[:4])
    return charged <= case['capacity'] - case['slots'] * 512


def step_gate(case, reset=False, visit=lambda: None):
    remaining = case['limit'] - case['reserve']
    consumed = 0
    for attempt, demand in enumerate(case['charges']):
        if reset and attempt == 2:
            remaining = case['limit'] - case['reserve']
        for _ in range(demand):
            if remaining == 0:
                return consumed
            visit()
            consumed += 1
            remaining -= 1
    return consumed


def queue_gate(case):
    return (case['queued'] < case['queue_limit']
            and case['slots'] >= case['active'] + case['queue_limit'])


def emergency(case, repaired=False, visit=lambda: None):
    payload = bytearray(512)
    payload[:8] = b'T81EFv1\x00'
    payload[8:10] = (1).to_bytes(2, 'little')
    for offset, value in ((16, case['request']), (24, 1), (32, 1),
                          (40, case['step'])):
        payload[offset:offset + 8] = value.to_bytes(8, 'little')
    payload[48:51] = b'\x01\x02\x03'  # test-only tags
    reason = case['reason'].encode('ascii')
    payload[56:56 + len(reason)] = reason
    payload[480:] = hashlib.sha3_512(
        b't81:ms:emergency:v1:' + payload[:480]).digest()[:32]
    durable = bytearray(512)
    volatile = bytearray(512)
    acknowledged = False
    snapshots = []
    for action in ('construct', 'hash', 'write', 'sync', 'flag', 'crash'):
        visit()
        if action == 'write':
            volatile[:case['prefix']] = payload[:case['prefix']]
        elif action == 'sync' and case['sync_ok']:
            durable[:] = volatile
        elif action == 'flag' and case['sync_ok']:
            payload[10] |= 1
            acknowledged = True
            if repaired:
                # Explicit counterfactual second write+sync, not a draft repair.
                payload[480:] = hashlib.sha3_512(
                    b't81:ms:emergency:v1:' + payload[:480]).digest()[:32]
                prefix = case.get('second_prefix', 512)
                volatile[:prefix] = payload[:prefix]
                durable[:] = volatile
        snapshots.append({'action': action, 'volatile': bytes(volatile).hex(),
                          'durable': bytes(durable).hex(),
                          'record': bytes(payload).hex(), 'ack': acknowledged})
        if action == case['crash_after']:
            break
    return snapshots


def schema_subset(manifest, schema):
    """Independent small evaluator for only keywords present in pinned D3."""
    def visit(value, rule):
        if '$ref' in rule:
            return visit(value, schema['$defs'][rule['$ref'].split('/')[-1]])
        kind = rule.get('type')
        valid_type = {'object': type(value) is dict,
                      'array': type(value) is list,
                      'string': type(value) is str,
                      'integer': type(value) is int}
        if kind and not valid_type[kind]:
            return False
        if 'const' in rule and value != rule['const']:
            return False
        if 'enum' in rule and value not in rule['enum']:
            return False
        if kind == 'object':
            props = rule.get('properties', {})
            if not set(rule.get('required', ())) <= value.keys():
                return False
            if rule.get('additionalProperties') is False and value.keys() - props.keys():
                return False
            return all(visit(v, props[k]) for k, v in value.items() if k in props)
        if kind == 'array':
            return (len(value) >= rule.get('minItems', 0)
                    and all(visit(v, rule['items']) for v in value))
        if kind == 'string':
            return len(value) >= rule.get('minLength', 0)
        if kind == 'integer':
            return value >= rule.get('minimum', value)
        return True
    return visit(manifest, schema)


def admission(case, schema):
    metadata_ok = schema_subset(case['manifest'], schema)
    binding_ok = (case['manifest'].get('model_canon_hash81') == case['weight_hash']
                  and isinstance(case['manifest'].get('state_schema_hash81'), str)
                  and bool(case['manifest']['state_schema_hash81']))
    return {'schema': metadata_ok, 'binding': binding_ok,
            'admitted': metadata_ok and binding_ok}


def fixture_predicates(manifest, symbolic_digest, schema):
    return {'schema_valid': schema_subset(manifest, schema),
            'root_model_equality': ('model_canon_hash81' in manifest
                                    and type(manifest['model_canon_hash81']) is str
                                    and manifest['model_canon_hash81'] == symbolic_digest)}


def proof_fact(index, context, prior):
    """One observed symbolic fact, no reference helpers or fixture expectations."""
    s = context['schema']
    p = s['properties']
    k = 'model_canon_hash81'
    clause = context['rfc_text'].split('### 4.1 ', 1)[1].split('### 4.2 ', 1)[0]
    names = ('schema', 'source_path', 'source_format', 'normalized_artifact_type',
             'imported_format_summary', 'tensor_count', 'parameters', 'trits',
             'file_size_bytes', 'provenance', 'tensors')
    if index == 1:
        return hashlib.sha256(context['schema_bytes']).hexdigest() == context['schema_sha256']
    if index == 2:
        return hashlib.sha256(context['rfc_bytes']).hexdigest() == context['rfc_sha256']
    if index == 3:
        return all(context['assumptions'][key] == 'accepted'
                   for key in ('same_root', 'ordinary_immutable_json'))
    if index == 4:
        return (s['$schema'] == 'https://json-schema.org/draft/2020-12/schema'
                and s['type'] == 'object')
    if index == 5:
        return s['properties']['schema']['const'] == 't81.model-manifest.v1'
    if index == 6:
        return s['additionalProperties'] is False
    if index == 7:
        return 'patternProperties' not in s
    if index == 8:
        return set(p) == set(names) and len(p) == 11
    if 9 <= index <= 19:
        return names[index - 9] in p and names[index - 9] != k
    if index == 20:
        return ('manifest.model_canon_hash81 == model_ref.model_canon_hash81' in clause
                and context['assumptions']['root_member_reading'] == 'accepted')
    if index == 21:
        return context['assumptions']['nonnull_symbolic_reference'] == 'accepted'
    if index == 22:
        return prior[6] and prior[7] and prior[8] and all(prior[n] for n in range(9, 20))
    if index == 23:
        return (prior[3] and prior[20] and prior[21]
                and context['assumptions']['pinned_schema_version'] == 'accepted'
                and 'RFC-00D3-model-manifest-schema.json' in clause)
    if index == 24:
        return prior[22] and prior[23]
    raise ValueError('unknown proof fact')
