# T81 Project Boundaries

## **Stable Core (Production-Ready)**

### What IS Stable Core:
- **Deterministic VM**: Bit-exact reproducible execution (100% tested)
- **Axion Policy Engine**: Pre-side-effect governance (100% tested)
- **CanonFS**: Immutable storage with provenance (100% tested)
- **Decision Bundles**: Verifiable AI task results (100% tested)
- **Bounded AI OS-Object Family**: assess-fixed, route-fixed, classify-fixed (100% tested)

### Location:
- `src/core/`, `src/vm/`, `src/axion/`, `src/isa/`
- `tools/cli/core/` (essential CLI only)
- `include/t81/` (public APIs)
- `tests/cpp/` (core test suite)

### Status:
- **393/393 tests passing** (100%)
- **RFC-00D1 CanonFS interchange**: Stable v1 contract
- **TISC ISA v1.9.0**: Frozen specification

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

#### **Production T81 Runtime: ALWAYS DETERMINISTIC**
- **Guarantee**: Bit-exact reproducibility
- **Markets**: Regulated industries (finance, healthcare, legal)
- **Use Cases**: Production systems requiring audit trails
- **API Contract**: Deterministic execution guaranteed

#### **Experimental Research: MAY EXPLORE NONDETERMINISM**
- **Purpose**: Research and education only
- **Location**: `tools/experimental/nondeterministic_research.cpp`
- **Disclaimers**: Prominent warnings about experimental status
- **No Production Use**: Never integrated into production runtime
- **Educational Value**: Demonstrates WHY determinism matters

---

## **Usage Guidelines**

### **For Production Users:**
- Use **stable core** components only
- Rely on **100% tested** functionality
- Follow **RFC-governed** APIs
- Expect **deterministic guarantees**

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
