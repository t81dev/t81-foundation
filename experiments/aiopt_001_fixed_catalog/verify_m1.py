"""Independent arithmetic verification, no timing, selectors or third-party code."""
import argparse
from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import platform
import subprocess
import sys


def reference_vector(ordinal):
    # Base-three ordinal gives lexicographic order, independently of product().
    digits = []
    for weight in (27, 9, 3, 1):
        digit, ordinal = divmod(ordinal, weight)
        digits.append(digit - 1)
    return tuple(digits)


def reference_indices(values):
    rank = 27 * (values[0] + 1) + 9 * (values[1] + 1) + 3 * (values[2] + 1) + values[3] + 1
    return [rank] if rank < 40 else []


def reference_unpack(payload):
    digits = []
    for _ in range(4):
        payload, code = divmod(payload, 4)
        if code == 3:
            return None
        digits.append(code - 1)
    return tuple(digits)


class Checks:
    def __init__(self):
        self.executed = Counter()
        self.failures = []

    def equal(self, group, label, expected, callback):
        self.executed[group] += 1
        try:
            observed = callback()
        except Exception as exc:
            observed = {'exception': type(exc).__name__, 'code': getattr(exc, 'code', None)}
        if observed != expected:
            self.failures.append(dict(group=group, label=label, expected=expected, observed=observed))

    def error(self, group, label, code, callback):
        def observed():
            try:
                callback()
            except Exception as exc:
                return {'exception': type(exc).__name__, 'code': getattr(exc, 'code', None)}
            return 'accepted'
        self.equal(group, label, {'exception': 'ContractError', 'code': code}, observed)


