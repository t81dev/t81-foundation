# RFC-00CF: Slice6 CanonFS Operator Actions

**Status:** draft  
**Type:** standards-track  
**Applies-To:** slice6 `t81sh`, freestanding CanonFS operator surface, CanonFS-backed EL0 execution flow  
**Created:** 2026-03-25  
**Updated:** 2026-03-25  

---

## Summary

RFC-00CF defines the first stateful, product-facing CanonFS action surface for
the real slice6 serial shell. The current shell can report storage state
(`canonfs`) and kernel/runtime status, but it cannot yet inspect CanonFS
artifacts or trigger a user-directed execution flow. RFC-00CF introduces a
narrow operator action set, starting with read-only CanonFS inspection and a
minimal governed program-launch path, so the bootable slice6 lane can do more
than demonstrate kernel plumbing.

## Motivation

The slice6 lane now has a coherent kernel/runtime foundation:

- bootable `t81sh`
- persistent CanonFS attachment
- EL0 process loading from CanonFS
- scheduler, fault, and recovery observability

But the operator shell still stops at introspection.

That means the real boot lane has an awkward gap:

- the system can load and run EL0 binaries from CanonFS during the boot proof
- the shell can tell you CanonFS is present
- the shell cannot ask CanonFS what is on disk
- the shell cannot trigger a governed launch itself

At this point the kernel lane is no longer the bottleneck. The missing value is
an operator-facing execution surface.

## Proposal

### 1. Scope

RFC-00CF is intentionally narrower than the hosted `t81sh` described in
RFC-00B9.

It does **not** attempt to bring the full hosted shell model into slice6. It
only defines the smallest useful CanonFS action surface for the real
freestanding shell:

- `canonfs ls`
- `canonfs hash`
- `canonfs run`

Optional future extensions such as `canonfs cat` or write surfaces are
explicitly out of scope for the first milestone.

### 2. Read-only CanonFS inspection

The first operator-facing actions should be read-only.

#### `canonfs ls`

Lists the small freestanding CanonFS-visible execution inventory.

In the slice6 lane, this may initially be a synthetic inventory view rather
than a full directory traversal API. One valid first implementation is to list
the known boot-proof artifacts present in the raw store:

- T81X sectors by LBA
- manifest sectors associated with those binaries
- compact metadata such as `kind`, `tid`, and `code_hash` presence

The key requirement is that the operator can see what executable/test artifacts
are currently available without leaving the shell or inspecting the disk image
offline.

#### `canonfs hash <artifact>`

Prints the retained code hash or CanonFS-visible identity for a named artifact.

This action should use the same canonical identity story already established by
RFC-00C0 / RFC-00C1 / RFC-0054 rather than inventing a shell-only naming rule.

### 3. Governed execution action

#### `canonfs run <artifact>`

Starts a governed EL0 execution session from a named CanonFS artifact.

The first freestanding version should stay narrow:

- only execute known-safe T81X/T81M-backed operator artifacts
- no arbitrary path parsing
- no full session/job-control model
- no writable CanonFS mutation

One valid first mapping is:

- operator names an artifact key or known LBA-backed alias
- EL1 validates the artifact shape and hash
- EL1 resets the scheduler session state
- EL1 loads the artifact into the EL0 code page
- EL1 runs it and reports a deterministic completion banner at the shell

This is an operator action, not a general executable-object API redesign.

### 4. Shell and product boundary

RFC-00CF keeps the distinction between:

- the **hosted** `t81sh` command model from RFC-00B9
- the **real freestanding** slice6 shell surface

The slice6 shell is allowed to implement a meaningful subset first, as long as:

- command names are not misleading
- state-changing actions are governed and auditable
- the implementation does not claim the broader session/filesystem semantics of
  the hosted lane before those semantics exist

### 5. First milestone

The first implementation milestone should prove:

1. a persistent CanonFS-backed boot lane can enumerate known executable
   artifacts from the prompt,
2. an operator can request one governed launch by artifact name,
3. the kernel loads and runs that artifact from CanonFS,
4. the shell returns to the prompt with deterministic success/failure output.

That is enough to make the slice6 shell materially more useful without trying
to become the full product shell in one step.

## Determinism / Safety Considerations

- Operator actions must remain deterministic under the same CanonFS image.
- `canonfs run` must validate artifact identity before launch.
- Launch must not widen privilege or bypass the existing EL0 bridge and
  scheduler accounting.
- Read-only inspection actions must not mutate CanonFS or scheduler state.
- State-changing operator actions must remain auditable.

## Compatibility

RFC-00CF is additive.

- Existing slice6 shell commands remain unchanged.
- RFC-00B9 remains the broader user-environment contract; RFC-00CF only defines
  a narrow freestanding subset.
- Existing boot proofs remain valid if the new shell actions are not invoked.

## Implementation Plan

1. Define the freestanding artifact inventory view exposed by `canonfs ls`.
2. Add a stable naming rule for the small set of launchable artifacts.
3. Implement `canonfs hash` over existing retained CanonFS/T81X metadata.
4. Implement one governed `canonfs run` path for a bounded artifact set.
5. Add a local shell smoke extension and CI smoke check that exercises the new
   command(s).

## Open Questions

1. Should the first artifact naming rule be LBA-based (`lba:12`) or alias-based
   (`fault-test`, `wait-test`)?
2. Should `canonfs run` be limited to pre-registered proof artifacts at first,
   or may it run any valid T81X sector discovered in the store?
3. Should `canonfs ls` expose only executable artifacts initially, or also
   manifest sectors and raw store metadata?

## Acceptance Criteria

- The slice6 shell gains at least one read-only CanonFS inspection command.
- The slice6 shell gains one governed CanonFS-backed execution action.
- Artifact identity/selection is deterministic and documented.
- A local and CI proof path shows the shell can enumerate and launch at least
  one CanonFS-backed artifact and return to the prompt.
