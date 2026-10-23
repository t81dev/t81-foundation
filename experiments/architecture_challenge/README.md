# Independent architecture challenge

A bounded, opt-in comparison on `main` after #553. Arm A runs the existing T81
interpreter, Axion final-authorization hook and persistent CanonFS. Arm B runs
Wasmtime with one host `submit` import and SHA-256 addressed ordinary files.
No production execution path, ISA, numeric contract or policy language changes.
The [external contract](workload-contract.json) and [decision report](REPORT.md)
define exactly what is measured and what remains unproved.

From the repository root, with C++23, CMake, Ninja and Python >=3.10:

```sh
python3 -m venv /tmp/t81-challenge-venv
/tmp/t81-challenge-venv/bin/python -m pip install -r experiments/architecture_challenge/requirements.txt
cmake -S . -B /tmp/t81-challenge-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DT81_BUILD_ARCHITECTURE_CHALLENGE=ON -DT81_STRICT_DETERMINISTIC_FLOAT=ON \
  -DT81_BUILD_TESTS=OFF -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_EXAMPLES=OFF \
  -DT81_BUILD_TUI=OFF -DT81_ENABLE_TERNARYOS=OFF -DT81_BUILD_FUZZ_TESTS=OFF
cmake --build /tmp/t81-challenge-build --target t81_architecture_arm_a -j 4
/tmp/t81-challenge-venv/bin/python experiments/architecture_challenge/test_challenge.py \
  --runner /tmp/t81-challenge-build/t81_architecture_arm_a
/tmp/t81-challenge-venv/bin/python experiments/architecture_challenge/run.py \
  --runner /tmp/t81-challenge-build/t81_architecture_arm_a \
  --output /tmp/t81-challenge-results --repetitions 30
/tmp/t81-challenge-venv/bin/python experiments/architecture_challenge/attacks.py \
  --output /tmp/t81-challenge-attacks
/tmp/t81-challenge-venv/bin/python experiments/architecture_challenge/count.py \
  --build /tmp/t81-challenge-build --output /tmp/t81-challenge-results/counts.json
```

Use fresh output directories. On Windows use a VS developer shell, `python` in a
venv and the `.exe` runner; the workflow supplies that environment. Tests are run
directly here so the build needs no unrelated CTest/Asio dependencies. This is not
a passing full production suite claim. `raw.jsonl` preserves stdout/stderr/exit
status before parsing, including the intentional exit-75 interruptions. Every
trial retains protected storage, native trace, independent verifier stdout/stderr,
evidence and its native archive. `corpus.jsonl` additionally preserves every file's
bytes in hex, so it can be reviewed without the original directories. The two
`snapshot-control` cases deliberately issue an unauthorized write and **must be
rejected**, rather than counted as successful authorization.

Copy only `verify.py`, `audit_corpus.py` and the artifact to another machine to
verify without T81, Wasmtime, the repository or network:

```sh
python3 experiments/architecture_challenge/verify.py \
  /tmp/t81-challenge-results/approved-A-0/evidence.json \
  --case approved --arm A --store /tmp/t81-challenge-results/approved-A-0/store
python3 experiments/architecture_challenge/audit_corpus.py \
  /tmp/t81-challenge-results/corpus.jsonl
```

The caller pins the case out of band. Detached verification checks bounded
arithmetic, the authorization schedule and evidence integrity. A live store
inspection adds an observation under the observer's trust boundary. A copied,
unsigned inventory does not authenticate occurrence. Neither command establishes
producer identity, event freshness, source inventory truth or absence of unreported
host writes. `attacks.py` preserves accepted fabrications/replays to demonstrate
this distinction. Native storage uses different hash/filename algorithms; the
verifier independently checks both without comparing internal trace bytes.

The [preserved native corpus and report](observed-macos-arm64/report.json) and
[adversarial evidence](observed-macos-arm64/attacks.json) are committed.
Unittest execution retains its real-arm output directory; `--output-root PATH`
chooses its location. CI packages this directory and all measurement outputs in
a tarball so native CanonFS filenames survive artifact-service restrictions.

The machine matrix is a separate experiment workflow for Linux x86_64, macOS
ARM64 and Windows x86_64. It uploads failures and raw files using `always()`.
Workflow definitions alone do not establish portability or cross-platform byte
identity; inspect each native artifact. No OPA/database third arm is included.

The committed native CI archives can also be audited without extraction (including
on filesystems that cannot represent CanonFS object names):

```sh
python3 experiments/architecture_challenge/audit_archives.py \
  experiments/architecture_challenge/observed-ci/linux.tar.gz \
  experiments/architecture_challenge/observed-ci/macos.tar.gz
```

This checks the actual tar member payloads against every recorded protected and
evidence file, then compares all protocol envelopes. It requires only the three
independent `verify.py`/`audit*.py` scripts and the archives.
