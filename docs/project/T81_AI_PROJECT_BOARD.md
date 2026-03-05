# T81 AI Integration - Project Board

## Board Configuration

| Column | Description |
|---------|-------------|
| **Backlog** | Tasks not yet ready for implementation |
| **RFC Approved** | RFCs approved and ready for task breakdown |
| **In Progress** | Currently being implemented |
| **Determinism Validation** | Awaiting determinism validation results |
| **Review** | Code review pending |
| **Completed** | Implementation complete and merged |

## Phase 1: AI Foundation (Weeks 1-6)

### RFC Approved
- [x] RFC-00A0: AI Experiment Sandbox and Repository Boundaries
- [x] RFC-00A1: Deterministic Evidence and Reproducibility Protocol
- [x] RFC-00A3: Model Artifact Identity and Provenance

### In Progress
- [ ] **Task 1**: Implement AI experiment sandbox infrastructure
  - *Issue*: #AI-001
  - *Assignee*: TBD
  - *Milestone*: Foundation
- [ ] **Task 2**: Create promotion gate system
  - *Issue*: #AI-002
  - *Assignee*: TBD
  - *Milestone*: Foundation
- [ ] **Task 3**: Build deterministic evidence collection framework
  - *Issue*: #AI-003
  - *Assignee*: TBD
  - *Milestone*: Foundation
- [ ] **Task 5**: Implement model provenance system
  - *Issue*: #AI-004
  - *Assignee*: TBD
  - *Milestone*: Foundation

### Backlog
- [ ] **Task 4**: Create standard benchmark suite
  - *Issue*: #AI-005
  - *Milestone*: Foundation
  - *Dependencies*: Task 3

---

## Phase 2: AI Integration Layer (Weeks 7-18)

### RFC Approved
- [ ] RFC-00A4: Ternary Quantization Codec Contract
- [ ] RFC-00A5: LLM Backend Adapter Interface
- [ ] RFC-00A6: Axion Policy Hooks for AI Events

### Backlog
- [ ] **Task 6**: Implement ternary quantization codecs
  - *Issue*: #AI-006
  - *Assignee*: TBD
  - *Milestone*: Integration
  - *Dependencies*: Tasks 1, 3
- [ ] **Task 7**: Build engine-agnostic LLM backend system
  - *Issue*: #AI-007
  - *Assignee*: TBD
  - *Milestone*: Integration
  - *Dependencies*: Tasks 1, 3, 5
- [ ] **Task 8**: Implement AI policy hooks
  - *Issue*: #AI-008
  - *Assignee*: TBD
  - *Milestone*: Integration
  - *Dependencies*: Tasks 1, 5, 7

---

## Phase 3: AI Experience Layer (Weeks 19-24)

### RFC Approved
- [ ] RFC-00A2: AI Benchmark Specification and Reporting Format
- [ ] RFC-00A7: UX Integration for AI in T81

### Backlog
- [ ] **Task 9**: Build AI CLI and observability tools
  - *Issue*: #AI-009
  - *Assignee*: TBD
  - *Milestone*: Experience
  - *Dependencies*: Tasks 1, 3, 7, 8
- [ ] **Task 10**: Implement core AI CLI commands
  - *Issue*: #AI-010
  - *Assignee*: TBD
  - *Milestone*: Experience
  - *Dependencies*: Task 9

---

## Phase 4: Experimental Optimization (Ongoing)

### RFC Approved
- [x] RFC-00A8: AI-Native VM Opcode Exploration *(Permanent Experimental)*

### Backlog
- [ ] **Task 11**: Explore AI-native VM opcodes
  - *Issue*: #AI-011
  - *Assignee*: TBD
  - *Milestone*: Optimization
  - *Dependencies*: Tasks 1, 3, 6, 7
  - *Labels*: research, experimental, high-risk

---

## Determinism Validation Queue

Tasks awaiting determinism validation results:
- Task 3: Evidence collection framework
- Task 6: Ternary quantization codecs
- Task 7: LLM backend system
- Task 8: AI policy hooks
- Task 11: VM opcodes

## Review Queue

Tasks pending code review:
- Task 1: Sandbox infrastructure
- Task 2: Promotion gates
- Task 5: Model provenance

## Completed

*None yet - implementation phase beginning*

---

## Issue Templates

### New Feature Task
```markdown
## RFC Reference
- **RFC**: RFC-00AX

## Problem Description
Brief description of what needs to be implemented.

## Implementation Scope
- **Affected Directories**: 
- **Dependencies**: 
- **Estimated Effort**: 

## Acceptance Criteria
- [ ] Criterion 1
- [ ] Criterion 2
- [ ] Criterion 3

## Determinism Validation
- [ ] Cross-platform testing
- [ ] Statistical variance analysis
- [ ] Hash consistency verification

## Related Issues
- Depends on: #AI-XXX
- Blocks: #AI-YYY
```

### Bug Fix
```markdown
## Problem Description
Clear description of the bug and its impact.

## Steps to Reproduce
1. Step one
2. Step two
3. Step three

## Expected Behavior
What should happen.

## Actual Behavior
What actually happens.

## Environment
- OS: 
- T81 Version:
- Hardware:

## Additional Context
Any relevant logs, traces, or configuration.
```

## Working Agreement

### Branch Naming
- `feature/ai-<task-name>` for new features
- `fix/ai-<task-name>` for bug fixes
- `rfc/ai-<rfc-number>` for RFC implementations

### PR Requirements
- All CI checks must pass
- Determinism validation results included
- Documentation updated
- Code review from at least one maintainer

### Definition of Done
- Acceptance criteria met
- Determinism validation passed
- Code review approved
- Merged to main branch
