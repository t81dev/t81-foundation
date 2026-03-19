---
title: "Axion Policy Language: A Deterministic DSL for Safety and Auditing"
status: accepted
author: Jules
date: 2026-03-08
vote: +1
supersedes: RFC-0009
---

## 1. Abstract
This RFC proposes the Axion Policy Language (APL), a formal S-expression based Domain Specific Language (DSL) for defining security, resource, and alignment policies for the HanoiVM.

## 2. Motivation
Currently, Axion policies are verified by a simple Python script or hardcoded in C++. To fulfill the "Axion Policy Language" TODO, we need a formal language that can be compiled into a deterministic binary format for the Axion Governance Kernel to execute efficiently.

## 3. Proposal

### 3.1. Language Grammar
APL will use S-expressions with the following core predicates:
-   `(policy ...)`: Root container.
-   `(tier <N>)`: Minimum cognitive tier required.
-   `(max-instructions <N>)`: Hard limit on TISC instructions.
-   `(max-recursion <N>)`: Hard limit on call depth.
-   `(require-loop (file <F>) (line <L>) (bound <B>))`: Ensures a specific loop is traced with bounds.
-   `(require-segment-event (action <A>) (segment <S>))`: Audits memory access.
-   `(require-alignment (reason <R>))`: Enforces alignment checkpoints.

### 3.2. Compiler Pipeline
-   **Frontend**: Lexer and Parser in the `t81` CLI.
-   **Middleware**: Semantic analyzer to check for conflicting policies.
-   **Backend**: Binary emitter that produces `.axionb` (Axion Binary) files.

### 3.3. Runtime Execution
The Axion `PolicyEngine` will be updated to load and execute `.axionb` files. Policy evaluation will happen at syscall/signal boundaries and before sensitive opcodes.

## 4. Impact
-   **Tooling**: New `t81 policy compile` and `t81 policy run` commands.
-   **Kernel**: Axion Governance Kernel changes to support binary policy loading.
-   **Auditability**: Policies become first-class, hashable artifacts.

## 5. Alternatives
-   **YAML/JSON**: Considered but rejected due to S-expressions' natural fit for nested logic and existing validator compatibility.
-   **T81Lang subset**: Using a subset of T81Lang for policies (rejected for complexity; policies should be declarative and simple).

## 6. Unresolved Questions
-   Version negotiation for policies.
-   Cross-policy references and imports.
