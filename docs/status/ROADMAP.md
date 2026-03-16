# Roadmap for the T81 Deterministic Ternary Stack

## Current strategic position

T81 Foundation, as described in your March 2026 status snapshot, is in a rare “maintenance-but-not-stagnant” posture: the deterministic substrate (frozen ISA + frozen data types) is stable, and the system-level differentiators (explicit governance interception, immutable provenance, deterministic parallelism) are already wired end-to-end. That matters because “determinism” is most valuable when it is **systemic**—when the compiler, runtime, storage, and governance plane agree on what reality is, and can prove it.

Balanced ternary is a credible mathematical and engineering foundation for this posture, not merely a philosophical aesthetic. Balanced ternary represents integers without a separate sign, and negation is achieved by swapping +1 and −1 trits (with 0 unchanged), avoiding the asymmetry of sign bits and reducing the conceptual surface area of “negative space.” That elegance has historical precedent: the Soviet Setun ternary computer (developed at entity["organization","Moscow State University","moscow, russia"] under entity["people","Sergei Sobolev","soviet mathematician"] and entity["people","Nikolay Brusentsov","soviet computer engineer"]) demonstrates that balanced ternary is not purely theoretical.

The “base‑e / radix economy” claim that often surrounds ternary is best treated as a **directional heuristic**, not a dispositive proof. In the classic formulation of optimal radix choice (cost proportional to base × number of digits), the continuous optimum is at \(e \approx 2.718\), and the best integer base is 3—yet the literature also notes that this is an asymptotic and model-dependent proxy for “efficiency,” not a guarantee of real-world advantage once device physics, noise margins, and system costs enter the picture.

Where T81 is unusually well-positioned is that it does not need to “win the hardware world” immediately to be strategically valuable. Today’s mainstream AI and HPC stacks still struggle to guarantee bitwise reproducibility across platforms and even across GPU architectures; for example, NVIDIA notes that cuDNN does not guarantee bitwise reproducibility across different GPU architectures. Mainstream frameworks offer determinism controls, but they are explicitly framed as tradeoffs and caveated by backend behavior: PyTorch documents that deterministic operations can be slower and not always available, implying that determinism is conditional rather than a universal contract. 

This creates a clear strategic thesis for the next decade: **be the reference stack for auditable, reproducible computation (including governed AI inference), first on commodity hardware via a deterministic VM contract, and later through selective hardware co-design.**

## External landscape shaping the next decade

The roadmap should be anchored to forces that are already “locked in” socially, economically, and technically.

A first force is the accelerating demand for **reproducibility and provenance** in software and ML supply chains. The Reproducible Builds community defines reproducibility as the ability for independent parties to recreate bit-for-bit identical artifacts from the same source, environment, and instructions—an operational definition that maps directly onto T81’s “bit-exact trace hash” aspiration (but extends it to the build pipeline itself).  In parallel, SLSA (a supply-chain security levels specification) formalizes “provenance” as verifiable information about what built an artifact, with what inputs and process. SLSA provenance is designed to interoperate with the entity["organization","in-toto","software supply chain framework"] attestation framework, which focuses on making supply chains transparent—what steps were performed, by whom, and in what order.  This is directly synergistic with CanonFS + audit trails, if T81 chooses to “speak” these emerging ecosystem formats rather than remain an island.

A second force is the rising baseline of **regulatory and standards pressure** for AI governance. The entity["organization","European Union","eu bloc"] AI Act timeline is explicit: it entered into force August 1, 2024; prohibited practices and AI literacy obligations applied February 2, 2025; obligations for general-purpose AI models applied August 2, 2025; and the Act becomes fully applicable August 2, 2026, with an extended transition for certain high-risk systems embedded in regulated products until August 2, 2027.  In the U.S. standards ecosystem, entity["organization","National Institute of Standards and Technology","us standards agency"] publishes the AI Risk Management Framework (AI RMF 1.0, January 2023) as a governance-oriented, lifecycle framework and notes it is intended to be a “living” document with a formal community review expected by 2028.  These timelines matter because they make “auditability-by-design” a market prerequisite rather than a luxury feature.

A third force is the inevitability of **post-quantum cryptography transition**, which strongly rewards deterministic execution and provenance (because cryptography migrations are brittle, compliance-driven, and verification-heavy). NIST’s post-quantum program states that FIPS 203/204/205 (derived from CRYSTALS‑Kyber, CRYSTALS‑Dilithium, and SPHINCS+) were published August 13, 2024, and that additional algorithms such as HQC have been selected for standardization (March 11, 2025).  The near-term operational consequence is that governments and regulated industries will spend the next decade demanding cryptographic assurances that are testable and repeatable.

