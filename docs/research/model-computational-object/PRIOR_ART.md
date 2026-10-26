# Prior art and bounded novelty assessment

Sources inspected 2026-10-25. Links below are primary papers, author material,
standards or implementer documentation. This is a scoped search, not a patent
search or proof that no equivalent abstraction exists. Product documentation
establishes declared facilities, not independent validation of every guarantee.

| Established approach / primary source | Relation to the proposal | What remains to be established |
| --- | --- | --- |
| Capability microkernel: [seL4 proofs](https://sel4.systems/Verification/proofs.html), [assumptions](https://sel4.systems/Verification/assumptions.html) | Explicit authority and isolation already have strong formal treatments. Verified configurations have precise theorem scopes and hardware/boot/other assumptions. | A model descriptor does not inherit the kernel proofs, policy correctness or protected-application correctness. |
| Runtime reference monitor: [Schneider, Enforceable Security Policies, Cornell TR99-1759](https://ecommons.cornell.edu/entities/publication/73f44364-f45e-40b5-b581-301a42619b0d) | Execution monitors characterize enforceable safety restrictions. A mediated effect boundary can stop a forbidden action. | It cannot ensure an LLM gives a useful answer or that an external event eventually succeeds. |
| Typed effects: [Leijen, Koka: Programming with Row-Polymorphic Effect Types, MSR-TR-2013-79](https://www.microsoft.com/en-us/research/publication/koka-programming-with-row-polymorphic-effect-types/) | Function signatures can expose effects, with semantics tied to a type system. | A JSON field describing capability is not such a theorem; opaque foreign calls still need enforcement. |
| Actors and messages: [Hewitt, Bishop, Steiger, 1973, in IJCAI proceedings bundle](https://www.ijcai.org/Proceedings/73/Papers/027.pdf), [Ray actor documentation](https://docs.ray.io/en/latest/ray-core/actors.html) | An addressable component can own state and accept messages. Ray implements stateful worker objects. | Scheduling, delivery order, trust and reproducibility need additional contracts. A model behind an actor is not a new computational class. |
| Deterministic state transitions: [Lamport, Computation and State Machines](https://lamport.azurewebsites.net/pubs/state-machine.pdf) | Make the wrapper transition relation deterministic by treating observed responses, timeouts and authority changes as explicit events. | Recording an input event permits replay of the wrapper; it does not reproduce the event's nondeterministic generator. |
| Proof-carrying computation: [Necula's author bibliography and PCC abstracts](https://people.eecs.berkeley.edu/~necula/papers.html) | An untrusted producer can accompany an artifact with evidence checked against a safety predicate. | A model rationale or citation is not a formal proof. A small verifier proves only its stated predicate under its own assumptions. |
| Evidence/provenance: [in-toto project](https://in-toto.io/) | Provenance can bind who performed which steps and in what order in a declared workflow. | Artifact integrity and attributed observations do not establish factual truth, inference quality or an unobserved external effect. |
| Model registry: [MLflow Model Registry](https://mlflow.org/docs/latest/ml/model-registry/) | Versioning, aliases, run lineage and metadata already exist; this is a close identity/lifecycle comparator. | A mutable alias must be resolved and pinned; a registry name alone is not a digest of weights or permission to act. |
| Inference runtime: [NVIDIA Triton model configuration](https://docs.nvidia.com/deeplearning/triton-inference-server/user-guide/docs/user_guide/model_configuration.html) | Backend declarations, tensor contracts and serving configuration already provide inspectable invocation boundaries. | Declared tensor shape does not establish semantic interchangeability, effect authorization or all resource bounds. |
| Reproducibility limits: [PyTorch 2.14 reproducibility notes](https://docs.pytorch.org/docs/2.14/notes/randomness.html) | Seeds and selected deterministic algorithms help within constrained conditions; cross-release/platform and CPU/GPU reproducibility are not generally promised. | Pinning a seed alone cannot justify bit-exact LLM replay, including greedy decoding. |
| Tool authorization: [MCP 2025-11-25 security guidance](https://modelcontextprotocol.io/docs/2025-11-25/tutorials/security/security_best_practices) | Confused-deputy and token-audience boundaries are established issues. Tool transport and discovery are distinct from valid authority. | Model-produced tool names, schemas or authorization text do not grant a capability. |
| Untrusted LLM proposals: [Beurer-Kellner et al., 2025, design patterns paper v1](https://arxiv.org/html/2506.08837v1) | Restricted action selection, quarantine and explicit information-flow boundaries directly address the proposal/effect separation. | Pattern guarantees depend on each threat model; a generic chat wrapper is not proven injection-resistant. |
| Multi-result authorization: [XACML 3.0 OASIS Standard, 22 January 2013](https://docs.oasis-open.org/xacml/3.0/xacml-3.0-core-spec-os-en.html), especially sections 7.10–7.14 and Appendix C | Permit, Deny, NotApplicable and Indeterminate, including extended indeterminate cases, precede this proposal. Combining behavior is policy-specific. | An arbitrary three-state collapse can lose the difference between no applicable policy and an evaluation error. |
| Conflicting evidence: [Sutcliffe, Pelletier and Hazen, AAAI 2018](https://cdn.aaai.org/ocs/17599/17599-77711-1-PB.pdf) | Their primary work on FDE describes True/False/Both/Neither and adds reasoning machinery. It provides a richer comparator than forcing every uncertainty into one trit. | An evidence logic is not automatically an authorization logic; this packet defines a separate fail-closed decision projection. |

The IJCAI URL is a multi-paper proceedings bundle: its beginning is Moore's
D-SCRIPT paper, while the actor paper appears later. It was not treated as a
standalone PDF with an actor title on page one. The original Belnap chapter was
not independently retrieved; the cited 2018 primary implementation paper supplies
the inspected four-valued comparator. No claim of a complete history of logic
or of access to unavailable artifacts is made.

## Closest abstraction and prospective contribution

The closest practical composite is a **versioned typed model service with a
reference monitor, explicit capability delegation, and an evidence envelope**.
Registries supply identity/lifecycle, serving runtimes supply invocation, and
ordinary service/process boundaries can enforce mediated effects. The component
set and the proposal/verifier separation already exist. Novel computation or an
OS requirement has not been established.

A possible T81 contribution is narrower: a reproducible conformance packet that
binds model replacement, output validation, final authorization, replay handling
and observation scope across providers. It would have to outperform or simplify
an equally explicit conventional composition at the same safety contract. Existing
T81 descriptor experiments already include a conventional comparator and negative
results; see [source inventory](RESEARCH_QUESTION.md). No unique contribution is
claimed merely for adding CanonFS hashes, a Model wrapper or three result names.

The semantic three-state algebra below is a small existing kind of ordered
composition, representable by an ordinary enum. Its potential human factors
benefit is testable; neither semantic elegance nor an extra result state proves
native-ternary storage, execution or hardware benefit.
