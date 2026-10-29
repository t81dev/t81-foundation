# S02 — corrected scientific-integrity pilot

**Decision: REVISE. H2 is UNTESTED.** This packet corrects merged PR #582; it does not initiate S03. Original results and the original STOP verdict are superseded, preserved byte-for-byte under [historical/pr582](historical/pr582/HISTORICAL_NOTICE.md), and are not valid hypothesis evidence.

The corrected runner evaluates immutable spin-glass tasks while adaptive algorithms change a separate computational graph. It produces 315 state trajectories and 1260 cost-repriced vertex-matched rows across 15 tasks. A separate Python oracle audits all 63315 initial/post-update states and every reference, task gap, occupancy and first hit. This is a **partial five-seed pilot**, not the full predefined sweep or an architectural/hardware validation.

- [Generated research report](RESEARCH_REPORT.md) and [machine-readable analysis](analysis.json)
- [Actual specification and omitted scope](SPECIFICATION.md)
- [Correction protocol and unchanged rubric](CORRECTION_PROTOCOL.md)
- [Five reproduced defects and verification](VERIFICATION.md)
- [Reproduction/provenance and limitations](METHODOLOGY.md)

Build only this experiment, without configuring T81VM/TISC/T81Lang or governance:

```sh
cmake -S experiments/s02_stochastic_topology -B /tmp/t81-s02-build \
  -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON
cmake --build /tmp/t81-s02-build --parallel 2
python3 experiments/s02_stochastic_topology/regenerate.py \
  --runner /tmp/t81-s02-build/t81_s02_stochastic_topology
ctest --test-dir /tmp/t81-s02-build --output-on-failure
python3 experiments/s02_stochastic_topology/analyze_results.py --check
```

The same experiment subdirectory remains available through the existing root CMake integration. No production build graph or source is changed. Local recorded execution is macOS ARM64; cross-platform certification is not claimed. Numerical comparison uses explicit tolerances, not an unsupported cross-platform bit-exact promise. The compressed state trace is deterministic for the recorded source/toolchain; PRNG words have fixed cross-language test vectors.
