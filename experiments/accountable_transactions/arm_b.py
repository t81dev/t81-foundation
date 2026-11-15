"""Untrusted Wasm calculator; named submit capability, no WASI."""
import json
import sys
from time import perf_counter_ns
import wasmtime

WAT = '''(module
(import "host" "submit" (func $submit (param i32) (result i32)))
(func (export "run") (param i32 i32) (result i32)
 local.get 0 local.get 1 i32.mul call $submit))'''


class Calculator:
    def __init__(self):
        config = wasmtime.Config()
        config.consume_fuel = True
        self.engine = wasmtime.Engine(config)
        self.module = wasmtime.Module(self.engine, WAT)

    def run(self, units, price):
        if not (type(units) is int and type(price) is int
                and 1 <= units <= 1000 and 1 <= price <= 1000):
            raise ValueError('bounded inputs required')
        store = wasmtime.Store(self.engine)
        store.set_fuel(100)
        proposed = []
        def submit(amount):
            proposed.append(amount)
            return amount
        function = wasmtime.Func(store, wasmtime.FuncType(
            [wasmtime.ValType.i32()], [wasmtime.ValType.i32()]), submit)
        linker = wasmtime.Linker(self.engine)
        linker.define(store, 'host', 'submit', function)
        instance = linker.instantiate(store, self.module)
        start = perf_counter_ns()
        instance.exports(store)['run'](store, units, price)
        return dict(amount=proposed[0], execution_ns=perf_counter_ns() - start)


if __name__ == '__main__':
    print(json.dumps(Calculator().run(*(int(x) for x in sys.argv[1:]))))