A fourth force is the steady maturation of **ternary and multi-valued device research**, especially in compute-in-memory forms where multi-level states are natural. Recent literature spans ternary logic circuit design using CNTFETs (including full adders) and optimized ternary arithmetic logic design.  It also includes tri-valued memristor and in-memory ternary logic work (including “stateful logic” and balanced ternary gate realizations).  Ternary memory is likewise active, including ternary SRAM concepts and ternary content-addressable memory proposals.  This suggests a plausibility path for “ternary acceleration islands” even if general-purpose ternary CPUs remain unlikely in the near term.

A fifth force is that ternary AI is not merely an aesthetic fit; it is an active pragmatic optimization track. Ternary neural networks are typically defined over \(\{-1,0,1\}\), enabling multiplication-reduced or multiplication-free inference patterns under certain implementations.  Moreover, the xTern work demonstrates a concrete bridge strategy: implement efficient ternary inference via ISA extensions on mainstream cores (here, RISC‑V), rather than waiting for fully ternary hardware—reporting throughput and energy efficiency gains under their evaluated conditions. 

## Future-state targets for the midterm and long horizon

This section defines two “arrival states” that can guide design choices without over-committing to fragile predictions. Dates are anchored to your current timestamp (March 16, 2026).

**Midterm arrival state (by March 16, 2031).**  
T81 becomes a widely trusted **deterministic execution and governance substrate** for high-assurance computation on commodity hardware. In practical terms, that means:

- Determinism extends beyond runtime traces to include the compilation and packaging story in a way aligned with reproducible-build expectations (“bit-by-bit identical artifacts from defined inputs”). 
- CanonFS provenance becomes interoperable with mainstream attestation ecosystems (SLSA provenance predicates inside in-toto attestations), making T81 artifacts verifiable by external tooling rather than only by internal conventions.   
- Axion-style governance maps cleanly to external governance regimes (EU AI Act obligations timeline; NIST AI RMF governance function), allowing adopters to treat the stack as “compliance-ready infrastructure” rather than a research novelty.   
- The deterministic parallel execution model matures into a “boring” reliability primitive: the default way to scale without losing auditability, aligned to the broader lineage of deterministic parallelism research (which shows both feasibility and overhead tradeoffs). 
  
**Long-horizon arrival state (by March 16, 2036).**  
T81 becomes the reference architecture for a new category: **verifiable computation environments**, where “what happened” is cryptographically tied to “what ran,” and where execution can be re-performed exactly (or equivalently verified) across multiple platforms. This does not require that the world becomes ternary; rather, it requires that T81:

- Standardizes an ecosystem-friendly provenance and signing pipeline (SLSA/in-toto + Sigstore transparency patterns), so that determinism is embedded into distribution and governance rather than remaining an internal property. 
- Offers credible hardware acceleration paths: ternary compute islands (e.g., compute-in-memory ternary primitives, ternary CAM, or ISA extensions on open ISAs) for selected workloads, without sacrificing the deterministic contract for auditable modes.   
- Provides a cryptography and identity layer that is post-quantum-native by default (reflecting the NIST PQC standardization trajectory), so that the provenance ledger and audit trail remain durable as quantum threats mature. 

In short: 2031 is about making determinism + governance **useful and adoptable**; 2036 is about making those properties **structural to how software and models are trusted**.

## Roadmap architecture and phased delivery

The roadmap is divided into five programs that can be run in parallel with explicit “determinism surface” boundaries. The goal is to preserve what is already strong (the frozen substrate) while extending trust outward into build, distribution, and regulated deployment.

### Program A: Determinism beyond execution

The build and release pipeline is where most “trust collapses” occur in real ecosystems. Reproducible Builds emphasizes that environment details, timestamps, ordering, and toolchain behavior can all break determinism unless systematically controlled.

**Near-term (through 2027).**  
Define and ship a “deterministic compilation profile” as a first-class contract: same source + same compiler version + same CanonFS inputs ⇒ bit-identical TISC output and metadata. Align the contract language with reproducible-builds terminology so external auditors can reason about it. 

**Midterm (to 2031).**  
Publish machine-verifiable build provenance for every release artifact, structured as SLSA provenance within in-toto attestations, and store these attestations as CanonFS-addressed objects. This makes CanonFS not merely “content-addressed storage,” but an auditable log of how content came to exist. 

