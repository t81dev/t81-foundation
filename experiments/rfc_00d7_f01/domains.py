"""Lazy finite domains. Nothing is enumerated at import time."""

import copy
import hashlib
import itertools


def cases(area, visit):
    """Charge generated/pruned input states; comparison charges are separate."""
    if area == 'A':
        for k in (1, 2, 4, 256):
            commits = {0, 1, k - 1, k, k + 1, 2 * k - 1}
            if k == 256:
                commits.add(100)
            for commit, tie, delay in itertools.product(
                    sorted(commits), ('before', 'after'), sorted({0, 1, k})):
                visit()
                latch = ((commit + k - 1) // k) * k
                if tie == 'after' and commit % k == 0:
                    latch += k
                yield {'kind': 'bound', 'k': k, 'commit': commit,
                       'latch': latch, 'halt': latch + delay, 'tie': tie}
    elif area == 'B':
        events = ('invoke', 'commit', 'acquire', 'check', 'publish', 'release', 'response')
        for order in itertools.permutations(events):
            visit()  # Rejected orders remain visible to accounting.
            position = {event: order.index(event) for event in events}
            if not (position['invoke'] < position['commit'] < position['response']
                    and position['acquire'] < position['check'] < position['publish']
                    < position['release']):
                continue
            for attempt, scope, target, initial in itertools.product(
                    ('primary', 'fallback'), ('epoch', 'selective'),
                    ('target', 'peer'), (False, True)):
                visit()
                yield {'kind': 'fence', 'order': order, 'attempt': attempt,
                       'scope': scope, 'target': target,
                       'applicable': scope == 'epoch' or target == 'target',
                       'initial_revoked': initial}
    elif area == 'C':
        alphabet = (('dispatch', 1), ('dispatch', 2), ('revoke', 1),
                    ('revoke', 2), ('cancel', 1), ('cancel', 2))
        for length in range(5):
            for history in itertools.product(alphabet, repeat=length):
                visit()
                created = set()
                valid = True
                for operation, subject in history:
                    visit()
                    if operation == 'dispatch':
                        if subject in created:
                            valid = False
                        created.add(subject)
                    if operation == 'cancel' and subject not in created:
                        valid = False
                if valid:
                    yield {'kind': 'session', 'history': history}
        for seed, coordinate in itertools.product((0, 42, 18446744073709551615), (1, 2)):
            visit()
            yield {'kind': 'trace', 'seed': str(seed), 'coordinate': coordinate}
    elif area == 'D':
        for capacity, slots in itertools.product((2048, 4096), (2, 3)):
            for sizes in itertools.product((0, 512, 2048), repeat=7):
                visit()
                if capacity >= slots * 512:
                    yield {'kind': 'memory', 'capacity': capacity,
                           'slots': slots, 'sizes': sizes}
        for charges, limit, reserve in itertools.product(
                itertools.product((0, 1, 2, 4), repeat=3), (1, 4, 8), (0, 1, 2)):
            visit()
            if reserve <= limit:
                yield {'kind': 'steps', 'charges': charges,
                       'limit': limit, 'reserve': reserve}
        visit()
        yield {'kind': 'steps', 'charges': (420, 0, 100), 'limit': 500, 'reserve': 50}
        for queue, active, queued, slots in itertools.product(
                (1, 2), (0, 1, 2), (0, 1, 2, 3), (1, 2, 3, 4)):
            visit()
            yield {'kind': 'queue', 'queue_limit': queue, 'active': active,
                   'queued': queued, 'slots': slots}
    elif area == 'E':
        for prefix, sync_ok, crash in itertools.product(
                range(513), (False, True),
                ('construct', 'hash', 'write', 'sync', 'flag', 'crash')):
            visit()
            yield {'kind': 'journal', 'request': 1, 'step': 256, 'reason': 'synthetic',
                   'prefix': prefix, 'sync_ok': sync_ok, 'crash_after': crash}
        boundaries = (0, 1, 7, 8, 10, 11, 12, 479, 480, 481, 511, 512)
        for first, second in itertools.product(boundaries, repeat=2):
            visit()
            yield {'kind': 'two_phase_control', 'request': 1, 'step': 256,
                   'reason': 'synthetic', 'prefix': first, 'second_prefix': second,
                   'sync_ok': True, 'crash_after': 'crash'}
        for offset in (0, 8, 10, 16, 48, 56, 479, 480, 511):
            visit()
            yield {'kind': 'bitflip', 'offset': offset, 'request': 1,
                   'step': 256, 'reason': 'synthetic'}
    elif area == 'F':
        for length, pattern in itertools.product((1, 2, 16), ('zero', 'one', 'alternating')):
            raw = (bytes(length) if pattern == 'zero' else bytes([1]) * length
                   if pattern == 'one' else bytes(0 if i % 2 == 0 else 255
                                                 for i in range(length)))
            digest = hashlib.sha3_512(raw).digest()[:32].hex()
            base = {'schema': 't81.model-manifest.v1', 'source_path': 'synthetic/w.bin',
                    'source_format': 'safetensors', 'normalized_artifact_type': 'model-tensor-manifest',
                    'imported_format_summary': 'synthetic', 'tensor_count': 1,
                    'parameters': 1, 'trits': 0, 'file_size_bytes': length,
                    'provenance': {'host_format_reader': 'safetensors'},
                    'tensors': [{'name': 'w', 'shape': [1], 'trits': 0}]}
            for fields, matches in itertools.product(((), ('model_canon_hash81',),
                    ('state_schema_hash81',), ('model_canon_hash81', 'state_schema_hash81')),
                    (True, False)):
                visit()
                manifest = copy.deepcopy(base)
                for name in fields:
                    manifest[name] = digest if matches else 'mismatch'
                yield {'kind': 'manifest', 'manifest': manifest, 'weight_hash': digest,
                       'weight_bytes': raw.hex(), 'pattern': pattern}
            for target in ('manifest', 'weight'):
                visit()
                manifest = copy.deepcopy(base)
                changed_raw = bytearray(raw)
                if target == 'manifest':
                    manifest['imported_format_summary'] = 'synthetid'
                else:
                    changed_raw[0] ^= 1
                yield {'kind': 'mutation_control', 'manifest': manifest,
                       'weight_hash': digest, 'weight_bytes': bytes(changed_raw).hex(),
                       'mutation': target + ':one-byte'}
            for key in sorted(base):
                for mutation in ('missing', 'null'):
                    visit()
                    manifest = copy.deepcopy(base)
                    if mutation == 'missing':
                        del manifest[key]
                    else:
                        manifest[key] = None
                    yield {'kind': 'schema_control', 'manifest': manifest,
                           'weight_hash': digest, 'weight_bytes': raw.hex(),
                           'mutation': mutation + ':' + key}


def control_cases(area):
    """Literal reviewed corpus recipe; never calls cases() to find controls."""
    if area == 'A':
        base = {'kind': 'bound', 'k': 256, 'commit': 100, 'latch': 256}
        return [('positive', dict(base, halt=256)), ('negative', dict(base, halt=512))]
    if area == 'B':
        base = {'kind': 'fence', 'initial_revoked': False, 'applicable': True}
        return [('positive', dict(base, order=('invoke', 'commit', 'acquire', 'check',
                                               'publish', 'release', 'response'))),
                ('negative', dict(base, order=('invoke', 'acquire', 'check', 'commit',
                                               'publish', 'release', 'response')))]
    if area == 'C':
        return [('positive', {'kind': 'collision', 'left': (('revoke', 1),),
                             'right': (('revoke', 2),)}),
                ('negative', {'kind': 'collision', 'left': (('revoke', 1),),
                             'right': (('revoke', 2),)})]
    if area == 'D':
        case = {'kind': 'memory', 'capacity': 4096, 'slots': 2,
                'sizes': (512, 0, 0, 2048, 2048, 0, 0)}
        steps = {'kind': 'steps', 'charges': (420, 0, 100), 'limit': 500, 'reserve': 50}
        return [('positive', case), ('negative', case),
                ('positive', steps), ('negative', steps)]
    if area == 'E':
        case = {'kind': 'journal', 'request': 1, 'step': 256, 'reason': 'synthetic',
                'prefix': 512, 'sync_ok': True, 'crash_after': 'crash'}
        return [('positive', case), ('negative', case),
                ('positive', dict(case, request=2, step=0, reason='')),
                ('positive', dict(case, request=2, step=1, reason='x' * 16))]
    if area == 'F':
        # Input construction is explicit and lazy; no domain search is involved.
        manifest = {'schema': 't81.model-manifest.v1', 'source_path': 'synthetic/w.bin',
                    'source_format': 'safetensors', 'normalized_artifact_type': 'model-tensor-manifest',
                    'imported_format_summary': 'synthetic', 'tensor_count': 1, 'parameters': 1,
                    'trits': 0, 'file_size_bytes': 1,
                    'provenance': {'host_format_reader': 'safetensors'},
                    'tensors': [{'name': 'w', 'shape': [1], 'trits': 0}]}
        return [('positive', {'kind': 'manifest', 'manifest': manifest, 'weight_hash': 'test'}),
                ('negative', {'kind': 'manifest', 'manifest': dict(manifest, unexpected=True),
                              'weight_hash': 'test'})]
    raise ValueError(area)
