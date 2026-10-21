"""Run explicit schedules and save descriptive costs and reproduction metadata."""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import statistics
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--runner", required=True, type=Path)
parser.add_argument("--output", required=True, type=Path)
parser.add_argument("--repetitions", default=30, type=int)
args = parser.parse_args()
runner, output = args.runner.resolve(), args.output.resolve()
subprocess.run([str(runner), str(output), str(args.repetitions)], check=True, timeout=300)
checked = subprocess.check_output([sys.executable, str(Path(__file__).with_name("verify_concurrent.py")),
                                   str(output / "results.json"), "--artifacts", str(output)], text=True)
data = json.loads((output / "results.json").read_text())
metadata = dict(verification=json.loads(checked), environment=platform.platform(), machine=platform.machine(),
                python=sys.version, compiler=subprocess.check_output(["c++", "--version"], text=True).splitlines()[0],
                runner_sha256=hashlib.sha256(runner.read_bytes()).hexdigest(),
                source_sha256={name: hashlib.sha256(Path(__file__).with_name(name).read_bytes()).hexdigest()
                               for name in ("concurrent_revocation.cpp", "verify_concurrent.py", "measure_concurrent.py")},
                performance_scope="Descriptive writer interval including scheduled waits; not compute or durability rankings",
                costs=[])
for row in data["rows"]:
    samples = row["writer_us"]
    metadata["costs"].append(dict(backend=row["backend"], mode=row["mode"], scenario=row["scenario"],
                                   median_us=statistics.median(samples), min_us=min(samples), max_us=max(samples),
                                   stdev_us=statistics.stdev(samples) if len(samples) > 1 else 0))
(output / "summary.json").write_text(json.dumps(metadata, indent=2) + "\n")
print(checked)
