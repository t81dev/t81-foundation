# Mutation-test obligations — specified, not executed

No candidate or authorization oracle is implemented in this contribution. These
are exact defects to introduce into a future implementation, with witnesses and
observable mismatches. Never report the following as four killed mutants until
the mutated binaries actually run and the independent checker rejects them.

| Mutation | Exact scope | Required witness and mismatch |
| --- | --- | --- |
| M-U: permit on unknown | Change the final summary's U branch to P, without changing the oracle or fixtures | F06-C01/02/03/04/05 must remain U; F03-C05 and F08-C03 also expose unavailable authority/replay. A wrong P kills the mutant even if a separate sink check still blocks the effect. |
| M-V: skip final revocation check | At A/B use R's cached active-policy decision instead of live policy_state; other facts still checked | F10-C02 and reachable RVAB: R=P, V revokes, A must D/POLICY_REVOKED with zero protected dispatches/rows. A cached P or a row kills it. Skipping only an earlier redundant check may be masked and is not equivalent to this mutation. |
| M-R: skip final replay check | Ignore replay_state=spent during broker admission, including duplicate B; preserve policy/budget | F08-C05 and RABV: A commits once; B must D/REPLAY and no second dispatch. Budget starts at 2 so a zero-budget guard cannot mask the missing replay check. A second row or wrong B decision kills it. |
| M-I: model-identity substitution | Omit observed_model_ref == approved_model_ref at response binding/final admission; preserve request/echo/grant equality checks | F09-C02 has approved/echoed A but independently observed B. Correct result is ResponseBinding/D/BINDING. Checking only the echoed model would wrongly reach P; stage/decision mismatch kills it even without dispatch. F09-C01 alone is inadequate because its bad echo is caught independently. |

For M-U and M-I, the blocked cases intentionally contain no admission event.
Their primary decision/stopping-stage mismatch is the required kill signal;
do not invent an observed sink write. A separate defense can contain a mutant
without making its wrong decision conformant. For M-R, the duplicate is a
broker probe, not an already authorized side effect, so observing two broker
probes is correct while two protected dispatches are not.

Oracle-only or expected-data mutations must also be rejected: change an expected
P to U, delete a diagnostic subject, change a fixture byte without its digest,
or label an invalid temporal order as a successful zero-row run. Data integrity
checks cover some such corruptions, but they do not substitute for executing
candidate mutations. If an expected semantic defect survives due to a redundant
check, report masking and strengthen the witness; do not delete the defense to
manufacture a vulnerability.

A future run must record parent and mutant source/binary hashes, exact patch,
case IDs, command, exit status, actual mismatch, before/after records and whether
the failure occurred before the intended assertion. A build failure or absent
runtime is not a killed semantic mutant. Keep all unmodified controls enabled.
