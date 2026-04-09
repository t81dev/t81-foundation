# Financial Services Vertical Demo Brief

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Financial Services Vertical Demo Brief](#financial-services-vertical-demo-brief)
  - [Purpose](#purpose)
  - [Industry Problem](#industry-problem)
  - [Target Buyer](#target-buyer)
  - [One-Line Positioning](#one-line-positioning)
  - [Internal Hard Positioning](#internal-hard-positioning)
  - [Demo Claim](#demo-claim)
  - [Demo Narrative](#demo-narrative)
  - [Demo Spine](#demo-spine)
  - [Preferred Current Chain](#preferred-current-chain)
  - [What To Show Explicitly](#what-to-show-explicitly)
  - [What This Demo Does Not Claim](#what-this-demo-does-not-claim)
  - [Success Condition](#success-condition)
  - [Failure Modes](#failure-modes)
  - [Suggested Follow-On](#suggested-follow-on)

<!-- T81-TOC:END -->


## Purpose

This brief defines one narrow industry-facing demo for T81's current strongest
usable surface:

- approval before execution
- immutable artifact identity
- reproducible bounded execution
- provenance-backed canonical output objects

This is not a general AI platform demo.
This is not an operating-system demo.
This is a controlled-execution and audit-evidence demo.

## Industry Problem

Financial institutions often need to answer four questions about model-assisted
or policy-governed decisions:

1. What exact artifact ran?
2. Was it approved before execution?
3. What exact output did it produce?
4. Can that output be reproduced and audited later?

Many current stacks answer these questions incompletely or only after the fact
through logs, tickets, or manually assembled evidence.

T81's current wedge is to make those answers immediate and object-backed.

## Target Buyer

- model risk governance
- compliance engineering
- platform security
- internal ML platform teams in regulated environments

## One-Line Positioning

T81 ensures only approved artifacts run and every result is a reproducible,
provenance-backed object.

## Internal Hard Positioning

T81 enforces approval before execution and produces outputs that are
cryptographically tied to provenance and policy.

## Demo Claim

This demo shows that:

- an unapproved artifact can be denied before execution
- an approved bounded task can run under the same governed surface
- the result is not just text or a log line
- the result ends as a canonical bundle object with explicit provenance
- identical reruns produce the same canonical object identity

## Demo Narrative

Use a financial-services framing such as risk classification under controlled
execution.

Narrative:

- an unapproved model or artifact cannot enter the trusted execution path
- an approved bounded classification task runs
- the system produces:
  - result artifact
  - provenance artifact
  - downstream record
  - canonical bundle
- an auditor can inspect the final bundle and its linked refs without needing
  an improvised evidence trail

## Demo Spine

Keep the live demo in this order:

1. Controlled entry
   - import a model or other artifact into CanonFS
   - show content-addressed identity

2. Policy gate
   - show a denied path first
   - then show the approved path

3. Execution
   - run one bounded chain

4. Object output
   - show the result artifact
   - show the provenance artifact
   - show the downstream record
   - show the canonical bundle

5. Deterministic proof
   - rerun the same chain
   - show identical canonical bundle identity

## Preferred Current Chain

Use `classify-fixed` as the primary vertical demo chain.

Reason:

- it maps cleanly to a financial-services classification story
- it already ends in a canonical rule-set selection bundle
- it is easier to explain as a governed decision object than a host action or
  path-selection flow

Current example and proof surfaces:

- `examples/ai-and-inference/model-load-canonfs/run_classify_fixed_rule_selection.sh`
- `tests/cpp/ai_task_classify_fixed_composition_test.cpp`
- `docs/reference/AI_OS_OBJECT_CHAIN_CATALOG.md`

## What To Show Explicitly

Show these objects by name:

- task result artifact
- provenance artifact
- downstream record
- canonical bundle

State explicitly:

- the first three are intermediate
- the bundle is the top-level persisted object

## What This Demo Does Not Claim

This demo does not claim:

- general-purpose AI orchestration
- generalized inference serving
- agent autonomy
- superior model capability
- superior raw inference performance
- system-wide guarantees beyond the currently validated bounded surfaces

## Success Condition

The demo is successful if the viewer can repeat back:

- nothing runs unless it is admitted
- the output is a persisted object, not just a log
- the object is tied to provenance
- the same run can be reproduced with the same identity

## Failure Modes

Do not weaken the demo by:

- adding extra chains
- mixing in broad AI platform claims
- leading with benchmarks
- replacing the canonical bundle with text output
- skipping the denied path
- skipping the rerun identity proof

## Suggested Follow-On

If this demo resonates, the next artifact should be:

- one recorded run
- one auditor-oriented walkthrough
- one buyer-facing short note using the same narrative and no broader claims
