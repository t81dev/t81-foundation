To the Program Chairs of USENIX OSDI 2026,

We are pleased to submit for your consideration our manuscript, "T81 Foundation: A Ternary-Native, Cognition-First Stack with Deterministic Structural Metadata and Provable Canonicalization Guarantees."

This paper addresses the critical and pervasive problem of non-determinism in modern software toolchains—a vulnerability that undermines reproducibility, complicates verification, and erodes trust in high-stakes computational systems. In response, we present the T81 Foundation, a complete, vertically integrated computing stack architected from first principles to deliver provable, end-to-end canonicalization.

Our core contribution is a novel methodology where high-level structural metadata is treated as a first-class citizen throughout the entire compilation and execution pipeline. We introduce T81Lang, a language whose structural type system ensures this metadata is preserved; the TISC intermediate representation, which propagates it into a self-describing binary; and the HanoiVM, a runtime that leverages this information to guarantee deterministic memory layouts and execution.

We provide empirical evidence that our `t81` toolchain produces bit-for-bit identical binaries from the same source code across different architectures and build environments. This result offers a strong guarantee of reproducibility that is largely absent in contemporary systems. We believe this work on achieving determinism by design represents a significant and timely contribution to the systems community, particularly for the future of auditable AI and verifiable software.

Thank you for your time and consideration. We are confident that this paper will be of great interest to the OSDI audience.

Sincerely,
The T81 Foundation Collective
