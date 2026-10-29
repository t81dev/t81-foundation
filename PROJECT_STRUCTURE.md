# T81 Foundation — Project Structure

T81 is an experimental computing architecture and systems research platform.
This map describes checked-in locations, not evidence that every subsystem is
integrated, production-ready, or comparatively superior. Begin with the
[README](README.md), [contributor guide](CONTRIBUTING.md), and [agent instructions](AGENTS.md).

## Research and evidence

- `docs/research/continuity/`: Master Continuity Charter, canonical H01–H08 register, Hanoi/CWEB lineage, and preserved probes.
- `docs/research/`: investigation reports and [consolidated evidence](docs/research/consolidation/README.md); historical conclusions retain their pinned scope.
- `experiments/`: bounded experimental implementations, controls, and raw results. Build flags and portability differ by experiment.
- `experimental/`: research implementations, including the historical `experimental/hanoi/` import. Historical arithmetic is evidence to test, not a correctness oracle.
- `benchmarks/`: performance workloads; measured results require equivalent baselines and explicit conditions.
- `examples/`: runnable examples and experiment adapters; their existence does not promote an interface to the deterministic core.

## Implementation and contracts

| Location                     | Role                                                                                              |
| ---------------------------- | ------------------------------------------------------------------------------------------------- |
| `spec/`, `spec/rfcs/`        | Specifications and RFCs; distinguish accepted contracts from drafts                               |
| `include/t81/`               | Public C++ headers and contracts                                                                  |
| `vm/`                        | Interpreter, decoder, tensor helpers, JIT and execution support                                   |
| `isa/`, `codec/`, `crypto/`  | ISA support, encodings, and cryptographic mechanisms                                              |
| `lang/`                      | Frontend, standard library, and language support                                                  |
| `kernel/axion/`              | Policy parsing, evaluation, and governance mechanisms                                             |
| `fs/`                        | CanonFS drivers, interchange, identity, and storage operations                                    |
| `tools/cli/`, `tools/model/` | CLI commands and model artifact tooling                                                           |
| `src/`                       | Primarily CMake target assembly; also contains experimental implementations such as `direct_api/` |
| `inference/`, `ffi/`         | Inference support and language/host interfaces; inspect each build and call path                  |
| `ternaryos/`, `drivers/`     | Systems and hardware/guest support; no general-purpose OS claim follows                           |

Surface status comes from the [governance register](docs/governance/GOVERNANCE_SURFACE_REGISTER.md)
and [determinism registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md), not directory names.
The root [support guide](SUPPORT_TAXONOMY.md) explains this distinction.

## Build, tests, and documentation

- `CMakeLists.txt`, `cmake/`, `CMakePresets.json`: authoritative build configuration. Use an out-of-source build; the tracked root `Makefile` is generated and contains machine-specific paths.
- `tests/`: unit, contract, conformance, fixture, and regression tests. Enabled targets depend on CMake options.
- `scripts/`, `.github/`: validation tooling, workflows, and contributor templates. Workflow definitions are not evidence that a particular run passed.
- `docs/`: research, governance, references, maintenance handoff, and historical records.
- `development_tests/`, `execution_reports/`, `strategic_docs/`, `market_analysis/`, `customer_tools/`, `market-demos/`: additional development and historical product/market material. These names and documents do not establish validated customer, performance, or security capabilities.
- `artifacts/`, `logs/`, root `build.log`, and root sample records: recorded/generated material; inspect provenance before treating it as reproducible evidence.
- `third_party/`: external dependencies. Some dependencies are fetched during configuration; see [build instructions](CONTRIBUTING.md).

No files or directories are relocated by this guide. Root additions and governance
changes remain subject to the [authority model](docs/governance/SPEC_AUTHORITY_MODEL.md).
See the [root audit](docs/research/ROOT_DOCUMENTATION_ALIGNMENT_AUDIT.md) for unresolved historical and build-metadata issues.
