# Deterministic execution and provenance — Phase 1

An opt-in Candidate 3 experiment using the production interpreter, Axion policy
engine, persistent CanonFS driver and SHA3-512/256 hashing. No ISA or public API
changes. [Audit, observations and limits](REPORT.md).

From the repository root (C++23 compiler, CMake, Ninja and Python >=3.10):

```sh
cmake -S . -B /tmp/t81-provenance-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_PROVENANCE_EXPERIMENT=ON \
  -DT81_BUILD_TESTS=ON -DT81_STRICT_DETERMINISTIC_FLOAT=ON \
  -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_EXAMPLES=OFF \
  -DT81_BUILD_TUI=OFF -DT81_ENABLE_TERNARYOS=OFF -DT81_BUILD_FUZZ_TESTS=OFF
cmake --build /tmp/t81-provenance-build --target t81_provenance_experiment -j 4
ctest --test-dir /tmp/t81-provenance-build --output-on-failure \
  -R '^deterministic_provenance_experiment$'
python3 experiments/deterministic_provenance/run.py \
  --runner /tmp/t81-provenance-build/t81_provenance_experiment \
  --output /tmp/t81-provenance-results --repetitions 30
```

Use a fresh output directory. CMake fetches the repo's pinned Asio test dependency;
an offline build can set `FETCHCONTENT_SOURCE_DIR_ASIO` to an existing
`asio-1-30-2` checkout. No Python packages or models are required.

Independent replay verification, without loading T81 or importing producer code:

```sh
python3 experiments/deterministic_provenance/verify.py \
  experiments/deterministic_provenance/fixture.json \
  --input 40 --divisor 2 --profile allow
python3 experiments/deterministic_provenance/verify.py \
  /tmp/t81-provenance-results/allow-0/evidence.json \
  --input 40 --divisor 2 --profile allow \
  --store /tmp/t81-provenance-results/allow-0/store
```

The caller pins input and profile. The first command verifies the detached
fixture by recomputing this admitted transformation and its exact execution
sequence. The second additionally checks the actual CanonFS file inventory and
payload bytes. Copy `verify.py` and `fixture.json` to another directory to verify
without the repository or producer executable.

`run.py` executes eight cases in rotating order: allowed, changed input, VM deny,
storage deny, division by zero, input bounds and an unconfigured-policy baseline.
Every case must repeat with exactly identical canonical bytes. The default is
30 samples per case, with five interpreter/hash warm-ups within each process.
`report.json` contains source SHA/digests, toolchain/platform, raw samples,
mean/median/stdev/min/max, verifier outcomes and explicit exclusions. The
checked-in `observed-arm64.json` preserves one native run.

Canonical evidence is sorted compact ASCII JSON (`ensure_ascii=True`), with
integers and strings; SHA3-512 truncated to the first 32 bytes. The experiment's
program/trace text uses decimal integers, opcode names and LF, never C++ object
memory or host endianness. CanonFS references retain production Base81 encoding.
`record_hash` covers the complete record, including program/input/policy/output
hashes, actual ordered trace and policy audit, content reference and numerical
probes. Platform, compiler, Git SHA, timings, memory and file paths live only in
the measurement report. No timestamps enter canonical evidence.

A changed arithmetic input can preserve the control-flow trace hash while
changing output and complete evidence hashes. Hashing only PC/opcode/trap is
insufficient to bind execution to inputs; the complete record provides that link.

The evidence is unsigned. Verification establishes agreement with the pinned
bounded computation, not producer identity or absence of unreported host writes.
The protected storage side effect is creation of the result `.blk` payload;
constructing a denied persistent store may create empty administrative directories.
This experiment does not claim transactionality or concurrent revocation.
