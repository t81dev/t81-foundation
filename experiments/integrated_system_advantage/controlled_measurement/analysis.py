"""Offline analysis, independent of the execution harness and comparator arithmetic.

Only explicit synthetic fixtures or future authorized confirmatory datasets are
accepted. Instrumentation-validation records cannot become performance evidence.
"""
import argparse
from collections import Counter
from fractions import Fraction
import hashlib
import json
import math
from pathlib import Path
import statistics

CELLS = tuple(f"{p}/{s}" for p in ("greet_hello", "hello_world", "greet")
              for s in ("empty_store", "populated_store"))
ADVANTAGE = "ADVANTAGE_SUPPORTED"
DISADVANTAGE = "DISADVANTAGE_SUPPORTED"
EQUIVALENT = "PRACTICAL_EQUIVALENCE_SUPPORTED"
INCONCLUSIVE = "INCONCLUSIVE"
SCHEDULE_SHA256 = "b9f3fdee3424840dd10c54932f42185df7d20213bb730cf4464cf136289aa5ef"


def registered_schedule():
    raw = Path(__file__).with_name("schedule.json").read_bytes()
    if hashlib.sha256(raw).hexdigest() != SCHEDULE_SHA256:
        raise ValueError("Frozen schedule identity changed")
    return json.loads(raw)


