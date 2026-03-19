# RFC-0056: Google Axion-T81 Integration Contract

**Status:** draft
**Type:** standards-track
**Applies-To:** Google Axion processor integration, ARM-based hardware targets, cloud-native T81 deployment
**Created:** 2026-03-19
**Updated:** 2026-03-19
**Supersedes:** None
**Superseded-By:** None
**Discussion:** Builds on RFC-0000, RFC-0055, RFC-00B0, RFC-0042, and RFC-0047

---

## Summary

This RFC defines the integration contract for deploying T81 on Google Axion processors, establishing a governed path for leveraging Axion's ARM Neoverse V2 architecture while preserving T81's ternary semantic guarantees and determinism requirements.

It specifies:

- The integration mode for Axion as a **verified lowering target** within the T81 hardware target framework
- Hardware interop layer requirements for ARM-to-ternary semantic preservation
- Titanium offload integration boundaries and governance requirements
- Cloud-native deployment patterns for T81 on Axion-based C4A/N4A instances
- Performance optimization strategies while maintaining determinism

## Motivation

Google Axion represents the first production-ready, cloud-scale ARM processor specifically optimized for AI workloads. Key characteristics make it an attractive target for T81:

- **ARM Neoverse V2 architecture** with proven performance for AI inference and general computing
- **Titanium offload infrastructure** that can accelerate platform operations while preserving T81 governance
- **Cloud-native deployment** through Google Compute Engine, GKE, and related services
- **Energy efficiency** gains of up to 60% over comparable x86 instances
- **AI workload optimization** with demonstrated 2.5-3x performance improvements for RAG and recommendation systems

However, T81's ternary semantic model and determinism requirements create specific integration challenges that must be addressed through a governed contract rather than ad hoc porting efforts.

## Proposal

### 1. Integration Mode Classification

Google Axion is classified as a **Verified Lowering Target** under RFC-0055, with the following characteristics:

- **Target ISA:** ARMv9 (Neoverse V2)
- **Integration Mode:** Verified Lowering from TISC to ARM64
- **Hardware Class:** General-purpose native target (not narrow accelerator)
- **Deployment Environment:** Cloud-native (Google Cloud Platform)

This classification requires:
- A verified TISC-to-ARM64 lowering layer
- Semantic equivalence proof under RFC-0042
- Full preservation of Axion governance and trace semantics

### 2. Hardware Interop Layer Specification

#### 2.1 Boot and Runtime Environment

The Axion interop layer MUST define:

- **Container-based deployment** using Container-Optimized OS or compatible ARM64 Linux distributions
- **T81 runtime isolation** through container boundaries while preserving inter-instance determinism
- **Ethics-first boot gating** implemented at container startup rather than hardware boot
- **Privilege separation** using standard Linux container security models augmented by T81 capability grants

#### 2.2 Memory and Addressing Contract

Axion integration MUST specify:

- **Virtual memory model** using standard ARM64 page tables with T81-specific allocation patterns
- **Ternary data representation** mapped to ARM64 binary words with canonical packing rules
- **Alignment requirements** for T81 data structures (81-trit words aligned to 64-bit boundaries)
- **DMA rules** for Titanium offload interactions, ensuring no bypass of T81 governance

#### 2.3 Titanium Offload Integration

Titanium offloads MUST be integrated under T81 governance:

- **Network offloads** may be used for performance but MUST be auditable through T81 trace hooks
- **Storage offloads** (Hyperdisk) MUST preserve CanonFS object identity and semantics
- **Security offloads** MUST NOT bypass Axion policy evaluation or capability checks
- **Performance monitoring** from Titanium may be recorded as diagnostics only, not as deterministic state

### 3. TISC-to-ARM64 Lowering Requirements

#### 3.1 Semantic Preservation

The lowering layer MUST provide:

- **Bit-exact equivalence** for all TISC arithmetic operations on supported data types
- **Deterministic ordering** for memory operations and side effects
- **Exception preservation** ensuring TISC faults map to appropriate ARM64 exceptions
- **Trace continuity** maintaining TISC-level trace semantics in ARM64 execution

#### 3.2 Performance Optimization Strategy

Optimizations MUST respect determinism:

- **SIMD utilization** (Neon) for vectorized ternary operations with verified equivalence
- **Cache-aware layout** for T81 data structures without changing observable behavior
- **Branch prediction hints** that do not affect control flow semantics
- **Speculative execution** limited to non-side-effecting operations

### 4. Cloud-Native Deployment Patterns

#### 4.1 Instance Configuration

Recommended Axion instance configurations for T81:

- **C4A standard** (1:4 vCPU:memory) for general T81 workloads
- **C4A high-memory** (1:8) for memory-intensive ternary computations
- **N4A** (Neoverse N3) for cost-optimized deployments
- **Tier_1 networking** (100 Gbps) for distributed T81 operations

#### 4.2 Orchestration Integration

T81 on Axion MUST integrate with:

- **Google Kubernetes Engine** for containerized T81 deployments
- **Google Cloud Batch** for large-scale T81 job processing
- **Dataproc/Dataflow** for T81-based analytics pipelines
- **Cloud Load Balancing** for T81 service distribution

