# AI OS-Object Bundle Integration Matrix

This matrix describes the most realistic current integration targets for the
canonical bundle in the admitted bounded AI OS-object family.

It does not define a general bundle platform.
It describes the current baseline-safe uses of the bundle as a consumer-facing
system boundary.

## Scope

This matrix applies only to the current admitted family:

- `assess-fixed`
- `route-fixed`
- `classify-fixed`

Use it together with:

- [AI_OS_OBJECT_BUNDLE_CONSUMPTION_CONTRACT.md](./AI_OS_OBJECT_BUNDLE_CONSUMPTION_CONTRACT.md)
- [STABLE_BASELINE_CONTRACT.md](./STABLE_BASELINE_CONTRACT.md)

## Integration Matrix

| Integration target | Reads bundle only? | Follows `record_ref`? | Follows `action_ref`? | Writeback needed? | Baseline-safe? | Notes |
|---|---|---|---|---|---|---|
| Host-side action runner | no | yes | yes | no | yes | Natural fit for `assess-fixed` and `route-fixed`; reads bundle first, then uses typed downstream record plus canonical action identity. |
| Rule-following service | no | yes | yes | no | yes | Natural fit for `classify-fixed`; uses `selected_rule_set` and `rule_set_ref` after bundle validation. |
| Routing handoff component | no | yes | yes | no | yes | Reads `selected_path` from the typed route record after validating the bundle schema. |
| Classification result consumer | no | yes | yes | no | yes | Consumes the completed classification chain from the bundle rather than the raw model result. |
| Audit review system | yes | optional | optional | no | yes | Bundle alone is enough to establish the chain boundary; deeper refs are optional when more evidence is needed. |
| Compliance evidence pipeline | yes | optional | optional | no | yes | Can archive the bundle as the top-level evidence object and dereference provenance only on demand. |
| Governance dashboard | yes | optional | optional | no | yes | May display bundle-level status and schema first, then drill into record/provenance refs. |
| Approval trail storage | yes | optional | optional | no | yes | Good fit for storing completed decision objects without reconstructing them from logs. |
| Release/deployment gate | yes | optional | optional | no | yes | Bundle can act as the canonical approved decision object handed into a release gate. |
| Artifact attestation pipeline | yes | optional | optional | no | yes | Can anchor attestation to the bundle as the final authoritative object. |
| CanonFS archival store | yes | no | no | no | yes | Stores bundle as the smallest stable object naming the full completed chain. |
| Historical replay/re-read surface | yes | optional | optional | no | yes | Starts from bundle and dereferences only what is required for comparison or review. |
| Streaming inference consumer | no | no | no | n/a | no | Out of scope; the bundle is post-execution consumption, not live runtime control. |
| Agent loop/orchestrator | no | no | no | n/a | no | Out of scope for the current baseline; bundle is not a general workflow object. |
| General workflow engine | no | no | no | n/a | no | Out of scope; would widen the admitted family beyond the stable baseline. |
| Arbitrary external API contract | no | no | no | n/a | no | Not frozen; current bundle contract is family-only and not a general network API. |

## Reading The Columns

- `Reads bundle only?`
  - means the integration can do useful work from the bundle artifact itself
    without immediately dereferencing deeper objects
- `Follows record_ref?`
  - means the integration needs the typed downstream record for family-specific
    fields
- `Follows action_ref?`
  - means the integration needs the canonical downstream action identity
- `Writeback needed?`
  - means the integration must mutate or extend the admitted chain to function
- `Baseline-safe?`
  - means the integration fits the current stable baseline without widening the
    family contract

## Operating Rule

The current bundle integrates cleanly with systems that need a:

- completed
- canonical
- policy-approved
- replayable

decision object.

It does not yet integrate cleanly with systems that require:

- streaming inference
- open-ended orchestration
- dynamic agent loops
- generalized workflow composition

The current bundle is strongest when treated as the authoritative object for
post-execution consumption.
