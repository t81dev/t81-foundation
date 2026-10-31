# Chapter 6: CLI and API Usage

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Chapter 6: CLI and API Usage](#chapter-6-cli-and-api-usage)
  - [6.1 The T81 Command Line Interface](#61-the-t81-command-line-interface)
    - [6.1.1 Compilation (`compile`)](#611-compilation-`compile`)
    - [6.1.2 Execution (`run`)](#612-execution-`run`)
    - [6.1.3 Trace Analysis (`trace`)](#613-trace-analysis-`trace`)
    - [6.1.4 Interactive Mode (`repl`)](#614-interactive-mode-`repl`)
  - [6.2 Embedding T81 (C++ API)](#62-embedding-t81-c++-api)
  - [6.3 Embedding T81 (Python API)](#63-embedding-t81-python-api)
  - [6.4 Debugging](#64-debugging)
    - [Onboarding Drill](#onboarding-drill)
    - [Role-Based Learning Path](#role-based-learning-path)
    - [Worked Example](#worked-example)
    - [Hands-On Lab](#hands-on-lab)
    - [Cross-Chapter Continuity](#cross-chapter-continuity)
    - [Expected Outcomes](#expected-outcomes)
    - [Chapter Summary](#chapter-summary)
    - [Read Next](#read-next)

<!-- T81-TOC:END -->


This chapter teaches how to operate T81 day-to-day without losing assurance discipline. The command line and APIs are not competing interfaces; they are complementary ways to run the same accountable workflow.

A useful model for onboarding is:

1. use CLI for explicit, replayable operations,
2. use APIs for integration and automation,
3. use traces and policy context to explain outcomes.

## 6.1 The T81 Command Line Interface

The CLI is the most transparent way to learn the system because each command is explicit about intent. You can see what was executed, with what arguments, and in what order.

For new users, this matters because reproducibility starts with observable behavior. Hidden defaults and ad hoc shell history are common sources of confusion in other ecosystems.

### 6.1.1 Compilation (`compile`)

Compilation turns source intent into executable machine form. In T81 workflows, compilation artifacts are part of the evidence chain, not disposable byproducts.

When teaching teams, stress two practices:

1. compile from known source state,
2. capture enough metadata to re-run compilation exactly.

If those habits are present, downstream debugging is much faster.

### 6.1.2 Execution (`run`)

Execution answers a simple but high-stakes question: what happened when this program met this policy and this input set?

A common onboarding mistake is to treat `run` as a generic launch command. In T81, execution context is part of the result. The same binary under different policy context can produce intentionally different outcomes.

### 6.1.3 Trace Analysis (`trace`)

Trace analysis is where T81 becomes operationally powerful. Instead of debating hypotheses, teams compare concrete records of execution path and decisions.

During incidents, this shortens mean-time-to-understanding:

1. identify the first divergence point,
2. confirm whether policy affected control flow,
3. classify expected vs unexpected behavior.

### 6.1.4 Interactive Mode (`repl`)

REPL is excellent for exploration and training. It lowers friction for trying ideas and validating assumptions quickly.

The maturity step is to graduate important REPL discoveries into source-controlled scripts or programs. Exploration is useful; reproducible evidence is required.

## 6.2 Embedding T81 (C++ API)

C++ embedding gives you explicit lifecycle control over VM construction, program loading, execution boundaries, and output capture. This is ideal when T81 is part of a larger deterministic system.

In onboarding terms, C++ embedding teaches that T81 can be treated as a deterministic subsystem with clear contracts, rather than a black box process.

Recommended integration flow:

1. initialize runtime with explicit configuration,
2. load program and policy context,
3. execute with controlled inputs,
4. collect result plus evidence artifacts.

## 6.3 Embedding T81 (Python API)

Python API usage is often the fastest path for test harnesses, orchestration, and experiment pipelines. It enables broad scenario coverage with less integration overhead.

The key discipline is consistency with release paths. Use Python to automate breadth, but keep critical assertions aligned with the same deterministic boundaries used in production evidence.

Good onboarding exercise:

1. run one scenario via CLI,
2. run the same scenario via Python,
3. compare artifacts and explain any mismatch.

## 6.4 Debugging

Debugging in T81 is strongest when framed as divergence analysis rather than symptom chasing. Ask: where did actual behavior first depart from expected constrained behavior?

Practical debugging loop:

1. reproduce with fixed inputs and policy,
2. inspect trace at divergence boundary,
3. check whether behavior is policy-enforced, runtime defect, or expectation defect,
4. validate fix using the same reproducible setup.

### Onboarding Drill

Choose one small program and intentionally trigger a policy denial. Then:

1. run it in a controlled environment,
2. inspect trace output,
3. explain in writing why denial occurred,
4. modify policy or program intentionally and rerun.

If a new user can complete this drill, they usually become productive quickly.

### Role-Based Learning Path

| Role | Focus In This Chapter | You Are Ready When |
| --- | --- | --- |
| New User | Use CLI commands as replayable workflows | You can rerun compile/run/trace on demand with identical context |
| Integrator | Choose CLI vs API intentionally | You can justify interface choice for one production task |
| Auditor | Interpret traces as evidence, not logs | You can explain one behavior change via trace comparison |

### Worked Example

A command succeeds locally but fails in CI. Rather than patching around it, you compare policy context and command parameters, then replay with trace enabled to isolate divergence.

### Hands-On Lab

1. Run one workflow using CLI only.
2. Re-run same workflow via Python API wrapper.
3. Compare outputs, traces, and policy context fields.

### Cross-Chapter Continuity

Use this chapter output as input for `Lab A` and `Lab B` in [README](./README.md#cross-chapter-end-to-end-labs).

### Expected Outcomes

- You can treat execution as accountable workflow rather than shell trial-and-error.
- You can explain mismatches with evidence.

### Chapter Summary

You should now view CLI/API usage as one coherent execution discipline: explicit intent, replayable commands, and evidence-driven interpretation.

### Read Next

Proceed to Chapter 7 to learn how T81Lang coding patterns reinforce deterministic reasoning.

<!-- chapter-nav-start -->

---

**Navigation**

- [Book Index](./README.md)
- [Previous: Chapter 5: Installation and Build Verification](./05_Installation.md)
- [Next: Chapter 7: Programming in T81Lang](./07_Programming_in_T81Lang.md)

<!-- chapter-nav-end -->
