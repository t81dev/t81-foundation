"""Exercise checker failures directly and through the verbatim CI threshold step."""

import copy
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest

ROOT = Path(__file__).resolve().parents[2]
CHECKER = ROOT / "scripts/ci/check_simd_regression.py"
WORKFLOW = ROOT / ".github/workflows/ci.yml"
SIZES = (16, 64, 256, 1024, 4096, 16384, 65536)


def workflow_step(name):
    lines = WORKFLOW.read_text().splitlines()
    start = lines.index("      - name: " + name)
    run = next(i for i in range(start + 1, len(lines)) if lines[i] == "        run: |")
    body = []
    for line in lines[run + 1:]:
        if line.strip() and not line.startswith("          "):
            break
        body.append(line)
    return textwrap.dedent("\n".join(body)) + "\n"


def fixture(implementation="AVX2"):
    names = [f"BM_Kernel_TAnd_{impl}/{size}"
             for impl in ("SWAR", implementation) for size in SIZES]
    data = {"benchmarks": [{"name": name, "run_type": "iteration", "iterations": 10,
                             "real_time": 100.0, "cpu_time": 100.0, "time_unit": "ns"}
                            for name in names]}
    return data, "\n".join(names) + "\n"


class SIMDRegressionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.script = workflow_step("Enforce SIMD benchmark non-regression thresholds")
        cls.bash = shutil.which("bash")
        if not cls.bash:
            raise RuntimeError("Bash is required to test the actual workflow pipeline")
        cls.temp = tempfile.TemporaryDirectory(prefix="simd-regression-tests-")
        cls.root = Path(cls.temp.name)
        (cls.root / "scripts/ci").mkdir(parents=True)
        shutil.copyfile(CHECKER, cls.root / "scripts/ci/check_simd_regression.py")
        (cls.root / ".github/logs").mkdir(parents=True)
        (cls.root / "build/benchmarks").mkdir(parents=True)
        cls.bench = cls.root / "bench.json"
        cls.inventory = cls.root / "build/benchmarks/simd-regression-inventory.txt"

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def exercise(self, payload, inventory, expected, script=None):
        if payload is None:
            self.bench.unlink(missing_ok=True)
        else:
            self.bench.write_bytes(payload)
        if inventory is None:
            self.inventory.unlink(missing_ok=True)
        else:
            self.inventory.write_text(inventory)
        env = os.environ.copy()
        for key in ("BASH_ENV", "ENV", "SHELLOPTS"):
            env.pop(key, None)
        env["PYTHONOPTIMIZE"] = str(sys.flags.optimize)
        env["PATH"] = str(Path(sys.executable).parent) + os.pathsep + env.get("PATH", "")
        commands = [("direct", [sys.executable, *(["-O"] if sys.flags.optimize else []),
                                str(CHECKER), str(self.bench), "--inventory", str(self.inventory)])]
        path = self.root / "workflow-step.sh"
        path.write_text(self.script if script is None else script)
        # Deliberately no caller-supplied -o pipefail: the actual workflow body
        # must propagate the checker's failure through tee on its own.
        commands.append(("workflow", [self.bash, "--noprofile", "--norc", "-e", str(path)]))
        for route, command in commands:
            if script is not None and route == "direct":
                continue
            result = subprocess.run(command, cwd=self.root, env=env, capture_output=True,
                                    text=True, timeout=10)
            self.assertEqual(result.returncode, expected,
                             f"{route}: {command}: {result.stdout}\n{result.stderr}")
            self.assertNotIn("Traceback", result.stderr)
            if expected:
                self.assertIn("[SIMD REGRESSION ERROR]", result.stdout + result.stderr)
            elif script is None:
                self.assertIn("Regression check passed:", result.stdout)

    def test_positive_platform_controls(self):
        for implementation in ("AVX2", "NEON"):
            with self.subTest(implementation=implementation):
                data, inventory = fixture(implementation)
                self.exercise(json.dumps(data).encode(), inventory, 0)

    def test_threshold_boundary_and_units(self):
        data, inventory = fixture()
        for row in data["benchmarks"]:
            if "AVX2" in row["name"]:
                row.update(real_time=0.115, cpu_time=0.1, time_unit="us")
        self.exercise(json.dumps(data).encode(), inventory, 0)

    def test_reject_bad_inputs_direct_and_workflow(self):
        data, inventory = fixture()
        cases = [("missing file", None), ("empty file", b""),
                 ("malformed JSON", b'{"benchmarks":'), ("trailing JSON", b'{}{}'),
                 ("invalid UTF-8", b'\xff'), ("null root", b'null'),
                 ("array root", b'[]'), ("missing results", b'{}'),
                 ("empty results", b'{"benchmarks":[]}'),
                 ("wrong results type", b'{"benchmarks":{}}'),
                 ("duplicate JSON key", b'{"benchmarks":[],"benchmarks":[]}')]
        for label, mutation in [
            ("non-object row", lambda d: d["benchmarks"].__setitem__(0, None)),
            ("missing SWAR pair", lambda d: d["benchmarks"].pop(5)),
            ("missing comparator pair", lambda d: d["benchmarks"].pop(-1)),
            ("missing size 65536", lambda d: d.__setitem__("benchmarks", [r for r in d["benchmarks"] if not r["name"].endswith('/65536')])),
            ("missing size 16384", lambda d: d.__setitem__("benchmarks", [r for r in d["benchmarks"] if not r["name"].endswith('/16384')])),
            ("missing small size", lambda d: d.__setitem__("benchmarks", [r for r in d["benchmarks"] if not r["name"].endswith('/16')])),
            ("duplicate result", lambda d: d["benchmarks"].append(d["benchmarks"][0])),
            ("disabled implementation", lambda d: d["benchmarks"][0].__setitem__("name", "BM_Kernel_TAnd_NEON/16")),
            ("nonexistent TOr", lambda d: d["benchmarks"][0].__setitem__("name", "BM_Kernel_TOr_SWAR/16")),
            ("benchmark reported error", lambda d: d["benchmarks"][0].__setitem__("error_occurred", True)),
            ("aggregate result", lambda d: d["benchmarks"][0].__setitem__("run_type", "aggregate")),
            ("missing real timing", lambda d: d["benchmarks"][0].pop("real_time")),
            ("missing CPU timing", lambda d: d["benchmarks"][0].pop("cpu_time")),
            ("invalid unit", lambda d: d["benchmarks"][0].__setitem__("time_unit", [])),
            ("zero iterations", lambda d: d["benchmarks"][0].__setitem__("iterations", 0)),
            ("threshold violation", lambda d: d["benchmarks"][-1].__setitem__("real_time", 116)),
        ]:
            changed = copy.deepcopy(data)
            mutation(changed)
            cases.append((label, json.dumps(changed).encode()))
        for field in ("real_time", "cpu_time"):
            for value in (None, True, "100", 0, -1, float("nan"), float("inf"), float("-inf"), 10**400):
                changed = copy.deepcopy(data)
                changed["benchmarks"][0][field] = value
                cases.append((f"invalid {field}: {value!r}", json.dumps(changed).encode()))
        cases.append(("float overflow", json.dumps(data).replace('100.0', '1e999', 1).encode()))
        for label, payload in cases:
            with self.subTest(case=label):
                self.exercise(payload, inventory, 1)
        print(f"Verified {len(cases)} rejection cases directly and through the actual workflow threshold pipeline.")

    def test_reject_incomplete_compiled_inventory(self):
        data, inventory = fixture()
        payload = json.dumps(data).encode()
        cases = [("missing inventory", None), ("empty inventory", ""),
                 ("no baseline", "\n".join(n for n in inventory.splitlines() if "SWAR" not in n)),
                 ("no enabled comparator", "\n".join(n for n in inventory.splitlines() if "AVX2" not in n)),
                 ("missing enabled size", "\n".join(inventory.splitlines()[:-1])),
                 ("duplicate registration", inventory + inventory.splitlines()[0] + "\n"),
                 ("unknown registration", inventory + "BM_Kernel_TNot_SWAR/16\n")]
        for label, expected in cases:
            with self.subTest(case=label):
                self.exercise(payload, expected, 1)
        print(f"Verified {len(cases)} inventory rejection cases through both routes.")

    def test_pipefail_negative_control(self):
        data, inventory = fixture()
        data["benchmarks"][-1]["real_time"] = 200
        self.assertIn("set -euo pipefail", self.script)
        weakened = self.script.replace("set -euo pipefail", "set -eu", 1)
        # Removing only pipefail recreates the false-success pipeline. The
        # actual workflow version is required to reject this same payload.
        self.exercise(json.dumps(data).encode(), inventory, 0, script=weakened)
        self.exercise(json.dumps(data).encode(), inventory, 1)


if __name__ == "__main__":
    unittest.main(verbosity=2)
