# Contributing to T81 Foundation

Thank you for your interest in contributing to the T81 Foundation project.  
This repository defines the core specifications for the entire T81 Ecosystem and must remain consistent, deterministic, and formally defined.  
All contributions should follow the guidelines below to maintain coherence and correctness.

---

## 1. Contribution Types

### **1.1 Specification Proposals (RFCs)**
For new features, architectural changes, or modifications to existing T81 components:

- Submit an issue using the **RFC template**.
- Provide a formal definition of rules, semantics, invariants, and constraints.
- Include rationale, alternatives considered, and compatibility notes.
- After discussion, RFCs may be accepted and merged into the `spec/` directory.

### **1.2 Bug Reports**
For errors, inconsistencies, or conflicting rules:

- Use the **Bug Report template**.
- Clearly identify the section and expected vs. actual behavior.
- Provide minimal examples if helpful.

### **1.3 Clarifications**
If any part of the specification is underspecified or ambiguous:

- Use the **Spec Clarification** template.
- Describe the ambiguity and why it matters for implementation.

---

## 2. Workflow for Changes

### **2.1 Open an Issue**
All contributions begin as issues (RFC, bug, clarification).  
No direct pull request without an associated issue.

### **2.2 Discussion Phase**
The community reviews and refines the issue until reaching consensus.  
Changes may require:

- revisions to wording  
- updated invariants  
- compatibility checks  
- additional examples  

### **2.3 Draft Pull Request**
Once discussion stabilizes:

- open a PR linking to the issue  
- include changes to relevant files in `spec/`, `docs/`, or `tools/`  
- follow the formatting conventions defined in the spec files  

### **2.4 Review & Approval**
A change is approved when:

- all raised concerns are resolved  
- it does not introduce undefined or nondeterministic behavior  
- it maintains logical and ethical constraints  
- it adheres to Axion invariants (where relevant)  

### **2.5 Merge**
After approval, the PR is merged and the related issue is closed.

---

## 3. Style & Formatting Guidelines

### **3.1 Markdown Structure**
- Use consistent heading levels.  
- Keep lines under ~120 characters.  
- Use code blocks for formal definitions.  

### **3.2 Specification Tone**
- Use clear, unambiguous, normative language.  
- MUST / MUST NOT define strict rules.  
- SHOULD / SHOULD NOT express recommended practice.  
- MAY indicates optional but valid behavior.  

### **3.3 Determinism Requirement**
All contributions must uphold:

- deterministic semantics  
- base-81 and balanced ternary correctness  
- zero undefined behavior  
- reproducibility across implementations  

---

## 4. Repository Structure Rules

### **spec/**  
Formal, normative documents.  
Changes require RFC-level justification.

### **docs/**  
Explanatory or supporting documentation.  
May evolve more flexibly.

### **examples/**  
Minimal programs illustrating T81 concepts.  
Must remain deterministic and spec-compliant.

### **tools/**  
Validation or utility scripts.  
Must not introduce nondeterministic behavior.

---

## 5. Code of Conduct
Contributors are expected to maintain a professional, respectful environment.  
Focus comments and discussion on technical accuracy, clarity, and iterative improvement.

---

## 6. License
All contributions are accepted under the **Apache 2.0 License**, consistent with the repository’s LICENSE file.

---

Thank you for helping build the foundation of the T81 Ecosystem.