def verify(module):
    check = Checks()
    domain = tuple(reference_vector(i) for i in range(81))
    catalog = domain[:40]
    check.equal('mathematics', 'cardinality', 81, lambda: len(set(domain)))
    check.equal('mathematics', 'first', (-1, -1, -1, -1), lambda: domain[0])
    check.equal('mathematics', 'index39', (0, 0, 0, -1), lambda: catalog[39])
    check.equal('mathematics', 'index40', (0, 0, 0, 0), lambda: domain[40])
    check.equal('mathematics', 'last', (1, 1, 1, 1), lambda: domain[-1])
    check.equal('catalog', 'exact derived catalog', catalog, lambda: module.CATALOG)
    observations = []
    byte_encodings, swar_encodings = set(), set()
    for rank, values in enumerate(domain):
        expected = [rank] if rank < 40 else []
        byte = bytes({-1: 0, 0: 1, 1: 2}[v] for v in values)
        swar = sum({-1: 0, 0: 1, 1: 2}[v] * (4 ** i) for i, v in enumerate(values))
        check.equal('reference', str(rank), expected, lambda: reference_indices(values))
        check.equal('encodings', f'byte/{rank}', byte, lambda: module.encode_bytes(values))
        check.equal('encodings', f'swar/{rank}', swar, lambda: module.encode_swar(values))
        check.equal('roundtrips', f'byte/{rank}', values, lambda: module.decode_bytes(byte))
        check.equal('roundtrips', f'swar/{rank}', values, lambda: module.decode_swar(swar))
        actual = {'oracle': module.oracle(values), 'byte': module.filter_bytes(byte),
                  'swar': module.filter_swar(swar)}
        for name, result in actual.items():
            check.equal('filter_outputs', f'{name}/{rank}', expected, lambda: result)
            for index in range(40):
                check.equal('predicate_decisions', f'{name}/{rank}/{index}', rank == index,
                            lambda: index in result)
        byte_encodings.add(module.encode_bytes(values))
        swar_encodings.add(module.encode_swar(values))
        observations.append(dict(input=values, matches=actual['oracle'], byte=list(byte), swar=swar))
    check.equal('uniqueness', 'byte', 81, lambda: len(byte_encodings))
    check.equal('uniqueness', 'swar', 81, lambda: len(swar_encodings))
    valid_swar = 0
    for payload in range(256):
        values = reference_unpack(payload)
        if values is None:
            for name in ('decode_swar', 'filter_swar'):
                check.error('all_swar_payloads', f'{name}/{payload}', 'INVALID_ENCODING',
                            lambda: getattr(module, name)(payload))
        else:
            valid_swar += 1
            check.equal('all_swar_payloads', f'decode/{payload}', values,
                        lambda: module.decode_swar(payload))
            check.equal('all_swar_payloads', f'filter/{payload}', reference_indices(values),
                        lambda: module.filter_swar(payload))
    check.equal('mathematics', 'valid/invalid swar', (81, 175), lambda: (valid_swar, 256-valid_swar))
    for position in range(4):
        for code in range(3, 256):
            byte = bytearray(4)
            byte[position] = code
            for name in ('decode_bytes', 'filter_bytes'):
                check.error('invalid_byte_values', f'{name}/{position}/{code}', 'INVALID_ENCODING',
                            lambda: getattr(module, name)(byte))
        for bad in (-2, 2, True, False, 0.0, None, '0'):
            values = [0] * 4
            values[position] = bad
            for name in ('oracle', 'encode_bytes', 'encode_swar'):
                check.error('invalid_trits', f'{name}/{position}/{bad!r}', 'INVALID_TRIT',
                            lambda: getattr(module, name)(values))
    for position in range(4):
        for bad in (-1, 256, True, False, 0.0, None, '0'):
            payload = [0] * 4
            payload[position] = bad
            for name in ('decode_bytes', 'filter_bytes'):
                check.error('invalid_byte_types', f'{name}/{position}/{bad!r}', 'INVALID_ENCODING',
                            lambda: getattr(module, name)(payload))
    for n in (0, 1, 2, 3, 5, 6):
        for name in ('oracle', 'encode_bytes', 'encode_swar', 'decode_bytes', 'filter_bytes'):
            check.error('wrong_lengths', f'{name}/{n}', 'INVALID_LENGTH',
                        lambda: getattr(module, name)([0] * n))
    # Unsupported objects must not escape as arbitrary TypeError/KeyError.
    factories = [('none', lambda: None), ('integer', lambda: 4), ('bool', lambda: True),
                 ('float', lambda: 4.0), ('string', lambda: '0000'),
                 ('dictionary', lambda: {i: 0 for i in range(4)}),
                 ('set', lambda: {0, 1, 2, 3}), ('iterator', lambda: iter((0, 0, 0, 0)))]
    class Hostile:
        def __len__(self): raise AssertionError('unexpected user length execution')
        def __iter__(self): raise AssertionError('unexpected user iteration execution')
    factories.append(('hostile', Hostile))
    for label, factory in factories:
        for name in ('oracle', 'encode_bytes', 'encode_swar'):
            check.error('unexpected_types', name+'/'+label, 'INVALID_LENGTH',
                        lambda: getattr(module, name)(factory()))
        for name in ('decode_bytes', 'filter_bytes', 'decode_swar', 'filter_swar'):
            if label == 'integer' and name in ('decode_swar', 'filter_swar'):
                continue  # Integer 4 is valid SWAR, covered by all-256 checks.
            check.error('unexpected_types', name+'/'+label, 'INVALID_ENCODING',
                        lambda: getattr(module, name)(factory()))
    for bad in (-1, 256, True, False, 0.0, -1.0, float('nan'), float('inf'), b'\0', [0], (0,)):
        for name in ('decode_swar', 'filter_swar'):
            check.error('invalid_swar_types_range', f'{name}/{bad!r}', 'INVALID_ENCODING',
                        lambda: getattr(module, name)(bad))
    bad_catalogs = [
        ('short', lambda: catalog[:-1]), ('long', lambda: catalog+(domain[40],)),
        ('reversed', lambda: catalog[::-1]), ('duplicate', lambda: catalog[:-1]+(catalog[0],)),
        ('wrong_member', lambda: catalog[:-1]+(domain[40],)),
        ('boolean', lambda: catalog[:-1]+((0, False, 0, -1),)),
        ('float', lambda: ((-1.0, -1, -1, -1),)+catalog[1:]),
        ('invalid_trit', lambda: ((-2, -1, -1, -1),)+catalog[1:]),
        ('bad_row_length', lambda: ((-1, -1, -1),)+catalog[1:]),
        ('none_row', lambda: (None,)+catalog[1:]),
        ('none', lambda: None), ('integer', lambda: 40),
        ('string', lambda: 'x'*40), ('dictionary', lambda: dict.fromkeys(catalog)),
        ('generator', lambda: (row for row in catalog)),
    ]
    for label, factory in bad_catalogs:
        for name, value in [('oracle', (-1,)*4), ('filter_bytes', b'\0'*4), ('filter_swar', 0)]:
            check.error('invalid_catalogs', name+'/'+label, 'INVALID_CATALOG',
                        lambda: getattr(module, name)(value, factory()))
            check.error('error_precedence', name+'/'+label, 'INVALID_CATALOG',
                        lambda: getattr(module, name)(None, factory()))
    for name, good, bad in [('oracle', (-1,)*4, [2]), ('filter_bytes', b'\0'*4, [255])]:
        check.error('error_precedence', name+'/length-before-element', 'INVALID_LENGTH',
                    lambda: getattr(module, name)(bad))
    for container in (list, tuple):
        rows = container(container(row) for row in catalog)
        for name, value in [('oracle', (-1,)*4), ('filter_bytes', b'\0'*4), ('filter_swar', 0)]:
            check.equal('supported_catalogs', name+'/'+container.__name__, [0],
                        lambda: getattr(module, name)(value, rows))
    for payload in ([0, 0, 0, 0], (0, 0, 0, 0), b'\0'*4, bytearray(4)):
        check.equal('supported_bytes', type(payload).__name__, [0], lambda: module.filter_bytes(payload))
    # Instrument actual runner calls; never trust the summary's manual count.
    expect = module.expect_error
    byte_fn, swar_fn = module.filter_bytes, module.filter_swar
    errors, calls = Counter(), Counter()
    def counted_error(code, func, *args):
        result = expect(code, func, *args)
        errors[code] += 1
        return result
    def counted_byte(*args, **kwargs):
        p = args[0]
        if len(args) == 1 and type(p) is bytes and len(p) == 4 and all(b < 3 for b in p):
            calls['valid_byte_filters'] += 1
        return byte_fn(*args, **kwargs)
    def counted_swar(*args, **kwargs):
        p = args[0]
        if len(args) == 1 and type(p) is int and 0 <= p < 256 and reference_unpack(p) is not None:
            calls['valid_swar_filters'] += 1
        return swar_fn(*args, **kwargs)
    module.expect_error, module.filter_bytes, module.filter_swar = counted_error, counted_byte, counted_swar
    try:
        run = module.run()
    finally:
        module.expect_error, module.filter_bytes, module.filter_swar = expect, byte_fn, swar_fn
    check.equal('runner_integrity', 'counted malformed calls', sum(errors.values()), lambda: run['malformed_assertions'])
    check.equal('runner_integrity', 'byte valid calls', 81, lambda: calls['valid_byte_filters'])
    check.equal('runner_integrity', 'swar valid calls', 81, lambda: calls['valid_swar_filters'])
    def mutation_rejected():
        original = module.filter_bytes
        def wrong_filter(payload, *args, **kwargs):
            value = original(payload, *args, **kwargs)
            return [1] if not args and payload == b'\0'*4 else value
        module.filter_bytes = wrong_filter
        try:
            module.run()
        except AssertionError:
            return True
        finally:
            module.filter_bytes = original
        return False
    def wrong_error_rejected():
        def wrong_error():
            raise module.ContractError('INVALID_TRIT')
        try:
            module.expect_error('INVALID_ENCODING', wrong_error)
        except AssertionError:
            return True
        return False
    check.equal('runner_integrity', 'wrong valid adapter result rejected', True, mutation_rejected)
    check.equal('runner_integrity', 'wrong error category rejected', True, wrong_error_rejected)
    raw = json.dumps(observations, sort_keys=True, separators=(',', ':')).encode()
    digest = hashlib.sha256(raw).hexdigest()
    check.equal('runner_integrity', 'independent canonical digest', digest, lambda: run['sha256_results'])
    return dict(status='PASS' if not check.failures else 'FAIL',
                source_commit=subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
                implementation_sha256=hashlib.sha256(Path(module.__file__).read_bytes()).hexdigest(),
                verifier_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                python=platform.python_version(), platform=platform.platform(),
                optimization_flag=sys.flags.optimize, checks=dict(check.executed),
                tests_executed=sum(check.executed.values()),
                tests_passed=sum(check.executed.values())-len(check.failures),
                tests_failed=len(check.failures), tests_skipped=0, failures=check.failures,
                instrumented_malformed_calls=dict(errors), instrumented_valid_filter_calls=dict(calls),
                canonical_result_digest=digest, observations=observations)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--module', type=Path, default=Path(__file__).with_name('m1.py'))
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('subject_m1', args.module)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    result = verify(module)
    encoded = json.dumps(result, sort_keys=True, indent=2, allow_nan=False)+'\n'
    if args.output:
        args.output.write_text(encoded)
    print(json.dumps({k: v for k, v in result.items() if k not in ('observations', 'failures')}, sort_keys=True))
    return 0 if result['status'] == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
