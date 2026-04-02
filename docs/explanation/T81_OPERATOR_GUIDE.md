# T81 Operator Guide

This guide answers a simple question:

If you want to use T81 today, what should you run?

It is not a full architecture overview. It is a practical operator path for the
current real surfaces in the repo.

## Start Here

Use T81 today as three practical systems:

1. governed CanonFS interchange
2. bounded AI OS-object chains
3. bundle-first object consumption

If you understand those three, you understand the strongest current usable
value in the repo.

## What This System Actually Is

T81 is a governed execution pipeline where:

- execution is allowed or denied before it runs
- results are stored as canonical objects
- completed work is consumed from a stable bundle, not reconstructed from logs

The system is not centered on model outputs.

It is centered on:

- controlled execution
- canonical artifact chains
- and bundle-first consumption

## How This Differs From Typical AI Systems

Typical systems:

- run models first, evaluate later
- emit logs or responses
- reconstruct meaning from execution traces

T81:

- evaluates policy before execution
- produces canonical object chains
- treats the bundle as the final, authoritative object

This shifts the system from:

- "generate and observe"

to:

- "approve, execute, and preserve"

## 1. Governed CanonFS Interchange

Use this when you want to:

- import host files or directories into CanonFS
- export CanonFS objects back out
- apply policy before import/export side effects
- get stable JSON results with provenance and manifest linkage

Start here:

- [RFC-00D1 example](../../examples/storage-and-canonfs/canonfs-interchange/README.md)
- [RFC-00D1 draft contract](../../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)

Core commands:

```bash
./build/t81 canonfs import <path> --canonfs-root <root> --json
./build/t81 canonfs export <ref> --canonfs-root <root> --out <path> --json
```

What you get back:

- `status`
- `policy_result`
- `policy_profile`
- `provenance_ref`
- sometimes `manifest_ref`

Use this surface when the main question is:

- what entered CanonFS
- whether it was allowed
- what evidence object was produced

## 2. Bounded AI OS-Object Chains

Use this when you want AI execution to end as a canonical object chain instead
of a loose model output.

Current admitted family:

- `assess-fixed`
- `route-fixed`
- `classify-fixed`

Start here:

- [Bounded family status](../status/BOUNDED_AI_OS_OBJECT_FAMILY_STATUS.md)
- [Chain catalog](../reference/AI_OS_OBJECT_CHAIN_CATALOG.md)
- [First chain explainer](./FIRST_DETERMINISTIC_AI_OS_OBJECT_CHAIN.md)

Run the current examples:

```bash
bash examples/ai-and-inference/model-load-canonfs/run_assess_fixed_host_action.sh
bash examples/ai-and-inference/model-load-canonfs/run_route_fixed_path_selection.sh
bash examples/ai-and-inference/model-load-canonfs/run_classify_fixed_rule_selection.sh
```

What each chain does:

1. run one bounded AI task
2. store result artifact
3. store provenance artifact
4. store typed downstream record
5. store canonical bundle

Use this surface when the main question is:

- can I get a deterministic AI decision chain as a canonical object

## 3. Bundle-First Consumption

Use this when you want to start from the final object and consume the chain
safely.

The bundle is the system boundary.

It is the only object required to:

- verify what happened
- trace how it happened
- safely consume the result

Consumers should not reconstruct execution from logs or intermediate state.
They should begin from the bundle and follow canonical references.

Start here:

- [Bundle consumption contract](../reference/AI_OS_OBJECT_BUNDLE_CONSUMPTION_CONTRACT.md)
- [Bundle versioning boundary](../reference/AI_OS_OBJECT_BUNDLE_VERSIONING_BOUNDARY.md)
- [Stable baseline contract](../reference/STABLE_BASELINE_CONTRACT.md)

Run the current consumer examples:

```bash
bash examples/ai-and-inference/model-load-canonfs/run_assess_fixed_bundle_consumer.sh
bash examples/ai-and-inference/model-load-canonfs/run_route_fixed_bundle_consumer.sh
bash examples/ai-and-inference/model-load-canonfs/run_classify_fixed_bundle_consumer.sh
```

What these prove:

1. start from `bundle_ref`
2. read the bundle first
3. verify bundle schema
4. read `record_ref` and `action_ref`
5. follow the typed downstream record only after the bundle check

Use this surface when the main question is:

- can another component consume the completed chain without reconstructing it
  from logs

## What T81 Is Capable Of Right Now

- policy-gated artifact import and export
- CanonFS content-addressed storage
- deterministic bounded AI object chains
- canonical bundle production
- canonical bundle consumption
- a stable baseline for the admitted bounded family

## What T81 Is Not Yet

- a general AI platform
- an orchestration engine
- a broad agent runtime
- a general bundle platform
- a fully general DAIOS in everyday use

## Best Learning Order

If you want the shortest useful path, do this in order:

1. run the RFC-00D1 CanonFS interchange example
2. run `run_assess_fixed_host_action.sh`
3. run `run_assess_fixed_bundle_consumer.sh`
4. read [STABLE_BASELINE_CONTRACT.md](../reference/STABLE_BASELINE_CONTRACT.md)

That sequence teaches:

- governed artifact movement
- governed bounded AI object production
- governed object consumption

## If You Want X, Run Y

- If you want to prove import/export plus policy gating:
  - use the CanonFS interchange example under
    `examples/storage-and-canonfs/canonfs-interchange/`
- If you want a bounded AI decision chain:
  - run one of the three admitted family examples under
    `examples/ai-and-inference/model-load-canonfs/`
- If you want to consume the final object instead of the intermediate model
  output:
  - run one of the `*_bundle_consumer.sh` scripts
- If you want to know what is frozen:
  - read [STABLE_BASELINE_CONTRACT.md](../reference/STABLE_BASELINE_CONTRACT.md)

## One-Line Operating Rule

Use T81 today as a system that:

- controls what is allowed to execute
- produces canonical object chains instead of transient outputs
- and exposes completed work through bundle-first consumption

That is the clearest current truth of the repo.
