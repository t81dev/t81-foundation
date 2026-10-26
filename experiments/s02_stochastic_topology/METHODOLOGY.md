# S02 correction methodology and reproduction

Date: October 26, 2026. Baseline: PR #582 merge `551d5075f35355b9ef96d9ce659d71be0891ee27`. Scope is this experiment directory only. No T81VM, TISC, T81Lang, production CMake graph, API lock or governance mechanism changes are included.

## Preservation and defect reproduction

All eleven original files are preserved under historical/pr582/ with exact source-byte hashes and an explicit superseded-evidence notice. Original report/results must not be treated as valid evidence for or against H1–H3. The original STOP directive is not adopted as an assumption.

The five behavioral regression methods were first run against that preserved packet:

```sh
cd experiments/s02_stochastic_topology
python3 test_integrity.py --experiment-dir historical/pr582 \
  --report /tmp/s02-before.json
```

Expected original result: failure, five methods, six failing assertions/subtests (the sampling method separately rejects C++ and Python), zero errors. This standalone historical probe uses a local `c++` compiler and white-box private-state access only to isolate the bug; it never changes the historical header. The normal corrected CTest path builds its probe through CMake, so Windows/MSVC does not need a Unix `c++` command. Actual review comments and observed failures are committed under evidence/.

## Corrected data generation

```sh
cmake -S experiments/s02_stochastic_topology -B /tmp/t81-s02-build \
  -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON
cmake --build /tmp/t81-s02-build --parallel 2
python3 experiments/s02_stochastic_topology/regenerate.py \
  --runner /tmp/t81-s02-build/t81_s02_stochastic_topology
ctest --test-dir /tmp/t81-s02-build --output-on-failure
python3 experiments/s02_stochastic_topology/analyze_results.py --check
```

The runner emits coefficients, 1260 repriced rows and 315 JSONL trajectories. The regeneration tool runs into temporary files, independently audits all task states/references and analyzes the data before replacing active outputs. It compresses traces with gzip mtime=0 and records source, binary and data hashes in evidence/generation.json. Partial replacement/interruption cannot pass the report/data audit. The original packet is never an output destination.

The independent Python implementation checks C++-generated coefficients for every one of the fifteen immutable problems, separately enumerates every binary endpoint, and reevaluates all 63315 trace states. For each row it verifies final/mean task energy, nonnegative reference gap, initial/post-step first-hit index, occupancy numerator/denominator, unique sweep identity and declared proxy-cost arithmetic. Every trajectory has exactly four price cells; other fields must match. All original-seed spot-check blind spots are removed. All numerical table cells are additionally reduced from raw rows in a separate regression test.

Binary enumeration also supplies the ternary optimum by the explicitly restricted multi-affine endpoint theorem. Independent 3^N enumeration checks N1–6 across three seeds; this is not a generic ternary-oracle assumption for other energies. Binary64 numeric tolerances are explicit; no arbitrary-precision or cross-platform bit-exact ground-energy claim is made.

## Randomness and interpretation

The old low-bit LCG and biased modulo mapping are replaced by published SplitMix64 plus rejection sampling in C++ and Python. Fixed test vectors, forced rejection and parity witnesses test actual range/stream behavior; these are not a statistical certification of physical IID randomness. Domain-separated seed constants separate task generation, initialization, spin proposals, acceptance and topology. Corrected problem instances therefore differ from the invalid original packet; old/new absolute gaps are not treated as a paired experiment. Corrected arms are paired only within the same immutable corrected task.

Four restructuring prices are post-hoc weights on the same trajectory. They do not increase sample size. The zero-temperature control repeats the same acceptance policy under nominal temperature labels; those rows likewise add no independent replication. Each size has only five problem seeds, below the unchanged rubric floor and without a pre-unblinding power calculation. Paired effects are descriptive; no p-values, equivalence, universal no-advantage or hardware claims are made.

The proxy counts attempted spin proposals, degree communications and logical undirected edge edits. It excludes full weighting of global network-energy scans, pair ranking/sorting, adjacency clearing/storage, RNG, task audit, reference computation and initialization. Additional counters expose some of that work, but they do not repair the full-cost evidence gap. The central control is a global correlation-ranking heuristic, not a strong optimized baseline portfolio. H2 remains untested; unsupported capacity flags reject instead of adding unused binary nodes.

The frozen rubric bytes/hash are unchanged. Its 20% full-cost effect and simultaneous 95% confidence criterion are not replaced after observing results. The data do not satisfy its capacity, cost, baseline and independent-instance prerequisites. The resulting **REVISE** decision limits architectural interpretation, rather than accepting or rejecting Track S from invalid or incomplete evidence. No S03 is initiated.

## Platform and validation limits

Recorded runs use macOS ARM64, AppleClang, Python 3.14 and CMake 4.3; exact versions/commands are retained in evidence/after/. Fixed-width unsigned PRNG arithmetic, explicit Python masking, C++17-only experiment code, CMake-built probes and UTF-8 text I/O avoid host-width/Unix-compiler/locale assumptions. Scoped Git attributes preserve historical/frozen evidence bytes through Windows checkout. Textual numbers are emitted at 17 significant digits. Standard-library sort comparison counts may vary across implementations, and regenerated metadata records its actual platform. Linux/Windows hosted execution is not claimed from local tests; production baseline CI is captured separately and is not weakened or repaired here.