**Long horizon (to 2036).**  
Turn trace hashes and provenance attestations into a “universal receipt” format for deterministic workloads. The design goal is simple: if someone has the receipt, they can re-run, reproduce, or independently verify what you claim occurred.

### Program B: Governance interoperability as a product surface

AI governance is shifting from “ethics rhetoric” to compliance requirements with explicit dates. The EU timeline alone makes it impractical to treat auditability as an optional add-on. In parallel, NIST’s AI RMF formalizes governance as a lifecycle discipline and signals continued evolution via scheduled review. 

**Near-term (through 2027).**  
Build a mapping layer that translates Axion audit events and canonical reason strings into compliance-friendly artifacts: structured logs, model cards / system cards attachments, and policy attestations. Treat this as an adapter layer, not a rewrite of the kernel.

**Midterm (to 2031).**  
Provide pre-authored governance “profiles” (policy bundles) for major operators: regulated inference, high-assurance simulation, research reproducibility, and safety-critical control. The key is that the policy bundles should be evidence-producing by default, aligning with AI RMF’s emphasis on measurable risk management practices. 

**Long horizon (to 2036).**  
Evolve governance into a composable “policy supply chain,” where policies themselves are provenance-addressed artifacts with change control, signatures, and verifiable deployment history (mirroring the way code is handled).

### Program C: Supply-chain-grade provenance and signing

Supply chain attacks like SolarWinds are a vivid example of why “trust the binary” is no longer a safe default; CISA and MITRE describe the SolarWinds compromise as a software supply chain operation where malicious code was inserted into a build process and distributed via normal updates. This is exactly the class of event where reproducibility + provenance + governance can provide structural defenses. 

A practical interoperability stack exists: SLSA provenance, in-toto attestations, and Sigstore signing/transparency patterns. In addition, content-addressed storage is already a mainstream conceptual primitive: Git describes itself as a content-addressable filesystem (objects addressed by hash), and IPFS frames content-addressed block storage as a Merkle DAG for building verifiable structures over distributed content. 

**Near-term (through 2027).**  
Make “release provenance” a non-optional part of the C2 model: every release artifacts set emits (1) trace hash receipts, (2) build provenance attestations, and (3) signatures.

**Midterm (to 2031).**  
Adopt entity["organization","Sigstore","open signing ecosystem"]-style signing approaches for distribution, emphasizing transparency logs and verifiable signing events (keyless where appropriate). This is not about copying a tooling brand; it is about aligning CanonFS immutability with external verification expectations.

**Long horizon (to 2036).**  
Offer “provenance gateways” so that external ecosystems can import/export CanonFS artifacts while preserving attestations and signatures, enabling a broader trust fabric.

### Program D: Deterministic parallelism and distributed execution

Deterministic concurrency has decades of research showing both the value and the cost: CoreDet and DThreads demonstrate approaches to deterministic multithreading, often trading overhead or constraints for reproducibility and debuggability. Your DPE model is strategically aligned with this lineage (explicit ordering, buffered effects, committed epochs), but already grounded in your own frozen ISA contract rather than ad hoc runtime trickery.

**Near-term (through 2027).**  
Treat DPE as a “deterministic transaction system” and finish the operational story: observability, replay tooling, and audit-grade explanations for why epochs did or didn’t commit.

**Midterm (to 2031).**  
Add a deterministic “workflow layer” that makes DPE graphs exportable, cacheable, and attestable—so that a DPE computation becomes an artifact with a receipt, not just an execution.

**Long horizon (to 2036).**  
Extend determinism across machine boundaries with explicit ordering and verification. The research frontier includes deterministic parallel RPC runtimes (e.g., DORADD-style work) that aim to preserve determinism while remaining work-conserving under contention. Your advantage is that you can tie these execution histories directly into CanonFS + governance logging, making distributed determinism auditable rather than merely operational.

### Program E: Governed AI inference that is actually deterministic

Mainstream ML stacks can offer determinism “on the same hardware,” but reproducibility across devices and architectures can be elusive, especially when GPU libraries do not guarantee bitwise matches across architectures. This is where T81’s deterministic substrate can become more than a philosophy—it can become a product category.

However, the roadmap should distinguish two modes:

- **Governed inference mode:** policy interception, audit, and provenance, but not necessarily bit-exact across heterogeneous accelerators.
- **Deterministic inference mode:** bit-exact reproducibility guaranteed under a controlled arithmetic and kernel set.

