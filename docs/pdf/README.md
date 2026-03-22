# PDF Archive

This directory contains **historical and aspirational** whitepapers and specification drafts.

## Status

The PDF documents in this directory (e.g., `HEXANARY`, `OCTANARY`) represent **aspirational architectural visions** and historical context. They do not necessarily reflect the current normative implementation of the T81 runtime.

**For authoritative specifications, please refer to the [`spec/`](../spec/) directory.**

## Implementation Status

See [`IMPLEMENTATION_STATUS.md`](IMPLEMENTATION_STATUS.md) for a detailed map of which concepts are currently implemented in the C++ runtime.

## Document Inventory & Status

The following table classifies each document relative to the current codebase.

| Document | Description | Status | Authoritative Replacement / Implementation |
| :--- | :--- | :--- | :--- |
| **TERNARY - T81Lang.pdf** | Early language draft | **Superseded** | [`spec/t81lang-spec.md`](../../spec/t81lang-spec.md) |
| **TRENARY - T81TISC.pdf** | Early ISA draft | **Superseded** | [`spec/tisc-spec.md`](../../spec/tisc-spec.md) |
| **TRYNARY - T81AxionAI.pdf** | Early Axion draft | **Superseded** | [`spec/axion-kernel.md`](../../spec/axion-kernel.md) |
| **TOPNARY - T81Ternary.pdf** | Early ecosystem overview | **Superseded** | [`spec/t81-overview.md`](../../spec/t81-overview.md) |
| **TYRNARY - T81Analysis.pdf** | Early analysis notes | **Superseded** | [`docs/explanation/ANALYSIS.md`](../../docs/explanation/ANALYSIS.md) |
| **PENTANARY - T729DataTypes.pdf** | Base-729 types | **Superseded** | [`include/t81/types/T729Tensor.hpp`](../../include/t81/types/T729Tensor.hpp) |
| **QUATERNARY - T243DataTypes.pdf** | Base-243 types | **Superseded** | [`include/t81/codec/base243.hpp`](../../include/t81/codec/base243.hpp) |
| **HEXANARY - T2187DataTypes.pdf** | Base-2187 types | **Aspirational** | N/A |
| **SEPTANARY - T6561DataTypes.pdf** | Base-6561 types | **Aspirational** | N/A |
| **OCTANARY - T19683DataTypes.pdf** | Base-19683 types | **Aspirational** | N/A |
| **SEENARY - T81Bridge.pdf** | Bridge concepts | **Historical** | N/A |
| **SETNARY - T81Setun.pdf** | Setun emulation notes | **Historical** | [`docs/records/archive/temporal-guides/guides/setun-bridge.md`](../../docs/records/archive/temporal-guides/guides/setun-bridge.md) |
| **TANNARY - T81Applications.pdf** | Application concepts | **Historical** | N/A |
| **TENNARY - T81Promo.pdf** | Promotional material | **Historical** | N/A |
| **TRCNARY - T81TrinaryExplorations.pdf**| Exploratory notes | **Historical** | N/A |
| **TRUNARY - T81Eratta.pdf** | Errata for early drafts | **Historical** | N/A |
| **TYNARY - T81Source.pdf** | Legacy source listing | **Historical** | N/A |
| **TΩNARY – T81Recursive AGI Codex.pdf**| Recursive AGI concepts | **Partially Implemented** | [`spec/cognitive-tiers.md`](../../spec/cognitive-tiers.md) & [`experimental/tiers/cog/tier5/`](../../experimental/tiers/cog/tier5/) |

**Status Definitions:**
- **Superseded:** The concept is implemented or replaced by a newer spec/code.
- **Aspirational:** Describes potential future extensions (N-ary bases) not currently in the core roadmap.
- **Historical:** Preserved context from early project phases.
- **Partially Implemented:** Core concepts are present in the codebase (e.g., skeletons, stubs) but not fully realized.

## Re-generation

These PDFs are build artifacts. Source content for normative sections has been migrated to `spec/` markdown files.
