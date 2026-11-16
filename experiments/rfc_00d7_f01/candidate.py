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
