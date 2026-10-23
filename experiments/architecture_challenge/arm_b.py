"""Wasm gets exactly one host capability; no WASI, filesystem or network imports."""
import json
import os
from pathlib import Path
import re
import sys
from time import perf_counter_ns as now
import hashlib
import wasmtime


def run(scenario, input_path, root, guest_path=None):
    if scenario not in ('approved', 'denied', 'malformed', 'malformed-policy', 'replayed',
                         'interrupted-before', 'interrupted-after', 'revoke-after-compute',
                         'revoke-after-check', 'revoke-after-commit', 'snapshot-control'):
        raise ValueError('unknown case')
    begin = now()
    root = Path(root)
    root.mkdir(parents=True, exist_ok=False)
    raw = Path(input_path).read_bytes()
    match = re.fullmatch(rb'([1-9][0-9]{0,3}) ([1-9][0-9]{0,3})\n', raw)
    valid = match is not None and all(1 <= int(v) <= 1000 for v in match.groups())
    amount, limit = 0, 10000 if scenario == 'denied' else 50000
    consumed = False
    status = 'malformed'
    events = []
    execution = authorization = storage = callback_ns = 0
    def revoke():
        nonlocal limit
        limit = 10000
        events.append('policy.reduce.10000')
    def submit(proposed):
        nonlocal amount, consumed, authorization, storage, callback_ns, status, limit
        entry = now()
        amount = proposed
        events.append('compute')
        if scenario == 'revoke-after-compute':
            revoke()
        if scenario in ('revoke-after-check', 'snapshot-control'):
            events.append('precheck.allow' if amount <= limit else 'precheck.deny')
            revoke()
        if scenario == 'interrupted-before':
            events.append('interrupt.before')
            status = 'interrupted'
        else:
            def attempt():
                nonlocal consumed, authorization, storage
                start = now()
                active = 50000 if scenario == 'snapshot-control' else limit
                allow = valid and not consumed and amount == int(match[1]) * int(match[2]) and 0 < amount <= active and scenario != 'malformed-policy'
                events.append('authorize.allow' if allow else 'authorize.deny')
                authorization += now() - start
                if allow:
                    start = now()
                    payload = f'8491 {amount}\n'.encode('ascii')
                    (root / hashlib.sha256(payload).hexdigest()).write_bytes(payload)
                    storage += now() - start
                    consumed = True
                    events.append('write.commit')
                return allow
            status = 'committed' if attempt() else 'denied'
            if scenario == 'revoke-after-commit':
                revoke()
            if scenario == 'replayed':
                attempt()
            if scenario == 'interrupted-after':
                events.append('interrupt.after')
                status = 'interrupted'
        callback_ns += now() - entry
        return int(consumed)
    if valid:
        config = wasmtime.Config()
        config.consume_fuel = True
        engine = wasmtime.Engine(config)
        module = wasmtime.Module.from_file(engine, str(guest_path or Path(__file__).with_name('guest.wat')))
        store = wasmtime.Store(engine)
        store.set_fuel(100)
        function = wasmtime.Func(store, wasmtime.FuncType([wasmtime.ValType.i32()], [wasmtime.ValType.i32()]), submit)
        linker = wasmtime.Linker(engine)
        linker.define(store, 'host', 'submit', function)
        instance = linker.instantiate(store, module)
        setup = now() - begin
        start = now()
        instance.exports(store)['run'](store, *(int(v) for v in match.groups()))
        execution = now() - start - callback_ns
    else:
        events.append('input.reject')
        setup = now() - begin
    return dict(status=status, amount=amount, limit=limit, events=events,
                native_trace='Wasm i32.mul -> host.submit; fuel bounded; no internal trace enabled',
                measurements=dict(setup_ns=setup, execution_ns=execution,
                                  authorization_ns=authorization, storage_ns=storage))

if __name__ == '__main__':
    print(json.dumps(run(*sys.argv[1:])), flush=True)
    if sys.argv[1] in ('interrupted-before', 'interrupted-after'):
        os._exit(75)  # No cleanup/unwinding; recovery evidence belongs to the observer.
