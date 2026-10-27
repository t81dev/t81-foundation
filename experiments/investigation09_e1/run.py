#!/usr/bin/env python3
"""Run the preregistered E1 gate. Never edits production implementation files."""
import argparse
import ctypes
import hashlib
import importlib.metadata
import itertools
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tempfile

from oracle import cases, decode, expected

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
MODULES = ("combined", "neg", "meet", "join", "decision", "boolean", "integer", "identity")
STYLES = ("branch", "select", "table")
LEVELS = ("O2", "O3", "Oz")
PROTOCOL = "f1054438b805b541c4acfd294e6dba5a71711c05"


def dump(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def t81_source(module):
    if module == "boolean":
        declarations = "let k: i32 = 0;\nlet a: bool = false;\nlet b: i32 = 0;\nlet c: i32 = 0;\n"
        fn = "fn run(k: i32, a: bool, b: i32, c: i32) -> i32 {\n"
        # Division is deliberate: its fault makes skipped RHS observable.
        fn += "if k == 0 { if a && ((1 / c) == b) { return 1; } return 0; }\nif a || ((1 / c) == b) { return 1; } return 0;\n}"
    elif module in ("integer", "identity"):
        declarations = "let k: i32 = 0;\nlet a: i32 = 0;\nlet b: i32 = 0;\nlet c: i32 = 0;\n"
        fn = "fn run(k: i32, a: i32, b: i32, c: i32) -> i32 {\n"
        fn += ("return a;\n}" if module == "identity" else
               "if k == 0 { return a + b; }\nif a < b { return 1; } return 0;\n}")
    else:
        declarations = "let k: i32 = 0;\nlet a: T81Qutrit = 0t;\nlet b: T81Qutrit = 0t;\nlet c: T81Qutrit = 0t;\n"
        fn = "fn run(k: i32, a: T81Qutrit, b: T81Qutrit, c: T81Qutrit) -> i32 {\n"
        bodies = {
            "neg": "return ((-a) as i32) + 1;",
            "meet": "return ((a & b) as i32) + 1;",
            "join": "return ((a | b) as i32) + 1;",
            "decision": "let m: T81Qutrit = a & b;\nlet n: T81Qutrit = -c;\nlet r: T81Qutrit = m | n;\nreturn ((m as i32) + 1) * 9 + ((n as i32) + 1) * 3 + (r as i32) + 1;",
        }
        if module == "combined":
            for k, name in enumerate(("neg", "meet", "join")):
                fn += f"if k == {k} {{ {bodies[name]} }}\n"
            fn += bodies["decision"]
        else:
            fn += bodies[module]
        fn += "\n}"
    return declarations + "let result = run(k, a, b, c);\n" + fn + "\n"


def c_source(module, style):
    # All tables enumerate the complete declared domain, without workload sampling.
    preamble = "typedef long long V;\n"
    if style == "table":
        lo = [min(a, b) for a, b in itertools.product((-1, 0, 1), repeat=2)]
        hi = [max(a, b) for a, b in itertools.product((-1, 0, 1), repeat=2)]
        preamble += "static const V lows[9]={" + ",".join(map(str, lo)) + "};\n"
        preamble += "static const V highs[9]={" + ",".join(map(str, hi)) + "};\n"
        preamble += "static const V negs[3]={1,0,-1};\n"
        preamble += "static V lo(V a,V b){return lows[(a+1)*3+b+1];}\nstatic V hi(V a,V b){return highs[(a+1)*3+b+1];}\nstatic V neg(V a){return negs[a+1];}\n"
    elif style == "branch":
        preamble += "static V lo(V a,V b){if(a<b)return a;return b;}\nstatic V hi(V a,V b){if(a>b)return a;return b;}\nstatic V neg(V a){return -a;}\n"
    else:
        preamble += "static V lo(V a,V b){return a<b?a:b;}\nstatic V hi(V a,V b){return a>b?a:b;}\nstatic V neg(V a){return -a;}\n"
    bodies = {
        "neg": "return neg(a)+1;",
        "meet": "return lo(a,b)+1;",
        "join": "return hi(a,b)+1;",
        "decision": "V m=lo(a,b),n=neg(c),r=hi(m,n);return 9*(m+1)+3*(n+1)+r+1;",
        "integer": "if(k==0)return a+b;return a<b;",
        "identity": "return a;",
        # Explicit check prevents C undefined behavior on zero division and
        # retains the required fault only when the short-circuit RHS is needed.
        "boolean": "if(k==0){if(!a)return 0;if(!c)__builtin_trap();return (1/c)==b;}if(a)return 1;if(!c)__builtin_trap();return (1/c)==b;",
    }
    body = bodies.get(module)
    if module == "combined":
        body = "".join(f"if(k=={k}){{{bodies[name]}}}" for k, name in enumerate(("neg", "meet", "join"))) + bodies["decision"]
    return preamble + "__attribute__((visibility(\"default\"))) V run(V k,V a,V b,V c){" + body + "}\n"


def wasm_worker(artifact, matrix):
    import wasmtime
    engine = wasmtime.Engine()
    module = wasmtime.Module.from_file(engine, artifact)
    if list(module.imports):
        raise RuntimeError("unexpected Wasm imports")
    rows = []
    for case in matrix:
        store = wasmtime.Store(engine)
        instance = wasmtime.Instance(store, module, [])
        try:
            value = instance.exports(store)["run"](store, *case)
            rows.append({"inputs": case, "fault": None, "result": value})
        except wasmtime.Trap as exc:
            rows.append({"inputs": case, "fault": "trap", "message": str(exc)})
    print(json.dumps({"rows": rows, "imports": [], "exports": [e.name for e in module.exports]}))


def checked_input(module, case):
    if len(case) != 4 or any(type(v) is not int for v in case):
        raise ValueError("four integer inputs required")
    k, a, b, c = case
    allowed = {"combined": (0,1,2,3), "neg": (0,), "meet": (1,), "join": (2,),
               "decision": (3,), "boolean": (0,1), "integer": (0,1), "identity": (0,)}
    if k not in allowed[module]: raise ValueError("selector outside entry contract")
    if module == "boolean":
        valid = a in (0,1) and b in (0,1) and c in (0,1)
    elif module == "integer":
        valid = -3 <= a <= 3 and -3 <= b <= 3 and c == 0
    else:
        valid = all(v in (-1,0,1) for v in (a,b,c))
    if not valid: raise ValueError("input outside typed domain")
    return case


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--driver", type=Path, required=True)
    ap.add_argument("--wasm-opt", type=Path, required=True)
    ap.add_argument("--clang", default=shutil.which("clang"))
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    args.driver = args.driver.resolve()
    args.wasm_opt = args.wasm_opt.resolve()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    sources = out / "sources"
    artifacts = out / "artifacts"
    logs = out / "logs"
    for p in (sources, artifacts, logs): p.mkdir()
    commands, rows, inventory = [], [], []

    def command(argv, *, input=None, env=None):
        cp = subprocess.run(list(map(str, argv)), input=input, text=True, capture_output=True, env=env)
        idx = len(commands)
        commands.append({"argv": list(map(str, argv)), "returncode": cp.returncode,
                         "stdin": input, "stdout": cp.stdout, "stderr": cp.stderr})
        dump(logs / f"command-{idx:04d}.json", commands[-1])
        if cp.returncode:
            raise RuntimeError(f"command {idx} failed: {cp.stderr}")
        return cp.stdout

    def gate(module, matrix, observations, label):
        for case, observed in zip(matrix, observations, strict=True):
            oracle = expected(module, case)
            row = {"module": module, "implementation": label, "inputs": case,
                   "oracle": oracle, "observed": observed}
            rows.append(row)
            if oracle["fault"]:
                if not observed["fault"]: raise RuntimeError(f"missing expected fault: {row}")
            elif observed["fault"] or not observed.get("halted", True) or observed["result"] != oracle["result"]:
                raise RuntimeError(f"semantic mismatch: {row}")
            elif decode(module, case, observed["result"]) != oracle["values"]:
                raise RuntimeError(f"intermediate mismatch: {row}")
            if observed.get("printed_effects", 0): raise RuntimeError(f"unexpected effect: {row}")

    manifest = {"protocol_commit": PROTOCOL, "source_baseline": "c930b8c2ee7478782bf3c6d231bdd59984595ef2",
                "checkout": command(["git", "rev-parse", "HEAD"]).strip(), "platform": platform.platform(),
                "python": sys.version, "machine": platform.machine(), "wasmtime": importlib.metadata.version("wasmtime"),
                "clang": command([args.clang, "--version"]), "binaryen": command([args.wasm_opt, "--version"]),
                "linker": command([shutil.which("wasm-ld"), "--version"]),
                "entry_contract_sha256": sha(HERE / "ENTRY_CONTRACT.md"),
                "protocol_sha256": sha(REPO / "docs/research/INVESTIGATION_09_TISC_T81LANG_ADVANTAGE_PROPOSAL.md")}
    dump(out / "manifest.json", manifest)
    try:
        boundary = []
        for case in ((0,-2,0,0),(0,2,0,0),(0,-2147483648,0,0),(0,2147483647,0,0),(4,0,0,0)):
            try:
                checked_input("combined", case)
            except ValueError as exc:
                boundary.append({"inputs": case, "tisc": "rejected_before_entry", "wasm": "rejected_before_entry", "reason": str(exc)})
            else:
                raise RuntimeError("invalid boundary input accepted")
        dump(out / "boundary-rejections.json", boundary)
        for module in MODULES:
            matrix = [checked_input(module, case) for case in cases(module)]
            dump(sources / f"{module}-matrix.json", [{"inputs": c, "expected": expected(module, c)} for c in matrix])
            src = sources / f"{module}.t81"
            src.write_text(t81_source(module))
            tisc = artifacts / f"{module}.tisc"
            compile_info = json.loads(command([args.driver, "compile", src, tisc]))
            # Fresh process and empty artifact store per VM invocation. Never
            # preserve a VM state or override program bytes between cases.
            trows = []
            for case in matrix:
                with tempfile.TemporaryDirectory(prefix="t81-i09-case-") as store:
                    assert not list(Path(store).iterdir())
                    env = dict(os.environ, T81_CANONFS_ROOT=store)
                    text = command([args.driver, "run", tisc], input=" ".join(map(str, case)) + "\n", env=env)
                    record = json.loads(text)
                    trows.append(record)
                    gate(module, [case], [record], "compiler-tisc")
            inventory.append({"module": module, "kind": "tisc", "path": str(tisc.relative_to(out)), **compile_info})
            # Native optimized C is an additional value control. Expected trap
            # cases are isolated as subprocesses below, never invoked in ctypes.
            for style in STYLES:
                csrc = sources / f"{module}-{style}.c"
                csrc.write_text(c_source(module, style))
                native = artifacts / f"{module}-{style}.dylib"
                command([args.clang, "-O3", "-dynamiclib" if sys.platform == "darwin" else "-shared", "-fPIC", csrc, "-o", native])
                native_rows = json.loads(command([sys.executable, __file__, "native-worker", native, json.dumps(matrix), module]))
                gate(module, matrix, native_rows, f"native-{style}-O3")
                for level in LEVELS:
                    wasm = artifacts / f"{module}-{style}-{level}.wasm"
                    command([args.clang, "--target=wasm32", "-nostdlib", f"-{level}",
                             "-Wl,--no-entry", "-Wl,--export=run", "-Wl,--strip-all", csrc, "-o", wasm])
                    for optimized in (False, True):
                        target = wasm
                        if optimized:
                            target = wasm.with_name(wasm.stem + "-binaryen-Oz.wasm")
                            command([args.wasm_opt, wasm, "-Oz", "--strip-debug", "-o", target])
                        measured = json.loads(command([sys.executable, __file__, "wasm-worker", target, json.dumps(matrix)]))
                        gate(module, matrix, measured["rows"], target.name)
                        inventory.append({"module": module, "kind": "wasm", "path": str(target.relative_to(out)),
                                          "style": style, "level": level, "binaryen_Oz": optimized, "exports": measured["exports"]})
        for kernel in ("meet", "join"):
            for variant in ("native", "expanded"):
                target = artifacts / f"authored-{kernel}-{variant}.tisc"
                info = json.loads(command([args.driver, "author", target, kernel, variant]))
                matrix = cases(kernel)
                result = []
                for case in matrix:
                    with tempfile.TemporaryDirectory(prefix="t81-i09-authored-") as store:
                        env = dict(os.environ, T81_CANONFS_ROOT=store)
                        result.append(json.loads(command([args.driver, "run", target], input=" ".join(map(str, case))+"\n", env=env)))
                gate(kernel, matrix, result, target.name)
                inventory.append({"module": kernel, "kind": "authored-tisc", "variant": variant,
                                  "path": str(target.relative_to(out)), **info})
        # Cost observations are collected only after the entire parity gate.
        for entry in inventory:
            path = out / entry["path"]
            entry.update(bytes=path.stat().st_size, sha256=sha(path))
        suite = [e for e in inventory if e["module"] == "combined"]
        tisc = next(e for e in suite if e["kind"] == "tisc")
        baseline = min((e for e in suite if e["kind"] == "wasm"), key=lambda e:e["bytes"])
        ratio = tisc["bytes"] / baseline["bytes"]
        disposition = {"status": "NOT SUPPORTED" if ratio > .95 else "LIVE_STATE_GATE_PENDING", "ratio": ratio,
                       "tisc_bytes": tisc["bytes"], "smallest_wasm_bytes": baseline["bytes"], "baseline": baseline["path"],
                       "parity": "PASS", "threshold": .95, "timing": "not measured"}
    except Exception as exc:
        disposition = {"status": "INCONCLUSIVE", "parity": "FAIL_OR_INELIGIBLE", "reason": str(exc),
                       "footprint_comparison": "not performed", "timing": "not measured"}
    dump(out / "comparison.json", {"disposition": disposition, "inventory": inventory})
    dump(out / "raw-parity.json", rows)
    dump(out / "commands.json", commands)
    hashes = {str(p.relative_to(out)): sha(p) for p in sorted(out.rglob("*")) if p.is_file()}
    dump(out / "sha256.json", hashes)
    print(json.dumps(disposition, indent=2))
    return 0 if disposition["parity"] == "PASS" else 2


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "wasm-worker":
        wasm_worker(sys.argv[2], json.loads(sys.argv[3]))
    elif len(sys.argv) > 1 and sys.argv[1] == "native-trap-worker":
        lib = ctypes.CDLL(sys.argv[2])
        fn = lib.run
        fn.argtypes = [ctypes.c_longlong] * 4
        fn.restype = ctypes.c_longlong
        fn(*json.loads(sys.argv[3]))
    elif len(sys.argv) > 1 and sys.argv[1] == "native-worker":
        module, matrix, name = sys.argv[2], json.loads(sys.argv[3]), sys.argv[4]
        lib = ctypes.CDLL(module)
        fn = lib.run
        fn.argtypes = [ctypes.c_longlong] * 4
        fn.restype = ctypes.c_longlong
        observations = []
        for case in matrix:
            if expected(name, case)["fault"]:
                cp = subprocess.run([sys.executable, __file__, "native-trap-worker", module, json.dumps(case)], capture_output=True)
                if cp.returncode >= 0:
                    raise RuntimeError("native trap worker did not terminate by signal")
                observations.append({"fault": "native_trap", "signal": -cp.returncode})
            else:
                observations.append({"fault": None, "result": fn(*case)})
        print(json.dumps(observations))
    else:
        raise SystemExit(main())
