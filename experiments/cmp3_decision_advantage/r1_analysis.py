"""Approved r1 offline mathematics. No measurement authorization is implied.
Integer raw data and Fraction decisions; the authenticated CLI is analyze_r1.py.
"""
import functools
import math
import re
from fractions import Fraction

OUTCOMES = ('BLOCKED', 'NOT SUPPORTED', 'INCONCLUSIVE', 'SUPPORTED')
HEADER = 'pattern,mode,repetition,instructions,branch_misses,elapsed_ns,checksum'
U64_MAX = (1 << 64) - 1
I64_MAX = (1 << 63) - 1
MINIMUM = Fraction(3, 20)
REGRESSION_ALPHA = Fraction(1, 1800)  # family alpha .01 / 18

class Invalid(ValueError):
    pass

class PCG64:
    """Specified old-state PCG XSL-RR 128/64, arbitrary precision modulo 2^128."""
    mask = (1 << 128) - 1
    multiplier = 0x2360ED051FC65DA44385C511560B53A9
    def __init__(self):
        seed = 0x5438315F434D50335F53454544303131
        self.state = ((3 + seed) * self.multiplier + 3) & self.mask
    def next(self):
        old = self.state
        self.state = (old * self.multiplier + 3) & self.mask
        x = ((old >> 64) ^ old) & U64_MAX
        rotation = old >> 122
        return ((x >> rotation) | (x << ((-rotation) & 63))) & U64_MAX
    def bounded(self, n):
        if type(n) is not int or not 1 <= n <= U64_MAX:
            raise Invalid('INVALID_BOUND')
        threshold = (1 << 64) % n
        while True:
            x = self.next()
            if x >= threshold:
                return x % n

@functools.cache
def bootstrap_indices():
    generator = PCG64()
    return tuple(tuple(generator.bounded(10) for _ in range(10)) for _ in range(10000))

def validate_series(values, positive=False, maximum=U64_MAX):
    if len(values) != 10:
        raise Invalid('TEN_MATCHED_OBSERVATIONS_REQUIRED')
    if any(type(x) is not int or x < (1 if positive else 0) or x > maximum for x in values):
        raise Invalid('INVALID_INTEGER_OBSERVATION')

@functools.lru_cache(maxsize=128)
def reduction_bounds(candidate, baseline):
    validate_series(candidate, True)
    validate_series(baseline, True)
    reductions = []
    for indices in bootstrap_indices():
        b = sum(baseline[i] for i in indices)
        c = sum(candidate[i] for i in indices)
        reductions.append(Fraction(b - c, b))
    reductions.sort()
    return reductions[99], reductions[9899]

@functools.lru_cache(maxsize=1024)
def student_pvalue(t, df=9):
    """Two-sided Student-t tail: normalized integral of sin(theta)^(df-1).
    Composite Simpson quadrature; zero variance is handled before this routine.
    """
    if math.isinf(t):
        return 0.0
    if t == 0:
        return 1.0
    theta = math.atan2(math.sqrt(df), abs(t))
    panels = 4096
    h = theta / panels
    total = math.sin(theta) ** (df - 1)
    total += sum((4 if i % 2 else 2) * math.sin(i * h) ** (df - 1) for i in range(1, panels))
    normalizer = math.sqrt(math.pi) * math.gamma(df / 2) / (2 * math.gamma((df + 1) / 2))
    return min(1.0, max(0.0, total * h / (3 * normalizer)))

@functools.cache
def critical_value(alpha):
    low, high = 0.0, 100.0
    for _ in range(60):
        middle = (low + high) / 2
        if student_pvalue(middle) > float(alpha):
            low = middle
        else:
            high = middle
    return (low + high) / 2

def contrast(a, b):
    validate_series(a)
    validate_series(b)
    differences = [x - y for x, y in zip(a, b)]
    mean = Fraction(sum(differences), 10)
    variance = sum((Fraction(x) - mean) ** 2 for x in differences) / 9
    sd = math.sqrt(variance)
    se = sd / math.sqrt(10)
    p = (1.0 if mean == 0 else 0.0) if variance == 0 else student_pvalue(float(mean) / se)
    margin95 = critical_value(Fraction(1, 20)) * se
    margin99 = critical_value(Fraction(1, 100)) * se
    margin_family = critical_value(REGRESSION_ALPHA) * se
    return {'raw': differences, 'mean': mean, 'sample_variance': variance, 'sample_sd': sd,
            'p_two_sided': p, 'df': 9, 'degenerate': variance == 0,
            'ci95': [float(mean) - margin95, float(mean) + margin95],
            'ci99': [float(mean) - margin99, float(mean) + margin99],
            'family_interval': [float(mean) - margin_family, float(mean) + margin_family],
            'demonstrated_regression': mean > 0 and p < float(REGRESSION_ALPHA)}