Ternary neural networks offer a plausible bridge: they align naturally with \(\{-1,0,1\}\) encodings and can reduce reliance on floating-point behaviors that vary by backend. The xTern work is especially relevant because it demonstrates that ternary inference acceleration can be achieved via ISA extensions on commodity cores, reducing the need for specialized accelerators while maintaining a more controllable execution environment. 

**Near-term (through 2027).**  
Keep llama.cpp-style integration explicitly labeled as “governed but not determinism-verified” unless and until you can constrain the kernels and arithmetic end-to-end. cuDNN’s cross-architecture caveat is the kind of reality check that should inform an honest product boundary. 

**Midterm (to 2031).**  
Deliver a deterministic inference lane based on fixed-point / ternary-friendly kernels and restricted operator sets. Use the T81Graph layer to make inference graphs auditable and replayable.

**Long horizon (to 2036).**  
Co-design inference primitives with hardware acceleration islands (compute-in-memory ternary ops, ternary CAM, or ISA extensions), while maintaining the means to produce a deterministic “receipt” even when acceleration is used. The compute-in-memory and ternary device literature suggests this is a credible R&D bet, especially for edge and embedded regimes. 

## Ecosystem and governance strategy

A deterministic substrate is only as influential as the ecosystem that can *agree* on its contracts.

First, treat the frozen surfaces (ISA + type encodings) as a **standards-like commitment**, and the rest of the stack as evolving implementations around that core. This mirrors successful ISA governance patterns seen in bodies like entity["organization","RISC-V International","open isa standards body"], where specifications and extensions move through explicit development and ratification.  The lesson is not “be RISC-V,” but “make stability legible to outsiders.”

Second, make external verification easy. If you output provenance in in-toto/SLSA-compatible forms, you reduce the burden on adopters to “trust your special thing.” If you sign artifacts and log signing events in a transparency pattern (Sigstore/Cosign style), you reduce the burden on adopters to manage key ceremony for every build product.

Third, center the roadmap on an invariant: the deterministic trace is the **root narrative** of the system. That narrative is what turns governance from “policy words” into “policy evidence.” This is also how you avoid the trap where compliance features become bolted-on dashboards: the evidence should be produced as a consequence of execution, not as an afterthought.

Finally, build trust by being explicit about tradeoffs. Determinism often costs performance, and ML frameworks openly acknowledge that deterministic settings can reduce speed. Owning that tradeoff—then designing acceleration options that preserve auditability—is more persuasive than promising free lunches.

## Risks, tradeoffs, and decision gates

A decade roadmap is only useful if it includes the “doors you might not walk through.” The following risks should be promoted to explicit decision gates.

**Determinism versus throughput.**  
Deterministic modes can be slower, and in ML they may be unavailable for certain operations; this is explicitly documented in mainstream frameworks.  The gate is not “make it fast,” but: define which workloads justify deterministic mode, and ensure there is an auditable “governed but non-deterministic” lane that is still valuable.

**Heterogeneous accelerator nondeterminism.**  
GPU libraries may not guarantee bitwise reproducibility across architectures, which directly constrains cross-platform determinism claims if you depend on them. The gate: never expand the deterministic surface to include an accelerator path until you can prove it (or quarantine it behind a clearly labeled non-DCP boundary).

**Ternary hardware uncertainty.**  
Ternary and multi-valued research is active (memristive ternary logic, ternary IMC, ternary SRAM, ternary CAM), but research momentum is not the same as commoditized supply chains. The gate: the hardware program should be staged so that every step yields usable artifacts even if general-purpose ternary CPUs never arrive (e.g., ternary acceleration islands, deterministic instruction-set extensions, or verified emulation).

**Formal verification cost.**  
seL4 demonstrates that deep formal verification of an OS kernel is possible and valuable, but it is expensive and requires design-for-verification discipline. CompCert likewise shows that verified compilation can provide machine-checked semantic preservation, but it is a multi-year commitment. The gate: decide early which components warrant formal proof (likely the smallest, most security-critical deterministic surface), and keep the rest within strong testing + trace-based verification.

**Supply-chain trust and external credibility.**  
Software supply chain compromises (SolarWinds-class) are a persistent strategic threat class. The gate: shift from internal confidence (“we have determinism”) to external verifiability (“anyone can validate our artifacts and traces”), aligning with reproducible builds and SLSA/in-toto expectations. 

As a compact decision heuristic: if a proposed feature cannot produce **(a) an auditable event trail, (b) a provenance artifact, and (c) a deterministic or explicitly bounded execution claim**, it should stay outside the deterministic surface—even if it is exciting. That constraint is not a limitation; it is the artistic medium your system is built from.
