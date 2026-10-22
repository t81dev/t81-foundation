# T81 Non-Deterministic Inference Strategy

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Non-Deterministic Inference Strategy](#t81-non-deterministic-inference-strategy)
  - [Overview](#overview)
  - [Core Principles](#core-principles)
    - [1. Determinism First](#1-determinism-first)
    - [2. Architectural Separation](#2-architectural-separation)
  - [Implementation Strategy](#implementation-strategy)
    - [Phase 1: Policy Framework Enhancement](#phase-1-policy-framework-enhancement)
      - [AI Inference Policies](#ai-inference-policies)
      - [Evidence Collection](#evidence-collection)
    - [Phase 2: AI Backend Architecture](#phase-2-ai-backend-architecture)
      - [Core Components](#core-components)
      - [Configuration Schema](#configuration-schema)
    - [Phase 3: Integration Points](#phase-3-integration-points)
      - [CLI Integration](#cli-integration)
- [Enable non-deterministic AI with explicit consent](#enable-non-deterministic-ai-with-explicit-consent)
- [Strict deterministic mode (default)](#strict-deterministic-mode-default)
- [External AI integration](#external-ai-integration)
      - [VM Integration](#vm-integration)
  - [Benefits](#benefits)
  - [Implementation Timeline](#implementation-timeline)
  - [Risk Mitigation](#risk-mitigation)
    - [Technical Risks](#technical-risks)
    - [Mitigation Strategies](#mitigation-strategies)
  - [Success Metrics](#success-metrics)
    - [Determinism Compliance](#determinism-compliance)
    - [Performance Targets](#performance-targets)
    - [Security & Compliance](#security-&-compliance)

<!-- T81-TOC:END -->


## Overview

This document outlines T81's approach to supporting non-deterministic AI inference while maintaining the project's core deterministic identity.

## Core Principles

### 1. Determinism First
T81's fundamental value proposition is deterministic execution. Any non-deterministic features must be:
- **Explicitly enabled** - Never active by default
- **Policy controlled** - Governed by Axion policy engine
- **Fully auditable** - Complete evidence trail for all operations
- **Bounded impact** - Limited to specific, controlled contexts

### 2. Architectural Separation
```
┌─────────────────────────────────────────────────────────┐
│           T81 Core (Deterministic)              │
│  ┌─────────────────────────────────────┐    │
│  │ t81_vm (deterministic)        │    │
│  │ t81_canonfs (deterministic)    │    │
│  │ t81_axion (deterministic)      │    │
│  └─────────────────────────────────────┘    │
│                                         │
│           ┌─────────────────────────────┐    │
│    │ t81_ai_backend (controlled)   │    │
│    │  - Non-deterministic inference  │    │
│    │  - Evidence collection          │    │
│    │  - Policy enforcement          │    │
│    │  - External AI integration       │    │
│    └─────────────────────────────────────┘    │
│                                         │
│           External AI Services (Optional)     │
└─────────────────────────────────────────────────────────┘
```

## Implementation Strategy

### Phase 1: Policy Framework Enhancement

#### AI Inference Policies
Add new Axion policy types for AI operations:

```apl
policy "ai.inference.allow_non_deterministic" {
    description: "Allow non-deterministic AI inference operations",
    conditions: [
        "operation.context == 'ai.inference'",
        "operation.purpose == 'inference'",
        "user.has_explicit_consent == true"
    ],
    actions: ["allow", "audit", "log_evidence"]
}

policy "ai.inference.deny_non_deterministic" {
    description: "Deny non-deterministic AI inference operations", 
    conditions: [
        "operation.context == 'ai.inference'",
        "user.has_explicit_consent == false"
    ],
    actions: ["deny", "log_attempt"]
}
```

#### Evidence Collection
Extend `t81_ai_evidence` framework for AI operations:
- Operation context tracking
- Non-determinism markers
- External service calls logging
- Performance metrics collection
- User consent verification

### Phase 2: AI Backend Architecture

#### Core Components

1. **Deterministic Interface Layer**
   - `t81_ai_backend` with configurable determinism level
   - Fallback to deterministic VM when non-determinism disabled
   - Evidence collection for all operations

2. **Policy Enforcement Layer**
   - Integration with Axion policy engine
   - Real-time policy validation
   - Audit logging for policy violations

3. **External AI Integration**
   - Configurable external AI service endpoints
   - Request/response logging
   - Fallback to local models when external services unavailable

#### Configuration Schema
```json
{
  "determinism_level": "strict|controlled|permissive",
  "external_ai_endpoint": "optional|disabled",
  "evidence_collection": "enabled|disabled",
  "audit_logging": "enabled|disabled"
}
```

### Phase 3: Integration Points

#### CLI Integration
```bash
# Enable non-deterministic AI with explicit consent
t81 ai infer --model my_model --allow-non-deterministic --user-consent

# Strict deterministic mode (default)
t81 ai infer --model my_model --determinism strict

# External AI integration
t81 ai infer --model my_model --external-ai-endpoint https://api.ai-service.com
```

#### VM Integration
```cpp
// AI inference with controlled non-determinism
class AITask {
    determinism_level level;
    bool external_ai_enabled;
    // ... AI operation implementation
};

Result result = vm.execute_task(ai_task, {
    "determinism": "controlled",
    "allow_external_ai": policy_check("ai.inference.allow_non_deterministic"),
    "collect_evidence": true
});
```

## Benefits

1. **Preserves Core Identity**: T81 remains fundamentally deterministic by default
2. **Practical AI Integration**: Enables real-world AI capabilities when needed
3. **Policy Control**: Axion governs all AI operations with fine-grained control
4. **Evidence Trail**: Complete auditability for compliance and debugging
5. **Gradual Adoption**: Can be rolled out incrementally without breaking changes
6. **User Control**: Explicit consent mechanisms for privacy and compliance

## Implementation Timeline

- **Week 1-2**: Policy framework enhancement and evidence collection
- **Week 3-4**: AI backend with controlled non-determinism
- **Week 5-6**: CLI integration and external AI service support
- **Week 7-8**: Comprehensive testing and documentation

## Risk Mitigation

### Technical Risks
- **Complexity**: Added architectural layers increase system complexity
- **Performance**: Policy enforcement adds computational overhead
- **Security**: External AI integration expands attack surface

### Mitigation Strategies
- **Explicit Defaults**: Non-deterministic features disabled by default
- **Policy Gates**: Require explicit policy approval for AI operations
- **Evidence Requirements**: Mandatory audit trail for all AI operations
- **Staged Rollout**: Gradual deployment with extensive testing

## Success Metrics

### Determinism Compliance
- 100% of core operations remain deterministic
- AI operations clearly marked as non-deterministic
- Evidence collection covers 100% of AI interactions

### Performance Targets
- <5% overhead for policy enforcement
- <10% latency impact for evidence collection
- Zero impact when AI features disabled

### Security & Compliance
- All AI operations logged and auditable
- User consent verified before non-deterministic operations
- External AI interactions controlled through policy gateway

This strategy enables T81 to support practical AI workloads while maintaining its fundamental commitment to determinism and auditability.
