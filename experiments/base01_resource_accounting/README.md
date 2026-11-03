# BASE-01 resource accounting supplement

This standalone component completes the real-OS collector omitted from the
[accepted #691 foundation](https://github.com/t81dev/t81-foundation/tree/db0e7c8b976991396eed7d14ddc1e889e9c511c2/experiments/base01_b1_foundation).
It runs fixed resource-collector correctness fixtures only. It does not execute
T81 workloads, implement a performance dispatcher, or authorize B1 measurement.
Jules' integration work is outside this change.

The audited T81 source remains `cda9135d36144d4de2771af174e52ac7b30a83c9`.
The accepted foundation remains `db0e7c8b976991396eed7d14ddc1e889e9c511c2`.
These are historical context pins, not claims that this supplement executes
T81 at those revisions. This branch starts at the audited revision and adds
only this directory. It can be reviewed and integrated additively; it neither
copies nor requires changes to #691, Phase A/B0, VM code, frozen artifacts,
root CMake, or GitHub Actions. Integration order remains the maintainer's choice.

## Reproduce correctness evidence

Requirements: Linux or macOS, Python 3.12+ with `os.posix_spawn`, `os.wait4`,
`resource.getrusage`, and a C11 compiler. Windows and other operating systems
fail closed before compilation or fixtures and produce an unsupported record.
Unsupported-platform tests executed on supported hosts are synthetic contract
tests; no Windows execution is claimed.

From the repository root:

```sh
python3 experiments/base01_resource_accounting/validate.py \
  --validate-only --build-dir /tmp/base01-resource-build \
  --output /tmp/base01-resource-validation.json
```

The explicit compile flags are `-std=c11 -O2 -Wall -Wextra -Werror`.
`--cc gcc` selects a compiler; no root build or T81 executable is needed.
`build.json` records the exact command, compiler identification, source hash,
and executable hash. The collector verifies these source/executable identities
before launching each fixture. The supplied compiler, build manifest, local
source and filesystem are trusted inputs, not an adversarial sandbox.

The CLI has only validation/build/output/compiler options. All unknown options,
including measurement, timing, warmup, pilot, trial, campaign, benchmark and
permit requests, fail during argument parsing before compiler or fixture
execution. Environment variables cannot enable additional modes. The native
fixture accepts exactly `success`, `exit7`, and `signal`; each performs the same
bounded correctness operation and can never select a workload. There is no
timer, campaign, PMU, SPICE, CMP3, H04-R03 Phase C or signed-permit activation
interface. No automatic workflow is added. Existing #688/#691 entry points
are untouched.

`validate.py` drives unittest results directly and replaces unittest's internal
duration clock with a constant sentinel. No durations are collected, retained
or used for acceptance. Negative CLI tests confirm rejection even with
`BASE01_MEASUREMENT_AUTHORIZED=true` and `BASE01_MEASURE=1`.

See [SEMANTICS.md](SEMANTICS.md) for attribution and limitations,
[READINESS.md](READINESS.md) for execution scope, and [evidence](evidence/)
for retained observations and identities. These are implementation-author
correctness checks, not independent scientific validation.
