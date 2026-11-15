# Future authorization interface proposal — no executable activation

Current policy is unconditional denial of measurement. Only correctness-only
validation and native compile-identity inspection exist. Timing, warm-up, pilot,
trial, campaign, permit and authorization CLI options reject before source access
or worker launch. Environment variables and JSON fields do not enable anything.
There is no timer module, permit reader, signature verifier, dispatcher or
activation callback to invoke indirectly. No trusted-key enrollment is performed.

This document proposes the boundary for a future separately reviewed change;
it is not implemented API code and grants no authority. The fixed source and
manifest pins in README apply. PR #689 remains the design reference, without
modification or automatic promotion of its readiness matrix.

| Conceptual input to a future authorization decision | Requirement |
| --- | --- |
| Maintainer authorization | Authenticated, separately approved signature/trust root, scope, nonce, validity, revocation/freshness and budget. A caller-supplied key or a flag cannot grant authority. |
| Exact implementation/input identities | Audited source, new runner/executable, protocol/manifest, fixed workload/corpus and configuration hashes. Unknown/mismatched identity denies. |
| Host qualification evidence | Independently reviewed host/boot/control/timer/accounting report. A valid permit does not qualify the host. |
| Workload engineering evidence | Correctness, retained-output bounds, metadata/observer validity, and any required independent semantic review. PROVISIONAL VM checks cannot self-promote. |
| Proposed action | Distinguish bounded qualification-only probes from a separately authorized campaign. Correctness evidence or a qualification permit is not campaign authorization. |

The proposed result is a decision with explicit deny reasons and bound evidence
identities, not an imperative to run code. Eligibility is the conjunction of
valid authorization, qualified host and approved workload/implementation evidence;
none substitutes for another. Future launch and worker boundaries must both
check it, prevent replay and preserve failures. An expired, forged, revoked,
stale or mismatched decision must deny before timer use, warm-up or launch.
The exact serialized permit/parser/key-management design is future review work.

No conceptual allow outcome is constructible through current code. Source edits
and approval would be required to add any measurement path, followed by explicit
maintainer authorization. This is a trusted-code/operator boundary, not protection
against root replacing code, compilers or trust stores. It changes no frozen
CMP3/H04-R03 rule, governance state, Protocol 001 activation or human merge policy.
