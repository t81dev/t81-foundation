# EXPLORATORY — NONCONFIRMATORY

On October 28, 2026 the researcher explicitly instructed OpenAI Codex to collect
ten randomized paired observations on one predefined input with empty stores,
using the candidate `05639a677b11ea1f4ef0328b3f67140784a4ead1` apparatus. A minimal
research-local timing wrapper was expressly permitted if needed. Measurement
execution is capped at 30 minutes; every attempt is retained and correctness or
identity mismatch stops collection. The deliverable belongs on draft PR #628.

This author selects `greet hello` before timing. The separate exploratory seed
6281028 shuffles five AB and five BA pairs. `collect.py` records the entire plan
and its own SHA-256 before the first timed launch. It launches the unchanged
apparatus single-worker `run` operation in fresh processes, with independent
initially empty stores. It does not call `validate_pair`, any older benchmark
runner, or any batch/confirmatory collection entry point.

The clock starts immediately before `Popen` and ends after `communicate` returns.
The worker has executed, persisted and independently hash-verified all seven
logical roles before returning. The interval includes interpreter/module startup,
fixture reads and A's temporary fixture writes, public CLI subprocesses,
serialization, storage/readback, nested durable instrumentation and process
teardown/pipe capture. It excludes fixture generation, store setup, outer
attempt-journal fsync and parent JSON/parity verification. That verification must
pass before accepting a pair. Every interval is retained even if later invalid.
No replay or internal phase timing is collected.

Existing worker responses/journals remain explicitly validation-only correctness
payloads. Their audit timestamps are never converted into durations. The separate
outer clock records carry the exploratory label and are permanently ineligible
for the preregistered dataset. No confidence intervals, materiality classification,
or architectural/ternary-specific causal conclusion is authorized here.

The frozen 360-pair schedule, protocol, accepted comparator, statistics code,
production implementation and historical conclusions must remain byte-identical.
Independent review and separate confirmatory authorization remain pending. This
instruction grants neither merging nor conversion of the PR out of draft.
