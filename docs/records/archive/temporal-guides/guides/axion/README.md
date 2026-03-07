# Axion Guides

This folder gathers every Axion-focused reference that lives under [`docs/guides/`](../). Open this README before digging into the individual policies, trace explanations, or observability tips so you understand how the pieces fit together.

## Active content
- [`axion-trace.md`](../axion-trace.md) describes `t81 run`/`t81 repl` trace output, which guard logs to capture, and how Axion verdicts tie back to policy definitions.
- [`axion-policy-manual.md`](../axion-policy-manual.md) explains how to author and inspect Axion policies, including `VERDICT_*` labels, guard parameters, and canonical metadata required for deterministic audits.
- [`axion-tracing-manual.md`](../axion-tracing-manual.md) and [`runtime-observability-manual.md`](../runtime-observability-manual.md) together show how to switch between enforcement and observability modes (`AXION_MODE=observability`), what extra logging is safe to emit, and how to pair traces with research artifacts for reproducibility.

## How to read them
- Start with [`axion-trace.md`](../axion-trace.md) when you need to interpret a log emitted by any `t81` command (`run`, `compile`, `weights import`, etc.).
- Use [`axion-policy-manual.md`](../axion-policy-manual.md) when you edit or audit policy files (`docs/governance/archive/policy/*.axion`). Pair it with [`policy/README`](../../governance/archive/policy/README.md) and [`spec/axion-kernel.md`](../../spec/axion-kernel.md) for normative semantics.
- Refer to [`axion-tracing-manual.md`](../axion-tracing-manual.md) and [`runtime-observability-manual.md`](../runtime-observability-manual.md) before running long trace captures; they remind you to keep Axion logs (`build/artifacts/*axion*.log`) alongside the artifacts you ship.

## Keeping it tidy
- When you update or add a guide in this folder, mention the change in [`docs/navigation.md`](../../navigation.md) and, if the topic touches CLI habits, adjust [`docs/guides/README.md`](../README.md) so the category description remains accurate.
- Archived Axion guidance should live under `docs/` with an `.archived` suffix so the README here only highlights operational content.
