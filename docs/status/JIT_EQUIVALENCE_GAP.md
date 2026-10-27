# JIT Equivalence Gap

## Evidence review — 2026-10-27

Purpose: track interpreter/JIT equivalence evidence. The three unconditional MATCHING cells below are **Historical / Unsupported as whole-surface claims**. Current source is [vm/jit/jit_compiler.cpp](../../vm/jit/jit_compiler.cpp), with unsupported operations falling back to the interpreter. JIT is an executable bounded implementation, not a placeholder.

| Boundary        | Evidence available                                                                                                                         | Current disposition                                                                                             |
| --------------- | ------------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------------------------------------------------------------------------------- |
| Trace/math      | [trace tests](../../tests/cpp/jit_trace_equivalence_test.cpp), [tensor trace tests](../../tests/cpp/jit_tensor_trace_equivalence_test.cpp) | Trace test built and attempted, timed out at 30 seconds; tensor trace not rerun. Full equivalence Inconclusive. |
| Policy          | [per-instruction policy test](../../tests/cpp/vm_jit_per_instruction_policy_test.cpp), interpreter fallback                                | Scoped enforcement coverage; no blanket governance equivalence.                                                 |
| Faults / memory | [oracle test](../../tests/cpp/jit_repro_oracle_test.cpp), compiler guards                                                                  | Corpus-bounded; all faults/platforms not established.                                                           |

The existing registry excludes full JIT verification. Next useful experiment: execute a pinned differential corpus comparing outputs, traps, policy denials and trace bytes with fallback counts on the declared platform matrix. Any hard divergence falsifies the scoped equivalence claim; passing a corpus does not promote JIT.

Review basis: audited main `c930b8c2ee7478782bf3c6d231bdd59984595ef2`. This editorial review does not grant promotion or release approval. See the [reconciliation report](STATUS_RECONCILIATION_2026-10-27.md) for executed checks and limitations.

The local trace-equivalence test timed out at 30 seconds after an earlier longer attempt was interrupted. No new equivalence result or source regression is established by that incomplete run. See the report for environment and failure details.

## Retained record

## Overview
This document tracks the execution equivalence gap between the Tier 1 (interpreter) and JIT tiers of the T81VM.

## Status

| Component | Status | Tracking Issue |
|---|---|---|
| Interpreter vs JIT Math | MATCHING | |
| Interpreter vs JIT Governance | MATCHING | |
| Interpreter vs JIT Faults | MATCHING | |
