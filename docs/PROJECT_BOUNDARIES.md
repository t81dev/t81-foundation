# T81 Project Boundaries

> **Research Posture Alignment:** Per [`AGENTS.md`](../AGENTS.md) and [`docs/releases/v2.1.0.md`](releases/v2.1.0.md), T81 Foundation is an experimental computing architecture and systems research platform. "Stable Core" denotes strictly verified, fail-closed, deterministic software surfaces—not commercial production readiness, general ternary superiority, or physical ternary hardware advantage.

## **Stable Core (Verified / Bounded Surfaces)**

### What IS Stable Core:
- **Deterministic VM**: Bit-exact reproducible interpreter execution + Dual-Rail `(P, N)` / `SBDR1T2D` fast paths (DCP Verified for interpreter path)
- **Axion Policy Engine**: Fail-closed pre-side-effect governance (Stable Bounded, governed non-DCP)
- **CanonFS**: Immutable content-addressed storage with `RFC-00D1` v1 interchange seed and provenance (Stable Bounded, governed non-DCP)
- **Native C++ MCP Control Plane**: 15-tool `RFC-00D8` JSON-RPC server with `--root` jail and `tx_begin`/`tx_commit`/`tx_abort` isolation (Stable Bounded)
- **Bounded AI OS-Object Family**: `assess-fixed`, `route-fixed`, `classify-fixed` (Frozen 3-member baseline)
- **Decision Bundles & Provider Evidence**: Draft `RFC-00D4` bundles and `M1` `ProviderGovernor` (`t81.provider.evidence.v1`, staged under `src/provider/`)

### Location:
- `src/core/`, `src/vm/`, `src/axion/`, `src/isa/`, `vm/`, `kernel/axion/`, `fs/`
- `tools/cli/core/` (essential CLI and native C++ MCP server)
- `include/t81/` (locked public APIs: `PUBLIC_API_LOCK.json` `v2.2.0`)
- `tests/cpp/` (core test suite)

### Status:
- **432+ configured CTest targets** (100% green post-merge CI on `main@8aa086906`)
- **RFC-00D1 CanonFS interchange**: Stable v1 seed contract (RFC remains draft overall)
- **TISC ISA v1.9.0**: Frozen specification (`PUBLIC_API_LOCK.json` `v2.2.0`)

---

## **Experimental Frontier (Research-Grade)**

### What IS Experimental:
- **AI OS concepts**: Extended operating system features
- **Marketplace functionality**: Economic/transaction systems
- **Advanced governance**: Multi-tier policy frameworks
- **Hardware acceleration**: Ternary computing prototypes
- **Marketing demos**: Showcase and proof-of-concept

### Location:
- `experimental/` (all research prototypes)
- `tools/cli/experimental/` (experimental CLI features)
- `tools/cli/bundle_tools/` (advanced bundle concepts)
- `tools/cli/ai_tools/` (AI-specific tooling)
- `experimental/marketing_arguments/` (positioning materials)

### Status:
- **Not production-tested**: No comprehensive test coverage
- **Speculative features**: Forward-looking research
- **May change**: No API stability guarantees

### **Frozen / Non-Blocking Experimental Prototypes (Pruned from Core CI Gates)**

| Prototype / Draft | Classification | Rationale |
| :--- | :--- | :--- |
| **`Hanoi VM` (`experimental/hanoi/`)** | **Frozen / Non-Blocking Experimental** | Historical CWEB lineage retained for research continuity; outside DCP and non-blocking for core CI. |
| **Partial `c`/`rust`/`python` MLIR/LLVM subset lowerings** | **Frozen / Non-Blocking Experimental** | Source-inspected research lowerings; do not inherit CPA or DCP parity. |
| **Unratified Draft RFCs (`RFC-00D0`, `RFC-00D2`, `RFC-00D5`)** | **Frozen / Non-Blocking Experimental** | Design-led / informational proposals (`RFC-00D4` and `RFC-00D6` are ratified `Accepted — Governed non-DCP`). |

---

## **Decision Criteria**

### **Stable Core Addition Criteria:**
1. **100% test coverage** with deterministic verification
2. **RFC approval** through formal process
3. **Production use case** with proven value
4. **API stability** with backward compatibility
5. **Deterministic guarantees** - bit-exact reproducibility required

### **Experimental Retention Criteria:**
1. **Research value** for future development
2. **Clear documentation** of experimental nature
3. **No stable API dependencies** (isolated)
4. **Explicit disclaimers** in all user-facing materials
5. **May explore nondeterminism** - RESEARCH ONLY (never in production)

### **Critical Boundary: Determinism vs Nondeterminism**

#### **Stable Core T81 Runtime: ALWAYS DETERMINISTIC (DCP-Bounded)**
- **Guarantee**: Bit-exact reproducibility on surfaces verified in `DETERMINISM_SURFACE_REGISTRY.md`
- **Target Domains**: Regulated and high-assurance audit research (finance, healthcare, legal)
- **Use Cases**: Governed execution and verifiable audit trails
- **API Contract**: Deterministic execution guaranteed within DCP scope

#### **Experimental Research: MAY EXPLORE NONDETERMINISM**
- **Purpose**: Research and education only
- **Location**: `tools/experimental/nondeterministic_research.cpp`
- **Disclaimers**: Prominent warnings about experimental status
- **No Stable Core Use**: Never integrated into DCP-certified runtime paths
- **Educational Value**: Demonstrates WHY determinism matters

---

## **Usage Guidelines**

### **For Stable Core Users:**
- Use **stable core** components only
- Rely on **registry-verified** deterministic functionality
- Follow **RFC-governed** APIs (`PUBLIC_API_LOCK.json`)
- Expect **DCP-bounded deterministic guarantees**

### **For Researchers/Contributors:**
- **Experimental directory** available for exploration
- **Clear separation** from stable APIs
- **No stability guarantees** in experimental code
- **Contributions welcome** through RFC process

---

## **Project Positioning**

### **Primary Identity**: **"Deterministic AI Runtime"**
- **Accurate**: Describes what actually works
- **Tested**: Backed by 100% test coverage
- **Valuable**: Solves real governance and determinism problems

### **Secondary Identity**: **"AI Operating System Research"**
- **Aspirational**: Long-term research direction
- **Experimental**: No production guarantees
- **Optional**: For research and exploration only

This boundary clarification reduces confusion, focuses development effort, and sets clear user expectations.