### 5. Determinism and Governance Preservation

#### 5.1 Trace and Audit Requirements

Axion deployments MUST preserve:

- **Canonical trace semantics** across container and instance boundaries
- **Axion audit hook firing** for all policy-relevant events
- **CanonFS object identity** when using Hyperdisk or other storage services
- **Deterministic error classification** at ARM64 exception boundaries

#### 5.2 Performance Monitoring Separation

Performance monitoring MUST be separated from deterministic state:

- **Titanium performance counters** recorded as external diagnostics
- **ARM PMU events** used only for optimization, not as input to T81 logic
- **Google Cloud monitoring** integrated without affecting T81 determinism
- **Custom metrics** exported through T81's canonical monitoring interfaces

### 6. Compatibility Profile

#### 6.1 Supported TISC Features

The Axion compatibility profile MUST specify:

- **Fully supported:** Core arithmetic, tensor operations, control flow, memory management
- **Partially supported:** Advanced SIMD operations (with fallback to scalar)
- **Not supported:** Hardware-specific ternary operations (emulated in software)
- **Fallback behavior:** Deterministic software emulation for unsupported features

#### 6.2 Conformance Requirements

Axion integration MUST provide:

- **RFC-0042 equivalence proof** for all supported TISC operations
- **RFC-0043 conformance matrix** demonstrating 100% pass on standard corpus
- **RFC-0045/0046 memory ordering** guarantees in ARM64 execution environment
- **RFC-0048 boundary updates** for Axion-specific deployment considerations

## Implementation Plan

### Phase 1: Foundation (Experimental)
1. **TISC-to-ARM64 lowering layer** implementation with basic equivalence testing
2. **Container runtime** for T81 on Axion with governance preservation
3. **Titanium integration** proof-of-concept with audit hooks
4. **Basic conformance testing** against RFC-0043 corpus

### Phase 2: Optimization (Governed Non-DCP)
1. **Neon SIMD optimizations** for ternary vector operations
2. **Cache-aware data layout** for improved performance
3. **GKE integration** for orchestrated T81 deployments
4. **Performance benchmarking** against x86 baseline

### Phase 3: Production (DCP-Eligible)
1. **Full RFC-0042 equivalence proof** completion
2. **Comprehensive conformance testing** across all supported features
3. **Production deployment guides** and best practices
4. **Performance optimization** for specific T81 workload patterns

## Security Considerations

### ARM64-Specific Concerns
- **Speculative execution** vulnerabilities must be mitigated without affecting T81 determinism
- **Cache timing attacks** prevented through consistent memory access patterns
- **Side-channel resistance** maintained through deterministic execution paths

### Cloud Environment Security
- **Container isolation** augmented by T81 capability system
- **Network security** integrated with Axion policy evaluation
- **Data protection** ensured through CanonFS encryption and access controls

## Performance Expectations

Based on Google's published Axion benchmarks and T81 characteristics:

- **AI inference workloads:** 2-3x improvement over x86 for T81-based AI operations
- **General ternary computing:** 30-50% improvement for memory-bound operations
- **Energy efficiency:** Up to 60% reduction in energy consumption per operation
- **Cloud deployment benefits:** Native integration with Google Cloud services and tooling

## Backwards Compatibility

This RFC is additive and does not affect existing T81 deployments on other platforms. Existing T81 code will run unchanged on Axion with appropriate runtime support.

## Open Questions

1. **Determinism in distributed environments:** How to ensure cross-instance determinism when using cloud load balancers and auto-scaling?
2. **Titanium offload granularity:** What level of Titanium offload can be safely used without compromising T81 governance?
3. **Performance monitoring integration:** How to best leverage Google Cloud monitoring while preserving T81's deterministic boundaries?
4. **Multi-region deployment:** What are the determinism implications for T81 deployments across multiple Google Cloud regions?

## Acceptance Criteria

- [ ] TISC-to-ARM64 lowering layer implements verified semantic equivalence
- [ ] Axion hardware interop layer preserves T81 governance and trace semantics
- [ ] Titanium offload integration maintains Axion policy enforcement
- [ ] Cloud-native deployment patterns support scalable T81 operations
- [ ] Conformance testing demonstrates 100% compatibility with T81 specification
- [ ] Performance improvements meet or exceed published Axion benchmarks
- [ ] Security boundaries prevent bypass of T81 capability and policy systems

## References

- [RFC-0000: T81 Base-81 Ternary Computing Stack](RFC-0000-t81-base-81-ternary-computing-stack.md)
- [RFC-0055: Native Ternary Hardware Target and Interop Contract](RFC-0055-native-ternary-hardware-target-and-interop-contract.md)
- [Google Axion Processor Documentation](https://cloud.google.com/blog/products/compute/introducing-googles-new-arm-based-cpu)
- [ARM Neoverse V2 Architecture](https://www.arm.com/architecture/server/neoverse)
- [Google Cloud Titanium Infrastructure](https://cloud.google.com/titanium)
