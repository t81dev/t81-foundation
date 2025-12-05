______________________________________________________________________

vote: +1

# RFC-0013 — Axion Segment Trace Semantics

Version 0.1 — Draft (Standards Track)\
Status: Draft\
Author: Axion Governance Council\
Applies to: Axion, T81VM, T81Lang, Cognitive Tiers

______________________________________________________________________

# 0. Summary

This RFC formalizes the Axion-visible trace that documents every deterministic
segment transition inside the HanoiVM (stack, heap, tensor, and meta). It also
defines the canonical `verdict.reason` strings emitted when Axion guards such as
`AxRead` and `AxSet` inspect a segment address. Policies can now require these
strings for audits without inspecting the implementation.

# 1. Motivation

Axion’s determinism guarantees depend on a complete, replayable trace of every
memory segment transition and privileged guard evaluation. Prior to this RFC,
only stack/heap allocations and enum guards were reported. The instrumentation
implemented in `src/vm/vm.cpp` now exposes tensor slot allocations, meta slot
advances, and guard verdicts (stack/heap/tensor segment addresses). Specifying
the expected format ensures documentation, tooling, and Axion policies can rely
on these strings even as the runtime evolves.

# 2. Specification

1. The HanoiVM MUST emit `AxionEvent` entries for every stack, heap, tensor, or
   meta slot transition before executing the related opcode. Each event's
    `verdict.reason` MUST include:

   - a human-readable action (e.g., `stack frame allocated`, `tensor slot
     allocated`, `meta slot axion event segment=meta`, `stack frame freed`);
   - the canonical segment name (`stack`, `heap`, `tensor`, `meta`) and the
     segment address logged as `addr=<value>`;
   - the size when applicable (`size=<value>`).

2. Axion guard syscalls (`AxRead`/`AxSet`) MUST consult the segment kind and
   include it inside the `verdict.reason` string in the format:

   ```
   Ax{Read,Set} guard segment=<segment-name> addr=<address> [existing reason]
   ```

   Policies can then assert the guard had visibility into the same segment that
   triggered the policy action, ensuring no gaps exist between metadata and
   runtime behavior.

3. Meta slot events MUST advance the meta pointer before every Axion event so
   Axion can reconstruct deterministic metadata writes for audits. The
   `verdict.reason` for these entries includes `meta slot axion event segment=meta addr=<value>`.

4. The Axion CLI trace (`:trace`, `:trill`) and `vm::AxionEvent` log MUST retain
   the above strings verbatim so downstream policies or documentation can replay
   them. Regression tests (`tests/cpp/axion_segment_trace_test.cpp`,
   `tests/cpp/vm_memory_test.cpp`) are the canonical reference for the expected
   log entries.

# 3. Rationale

- Embedding segment names and addresses directly inside `verdict.reason` means
  Axion policies no longer need to parse opcodes to recover segment state.
- Meta slot advancement before Axion events guarantees deterministic metadata
  traces, which are required by the VM’s canonicalization and snapshot hooks.
- Guard strings such as `AxRead guard segment=stack addr=42` provide an
  unambiguous audit log that the CLI and Axion policy engine can cross-reference
  without implicit contracts.

# 4. Backwards Compatibility

- Legacy binaries without segment instrumentation will still execute but cannot
  satisfy policies that explicitly require the new strings. Policies SHOULD treat
  missing segment events as policy faults only when they `require` segment
  coverage.

# 5. Security Considerations

- The trace strings contain addresses but not pointer values derived from
  secrets, so they remain safe to log.  
- Ensuring the meta pointer advances before every Axion event prevents Axion
  from replaying stale metadata entries that would otherwise break canonicity.