def interval_indices(n=60, cells=6):
    """1-based indices; exact integer comparison, no floating CDF rounding."""
    if n != 60 or cells != 6:
        raise ValueError("Only the registered N=60, six-cell design is supported")
    tail = 0
    valid = []
    for k in range(1, n // 2 + 1):
        tail += math.comb(n, k - 1)
        # tail / 2**N <= (1/20) / (2*cells)
        if tail * 40 * cells <= 2**n:
            valid.append(k)
    k = max(valid)
    return k, n - k + 1


def decision(lower, upper):
    """Exact rational boundaries: equality never becomes a directional win."""
    lower, upper = Fraction(lower), Fraction(upper)
    if lower <= 0 or lower > upper:
        raise ValueError("Invalid confidence limits")
    if upper < Fraction(10, 11):
        return ADVANTAGE
    if lower > Fraction(11, 10):
        return DISADVANTAGE
    if lower >= Fraction(10, 11) and upper <= Fraction(11, 10):
        return EQUIVALENT
    return INCONCLUSIVE


def overall(outcomes):
    if set(outcomes) != set(CELLS):
        raise ValueError("Exactly the six registered cells are required")
    values = set(outcomes.values())
    allowed = {ADVANTAGE, DISADVANTAGE, EQUIVALENT, INCONCLUSIVE}
    if not values <= allowed:
        raise ValueError("Unknown cell decision")
    return next(iter(values)) if len(values) == 1 else INCONCLUSIVE


def autocorrelation(values, lag):
    mean = statistics.mean(values)
    denominator = sum((v - mean)**2 for v in values)
    if denominator == 0:
        return 0.0
    return sum((values[i] - mean) * (values[i-lag] - mean)
               for i in range(lag, len(values))) / denominator


def descriptive(values):
    ordered = sorted(values)
    median = statistics.median(ordered)
    quartiles = statistics.quantiles(ordered, n=4, method="inclusive")
    return dict(mean=statistics.mean(ordered), median=median,
                sample_sd=statistics.stdev(ordered), iqr=quartiles[2]-quartiles[0],
                mad=statistics.median(abs(v-median) for v in ordered),
                minimum=min(ordered), maximum=max(ordered))


def rational(value):
    return dict(numerator=value.numerator, denominator=value.denominator,
                value=float(value))


def analyze(dataset, *, synthetic=False):
    expected = "synthetic_unit_test" if synthetic else "confirmatory"
    if dataset.get("purpose") != expected:
        raise ValueError("Validation/pilot records are not confirmatory data")
    if not synthetic and not dataset.get("measurement_authorization_record"):
        raise ValueError("Separate explicit measurement authorization required")
    if dataset.get("schedule_sha256") != SCHEDULE_SHA256:
        raise ValueError("Dataset does not identify the registered schedule")
    scheduled_rows = registered_schedule()["pairs"]
    planned = {(r["cell"], r["block"]): r for r in scheduled_rows}
    rows = dataset.get("pairs", [])
    if len(rows) != 360:
        raise ValueError("Incomplete fixed sample; retain it, do not classify")
    groups = {cell: [] for cell in CELLS}
    for row in rows:
        if row.get("purpose") != expected or row.get("cell") not in groups:
            raise ValueError("Wrong purpose or unregistered cell")
        if row.get("status") != "complete" or row.get("equivalent") is not True:
            raise ValueError("Failed/incomplete/non-equivalent pair")
        if type(row.get("block")) is not int or not 0 <= row["block"] < 60:
            raise ValueError("Invalid block")
        if row.get("order") not in (["A", "B"], ["B", "A"]):
            raise ValueError("Invalid arm order")
        if row["order"] != planned[row["cell"], row["block"]]["order"]:
            raise ValueError("Arm order does not match the frozen schedule")
        for arm in ("A", "B"):
            value = row.get("arms", {}).get(arm, {})
            duration = value.get("elapsed_ns")
            if type(duration) is not int or duration <= 0 or value.get("verified") is not True:
                raise ValueError("Nonpositive/noninteger duration or unverified arm")
        groups[row["cell"]].append(row)
    if [(r["cell"], r["block"]) for r in rows] != [(r["cell"], r["block"]) for r in scheduled_rows]:
        raise ValueError("Raw pair chronology does not match the frozen schedule")
    lo, hi = interval_indices()
    results = {}
    for cell, sample in groups.items():
        if len(sample) != 60 or {r["block"] for r in sample} != set(range(60)):
            raise ValueError("Missing/duplicated blocks")
        if Counter(tuple(r["order"]) for r in sample) != {("A", "B"): 30, ("B", "A"): 30}:
            raise ValueError("Unbalanced or substituted schedule")
        sample.sort(key=lambda r: r["block"])
        a = [r["arms"]["A"]["elapsed_ns"] for r in sample]
        b = [r["arms"]["B"]["elapsed_ns"] for r in sample]
        ratios = [Fraction(x, y) for x, y in zip(a, b)]
        ordered = sorted(ratios)
        lower, upper = ordered[lo-1], ordered[hi-1]
        # log order is ratio order; geometric central median avoids rounding in decisions.
        median_ratio = math.sqrt(float(ordered[29] * ordered[30]))
        logs = [math.log(x.numerator)-math.log(x.denominator) for x in ratios]
        acf = [autocorrelation(logs, lag) for lag in range(1, 6)]
        warnings = []
        if any(abs(x) > 0.30 for x in acf):
            warnings.append("serial_dependence_warning")
        if any(r.get("environment_regime_changed", False) for r in sample):
            warnings.append("environment_regime_changed")
        outcome = INCONCLUSIVE if warnings else decision(lower, upper)
        results[cell] = dict(count=60, median_ratio=median_ratio,
                             simultaneous_interval=dict(lower=rational(lower), upper=rational(upper),
                                                        order_indices_1_based=[lo, hi]),
                             t81_latency_ns=descriptive(a), conventional_latency_ns=descriptive(b),
                             paired_difference_ns=descriptive([x-y for x, y in zip(a, b)]),
                             paired_ratio=descriptive([float(r) for r in ratios]),
                             log_ratio_autocorrelation_lags_1_to_5=acf,
                             warnings=warnings, disposition=outcome)
    verdict = overall({cell: result["disposition"] for cell, result in results.items()})
    return dict(schema="t81.i08.analysis.v1", purpose=expected,
                eligible_for_scientific_claim=not synthetic,
                overall_disposition=verdict, cells=results,
                scope="Matched implementation workflow only; conditional on sampling assumptions; no causal ternary/architecture claim")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--synthetic", action="store_true")
    args = parser.parse_args()
    result = analyze(json.loads(Path(args.input).read_text()), synthetic=args.synthetic)
    with Path(args.out).open("x") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
