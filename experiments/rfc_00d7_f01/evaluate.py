"""Comparisons and explicit ambiguity labels; no automatic experiment entry."""

import json

import candidate
import oracle
from interpretation_policy import outside_arena, validate


def check_control(area, polarity, case, schema, visit):
    visit()
    if area == 'A':
        actual = oracle.commitment_bound(case, visit)
        return actual if polarity == 'positive' else not actual
    if area == 'B':
        visit(2)
        actual = candidate.publication(case, broken=polarity == 'negative', visit=visit)
        expected = oracle.serial_publication(case)
        return actual == expected if polarity == 'positive' else actual != expected
    if area == 'C':
        visit(4)
        same = (candidate.session_projection(case['left'], complete=polarity == 'positive', visit=visit)
                == candidate.session_projection(case['right'], complete=polarity == 'positive', visit=visit))
        different = oracle.future_permission(case['left'], visit=visit) != oracle.future_permission(case['right'], visit=visit)
        return (not same and different) if polarity == 'positive' else same and different
    if area == 'D' and case['kind'] == 'steps':
        visit(2)
        actual = candidate.step_gate(case, reset=polarity == 'negative', visit=visit)
        expected = oracle.conserved_steps(case, visit=visit)
        return actual == expected if polarity == 'positive' else actual != expected
    if area == 'D':
        visit(2)
        allowed = candidate.memory_gate(case, complete=polarity == 'positive')
        expected, _ = oracle.live_allocation_bound(case, visit=visit)
        return allowed == expected if polarity == 'positive' else allowed != expected
    if area == 'E':
        visit()
        if polarity == 'positive':
            return oracle.record_valid(oracle.immutable_record(case))
        result = candidate.emergency(case, visit=visit)[-1]
        return not oracle.durable_ack_valid(result)
    if area == 'F':
        visit(2)
        expected = oracle.manifest_reference(case, schema)['schema']
        actual = candidate.admission(case, schema)['schema']
        # Mutant is the explicitly defective accept-all validator.
        return actual == expected and actual if polarity == 'positive' else not expected
    raise ValueError(area)


def result(subclaim, admissible, violates, detail, unresolved=None):
    return {'subclaim': subclaim, 'admissible': admissible,
            'violation': violates, 'detail': detail, 'unresolved': unresolved}


def compare(area, case, schema, interpretations, cache, visit, declarations):
    validate(interpretations, declarations)

    def emit(*args, **kwargs):
        visit()  # Every result state, including a subsequently suppressed row.
        return result(*args, **kwargs)

    visit(2)  # Candidate and reference comparison states, before evaluation.
    if area == 'A':
        return [emit('A', candidate.revocation(case), not oracle.commitment_bound(case, visit),
                       {'commit_distance': case['halt'] - case['commit'],
                        'latch_distance': case['halt'] - case['latch']})]
    if area == 'B':
        # Memoize only candidate transitions, never oracle verdicts or case visits.
        key = (case['order'], case['initial_revoked'], case['applicable'])
        if key not in cache:
            visit()  # Materialized memo entry is charged in addition to outputs.
            cache[key] = candidate.publication(case, visit=visit)
        visible = cache[key]
        safe = oracle.serial_publication(case)
        rows = [emit('B1', True, visible != safe, {'visible': visible, 'oracle': safe})]
        if visible and case['applicable'] and not case['initial_revoked']:
            rows.append(emit('B2', None, True, {'order': case['order']}, 'O1-race'))
        return rows
    if area == 'C' and case['kind'] == 'session':
        projected = candidate.session_projection(case['history'], visit=visit)
        permission = oracle.future_permission(case['history'], visit=visit)
        prior = cache.get(projected)
        if prior is None:
            # Finite cache is bounded independently of process-memory ceiling.
            if len(cache) >= 2048:
                from accounting import Limit
                raise Limit('collision bucket cap')
            cache[projected] = (permission, case['history'])
            return [emit('C1', True, False, {'projection': projected})]
        return [emit('C1', None if prior[0] != permission else True, prior[0] != permission,
                       {'projection': projected, 'prior': prior[1],
                        'future_permissions': [prior[0], permission]}, 'event-kind-payload')]
    if area == 'C':
        base = {'seed': case['seed'], 'events': [{'lip_kind': 'LIP-Queue',
                'detail': 'dispatch_seq=1'}], 'extensions': 'a',
                'non_deterministic_host_telemetry': {'completion_seq': 1}}
        other = dict(base, non_deterministic_host_telemetry={'completion_seq': case['coordinate']})
        changed = dict(base, extensions='b')
        nested = dict(base, events=[{'lip_kind': 'LIP-Queue',
                                    'detail': f'dispatch_seq={case["coordinate"]}'}])
        visit(8)
        candidate_equal = candidate.trace_projection(base) == candidate.trace_projection(other)
        reference_equal = oracle.canonical_trace(base) == oracle.canonical_trace(other)
        sensitivity = oracle.canonical_trace(base) != oracle.canonical_trace(changed)
        return [emit('C2', True, not (candidate_equal and reference_equal and sensitivity),
                       {'telemetry_equal': reference_equal, 'extension_sensitive': sensitivity}),
                emit('C2-nested', None if case['coordinate'] != 1 else True, candidate.trace_projection(base) != candidate.trace_projection(nested),
                       {'coordinate': case['coordinate']}, 'nested-LIP-coordinate')]
    if area == 'D' and case['kind'] == 'memory':
        visit(2)
        admitted = candidate.memory_gate(case)
        fits, total = oracle.live_allocation_bound(case, visit=visit)
        policy = outside_arena(interpretations)
        return [emit('D1', admitted if policy == 'permitted' else None,
                       admitted and not fits, {'gate': admitted, 'live_bytes': total},
                       None if policy == 'permitted' else 'outside-arena')]
    if area == 'D' and case['kind'] == 'steps':
        visit(2)
        actual = candidate.step_gate(case, visit=visit)
        expected = oracle.conserved_steps(case, visit=visit)
        return [emit('D2', True, actual != expected, {'consumed': actual, 'oracle': expected})]
    if area == 'D':
        visit(2)
        return [emit('D3', None, candidate.queue_gate(case) != oracle.capacity_bound(case),
                       {'retention': 'undefined'}, 'slot-lifecycle')]
    if area == 'E' and case['kind'] == 'bitflip':
        visit()
        raw = bytearray(oracle.immutable_record(case))
        raw[case['offset']] ^= 1
        return [emit('E-checksum-control', True, oracle.record_valid(raw),
                       {'offset': case['offset']})]
    if area == 'E':
        visit()
        history = candidate.emergency(case, repaired=case['kind'] == 'two_phase_control', visit=visit)
        final = history[-1]
        in_memory_valid = oracle.record_valid(bytes.fromhex(final['record']))
        ack_valid = oracle.durable_ack_valid(final)
        return [emit('E1', True if case['prefix'] == 512 or in_memory_valid else None, not in_memory_valid,
                       {'snapshots': history}, None if case['prefix'] == 512 else 'partial-write-continuation'),
                emit('E2', True if ack_valid else None, not ack_valid, {'snapshots': history}, 'acknowledgement')]
    if area == 'F':
        visit()
        actual = candidate.admission(case, schema)
        reference = oracle.manifest_reference(case, schema)
        return [emit('F-validator', True, actual['schema'] != reference['schema'],
                       {'candidate': actual, 'oracle': reference}),
                emit('F-satisfiability', True, not reference['binding_satisfiable'],
                       {'schema': actual['schema'], 'oracle': reference})]
    raise ValueError(area)
