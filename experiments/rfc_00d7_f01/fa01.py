"""Prepared FA01 phase driver. No entry point or import-time experiments."""

import hashlib
import json
import signal
from pathlib import Path

import candidate
import oracle
from accounting import Limit


def unique_object(pairs):
    value = {}
    for key, item in pairs:
        if key in value:
            raise ValueError('duplicate JSON name')
        value[key] = item
    return value


def load_plan(root):
    plan = json.loads((root / 'fa01-plan.json').read_text())
    raw = (root / 'fa01-corpus.json').read_bytes()
    if hashlib.sha256(raw).hexdigest() != plan['corpus_sha256']:
        raise ValueError('BLOCKED: corpus identity')
    packet = json.loads(raw, object_pairs_hook=unique_object)
    fixtures = packet['fixtures']
    if [item['id'] for item in fixtures] != plan['fixture_ids']:
        raise ValueError('BLOCKED: corpus membership/order')
    # Expectations and purpose strings are deliberately never model inputs.
    inputs = [{'id': item['id'], 'manifest': item['manifest'],
               'weight_hash': item['weight_hash']} for item in fixtures]
    return plan, inputs


def run(schema, authorization, budget, evidence):
    root = Path(__file__).resolve().parent
    plan, fixtures = load_plan(root)
    budget.enter('F')
    initial_states = budget.states['F']
    signal.setitimer(signal.ITIMER_REAL, 10)
    counts = {'admissible_falsifiers': 0, 'negative_controls': 0,
              'underdetermined_cases': 0, 'positive_controls': 0,
              'comparisons': 0, 'generated_cases': 0}
    coverage = {'mandatory_expected': 8, 'mandatory_observed': 0,
                'boundary_expected': 24, 'boundary_observed': 0,
                'proof_expected': 1, 'proof_observed': 0}
    charges = {'controls': 0, 'proof': 0, 'boundary': 0}
    trace = []
    occurrences = []
    verdict, stop, proof_status = 'BLOCKED', 'not_complete', 'incomplete'
    node_candidate, node_reference = {}, {}
    proof_agreed = False
    ledger_number = 0
    phase = 'controls'
    phase_start = budget.states['F']
    assumptions = authorization['f_assumptions']
    accepted = all(value == 'accepted' for value in assumptions.values())
    try:
        # Exactly four charged states per mandatory visit. The mutant replaces
        # the ordinary candidate output; there is no third model invocation.
        for seed in (0, 1, 42, 804):
            for index in range(2):
                budget.visit()
                fixture = fixtures[(index + seed % 2) % 2]
                budget.visit()
                budget.visit(2)
                positive = fixture['id'] == 'F00'
                actual = (candidate.schema_subset(fixture['manifest'], schema)
                          if positive else True)
                reference = oracle.schema_verdict(fixture['manifest'], schema)
                valid = actual == reference if positive else actual != reference
                counts['positive_controls' if positive else 'negative_controls'] += 1
                coverage['mandatory_observed'] += 1
                occurrences.append({'phase': 'mandatory', 'fixture_id': fixture['id'],
                                    'seed': seed, 'occurrence': coverage['mandatory_observed']})
                evidence.write(f'F-control-{coverage["mandatory_observed"]}.json',
                               {'input': fixture, 'seed': seed, 'candidate_or_mutant': actual,
                                'reference': reference, 'valid': valid}, 'F')
                if not valid or (positive and not reference) or (not positive and reference):
                    raise ValueError('mandatory control failure')
        charges['controls'] = budget.states['F'] - phase_start
        if charges['controls'] != 32:
            raise ValueError('control accounting mismatch')

        phase, phase_start = 'proof', budget.states['F']
        budget.visit()  # One proof input, separate from symbolic fixture strings.
        coverage['proof_observed'] = 1
        counts['generated_cases'] += 1
        budget.visit(2)  # Candidate/reference aggregate output slots.
        budget.visit()  # F working slot.
        rfc = (root / 'pinned-d7.rfc.md').read_bytes()
        schema_bytes = (root / 'pinned-d3.schema.json').read_bytes()
        context = {'schema': schema, 'schema_bytes': schema_bytes, 'rfc_bytes': rfc,
                   'rfc_text': rfc.decode('utf-8'), 'schema_sha256': plan['schema_sha256'],
                   'rfc_sha256': plan['rfc_sha256'], 'assumptions': assumptions}
        for index in range(1, 25):
            budget.visit()  # Descriptor state.
            descriptor = {'id': f'P{index:02d}', 'fact': plan['proof_facts'][index - 1]}
            budget.visit()
            node_candidate[index] = candidate.proof_fact(index, context, node_candidate)
            budget.visit()
            node_reference[index] = oracle.proof_fact(index, context, node_reference)
            descriptor.update(candidate=node_candidate[index], reference=node_reference[index])
            trace.append(descriptor)
        proof_agreed = node_candidate == node_reference
        if not proof_agreed:
            proof_status = 'invalid'
        elif not accepted:
            proof_status = 'underdetermined'
        elif not all(node_reference.values()):
            proof_status = 'invalid'
        else:
            proof_status = 'conditional_agreement'
        # Two aggregate result rows only; node observations live in the trace.
        for subclaim in ('F-proof-premises', 'F-satisfiability'):
            budget.visit()
            ledger_number += 1
            outcome = ('INVALID_RUN' if proof_status == 'invalid' else 'UNDERDETERMINED'
                       if proof_status == 'underdetermined' else 'FALSIFIED'
                       if subclaim == 'F-satisfiability' else 'NO_WITNESS')
            if subclaim == 'F-satisfiability':
                counts['admissible_falsifiers'] += int(outcome == 'FALSIFIED')
            counts['underdetermined_cases'] += int(outcome == 'UNDERDETERMINED')
            counts['comparisons'] += 1
            evidence.case('F', ledger_number, plan['proof_source_sha256'], outcome,
                          'ambiguous' if outcome == 'UNDERDETERMINED' else 'experiment', subclaim)
        charges['proof'] = budget.states['F'] - phase_start
        if charges['proof'] != 78 or proof_status == 'invalid':
            raise ValueError('proof disagreement, invalid premise or accounting mismatch')

        phase, phase_start = 'boundary', budget.states['F']
        for seed in (0, 1, 42, 804):
            for index in range(6):
                budget.visit()
                fixture = fixtures[2 + (index + seed % 6) % 6]
                counts['generated_cases'] += 1
                budget.visit(2)
                budget.visit()
                actual = candidate.fixture_predicates(fixture['manifest'], fixture['weight_hash'], schema)
                reference = oracle.fixture_reference(fixture['manifest'], fixture['weight_hash'], schema)
                digest = hashlib.sha256(json.dumps(fixture, sort_keys=True).encode()).hexdigest()
                for key, subclaim in (('schema_valid', 'F-schema-agreement'),
                                      ('root_model_equality', 'F-root-equality-agreement')):
                    budget.visit()
                    ledger_number += 1
                    counts['comparisons'] += 1
                    evidence.case('F', ledger_number, digest,
                                  'NO_WITNESS' if actual[key] == reference[key] else 'INVALID_RUN',
                                  'experiment', subclaim)
                coverage['boundary_observed'] += 1
                occurrences.append({'phase': 'boundary', 'fixture_id': fixture['id'],
                                    'seed': seed, 'occurrence': coverage['boundary_observed']})
                evidence.write(f'F-boundary-{coverage["boundary_observed"]}.json',
                               {'input': fixture, 'candidate': actual, 'reference': reference},
                               'F', witness=True)
                if actual != reference:
                    raise ValueError('boundary candidate/reference disagreement')
        charges['boundary'] = budget.states['F'] - phase_start
        if charges != {'controls': 32, 'proof': 78, 'boundary': 144}:
            raise ValueError('FA01 phase accounting mismatch')
        verdict = 'FALSIFIED' if proof_status == 'conditional_agreement' else 'UNDERDETERMINED'
        stop = 'amended_domain_complete'
    except Limit as error:
        verdict, stop = 'INCONCLUSIVE_LIMIT', str(error)
    except (ValueError, KeyError, IndexError, TypeError) as error:
        verdict, stop, proof_status = 'INVALID_RUN', str(error), 'invalid'
    finally:
        charges[phase] = budget.states['F'] - phase_start
        signal.setitimer(signal.ITIMER_REAL, 0)
        raw_trace = {'nodes': trace, 'status': proof_status, 'assumptions': assumptions,
                     'review_ref': authorization['f_assumption_review']}
        if len(json.dumps(raw_trace, ensure_ascii=True).encode()) + 1 > 32768:
            raise Limit('proof trace reservation')
        evidence.write('F-proof-trace.json', raw_trace, 'F', witness=True)
    return {'verdict': verdict, 'stop': stop, 'counters': counts,
            'states_reserved': budget.states['F'] - initial_states,
            'domain_complete': stop == 'amended_domain_complete',
            'area_f_plan_id': plan['id'], 'protocol_revision': 3,
            'protocol_sha256': plan['protocol_sha256'], 'amendment_id': 'RFC-00D7-F01-FA01',
            'amendment_sha256': plan['amendment_sha256'], 'corpus_sha256': plan['corpus_sha256'],
            'harness_sha': authorization['harness_sha'],
            'oracle_revision': authorization['harness_sha'],
            'assumptions': assumptions, 'assumption_review': authorization['f_assumption_review'],
            'proof_status': proof_status, 'proof_nodes_expected': 24, 'proof_nodes_observed': len(trace),
            'proof_trace_sha256': hashlib.sha256((evidence.root / 'F-proof-trace.json').read_bytes()).hexdigest(),
            'proof_trace': 'F-proof-trace.json', 'coverage': coverage,
            'occurrences': occurrences, 'state_breakdown': charges,
            'legacy_obligation': dict(plan['legacy_obligation'], executed_cases=0)}
