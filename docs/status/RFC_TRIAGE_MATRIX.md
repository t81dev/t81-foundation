# RFC Triage Matrix

Last Updated: 2026-03-09
Owner: @t81dev
Purpose: distinguish release-critical RFC closure from backlog RFC tracking.

This matrix is intentionally short. It is not a substitute for the full RFC
index; it is an execution aid for deciding whether work should block releases,
promotion claims, or determinism statements.

## Complete Now

| RFC | Disposition | Why It Is In This Bucket | Next Action |
| :--- | :--- | :--- | :--- |
| RFC-0019 | Complete status/integration closure | Runtime/spec/CLI already rely on canonical match metadata and guard-audit strings | Keep normative references aligned and treat new metadata changes as RFC-impacting |
| RFC-0020 | Complete status/integration closure | Runtime/spec/CLI already rely on canonical segment-trace strings | Keep verbatim trace strings stable and route incompatible changes through a new RFC |
| RFC-0022 | Complete status/integration closure | `t81 policy compile/run`, grammar, and binary policy flow already exist | Treat RFC-0009 as historical provenance and evolve APL through RFC-0022 |
| RFC-0025 | Complete operational closure | `TLOADHASH` and `allowed-tensor-hashes` are active repo surfaces | Continue CI/provenance hardening without reopening design status |
| RFC-0026 | Complete operational closure | Opcode/spec/runtime surface is real, deterministic kernel tightening is in place, and the remaining open item is narrow phase-1 promotion review rather than broad math cleanup | Keep the residual inventory current, decide whether `WLOAD` needs additional hardening before final closure, and treat broader float-domain work as RFC-0030 scope |
| RFC-0027 | Complete status/integration closure | `spec/conformance` is wired into CMake/CTest and used as executable conformance infrastructure | Advance optional language annotations separately from the core acceptance status |

## Keep In Backlog

| RFC | Disposition | Why It Stays Here | Next Action |
| :--- | :--- | :--- | :--- |
| RFC-0000 | Backlog umbrella | Foundational umbrella with open long-tail items; not a clean release gate by itself | Break remaining work into targeted implementation tickets |
| RFC-0001 | Backlog architecture | Mostly architectural framing; does not currently gate shipped behavior | Integrate selectively into normative docs when wording changes are needed |
| RFC-0002 | Backlog contract maintenance | Important contract, but better maintained through concrete enforcement work than RFC-chasing | Track violations through determinism/hardening backlogs |
| RFC-0003 | Backlog contract maintenance | Same pattern as RFC-0002 for Axion safety model | Advance only when safety semantics or enforcement boundaries actually change |
| RFC-0004 | Backlog contract maintenance | Large semantic surface; only specific dependent deltas should block work | Use focused follow-on RFCs/issues for concrete tensor semantic changes |
| RFC-0007 | Backlog surface expansion | Standard-library scope is broad and not a single release gate | Deliver per-module promotion and fixture coverage incrementally |
| RFC-0032 | Backlog roadmap/governance | Useful promotion roadmap, but should reflect implementation reality rather than block it | Update as promotion decisions change; do not treat as a prerequisite for core fixes |
| RFC-0033 | **Accepted** — All 4 phases complete | `t81 studio`, `t81 agent`, `t81 ui` shipped; FTXUI v5.0.0 via FetchContent; CanonFS Browser, Axion Dashboard, Trace Visualizer, command palette, session persistence, trit-probability bar wired to `llama-run` token output, snapshot tests, binary-size CI gate, user guide at `docs/user-guide/how-to/tui-guide.md` | Future work: custom theming, mouse support, distributed monitoring (see RFC Future Work section) |
| RFC-0030 | Backlog deterministic math subsystem | Necessary for true cross-arch deterministic replacement of host-float transcendental and similar math, but broader than current phase-1 AI opcode closure | Use it to track dmath/soft-math replacements (`exp`, `sqrt`, `log`, division-heavy float paths) rather than treating those as simple RFC-0026 cleanup |
| RFC-00A0 to RFC-00A8 | Backlog experimental track | These RFCs govern experiment lanes; implementation presence alone does not imply core-promotion urgency | Keep sandboxed unless a specific component is being promoted into core |

## Rule Of Thumb

- If the repo already depends on an RFC for released behavior, determinism
  claims, or public CLI/spec behavior, finish or reconcile it now.
- If the RFC is exploratory, umbrella, or experimental, keep it in backlog
  until a concrete promotion or release dependency appears.
