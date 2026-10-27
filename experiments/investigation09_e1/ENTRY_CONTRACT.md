# E1 execution contract (fixed before cost observations)

Protocol: PR #604 at `f1054438b805b541c4acfd294e6dba5a71711c05`;
implementation baseline `c930b8c2ee7478782bf3c6d231bdd59984595ef2`.
The proposal remains unchanged. This file resolves the experiment adapter ABI;
it does not change the workload, threshold, or eligibility requirements.

The common entry contract is `(selector, a, b, c) -> ordered result integers`.
No kernel performs external I/O. Inputs are supplied at execution time, not
specialized into the source or executable. Three-state input values are -1,0,1.
The selected operations are negation, meet/min, join/max, and
`max(min(a,b),-c)`. The combined module selects one of the four operations.
Per-kernel modules accept the same entry arguments and ignore unused ones.

To observe decision intermediates in the actual computation, return its
`(meet(a,b), -c, decision)` triple encoded as
`9*(meet+1) + 3*(-c+1) + (decision+1)` (0 through 26). Other three-state
kernels return `value+1`. The independent oracle decodes the result and checks
every component. Both toolchains carry the same arithmetic encoding in their
complete artifacts. The encoding is observation/ABI overhead, not an ISA benefit.
The original mathematical kernels are unchanged. Identity and boolean/integer
controls have their explicitly recorded integer outputs.

T81 sources initialize four top-level bindings, then invoke the compiled `run`
function. The adapter executes these four emitted initializer instructions,
then writes the input values to their destination registers through the public
VM API, preserving their tags. It does not edit bytecode, compiler IR, or VM
semantics. The adapter checks this instruction prefix, checks the compiled
result POP preceding HALT, and reads that return register after completion.
The saved artifact contains the complete emitted caller, callee and metadata.
This checkpoint ABI is experiment support, not a new language input facility;
its implementation/support cost is disclosed separately. Sources are compiled
with the existing analyzer attached to the existing IR generator.

Wasm32 modules expose a single `run` function, using signed i64 arguments and
result (Wasm32 describes the address space, not the integer width). This
matches physical scalar VM words and avoids giving the conventional side
narrower scalar storage. Kernel values are bounded and no signed overflow is
permitted. The abstract input/output contract matches; stack/register calling
conventions differ and their artifact overhead remains included. There are no
unused runtime imports. Branch, conditional-selection, and lookup-table forms
are each compiled at O2, O3 and Oz, with and without Binaryen Oz optimization.
Native C is a separate semantic control, never the footprint denominator.

Trusted valid-domain inputs enter the kernels. A separate common host boundary
wrapper validates selector/domain and rejects invalid values before entering
either engine; invalid input is not silently clamped. Its code belongs to the
shared experiment support, not to either module. This boundary condition must
be stated with any result; no standalone untrusted-input safety claim follows.

Logical live payload is assessed from the same abstract computation and declared
64-bit entry/result slots, independently of allocated registers/operand stacks.
Physical state, tags, program pools, runtime/support binaries, and saved-file
sizes are separate ledgers. Primary size is the complete saved executable;
13-byte encoded instruction streams are diagnostic only. Artifact loading and
input invocation must succeed in a fresh process before a comparison is eligible.

Stop at the first unexplained valid-domain mismatch, unsupported lowering, or
missing comparator. Preserve the case, source, generated artifact, and trace;
report INCONCLUSIVE rather than continuing to a footprint advantage claim.
No timings or unrelated architecture benchmarks are authorized by this run.
