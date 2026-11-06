# ARCH-003 correctness implementation draft

**I2: ACCOUNTING VERIFIED for bounded Linux ARM64 fixtures; independent review pending. No correctness campaign or performance authorization.**
This standalone project implements the ARCH-003-P0 source layout against accepted
ARCH-002-R1 protocol commit `1454bd63426d21686cfb5ef22dbaebc3c9cae94b`.
Parent hypothesis: canonical H03, with H05 context. It changes no production
CMake, CI, representation semantics, historical evidence or research governance.

Read [IMPLEMENTATION_REPORT.md](IMPLEMENTATION_REPORT.md) for the validation scope,
accounting closure, retained failures and remaining campaign authorization gates.
[ACCOUNTING_REVIEW.md](ACCOUNTING_REVIEW.md) is the current accounting assessment.
[CHANGED_FILES.txt](CHANGED_FILES.txt) is the exact PR-relative file inventory.

## Bounded development checks

Configure and compile serially; configuration/build never runs tests:

```sh
cmake -S experiments/arch003 -B /tmp/arch003-D \
  -DCMAKE_CXX_COMPILER=/absolute/path/to/clang++ \
  -DARCH003_PROFILE=D -DARCH003_COMPONENT_TESTS=ON
cmake --build /tmp/arch003-D --parallel 1
ctest --test-dir /tmp/arch003-D --output-on-failure
ARCH003_COMPONENT_BIN_DIR=/tmp/arch003-D \
  python3 -B -m unittest discover -s experiments/arch003/tests -p 'test_*.py' -v
```

R selects `-O3 -DNDEBUG`; S selects combined ASan+UBSan exactly as the pinned
protocol requires. For S component checks set:

```sh
export ASAN_OPTIONS=halt_on_error=1:detect_leaks=0:symbolize=0
export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=0:symbolize=0
```

The three CTests inspect representation/admission, SHA-256, wire sizes,
SplitMix64, schedule **metadata only**, and two separate five-symbol oracle
fixtures. They never call a normal count entry or a G1/G2 worker. The Python
suite uses fixed binary/parser fixtures, mocked accounting boundaries and two
five-record oracle wire fixtures. No complete corpus is generated. The S probe
binary is compiled but is never a CTest and has not been executed here.

The oracle and verifier review gates remain human decisions. Both oracles and
V must be accepted before any normal candidate or G1/G2 invocation. Separate
human authorization must name the exact implementation, host, compiler/runtime,
backend and output location before any full campaign. Environment variables in
internal worker interfaces are routing guards, not proof of authorization.

## Architecture

`contract.hpp` defines the operand-local request and fixed IDs. Candidate
`adapters.cpp`/`admission.cpp` implement the five encodings and strict precedence.
Each oracle TU owns a separate encoder, decoder, validation and mathematical
count; neither links the candidate library. Their shared driver, descriptor
schedule, SHA-256 and wire writer implement transport rather than mathematical
truth. Candidate entry points accept only Request. The common sink is a separate
translation unit, with LTO disabled.

`schedule.cpp` enumerates descriptors in frozen order; materialization is a
separate operation with an injected encoder. This permits metadata cardinality
checks without allocating LARGE inputs. Each oracle materializes through its
own encoder. The worker uses aligned exact-end/canary allocations and validates
input hashes, offsets, disjointness and immutability around each call.

`verifier.py` contains no mathematical encoding/decoding or candidate linkage.
It streams records, checks ordering, masks, null/count semantics and FNV state,
and compares every result with the sealed expected stream. `evidence.py`
provides explicit little-endian layouts, canonical JSON and bundle identities.

`campaign.py` is L; `verifier.py` is V; oracle/worker/probe binaries are W.
The owner has no default run mode. Its authorization JSON requires exact
implementation/protocol/host/compiler identities and human authorization plus
oracle/verifier/accounting review references. It refuses an existing output
location. References and hashes do not authenticate the decisions they describe.

## Accounting closure and remaining campaign gates

`accounting.py` and `linux_trace.py` collect Linux per-process peak observations
at birth/exec/exit boundaries and wait4 receipts at reap, in L without an extra
monitor process. The compiler-driver/tool hook no longer has an unconditional
block. The same 512 MiB / 1,800-second / 100 MiB limits and sampling-gap rejection
remain active. Missing identities/peaks, unavailable tracing, unexpected process
rosters and unsupported task transitions still block accounting.

Twenty bounded fixtures passed on the recorded Linux ARM64 kernel/toolchain.
The first failed Linux run and a later fixture setup failure are retained with
corrections. See [ACCOUNTING_REVIEW.md](ACCOUNTING_REVIEW.md) before interpreting
those checks. Darwin/Windows campaign accounting remains unsupported; Linux
x86-64 and other host tuples are untested. There is no full-campaign result.

A Linux reviewer can build the infrastructure fixture and opt in explicitly:

```sh
clang++ -std=c++20 -Wall -Wextra -Werror -fno-lto -O0 -pthread \
  experiments/arch003/tests/accounting_fixture.cpp -o /tmp/arch003-accounting-fixture
ARCH003_LINUX_FIXTURES=1 \
  ARCH003_ACCOUNTING_FIXTURE=/tmp/arch003-accounting-fixture \
  ARCH003_LINUX_EVIDENCE_DIR=/tmp/arch003-accounting-evidence \
  python3 -B -m unittest discover -s experiments/arch003/tests \
    -p test_accounting_linux.py -v
```

The fixture build must succeed before running tests. These tests use fixed
infrastructure payloads and injected clock/proc/PID faults, not campaign cases.
They never call a candidate count, G1/G2 or diagnostic sanitizer probe.
Independent acceptance of accounting, both oracles and V, plus new exact-commit
human authorization for a qualified host, remains required before a campaign.
