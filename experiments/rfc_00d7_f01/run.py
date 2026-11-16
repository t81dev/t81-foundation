"""Execution entry point. Requires future authorization; NOT RUN in preparation."""

import argparse
import hashlib
import json
import os
import signal
from pathlib import Path

import domains
import evaluate
from accounting import Budget, Evidence, Limit
from interpretation_policy import validate
import fa01


def run_area(area, schema, interpretations, budget, evidence, authorization=None):
    declarations = json.loads((Path(__file__).parent / 'interpretations.json').read_text())['choices']
    validate(interpretations, declarations)
    if area == 'F':
        if authorization is None:
            raise ValueError('BLOCKED: FA01 authorization metadata required')
        return fa01.run(schema, authorization, budget, evidence)
    budget.enter(area)
    signal.setitimer(signal.ITIMER_REAL, {'A': 5, 'B': 5, 'C': 10, 'D': 10, 'E': 10, 'F': 10}[area])
    counts = {'admissible_falsifiers': 0, 'negative_controls': 0,
              'underdetermined_cases': 0, 'positive_controls': 0,
              'comparisons': 0, 'generated_cases': 0, 'pruned_states': None}
    cache = {}
    solved = set()
    verdict = 'NO_COUNTEREXAMPLE_IN_DOMAIN'
    stop = 'domain_complete'
    number = 0
    comparison_number = 0
    try:
        corpus = domains.control_cases(area)
        if len(corpus) > 16:
            raise ValueError('unreviewed control corpus expansion')
        for seed in (0, 1, 42, 804):
            for index in range(len(corpus)):
                budget.visit()
                polarity, case = corpus[(index + seed % len(corpus)) % len(corpus)]
                number += 1
                ok = evaluate.check_control(area, polarity, case, schema, budget.visit)
                counts[polarity + '_controls'] += 1
                evidence.write(f'{area}-control-{number}.json',
                               {'seed': seed, 'category': polarity, 'input': case,
                                'control_detected_or_correct': ok}, area)
                if not ok:
                    verdict, stop = 'INVALID_RUN', 'control_failed'
                    return {'verdict': verdict, 'stop': stop, 'counters': counts,
                            'states_reserved': budget.states[area], 'domain_complete': False}
        for case in domains.cases(area, budget.visit):
            number += 1
            counts['generated_cases'] += 1
            digest = hashlib.sha256(json.dumps(case, sort_keys=True).encode()).hexdigest()
            targets = {'A': {'A'}, 'B': {'B1', 'B2'},
                       'C': {'C1'} if case['kind'] == 'session' else {'C2', 'C2-nested'},
                       'D': {'memory': {'D1'}, 'steps': {'D2'}, 'queue': {'D3'}}.get(case['kind'], set()),
                       'E': {'E1', 'E2'}, 'F': {'F-validator', 'F-satisfiability'}}[area]
            if targets and targets <= solved:
                continue
            rows = evaluate.compare(area, case, schema, interpretations, cache, budget.visit, declarations)
            for row in rows:
                if row['subclaim'] in solved:
                    continue
                counts['comparisons'] += 1
                subclaim = row['subclaim']
                is_control = (case['kind'] in ('schema_control', 'mutation_control', 'bitflip', 'two_phase_control'))
                if is_control:
                    # Diagnostic/control cases never become RFC falsifiers.
                    category = 'negative' if case['kind'] != 'two_phase_control' else 'positive'
                    counts[category + '_controls'] += 1
                    outcome = 'CONTROL'
                elif row['admissible'] is None:
                    category, outcome = 'ambiguous', 'UNDERDETERMINED'
                    counts['underdetermined_cases'] += 1
                elif row['admissible'] and row['violation']:
                    category, outcome = 'experiment', 'FALSIFIED'
                    counts['admissible_falsifiers'] += 1
                else:
                    category, outcome = 'experiment', 'NO_WITNESS'
                comparison_number += 1
                evidence.case(area, comparison_number, digest, outcome, category, subclaim)
                if outcome in ('FALSIFIED', 'UNDERDETERMINED') and subclaim not in solved:
                    evidence.write(f'{area}-{subclaim}-witness.json',
                                   {'input': case, 'comparison': row,
                                    'classification': outcome, 'minimization_attempts': 0,
                                    'minimality': 'not established'}, area, witness=True)
                    solved.add(subclaim)
                # Stop comparisons for solved subclaims; shared candidate
                # transitions may still be required by another open subclaim.
            if counts['admissible_falsifiers']:
                verdict = 'FALSIFIED'
            elif counts['underdetermined_cases']:
                verdict = 'UNDERDETERMINED'
            if area in ('A', 'F') and counts['admissible_falsifiers']:
                stop = 'first_witness_after_controls'
                break
    except Limit as error:
        stop = str(error)
        # Preserve prior witnesses, but mark the area coverage incomplete.
        verdict = 'INCONCLUSIVE_LIMIT'
    finally:
        signal.setitimer(signal.ITIMER_REAL, 0)
    return {'verdict': verdict, 'stop': stop, 'counters': counts,
            'states_reserved': budget.states[area], 'domain_complete': stop == 'domain_complete',
            'known_witness_subclaims': sorted(solved)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--authorization', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    # Supervisor checks identity, schemas, cgroups and human-scope record before
    # launching this entry. A JSON field alone never establishes authenticity.
    if os.environ.get('F01_QUALIFIED_CHILD') != '1':
        raise SystemExit('BLOCKED: invoke only through reviewed Linux supervisor')
    authorization = json.loads(args.authorization.read_text())
    if authorization['scope'] != 'one-bounded-run' or not authorization['human_record']:
        raise SystemExit('BLOCKED: execution authorization absent')
    schema = json.loads((Path(__file__).parent / 'pinned-d3.schema.json').read_text())
    interpretations = authorization['interpretations']
    budget = Budget()
    evidence = Evidence(args.output, budget)
    evidence.write('inputs-manifest.json', {
        'identities': json.loads((Path(__file__).parent / 'identities.json').read_text()),
        'interpretations': interpretations,
        'authorization_harness_sha': authorization['harness_sha'],
        'seeds': [0, 1, 42, 804],
        'command': 'reviewed Linux supervisor -> one sequential runner',
        'dependency_lock_sha256': hashlib.sha256((Path(__file__).parent / 'dependency-lock.json').read_bytes()).hexdigest(),
    })
    summary = {'evidence_schema': 't81.rfc-00d7-f01.evidence.v2',
               'counter_unit': 'atomic comparison case (input, subclaim, rotation)',
               'protocol': 'RFC-00D7-F01', 'areas': {}, 'experiment_execution': 'attempted',
               'counters': {'admissible_falsifiers': 0, 'negative_controls': 0,
                            'underdetermined_cases': 0}}

    def deadline(_signal, _frame):
        raise Limit('area external timer')

    signal.signal(signal.SIGALRM, deadline)
    try:
        for area in 'ABCDEF':
            # Tiny authenticated-parent status pipe, not a worker-service IPC.
            os.write(1, (area + '\n').encode('ascii'))
            summary['areas'][area] = run_area(area, schema, interpretations, budget, evidence, authorization)
            for name in summary['counters']:
                summary['counters'][name] += summary['areas'][area]['counters'][name]
            if summary['areas'][area]['verdict'] == 'INVALID_RUN':
                summary['stop'] = 'control_failure_blocks_remaining_areas'
                break
        summary['artifact_bytes_before_summary'] = sum(budget.bytes.values()) + budget.metadata_bytes
        summary['total_states_reserved'] = sum(budget.states.values())
        from jsonschema import Draft202012Validator
        definition = json.loads((Path(__file__).parent / 'evidence.schema.json').read_text())
        Draft202012Validator(definition['$defs']['summary']).validate(summary)
        evidence.write('summary.json', summary)
    finally:
        evidence.close()


if __name__ == '__main__':
    main()
