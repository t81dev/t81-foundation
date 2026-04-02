# AI Task Shared Framework Contract

This document defines the shared internal contract for deterministic governed AI tasks that run through the T81 AI task subsystem.

Canonical runnable example:

- For the current end-to-end example of this framework, see the `Assess-Fixed OS-Object Chain`
  section in [examples/ai-and-inference/model-load-canonfs/README.md](/Users/t81dev/Code/t81-foundation/examples/ai-and-inference/model-load-canonfs/README.md)
  and the companion script
  [run_assess_fixed_host_action.sh](/Users/t81dev/Code/t81-foundation/examples/ai-and-inference/model-load-canonfs/run_assess_fixed_host_action.sh).
- That example is the current canonical runnable chain for the framework, but this document
  remains the normative contract.
- The current bounded composition family built on this framework is listed in
  [AI_OS_OBJECT_CHAIN_CATALOG.md](/Users/t81dev/Code/t81-foundation/docs/reference/AI_OS_OBJECT_CHAIN_CATALOG.md).

## Shared Runner Responsibilities

The shared runner is responsible for:

- parsing the common task command shape
- enforcing fail-closed admission before execution
- requiring the strict deterministic `t81_reference_vm` lane only
- loading the approved model artifact
- running the single bounded native probe step used by the current task family
- writing result and provenance artifacts as CanonFS raw blocks
- emitting the shared CLI contract fields:
  - `result_summary`
  - `result_class`
  - `result_ref`
  - `provenance_ref`
  - `policy_result`
  - `termination_reason`

## Task Descriptor Responsibilities

Each task must provide a descriptor that defines:

- `cli_name`
- `task_kind`
- `task_name`
- `result_schema`
- required policy marker
- summary and meaning strings
- output kind string
- task materializer callback

Fixed-label tasks may additionally provide a closed vocabulary mapping through the descriptor.

## Materializer Boundary

The materializer is the only task-specific step after deterministic execution.

It must:

- transform the selected deterministic runtime token/result into a canonical result artifact
- remain deterministic for identical runtime inputs
- emit the task-specific CLI fields for the top-level JSON result
- avoid leaking probe structures into the result artifact

It must not:

- bypass admission
- invoke a second execution path
- write directly to CanonFS

## Artifact Rules

Result artifacts:

- are CanonFS raw blocks
- must include:
  - `schema`
  - `task`
  - `termination_reason`

Provenance artifacts:

- are CanonFS raw blocks
- use schema `t81.ai.task.provenance.v1`
- retain execution evidence and policy outcome

Retrieval:

- result retrieval uses `t81 canonfs get <result_ref>`
- provenance retrieval uses `t81 canonfs get <provenance_ref>`
- canonical AI task result fields can be read deterministically with:
  - `t81 ai task read-field <artifact-file|sha3-256:ref> --field <name>`
- fixed downstream records can be validated deterministically with:
  - `t81 artifact validate-record <file|sha3-256:hash> --schema <schema-id>`
- fixed downstream records can be written, stored, and read deterministically with:
  - `t81 artifact write-store-record --schema <schema-id> --field key=value ... [--canonfs-root <path>]`
  - `t81 artifact read-field <file|sha3-256:hash> --schema <schema-id> --field <name> [--canonfs-root <path>]`
- fixed downstream bundles can be stored and read deterministically with:
  - `t81 artifact store-bundle --schema <schema-id> --field key=value ... [--canonfs-root <path>]`
  - `t81 artifact read-field <file|sha3-256:hash> --schema <schema-id> --field <name> [--canonfs-root <path>]`

## Current Bundle Contract Family

The current downstream bundle family is intentionally narrow and exists only to persist
completed fixed-chain compositions as one top-level CanonFS object.

Supported bundle schemas:

- `t81.ai.task.assess-fixed.bundle.v1`
- `t81.ai.task.route-fixed.bundle.v1`

Required fixed fields:

- `source_result_ref`
- `source_provenance_ref`
- `action_ref`
- `record_ref`

Canonical field ordering:

1. `schema`
2. `source_result_ref`
3. `source_provenance_ref`
4. `action_ref`
5. `record_ref`

What the bundle represents:

- one persisted summary object for the current `assess_fixed.v1` composition
- a narrow link object that ties together:
  - the stored AI task result artifact
  - the stored AI task provenance artifact
  - the stored host action artifact
  - the stored downstream decision record

What the bundle must contain:

- `source_result_ref`: CanonFS ref to the canonical AI task result artifact for the chain
- `source_provenance_ref`: CanonFS ref to the canonical AI task provenance artifact
- `action_ref`: CanonFS ref to the non-AI action artifact selected from the AI result
- `record_ref`: CanonFS ref to the validated downstream record for the chain

What is intentionally out of scope:

- nested bundle trees
- arbitrary downstream artifact packaging
- schema inference
- general JSON bundling
- multi-step workflow orchestration

## Typed Helper Coverage

Supported helpers for the current downstream schema family:

- `t81 artifact write-store-record`
  - supports `t81.ai.task.assess-fixed.host-action-record.v1`
  - supports `t81.ai.task.route-fixed.path-selection-record.v1`
  - writes, validates, and stores the fixed downstream record
- `t81 artifact validate-record`
  - supports only the current fixed downstream record schema
  - validates local-file and CanonFS-ref selectors
- `t81 artifact store-record`
  - stores only validated supported downstream artifacts from file input
- `t81 artifact store-bundle`
  - supports `t81.ai.task.assess-fixed.bundle.v1`
  - supports `t81.ai.task.route-fixed.bundle.v1`
  - validates fixed bundle fields and stores the canonical bundle JSON as a CanonFS raw block
- `t81 artifact read-field`
  - supports only the current fixed downstream record and bundle schema family
  - returns one scalar field
  - rejects unsupported schemas and arbitrary JSON

CanonFS retrieval expectations:

- downstream records and bundles are CanonFS raw blocks
- retrieval uses `t81 canonfs get <ref>`
- typed scalar readback uses `t81 artifact read-field ... --schema <schema-id> --field <name>`
- the top-level persisted object for the current assess-fixed chain is the bundle artifact, not the
  downstream record alone
- the same object model is reused by the current `route-fixed` path-selection chain

Current convergence point:

- the current typed artifact helper surface converges in
  [main.cpp](/Users/t81dev/Code/t81-foundation/tools/cli/main.cpp)
- changes to helper behavior, help text, or supported schema names must remain aligned with:
  - `tests/cpp/cli_contract_test.cpp`
  - `tests/cpp/ai_task_assess_fixed_composition_test.cpp`
- treat that convergence point as contract-sensitive, not as an incidental CLI convenience layer

## Guardrails

- tasks must route through the shared runner
- tasks must supply a materializer callback
- the shared runner rejects result artifacts that omit:
  - `schema`
  - `task`
  - `termination_reason`

## Current Task Families

Fixed-label tasks:

- `answer_fixed.v1`
- `classify_fixed.v1`
- `route_fixed.v1`

Fixed-schema task:

- `assess_fixed.v1`

Current downstream schema family:

- `t81.ai.task.assess-fixed.host-action-record.v1`
- `t81.ai.task.assess-fixed.bundle.v1`
- `t81.ai.task.route-fixed.path-selection-record.v1`
- `t81.ai.task.route-fixed.bundle.v1`