def classify_pattern(modes):
    if len(modes) != 5:
        raise Invalid('ALL_FIVE_IMPLEMENTATIONS_REQUIRED')
    for mode in modes:
        if set(mode) != {'instructions', 'branch_misses', 'elapsed_ns'}:
            raise Invalid('INVALID_METRICS')
        validate_series(mode['instructions'], True)
        validate_series(mode['branch_misses'])
        validate_series(mode['elapsed_ns'], True, I64_MAX)
    candidate, baseline = modes[:2]
    ci, bi = tuple(candidate['instructions']), tuple(baseline['instructions'])
    low, high = reduction_bounds(ci, bi)
    reduction = Fraction(sum(bi) - sum(ci), sum(bi))
    primary = contrast(ci, bi)
    fastest = min(range(1, 5), key=lambda m: (sum(modes[m]['elapsed_ns']), m))
    regressions = {'instructions_vs_1': primary,
                   'branch_misses_vs_1': contrast(candidate['branch_misses'], baseline['branch_misses'])}
    regressions.update({f'latency_vs_{m}': contrast(candidate['elapsed_ns'], modes[m]['elapsed_ns'])
                        for m in range(1, 5)})
    assert len(regressions) == 6  # six fixed contrasts per pattern; divisor always 18
    reasons = [name for name, value in regressions.items() if value['demonstrated_regression']]
    qualifies = (reduction >= MINIMUM and low >= MINIMUM and primary['p_two_sided'] < .01 and
                 sum(candidate['branch_misses']) <= sum(baseline['branch_misses']) and
                 sum(candidate['elapsed_ns']) <= sum(modes[fastest]['elapsed_ns']))
    outcome = 'NOT SUPPORTED' if reasons else 'SUPPORTED' if qualifies else 'INCONCLUSIVE'
    reports = []
    for m, mode in enumerate(modes):
        reports.append({'mode': m, 'metrics': {metric: contrast(values, [0]*10) for metric, values in mode.items()},
                        'candidate_minus_baseline': {metric: contrast(candidate[metric], values)
                          for metric, values in mode.items()} if m else {}})
    return {'outcome': outcome, 'reasons': reasons, 'fastest_baseline': fastest,
            'instruction_reduction': reduction, 'bootstrap_lower': low, 'bootstrap_upper': high,
            'minimum_effect_status': 'ESTABLISHED' if low >= MINIMUM else 'BOUND_BELOW_TARGET' if high < MINIMUM else 'UNRESOLVED',
            'primary': primary, 'regressions': regressions, 'family_size': 18,
            'raw_and_intervals': reports, 'coverage': 'NOMINAL_NOT_EMPIRICALLY_CALIBRATED'}

def aggregate(outcomes):
    if len(outcomes) != 3 or any(x not in OUTCOMES for x in outcomes):
        raise Invalid('THREE_PATTERN_OUTCOMES_REQUIRED')
    return min(outcomes, key=OUTCOMES.index)

def parse_csv(raw):
    if not isinstance(raw, bytes) or len(raw) > 1024 * 1024:
        raise Invalid('INVALID_CSV_BYTES')
    try:
        text = raw.decode('ascii')
    except UnicodeDecodeError as error:
        raise Invalid('INVALID_CSV_ENCODING') from error
    lines = text.split('\n')
    if lines[-1] != '' or lines[0] != HEADER:
        raise Invalid('INVALID_CSV_HEADER_OR_TERMINATOR')
    records, checksums = {}, {}
    for line in lines[1:-1]:
        fields = line.split(',')
        if len(fields) != 7 or any(re.fullmatch(r'(0|[1-9][0-9]*)', s) is None for s in fields):
            raise Invalid('INVALID_INTEGER_OBSERVATION')
        if any(len(s) > 20 for s in fields):
            raise Invalid('INTEGER_OVERFLOW')
        p, m, r, instructions, misses, elapsed, checksum = map(int, fields)
        if p >= 3 or m >= 5 or r >= 10:
            raise Invalid('INVALID_RECORD_ID')
        if not 0 < instructions <= U64_MAX or not 0 <= misses <= U64_MAX or not 0 < elapsed <= I64_MAX or not 0 <= checksum <= U64_MAX:
            raise Invalid('INTEGER_OVERFLOW_OR_ZERO')
        key = (p, m, r)
        if key in records:
            raise Invalid('DUPLICATE_OBSERVATION')
        if p in checksums and checksum != checksums[p]:
            raise Invalid('PAYLOAD_PARITY')
        records[key] = (instructions, misses, elapsed)
        checksums[p] = checksum
    if len(records) != 150:
        raise Invalid('MISSING_OBSERVATIONS')
    return [[{metric: [records[p, m, r][j] for r in range(10)]
              for j, metric in enumerate(('instructions', 'branch_misses', 'elapsed_ns'))}
             for m in range(5)] for p in range(3)]

def diagnose(raw):
    patterns = [classify_pattern(modes) for modes in parse_csv(raw)]
    return {'kind': 'synthetic_diagnostic', 'patterns': patterns,
            'diagnostic_outcome': aggregate([p['outcome'] for p in patterns]),
            'measurement_outcome': 'BLOCKED', 'reason': 'TEST_ONLY_EVIDENCE_NOT_MEASUREMENTS'}

def json_value(value):
    if isinstance(value, Fraction):
        return {'numerator': value.numerator, 'denominator': value.denominator}
    if isinstance(value, dict):
        return {key: json_value(v) for key, v in value.items()}
    if isinstance(value, (tuple, list)):
        return [json_value(v) for v in value]
    return value
