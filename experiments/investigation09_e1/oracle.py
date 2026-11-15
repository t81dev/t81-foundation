"""Independent mathematical oracle; imports no code from either implementation."""
from itertools import product

D = (-1, 0, 1)


def cases(module):
    if module == "combined":
        return sum((cases(name) for name in ("neg", "meet", "join", "decision")), [])
    if module == "neg":
        return [(0, a, 0, 0) for a in D]
    if module in ("meet", "join"):
        k = 1 if module == "meet" else 2
        return [(k, a, b, 0) for a, b in product(D, repeat=2)]
    if module == "decision":
        return [(3, a, b, c) for a, b, c in product(D, repeat=3)]
    if module == "boolean":
        return [(k, a, b, c) for k, a, b, c in product((0, 1), (0, 1), (0, 1), (0, 1))]
    if module == "integer":
        return [(k, a, b, 0) for k, a, b in product((0, 1), range(-3, 4), range(-3, 4))]
    if module == "identity":
        return [(0, a, 0, 0) for a in D]
    raise ValueError(module)


def expected(module, case):
    k, a, b, c = case
    if module == "boolean":
        need_rhs = bool(a) if k == 0 else not bool(a)
        if need_rhs and c == 0:
            return {"fault": "division_by_zero", "values": [], "rhs_evaluated": True}
        rhs = (1 // c == b) if need_rhs else None
        v = (bool(a) and rhs) if k == 0 else (bool(a) or rhs)
        return {"fault": None, "values": [int(v)], "result": int(v), "rhs_evaluated": need_rhs}
    if module == "integer":
        v = a + b if k == 0 else int(a < b)
        return {"fault": None, "values": [v], "result": v}
    if module == "identity":
        return {"fault": None, "values": [a], "result": a}
    if k == 0:
        values = [-a]
    elif k == 1:
        values = [min(a, b)]
    elif k == 2:
        values = [max(a, b)]
    else:
        m, n = min(a, b), -c
        values = [m, n, max(m, n)]
    packed = 0
    for value in values:
        packed = packed * 3 + value + 1
    return {"fault": None, "values": values, "result": packed}


def decode(module, case, value):
    if module in ("boolean", "integer", "identity"):
        return [value]
    n = 3 if case[0] == 3 else 1
    values = []
    for _ in range(n):
        values.append(value % 3 - 1)
        value //= 3
    if value:
        raise ValueError("noncanonical packed output")
    return values[::-1]
