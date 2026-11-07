# MCO-R01 research handoff

This addition supersedes the next-step guidance for the executable experiment
only. All records in the frozen packet, including its original HANDOFF.md,
remain immutable. Start with this directory's README and RESULTS, then inspect
the source and complete archived execution evidence.

Recommendation: **REVISE** the research framing toward a conventional application
library/reference monitor. Do not introduce a subsystem, OS primitive, inference
engine, VM extension, language, production authorization change, or ternary-native
implementation. No missing expressive or enforcement property was demonstrated.

The frozen finite contract is implemented and checked. A future experiment
should preregister a measurable advantage of three-state composition over an
equally capable conventional service (for example measured review effort or
diagnostic error rate). Current conformance and mutation results show no such
advantage. Stop architecture escalation unless a new falsifiable requirement
cannot be met by the conventional baseline. Broader hostile-process or concurrent
enforcement would require a separately specified threat model and experiment.

To audit a run, extract its evidence archive, inspect `commands.json`, and use
the recorded trace with the independent Ruby reconstructor. For historical source
fingerprints, use the checkout containing this experiment's committed source.
Receipt hashes bind records and execution prefixes; they are not signatures or
proof that a hostile host honestly observed a ledger. The auditor must retain
the frozen fixtures/context and the independent owner's ledger observation.

PR #573 must remain draft. No merge or production integration is recommended.
